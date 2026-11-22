#include "mailbox.h"
#include "link.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>

// Ver mailbox.h. El cerrojo protege todo lo de abajo: la tarea de red (nucleo 0) y el bucle
// principal (nucleo 1) lo tocan los dos.

struct Friend {
  uint64_t pid;
  uint8_t key[LINK_KEY];
  MailBox box;
  bool hasBox;
};

static SemaphoreHandle_t mx = nullptr;
static Friend fr[MAIL_FRIENDS];
static uint8_t frN = 0;
static MailOut outq[MAIL_OUTQ];
static MailMsg inq[MAIL_INQ];
static uint8_t inN = 0;
static MailSeen seen;
static uint32_t cursor = 0;  // la hora (del buzon) del ultimo mensaje recogido
static char myBox[MAIL_BOX_MAX] = "";
static uint64_t self = 0;
static volatile bool wantNow = false;
static uint32_t lastSync = 0;
static bool synced = false;
// la hora de pared la pone el bucle principal (mailTick): leer el RTC por I2C desde la tarea de red
// chocaria con el tactil
static volatile uint32_t wallNow = 0;

void mailTick(uint32_t wall) { wallNow = wall; }

static void lock() { if (mx) xSemaphoreTake(mx, portMAX_DELAY); }
static void unlock() { if (mx) xSemaphoreGive(mx); }

static void boxKey(uint64_t pid, char *k) {  // clave NVS (15 como mucho): "b" + 12 hex
  snprintf(k, 15, "b%012llx", (unsigned long long)(pid & 0xFFFFFFFFFFFFull));
}

static void saveOut() {
  Preferences p;
  p.begin("mail", false);
  p.putBytes("out", outq, sizeof(outq));
  p.end();
}

static void saveIn() {
  Preferences p;
  p.begin("mail", false);
  p.putBytes("in", inq, sizeof(inq));
  p.putUChar("inN", inN);
  p.putBytes("seen", &seen, sizeof(seen));
  p.end();
}

static void saveCursor() {
  Preferences p;
  p.begin("mail", false);
  p.putUInt("cur", cursor);
  p.end();
}

void mailBegin() {
  if (!mx) mx = xSemaphoreCreateMutex();
  Preferences p;
  p.begin("mail", true);
  p.getString("box", myBox, sizeof(myBox));
  cursor = p.getUInt("cur", 0);
  if (p.getBytes("out", outq, sizeof(outq)) != sizeof(outq)) memset(outq, 0, sizeof(outq));
  if (p.getBytes("in", inq, sizeof(inq)) != sizeof(inq)) memset(inq, 0, sizeof(inq));
  inN = p.getUChar("inN", 0);
  if (inN > MAIL_INQ) inN = 0;
  if (p.getBytes("seen", &seen, sizeof(seen)) != sizeof(seen)) seen = MailSeen();
  p.end();
  Preferences l;
  l.begin("link", true);
  self = l.getULong64("pid", 0);
  l.end();
  mailRefreshFriends();
}

void mailRefreshFriends() {
  linkFriendsLoad();
  Friend tmp[MAIL_FRIENDS];
  uint8_t n = 0;
  Preferences p;
  p.begin("mail", true);
  for (uint8_t i = 0; i < linkFriendCount() && n < MAIL_FRIENDS; i++) {
    if (linkFriendRam(i)) continue;  // de prueba (LINK PAIRTEST): sin correo
    const LinkFriend &f = linkFriendAt(i);
    Friend &d = tmp[n++];
    d.pid = f.pid;
    memcpy(d.key, f.key, LINK_KEY);
    char k[16];
    boxKey(f.pid, k);
    d.hasBox = p.getBytes(k, &d.box, sizeof(d.box)) == sizeof(d.box) && mailBoxValid(d.box.box);
  }
  p.end();
  if (!self) {
    Preferences l;
    l.begin("link", true);
    self = l.getULong64("pid", 0);
    l.end();
  }
  lock();
  memcpy(fr, tmp, sizeof(Friend) * n);
  frN = n;
  unlock();
}

uint64_t mailSelf() { return self; }

void mailMyBox(MailBox &b) {
  if (!myBox[0]) {  // la primera vez (con la radio encendida: esp_random es aleatorio de verdad)
    snprintf(myBox, sizeof(myBox), "tp%08lx%08lx%04lx", (unsigned long)esp_random(), (unsigned long)esp_random(),
             (unsigned long)(esp_random() & 0xFFFF));
    Preferences p;
    p.begin("mail", false);
    p.putString("box", myBox);
    p.end();
  }
  memset(&b, 0, sizeof(b));
  strncpy(b.url, MAILBOX_URL, sizeof(b.url) - 1);
  strncpy(b.box, myBox, sizeof(b.box) - 1);
  strncpy(b.auth, MAILBOX_KEY, sizeof(b.auth) - 1);
}

// por "见面": url\0box\0auth\0
size_t mailBoxToWire(const MailBox &b, uint8_t *out, size_t cap) {
  size_t n = 0;
  for (const char *s : { b.url, b.box, b.auth }) {
    size_t l = strlen(s) + 1;
    if (n + l > cap) return 0;
    memcpy(out + n, s, l);
    n += l;
  }
  return n;
}

bool mailBoxFromWire(const uint8_t *b, size_t n, MailBox &out) {
  memset(&out, 0, sizeof(out));
  char *dst[3] = { out.url, out.box, out.auth };
  size_t cap[3] = { sizeof(out.url), sizeof(out.box), sizeof(out.auth) };
  size_t pos = 0;
  for (int f = 0; f < 3; f++) {
    size_t l = 0;
    while (pos + l < n && b[pos + l]) l++;
    if (pos + l >= n || l >= cap[f]) return false;  // sin el 0 final o demasiado largo
    memcpy(dst[f], b + pos, l);
    pos += l + 1;
  }
  bool http = !strncmp(out.url, "https://", 8) || !strncmp(out.url, "http://", 7);
  return http && mailBoxValid(out.box);
}

void mailSetFriendBox(uint64_t pid, const MailBox &b) {
  char k[16];
  boxKey(pid, k);
  Preferences p;
  p.begin("mail", false);
  MailBox old;
  bool igual = p.getBytes(k, &old, sizeof(old)) == sizeof(old) && !memcmp(&old, &b, sizeof(b));
  if (!igual) p.putBytes(k, &b, sizeof(b));  // solo si cambio (la NVS congela al escribir)
  p.end();
  lock();
  for (uint8_t i = 0; i < frN; i++)
    if (fr[i].pid == pid) {
      fr[i].box = b;
      fr[i].hasBox = true;
    }
  unlock();
  // sin la direccion: la de un Worker propio es de ese amigo (MAIL INFO la enseña si hace falta)
  if (!igual) Serial.printf("correo: buzon de un amigo apuntado (%s)\n", strncmp(b.url, "https://ntfy.sh", 15) ? "propio" : "ntfy.sh");
}

bool mailFriendReady(uint64_t pid) {
  lock();
  bool ok = false;
  for (uint8_t i = 0; i < frN; i++)
    if (fr[i].pid == pid) ok = fr[i].hasBox;
  unlock();
  return ok;
}

bool mailSend(uint64_t to, MailMsg m) {
  uint32_t now = wallNow;
  lock();
  int slot = -1;
  for (int i = 0; i < MAIL_OUTQ; i++)
    if (!outq[i].on) { slot = i; break; }
  if (slot >= 0) {
    m.from = self;
    do m.id = esp_random(); while (!m.id);
    MailOut &o = outq[slot];
    o = MailOut();
    o.on = true;
    o.to = to;
    o.firstAt = o.nextAt = now;
    o.msg = m;
    saveOut();
  }
  unlock();
  if (slot < 0) return false;
  wantNow = true;  // que salga cuanto antes
  return true;
}

uint8_t mailOutCount(uint64_t to) {
  lock();
  uint8_t n = 0;
  for (auto &o : outq)
    if (o.on && o.to == to && o.msg.type != MT_ACK) n++;
  unlock();
  return n;
}

bool mailPeekArrival(MailMsg &m) {
  lock();
  bool ok = inN > 0;
  if (ok) m = inq[0];
  unlock();
  return ok;
}

void mailPopArrival() {
  lock();
  if (inN) {
    memmove(&inq[0], &inq[1], sizeof(MailMsg) * (inN - 1));
    inN--;
    saveIn();
  }
  unlock();
}

bool mailFakeArrival(const MailMsg &m) {
  lock();
  bool ok = inN < MAIL_INQ;
  if (ok) inq[inN++] = m;  // sin guardar: solo para verlo
  unlock();
  return ok;
}

// en el bucle principal: lo que lleva 14 dias sin acuse se deja; un regalo vuelve a la mochila
bool mailTakeRefund(uint8_t &item) {
  uint32_t now = wallNow;
  if (!now) return false;
  bool got = false;
  lock();
  for (auto &o : outq) {
    if (!mailOutExpired(o, now)) continue;
    o.on = false;
    if (o.msg.type == MT_GIFT && !got) {
      item = o.msg.item;
      got = true;
    }
    saveOut();
    if (got) break;
  }
  unlock();
  return got;
}

void mailSyncSoon() { wantNow = true; }
void mailSyncCancel() { wantNow = false; }
bool mailSyncWanted() { return wantNow; }
bool mailSyncDue(bool usb) {
  if (wantNow) return true;
  return usb && (!synced || millis() - lastSync >= MAIL_EVERY_USB_MS);
}

// ---------- la red ----------

// El cuerpo de la respuesta, leido a mano. HTTPClient::getString() espera los datos dando vueltas
// sin soltar la CPU (delay(0)): en la tarea de red (CPU 0) el perro guardian salta a los 5 s y
// reinicia la placa, y al encenderla vuelta a empezar. Aqui se espera de verdad, como mucho
// MAIL_BODY_MS; si no llega entero se quedan solo las lineas completas (lo demas, la proxima vez).
#define MAIL_BODY_MS 8000
#define MAIL_BODY_MAX 16384
static void leerCuerpo(HTTPClient &h, String &out) {
  out = "";
  int len = h.getSize();  // -1: sin Content-Length, hasta que cierren (HTTP/1.0: nunca troceado)
  WiFiClient *st = h.getStreamPtr();
  if (!st || len > MAIL_BODY_MAX) return;
  if (len > 0) out.reserve(len);
  uint8_t buf[256];
  uint32_t t0 = millis();
  while (len != 0 && millis() - t0 < MAIL_BODY_MS && out.length() <= MAIL_BODY_MAX) {
    int n = st->available();
    if (n <= 0) {
      if (!st->connected()) break;  // ya no llega nada mas
      delay(10);
      continue;
    }
    if (n > (int)sizeof(buf)) n = sizeof(buf);
    if (len > 0 && n > len) n = len;
    n = st->read(buf, n);
    if (n <= 0) {
      delay(1);
      continue;
    }
    out.concat((const char *)buf, n);
    if (len > 0) len -= n;
  }
  bool entero = len == 0 || (len < 0 && !st->connected());
  Serial.printf("correo: %u B en %lu ms%s\n", out.length(), (unsigned long)(millis() - t0), entero ? "" : " (incompleto)");
  if (!entero) {
    int k = out.lastIndexOf('\n');
    out = k >= 0 ? out.substring(0, k + 1) : String();
  }
}

static int http(bool post, const String &url, const char *auth, const String &body, String *resp) {
  bool tls = url.startsWith("https://");
  WiFiClientSecure sc;
  WiFiClient pc;
  if (tls) sc.setInsecure();  // da igual quien conteste: el contenido va cifrado y firmado
  HTTPClient h;
  h.setTimeout(8000);
  h.setUserAgent("TamaPoke/2026.11 (+https://github.com/Mu-Muu/TamaPoke)");
  if (resp) h.useHTTP10(true);  // la respuesta se lee a mano (leerCuerpo): entera, sin trozos
  if (!h.begin(tls ? (WiFiClient &)sc : pc, url)) return -1;
  if (auth && *auth) h.addHeader("X-Key", auth);
  int code;
  if (post) {
    h.addHeader("Content-Type", "text/plain");
    code = h.POST(body);
  } else {
    code = h.GET();
  }
  if (resp && code == 200) leerCuerpo(h, *resp);
  h.end();
  return code;
}

static String boxUrl(const MailBox &b) {
  String u = b.url;
  if (!u.endsWith("/")) u += "/";
  return u + b.box;
}

// manda lo que toque (copia fuera del cerrojo: la red tarda)
static void sendDue(uint32_t now) {
  for (int i = 0; i < MAIL_OUTQ; i++) {
    lock();
    MailOut o = outq[i];
    bool due = mailOutDue(o, now);
    Friend f;
    bool found = false;
    for (uint8_t k = 0; k < frN && due; k++)
      if (fr[k].pid == o.to && fr[k].hasBox) {
        f = fr[k];
        found = true;
      }
    unlock();
    if (!due || !found) continue;
    uint8_t plain[MAIL_MSG_LEN], nonce[MAIL_NONCE], sealed[MAIL_SEALED_MAX];
    mailEncode(plain, o.msg);
    for (int k = 0; k < MAIL_NONCE; k += 4) linkPut32(nonce + k, esp_random());
    size_t sn = mailSeal(f.key, nonce, plain, sizeof(plain), sealed);
    char b64[(MAIL_SEALED_MAX + 2) / 3 * 4 + 1];
    if (!sn || !b64Encode(sealed, sn, b64, sizeof(b64))) continue;
    int code = http(true, boxUrl(f.box), f.box.auth, String(b64), nullptr);
    Serial.printf("correo: manda tipo %u a %08lx: http %d\n", o.msg.type, (unsigned long)(o.to >> 32), code);
    if (code != 200) continue;
    lock();
    if (outq[i].on && outq[i].msg.id == o.msg.id) {
      mailOutSent(outq[i], now);
      saveOut();
    }
    unlock();
  }
}

// un mensaje recogido: de que amigo es (la clave que lo abre) y que hacer. "otraVez": es de la hora
// del cursor, que ya se recogio la vez pasada (since la incluye): si ya se habia visto, nada (ni acuse)
static void received(uint32_t now, const char *b64, bool otraVez) {
  uint8_t sealed[MAIL_SEALED_MAX], plain[64];
  size_t sn = b64Decode(b64, sealed, sizeof(sealed)), pn = 0;
  if (!sn) return;
  lock();
  MailMsg m;
  int who = -1;
  for (uint8_t k = 0; k < frN && who < 0; k++)
    if (mailOpen(fr[k].key, sealed, sn, plain, pn) && mailDecode(plain, pn, m) && m.from == fr[k].pid) who = k;
  if (who < 0) {  // de nadie conocido (o estropeado): se ignora
    unlock();
    return;
  }
  bool cambio = false;
  if (m.type == MT_ACK) {
    for (auto &o : outq) cambio |= mailOutAcked(o, m.ackOf);
    if (cambio) saveOut();
    unlock();
    return;
  }
  bool nuevo = !seen.seen(m.id);
  if (!nuevo && otraVez) {
    unlock();
    return;
  }
  if (nuevo && inN < MAIL_INQ) {
    seen.add(m.id);
    inq[inN++] = m;
    saveIn();
    Serial.printf("correo: llega tipo %u de %08lx\n", m.type, (unsigned long)(m.from >> 32));
  }
  if (m.type != MT_VISIT && (!nuevo || seen.seen(m.id))) {  // acuse (tambien a un repetido: se perdio el anterior)
    for (auto &o : outq)
      if (!o.on) {
        o = MailOut();
        o.on = true;
        o.to = m.from;
        o.firstAt = o.nextAt = now;
        o.msg.type = MT_ACK;
        o.msg.from = self;
        do o.msg.id = esp_random(); while (!o.msg.id);
        o.msg.ackOf = m.id;
        saveOut();
        break;
      }
  }
  unlock();
}

void mailNetSync() {
  wantNow = false;
  lastSync = millis();
  synced = true;
  lock();
  uint8_t n = frN;
  unlock();
  if (!self || !n || WiFi.status() != WL_CONNECTED) return;
  uint32_t now = wallNow;
  sendDue(now);
  // lo mio: lo que haya en mi buzon desde la ultima vez (incluida esa hora: los repetidos se reconocen)
  if (myBox[0]) {
    MailBox me;
    mailMyBox(me);
    String resp;
    String url = boxUrl(me) + "/json?poll=1&since=" + (cursor ? String(cursor) : String("all"));
    int code = http(false, url, me.auth, String(), &resp);
    uint32_t maxT = cursor;
    int lines = 0;
    for (int a = 0; a < (int)resp.length();) {
      int b = resp.indexOf('\n', a);
      if (b < 0) b = resp.length();
      String line = resp.substring(a, b);
      a = b + 1;
      uint32_t t;
      char msg[(MAIL_SEALED_MAX + 2) / 3 * 4 + 1];
      if (!mailParseLine(line.c_str(), t, msg, sizeof(msg))) continue;
      lines++;
      received(now, msg, cursor && t <= cursor);
      if (t > maxT) maxT = t;
    }
    if (maxT != cursor) {
      cursor = maxT;
      saveCursor();
    }
    Serial.printf("correo: recoge http %d, %d mensajes\n", code, lines);
  }
  sendDue(now);  // los acuses de lo que acaba de llegar
}

void mailInfo() {
  lock();
  Serial.printf("correo: yo %08lx buzon %s en %s%s, cursor %lu, %u amigos, %u por ver\n", (unsigned long)(self >> 32),
                myBox[0] ? myBox : "(aun no)", MAILBOX_URL, MAILBOX_KEY[0] ? " (con clave)" : "", (unsigned long)cursor,
                frN, inN);
  for (uint8_t i = 0; i < frN; i++)
    Serial.printf("  amigo %08lx: %s\n", (unsigned long)(fr[i].pid >> 32), fr[i].hasBox ? fr[i].box.url : "(sin buzon: veros una vez)");
  for (auto &o : outq)
    if (o.on)
      Serial.printf("  sale tipo %u a %08lx, intentos %u, id %08lx\n", o.msg.type, (unsigned long)(o.to >> 32), o.tries,
                    (unsigned long)o.msg.id);
  unlock();
}
