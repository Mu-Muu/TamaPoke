#include "mail.h"
#include "linkproto.h"  // linkSha256, linkPut/Get
#include <string.h>
#include <ArduinoJson.h>

size_t mailEncode(uint8_t *b, const MailMsg &m) {
  b[0] = MAIL_VER;
  b[1] = m.type;
  linkPut32(b + 2, m.id);
  linkPut64(b + 6, m.from);
  linkPut16(b + 14, (uint16_t)m.species);
  b[16] = m.shiny;
  linkPut16(b + 17, m.level);
  b[19] = m.phrase;
  b[20] = m.sticker;
  b[21] = m.item;
  b[22] = m.qty;
  linkPut32(b + 23, m.ackOf);
  b[27] = 0;  // reservado
  return MAIL_MSG_LEN;
}

bool mailDecode(const uint8_t *b, size_t n, MailMsg &m) {
  if (n < MAIL_MSG_LEN || b[0] != MAIL_VER || b[1] < MT_VISIT || b[1] > MT_ACK) return false;
  m.type = b[1];
  m.id = linkGet32(b + 2);
  m.from = linkGet64(b + 6);
  m.species = (int16_t)linkGet16(b + 14);
  m.shiny = b[16];
  m.level = linkGet16(b + 17);
  m.phrase = b[19];
  m.sticker = b[20];
  m.item = b[21];
  m.qty = b[22];
  m.ackOf = linkGet32(b + 23);
  return true;
}

bool mailMsgUsable(const MailMsg &m, uint16_t itemCount, int16_t dexMax) {
  if (m.type == MT_ACK) return true;
  if (m.species < 1 || m.species > dexMax) return false;
  if (m.type == MT_CARD) return m.phrase < MAIL_PHRASES && m.sticker < MAIL_STICKERS;
  if (m.type == MT_GIFT) return m.item < itemCount;
  return m.type == MT_VISIT;
}

// ---------- cifrado y firma ----------

static void subKey(const uint8_t key[16], const char *tag, uint8_t out[32]) {
  uint8_t in[32];
  size_t t = strlen(tag);
  memcpy(in, tag, t);
  memcpy(in + t, key, 16);
  linkSha256(in, t + 16, out);
}

void mailHmac(const uint8_t *key, size_t kn, const uint8_t *msg, size_t n, uint8_t out[32]) {
  uint8_t k0[64] = {};
  if (kn > 64) linkSha256(key, kn, k0);
  else memcpy(k0, key, kn);
  uint8_t buf[64 + MAIL_SEALED_MAX + 32];
  if (n > MAIL_SEALED_MAX) n = MAIL_SEALED_MAX;  // aqui solo se firman mensajes cortos
  for (int i = 0; i < 64; i++) buf[i] = k0[i] ^ 0x36;
  memcpy(buf + 64, msg, n);
  uint8_t inner[32];
  linkSha256(buf, 64 + n, inner);
  for (int i = 0; i < 64; i++) buf[i] = k0[i] ^ 0x5c;
  memcpy(buf + 64, inner, 32);
  linkSha256(buf, 64 + 32, out);
}

static void keystream(const uint8_t enc[32], const uint8_t nonce[MAIL_NONCE], uint8_t *data, size_t n) {
  uint8_t in[32 + MAIL_NONCE + 4], ks[32];
  memcpy(in, enc, 32);
  memcpy(in + 32, nonce, MAIL_NONCE);
  for (size_t off = 0, blk = 0; off < n; off += 32, blk++) {
    linkPut32(in + 32 + MAIL_NONCE, (uint32_t)blk);
    linkSha256(in, sizeof(in), ks);
    for (size_t i = 0; i < 32 && off + i < n; i++) data[off + i] ^= ks[i];
  }
}

size_t mailSeal(const uint8_t key[16], const uint8_t nonce[MAIL_NONCE], const uint8_t *in, size_t n, uint8_t *out) {
  if (n > 64) return 0;
  uint8_t enc[32], mac[32], tag[32];
  subKey(key, "TPmail enc", enc);
  subKey(key, "TPmail mac", mac);
  memcpy(out, nonce, MAIL_NONCE);
  memcpy(out + MAIL_NONCE, in, n);
  keystream(enc, nonce, out + MAIL_NONCE, n);
  mailHmac(mac, 32, out, MAIL_NONCE + n, tag);
  memcpy(out + MAIL_NONCE + n, tag, MAIL_TAG);
  return MAIL_NONCE + n + MAIL_TAG;
}

bool mailOpen(const uint8_t key[16], const uint8_t *in, size_t n, uint8_t *out, size_t &outN) {
  if (n < MAIL_NONCE + MAIL_TAG || n > MAIL_SEALED_MAX) return false;
  size_t len = n - MAIL_NONCE - MAIL_TAG;
  uint8_t enc[32], mac[32], tag[32];
  subKey(key, "TPmail mac", mac);
  mailHmac(mac, 32, in, MAIL_NONCE + len, tag);
  uint8_t diff = 0;  // sin cortar al primer byte distinto
  for (int i = 0; i < MAIL_TAG; i++) diff |= tag[i] ^ in[MAIL_NONCE + len + i];
  if (diff) return false;
  subKey(key, "TPmail enc", enc);
  memcpy(out, in + MAIL_NONCE, len);
  keystream(enc, in, out, len);
  outN = len;
  return true;
}

// ---------- base64 ----------

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t b64Encode(const uint8_t *in, size_t n, char *out, size_t cap) {
  size_t need = (n + 2) / 3 * 4;
  if (need + 1 > cap) return 0;
  size_t o = 0;
  for (size_t i = 0; i < n; i += 3) {
    uint32_t v = (uint32_t)in[i] << 16 | (i + 1 < n ? in[i + 1] << 8 : 0) | (i + 2 < n ? in[i + 2] : 0);
    out[o++] = B64[(v >> 18) & 63];
    out[o++] = B64[(v >> 12) & 63];
    out[o++] = i + 1 < n ? B64[(v >> 6) & 63] : '=';
    out[o++] = i + 2 < n ? B64[v & 63] : '=';
  }
  out[o] = 0;
  return o;
}

static int b64Val(char c) {
  const char *p = c ? strchr(B64, c) : nullptr;
  return p ? (int)(p - B64) : -1;
}

size_t b64Decode(const char *in, uint8_t *out, size_t cap) {
  size_t n = strlen(in), o = 0;
  if (n % 4) return 0;
  for (size_t i = 0; i < n; i += 4) {
    int v[4];
    for (int k = 0; k < 4; k++) v[k] = in[i + k] == '=' ? 0 : b64Val(in[i + k]);
    if (v[0] < 0 || v[1] < 0 || v[2] < 0 || v[3] < 0) return 0;
    uint32_t w = (uint32_t)v[0] << 18 | v[1] << 12 | v[2] << 6 | v[3];
    uint8_t b[3] = { (uint8_t)(w >> 16), (uint8_t)(w >> 8), (uint8_t)w };
    int keep = in[i + 2] == '=' ? 1 : in[i + 3] == '=' ? 2 : 3;
    if (o + keep > cap) return 0;
    for (int k = 0; k < keep; k++) out[o++] = b[k];
  }
  return o;
}

bool mailBoxValid(const char *box) {
  size_t n = box ? strlen(box) : 0;
  if (n < 8 || n > 40) return false;
  for (size_t i = 0; i < n; i++) {
    char c = box[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_'))
      return false;
  }
  return true;
}

// ---------- salida ----------

bool mailOutDue(const MailOut &o, uint32_t now) { return o.on && (int32_t)(now - o.nextAt) >= 0; }

bool mailOutExpired(const MailOut &o, uint32_t now) {
  return o.on && now && o.firstAt && (int32_t)(now - o.firstAt) >= (int32_t)MAIL_GIVEUP_S;
}

bool mailOutFixClock(MailOut &o, uint32_t now) {
  if (!o.on || !now) return false;
  bool cambio = false;
  if (!o.firstAt || (int32_t)(o.firstAt - now) > 0) {  // sin hora, o "en el futuro"
    o.firstAt = now;
    cambio = true;
  }
  if ((int32_t)(o.nextAt - now) > (int32_t)MAIL_RESEND_S) {  // la proxima vez, mas lejos que un reintento
    o.nextAt = now;
    cambio = true;
  }
  return cambio;
}

void mailOutSent(MailOut &o, uint32_t now) {
  o.tries++;
  if (o.msg.type == MT_VISIT || o.msg.type == MT_ACK) {  // no esperan acuse
    o.on = false;
    return;
  }
  o.nextAt = now + MAIL_RESEND_S;
}

bool mailOutAcked(MailOut &o, uint32_t ackOf) {
  if (!o.on || o.msg.id != ackOf) return false;
  o.on = false;
  return true;
}

int mailQueueAck(MailOut *q, int n, uint64_t to, uint64_t from, uint32_t ackOf, uint32_t id, uint32_t now) {
  for (int i = 0; i < n; i++)
    if (q[i].on && q[i].msg.type == MT_ACK && q[i].to == to && q[i].msg.ackOf == ackOf) return 0;
  for (int i = 0; i < n; i++) {
    if (q[i].on) continue;
    MailOut &o = q[i];
    o = MailOut();
    o.on = true;
    o.to = to;
    o.firstAt = o.nextAt = now;
    o.msg.type = MT_ACK;
    o.msg.from = from;
    o.msg.id = id;
    o.msg.ackOf = ackOf;
    return 1;
  }
  return -1;
}

bool MailSeen::seen(uint32_t id) const {
  for (uint8_t i = 0; i < n; i++)
    if (ids[i] == id) return true;
  return false;
}

void MailSeen::add(uint32_t id) {
  if (seen(id)) return;
  ids[pos] = id;
  pos = (uint8_t)((pos + 1) % MAIL_SEEN);
  if (n < MAIL_SEEN) n++;
}

bool MailSeen::repair() {
  if (n <= MAIL_SEEN && pos < MAIL_SEEN) return false;
  *this = MailSeen();
  return true;
}

bool mailParseLine(const char *line, uint32_t &time, char *message, size_t cap) {
  JsonDocument d;
  if (deserializeJson(d, line)) return false;
  const char *ev = d["event"] | "";
  const char *msg = d["message"] | (const char *)nullptr;
  if (strcmp(ev, "message") || !msg || !cap) return false;
  time = d["time"] | 0u;
  strncpy(message, msg, cap - 1);
  message[cap - 1] = 0;
  return true;
}
