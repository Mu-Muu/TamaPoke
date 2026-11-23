#include "wifisync.h"
#include "rtcbat.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WebSocketsClient.h>
#include <Preferences.h>
#include <time.h>
#include <esp_sntp.h>
#include "walltime.h"
#include "config.h"
#include "i18n.h"  // gLang: la pagina del portal sale en el idioma de la placa
#include "wifipick.h"
#include "phonesteps.h"  // pasos del movil (servidor en la red de casa)
#include "mailbox.h"     // correo con amigos lejos
#include <mbedtls/platform.h>
#include <esp_heap_caps.h>

static WebServer *server = nullptr;
static DNSServer *dns = nullptr;
static bool guardado = false;
static uint32_t portalHasta = 0;
static volatile bool portalOn = false;
static volatile bool linkOn = false;  // la pagina "见面" tiene la radio (link.cpp)
#define PORTAL_MS (5UL * 60 * 1000)  // el punto de acceso se cierra solo a los 5 min

// la radio es de uno solo: la tarea de red, la sincronizacion del ajedrez y el
// portal se turnan con este cerrojo
static SemaphoreHandle_t netMx = nullptr;
static void netLock() { if (netMx) xSemaphoreTake(netMx, portMAX_DELAY); }
static void netUnlock() { if (netMx) xSemaphoreGive(netMx); }

static String credGet(const char *key) {
  Preferences p;
  p.begin("wifi", true);
  String s = p.getString(key, "");
  p.end();
  return s;
}

// hasta tres redes; la primera con las claves de siempre ("ssid"/"pass"). Los
// campos del formulario se llaman igual que las claves.
static const char *const KEY_SSID[WIFI_SLOTS] = { "ssid", "ssid2", "ssid3" };
static const char *const KEY_PASS[WIFI_SLOTS] = { "pass", "pass2", "pass3" };

bool wifiHasCreds() {
  for (uint8_t i = 0; i < WIFI_SLOTS; i++)
    if (credGet(KEY_SSID[i]).length()) return true;
  return false;
}
bool mcpConfigured() { return credGet("mcp").length() > 0; }

// el lugar (NVS "place"): q = la ciudad como se escribio en el portal, hasta que se busca; name,
// lat, lon y tz (IANA) = lo encontrado; auto = como: 0 escrito, 1 por la conexion, 2 la ciudad
// de la zona del movil; ptz = la zona del movil; off = el desfase de la ultima vez que llego el
// tiempo. Sin nada, el de config.h (WX_*, TZ_STD_OFFSET_H...)
static String lugarGet(const char *key) {
  Preferences p;
  p.begin("place", true);
  String s = p.getString(key, "");
  p.end();
  return s;
}

static String htmlEsc(const String &s) {
  String o;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') o += "&amp;";
    else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;";
    else o += c;
  }
  return o;
}

// ---------- pagina de configuracion ----------

// La pagina sale en el idioma de la placa (el que se esta leyendo al abrir el
// portal). No va en i18n.cpp: la pinta el navegador del movil, y los textos
// chinos de i18n.cpp acaban en la fuente del firmware. Manda tambien la zona
// horaria del movil (ptz, la de sus ajustes: no hace falta red), para el reloj
// de las placas sin lugar propio (buscarPorRed).
static const char PAGINA_EN[] PROGMEM = R"(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>TamaPoke</title>
<style>body{font-family:sans-serif;max-width:420px;margin:24px auto;padding:0 16px}
input{width:100%;padding:10px;margin:6px 0 14px;font-size:16px;box-sizing:border-box}
input[type=checkbox]{width:auto;margin:0 8px 0 0}
button{width:100%;padding:12px;font-size:17px}small{color:#666}</style></head><body>
<h2>TamaPoke Wi-Fi</h2><p>Your Wi-Fi: the board uses it to sync chess.com points every night and to
get the weather.</p>
<form method="post" action="/save"><p><b>Wi-Fi 1</b>{1}</p>
<label>Name</label><input name="ssid"><label>Password</label><input name="pass" type="password">
<p><b>Wi-Fi 2</b> (optional){2}</p>
<label>Name</label><input name="ssid2"><label>Password</label><input name="pass2" type="password">
<p><b>Wi-Fi 3</b> (optional){3}</p>
<label>Name</label><input name="ssid3"><label>Password</label><input name="pass3" type="password">
<small>Up to three networks, e.g. home, school and a phone hotspot: the board joins the
strongest one it can see. 2.4 GHz only: on an iPhone hotspot, turn on Maximize
Compatibility.</small><br><br>
<label>City (optional)</label>
<input name="city" maxlength="60" placeholder="e.g. Lyon, France">
<small>Sets the clock's time zone and where the weather comes from. If several places share the
name, add the state or country after a comma (Springfield, IL). Now: {P}</small><br><br>
<label>chess.com username (optional)</label>
<input name="chess" maxlength="25" autocapitalize="none" autocorrect="off" spellcheck="false">
<small>Turns your chess.com games into points. The first sync (and the first one after a new
username) only records your current results; earlier games don't pay.</small><br><br>
<label><input type="checkbox" name="easy" value="1"{E}>Easy regions</label><br>
<small>For a faster game: the next region opens once 10 Pokémon of a region are registered,
instead of every common and rare one. If you turn it off later, regions already open stay
open.</small><br><br>
<label>Stack-chan voice control (optional)</label>
<input name="mcp" placeholder="wss://api.xiaozhi.me/mcp/?token=...">
<small>Only if you have a Stack-chan or another xiaozhi voice assistant: paste the MCP endpoint
from its app. Otherwise leave this empty.</small><br><br>
<small>Leave a field empty to keep what was saved before. To remove a Wi-Fi network (in its name),
the city, the chess.com username or the voice control endpoint, enter a single -</small><br><br>
<button>Save</button><input type="hidden" name="ptz" id="ptz"></form>
<script>try{document.getElementById("ptz").value=Intl.DateTimeFormat().resolvedOptions().timeZone||""}catch(e){}</script>
</body></html>)";

static const char PAGINA_ZH[] PROGMEM = R"(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>TamaPoke</title>
<style>body{font-family:sans-serif;max-width:420px;margin:24px auto;padding:0 16px}
input{width:100%;padding:10px;margin:6px 0 14px;font-size:16px;box-sizing:border-box}
input[type=checkbox]{width:auto;margin:0 8px 0 0}
button{width:100%;padding:12px;font-size:17px}small{color:#666}</style></head><body>
<h2>TamaPoke Wi-Fi</h2><p>填 Wi-Fi，用来每晚同步 chess.com 的积分和更新天气。</p>
<form method="post" action="/save"><p><b>Wi-Fi 1</b>{1}</p>
<label>名称</label><input name="ssid"><label>密码</label><input name="pass" type="password">
<p><b>Wi-Fi 2</b>（可不填）{2}</p>
<label>名称</label><input name="ssid2"><label>密码</label><input name="pass2" type="password">
<p><b>Wi-Fi 3</b>（可不填）{3}</p>
<label>名称</label><input name="ssid3"><label>密码</label><input name="pass3" type="password">
<small>最多三个，比如家里、学校和手机热点：板子会连能看到的、信号最强的那个。只支持 2.4 GHz：iPhone 热点要打开"最大兼容性"。</small><br><br>
<label>城市（可不填）</label>
<input name="city" maxlength="60" placeholder="例如：上海，或 Lyon, France">
<small>用来设定时钟的时区和天气的地点。有重名的城市，可以在逗号后面加上省、州或国家（Springfield, IL）。现在：{P}</small><br><br>
<label>chess.com 用户名（可不填）</label>
<input name="chess" maxlength="25" autocapitalize="none" autocorrect="off" spellcheck="false">
<small>用来把 chess.com 上的对局换成积分。第一次同步（或换了用户名后）只记录当前成绩，不补发以前的分。</small><br><br>
<label><input type="checkbox" name="easy" value="1"{E}>轻松地区</label><br>
<small>想玩得快一点：一个地区登记 10 只宝可梦，下一个地区就开放，不用把普通和稀有的都登记齐。之后再取消，已经开放的地区也不会关上。</small><br><br>
<label>Stack-chan 语音控制（可选）</label>
<input name="mcp" placeholder="wss://api.xiaozhi.me/mcp/?token=...">
<small>只有 Stack-chan 或其他小智语音设备才需要：把它 App 里 MCP 页面的接入点地址粘贴到这里。没有就留空。</small><br><br>
<small>不改的项留空就行（会保留原来的）。要删掉某个 Wi-Fi（在它的名称里填）、城市、chess.com 用户名或语音控制地址，填一个减号 -</small><br><br>
<button>保存</button><input type="hidden" name="ptz" id="ptz"></form>
<script>try{document.getElementById("ptz").value=Intl.DateTimeFormat().resolvedOptions().timeZone||""}catch(e){}</script>
</body></html>)";

// respuestas de /save, en el orden de Lang (EN, ZH)
static const char *const MSG_CHESS[LANG_COUNT] = {
  "<meta charset=utf-8><p>A chess.com username has only letters, numbers, _ and - (3 to 25 of them). "
  "Please go back and fix it.</p>",
  "<meta charset=utf-8><p>chess.com 用户名只能有字母、数字、_ 和 -（3 到 25 个），请返回重填。</p>",
};
static const char *const MSG_BAD[LANG_COUNT] = {
  "<meta charset=utf-8><p>Something isn't right. Please go back and try again.</p>",
  "<meta charset=utf-8><p>填的不对，请返回重填。</p>",
};
static const char *const MSG_SAVED[LANG_COUNT] = {
  "<meta charset=utf-8><h2>Saved</h2><p>You can leave this hotspot now.</p>",
  "<meta charset=utf-8><h2>已保存</h2><p>可以断开这个热点了。</p>",
};

// regiones faciles (issue #13): la casilla sale como la tiene el juego y queda como la dejen
static bool easyPortal = false;

// la pagina con cada red marcada si ya hay una guardada (sin decir cual)
static void paginaPortal() {
  String page(gLang == LANG_ZH ? PAGINA_ZH : PAGINA_EN);
  for (uint8_t i = 0; i < WIFI_SLOTS; i++) {
    char marca[4] = { '{', (char)('1' + i), '}', 0 };
    page.replace(marca, credGet(KEY_SSID[i]).length() ? (gLang == LANG_ZH ? "（已保存）" : " - saved") : "");
  }
  String q = lugarGet("q"), lugar = lugarGet("name");
  bool zh = gLang == LANG_ZH;
  if (q.length()) {
    lugar = htmlEsc(q) + (zh ? "（联网后查找）" : " (looked up once the board is online)");
  } else if (lugar.length()) {
    Preferences p;
    p.begin("place", true);
    uint8_t como = p.getUChar("auto", 0);
    p.end();
    if (como == 1) lugar = htmlEsc(lugar) + (zh ? "（按网络自动定位）" : " (found from your internet connection)");
    else if (como == 2) lugar = htmlEsc(lugar) + (zh ? "（按手机的时区）" : " (from your phone's time zone)");
    else lugar = htmlEsc(lugar);
  } else {
    lugar = WX_DEFAULT_PLACE ? (zh ? "纽约，联网后自动定位" : "New York, until the board finds where it is online")
                             : (zh ? "固件里的默认地点" : "the built-in default");
  }
  page.replace("{P}", lugar);
  page.replace("{E}", easyPortal ? " checked" : "");
  server->send(200, "text/html; charset=utf-8", page);
}

static void paginaGuardado() {
  String ssid[WIFI_SLOTS], pass[WIFI_SLOTS];
  for (uint8_t i = 0; i < WIFI_SLOTS; i++) {
    ssid[i] = server->arg(KEY_SSID[i]);
    pass[i] = server->arg(KEY_PASS[i]);
    ssid[i].trim();
  }
  String mcp = server->arg("mcp");
  String chess = server->arg("chess");
  String city = server->arg("city");
  String ptz = server->arg("ptz");
  mcp.trim();
  chess.trim();
  city.replace("，", ",");  // la coma china: el geocodificador solo entiende la normal
  city.trim();
  char usuario[CHESS_USER_MAX + 1] = "";
  if (chess.length() && chess != "-" && !chessNormUser(chess.c_str(), usuario, sizeof(usuario))) {
    server->send(400, "text/html; charset=utf-8", MSG_CHESS[gLang]);
    return;
  }
  // como quedan las redes: "-" la borra, un nombre la pone, vacio la deja como estaba;
  // tiene que quedar al menos una
  bool quedaRed = false, mal = false;
  for (uint8_t i = 0; i < WIFI_SLOTS; i++) {
    if (ssid[i].length() > 32 || pass[i].length() > 64) mal = true;
    if (ssid[i] != "-" && (ssid[i].length() || credGet(KEY_SSID[i]).length())) quedaRed = true;
  }
  if (mal || !quedaRed || mcp.length() > 400 || city.length() > 180 ||
      (mcp.length() > 1 && !mcp.startsWith("wss://") && !mcp.startsWith("ws://"))) {
    server->send(400, "text/html; charset=utf-8", MSG_BAD[gLang]);
    return;
  }
  Preferences p;
  p.begin("wifi", false);
  for (uint8_t i = 0; i < WIFI_SLOTS; i++) {
    if (ssid[i] == "-") {
      p.remove(KEY_SSID[i]);
      p.remove(KEY_PASS[i]);
    } else if (ssid[i].length()) {
      p.putString(KEY_SSID[i], ssid[i]);
      p.putString(KEY_PASS[i], pass[i]);
    }
  }
  if (mcp == "-") p.remove("mcp");
  else if (mcp.length()) p.putString("mcp", mcp);
  p.end();
  if (chess == "-") chessSaveUser("");  // vuelve a valer la de config_private.h (si la hay)
  else if (usuario[0]) chessSaveUser(usuario);
  if (city.length()) {
    p.begin("place", false);
    if (city == "-") {  // vuelve el lugar de config.h
      p.clear();
      wallClearCityOffset();
    } else {
      p.putString("q", city);  // se busca al conectar (buscarLugar)
    }
    p.end();
  }
#if WX_DEFAULT_PLACE
  if (zoneOk(ptz.c_str())) {  // la del movil: la hora de las placas sin lugar propio
    p.begin("place", false);
    if (p.getString("ptz", "") != ptz) p.putString("ptz", ptz);
    p.end();
  }
#endif
  easyPortal = server->arg("easy") == "1";  // la casilla: sin marcar no llega
  server->send(200, "text/html; charset=utf-8", MSG_SAVED[gLang]);
  guardado = true;
}

static void portalCleanup() {
  if (server) { server->stop(); delete server; server = nullptr; }
  if (dns) { dns->stop(); delete dns; dns = nullptr; }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

static void mcpStop();

void wifiPortalStart(bool easyRegions) {
  easyPortal = easyRegions;
  netLock();  // espera a que la tarea de red suelte la radio
  portalOn = true;
  mcpStop();
  phoneStepsStop();  // el portal usa el puerto 80
  WiFi.disconnect(true);
  netUnlock();
  portalCleanup();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_NAME);
  dns = new DNSServer();
  dns->start(53, "*", WiFi.softAPIP());  // cualquier dominio -> la pagina (portal cautivo)
  server = new WebServer(80);
  server->on("/save", HTTP_POST, paginaGuardado);
  server->onNotFound(paginaPortal);
  server->begin();
  guardado = false;
  portalHasta = millis() + PORTAL_MS;
}

bool wifiPortalActive() { return server != nullptr; }
bool wifiPortalEasy() { return easyPortal; }

bool wifiPortalLoop() {
  if (!server) return false;
  dns->processNextRequest();
  server->handleClient();
  if ((int32_t)(millis() - portalHasta) > 0) wifiPortalStop();
  return guardado;
}

static volatile bool wxNow = true;  // pedir el tiempo en la proxima vuelta

static volatile bool redBuscada = false;  // ya se miro donde esta la conexion (buscarPorRed)

void wifiPortalStop() {
  portalCleanup();
  portalOn = false;
  wxNow = true;  // red nueva (o la misma): tiempo al momento
  redBuscada = false;  // y, sin lugar propio, donde esta esa red (buscarPorRed)
}

// la pagina "见面" (ESP-NOW, link.cpp) necesita la radio en un canal fijo: como el portal,
// se para lo de casa (Stack-chan, el servidor del movil) y se desconecta. La tarea de red no
// vuelve a conectar hasta netRadioGive
void netRadioTake() {
  netLock();
  linkOn = true;
  mcpStop();
  phoneStepsStop();
  WiFi.disconnect(true);
  netUnlock();
}

void netRadioGive() { linkOn = false; }  // link.cpp ya apago el Wi-Fi: la tarea vuelve a conectar sola

// El bucle principal va a dormir la CPU (light sleep). El wifi no lo aguanta: la radio se queda sin
// reloj en mitad de una conexion y los plazos de la red dejan de correr. Solo se duerme con la radio
// apagada, y con el cerrojo cogido para que la tarea de red no la encienda mientras tanto.
bool netSleepTake() {
  if (netMx && xSemaphoreTake(netMx, 0) != pdTRUE) return false;  // alguien esta con la radio
  if (WiFi.getMode() == WIFI_OFF) return true;
  netUnlock();
  return false;
}
void netSleepGive() { netUnlock(); }

// ---------- conexion ----------

static volatile bool usbPower = false;
void netSetUsb(bool usb) { usbPower = usb; }
// con USB la radio se queda encendida: Stack-chan (si hay punto MCP) puede hablar en
// cualquier momento y el movil puede mandar los pasos; con bateria solo se conecta un momento
static bool keepOnline() { return usbPower; }

static int8_t redBuena = -1;  // hueco de la ultima red con la que se conecto (en RAM)

static bool probarRed(const String &ssid, const String &pass, uint32_t ms) {
  WiFi.begin(ssid.c_str(), pass.c_str());
  for (uint32_t t = 0; t < ms && WiFi.status() != WL_CONNECTED; t += 100) delay(100);
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.disconnect();
  return false;
}

static bool conectar() {
  if (WiFi.status() == WL_CONNECTED) return true;
  String ssid[WIFI_SLOTS], pass[WIFI_SLOTS];
  uint8_t guardadas = 0, unica = 0;
  for (uint8_t i = 0; i < WIFI_SLOTS; i++) {
    ssid[i] = credGet(KEY_SSID[i]);
    pass[i] = credGet(KEY_PASS[i]);
    if (ssid[i].length()) {
      guardadas++;
      unica = i;
    }
  }
  if (!guardadas) return false;
  WiFi.mode(WIFI_STA);
  if (guardadas == 1) {  // una sola: como siempre, sin escanear (vale tambien para una red oculta)
    if (!probarRed(ssid[unica], pass[unica], 15000)) return false;
    redBuena = (int8_t)unica;
    return true;
  }
  // varias: se escanea (unos 2-3 s) y se prueban las que se ven, de mas a menos senal
  int16_t n = WiFi.scanNetworks();
  if (n < 0) n = 0;
  if (n > 32) n = 32;
  String vistas[32];
  const char *seen[32];
  int16_t rssi[32];
  for (int16_t j = 0; j < n; j++) {
    vistas[j] = WiFi.SSID(j);
    seen[j] = vistas[j].c_str();
    rssi[j] = (int16_t)WiFi.RSSI(j);
  }
  WiFi.scanDelete();
  // que redes guardadas se ven y con cuanta senal (sin nombres en el registro)
  Serial.printf("wifi: escaneo %d redes;", n);
  for (uint8_t i = 0; i < WIFI_SLOTS; i++) {
    if (!ssid[i].length()) continue;
    int16_t mejor = INT16_MIN;
    for (int16_t j = 0; j < n; j++)
      if (vistas[j] == ssid[i] && rssi[j] > mejor) mejor = rssi[j];
    if (mejor == INT16_MIN) Serial.printf(" red %u no se ve;", i + 1);
    else Serial.printf(" red %u %d dBm;", i + 1, mejor);
  }
  Serial.println();
  const char *saved[WIFI_SLOTS];
  for (uint8_t i = 0; i < WIFI_SLOTS; i++) saved[i] = ssid[i].c_str();
  uint8_t orden[WIFI_SLOTS];
  uint8_t m = wifiOrder(saved, seen, rssi, n, redBuena, orden);
  for (uint8_t k = 0; k < m; k++) {
    if (probarRed(ssid[orden[k]], pass[orden[k]], 12000)) {
      redBuena = (int8_t)orden[k];
      Serial.printf("wifi: conectado (red %u de %u)\n", orden[k] + 1, guardadas);  // sin nombres
      return true;
    }
  }
  return false;
}

static void desconectarSiSobra() {
  if (keepOnline()) return;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// hora real por NTP. Devuelve el UTC y deja el RTC en hora local (el firmware
// guarda la hora local "de pared" como epoch, ver rtcbat.cpp)
static time_t ponerEnHora() {
  configTzTime("UTC0", "pool.ntp.org", "time.google.com");  // TZ sigue en UTC (rtcbat.cpp cuenta con ello)
  time_t utc = 0;
  for (int i = 0; i < 100 && (utc = time(nullptr)) < 1700000000; i++) delay(100);
  if (utc < 1700000000) return 0;
  rtcSetEpoch(wallFromUtc((uint32_t)utc));  // con el desfase de la ciudad o la zona de config.h (walltime.cpp)
  return utc;
}

static bool bajarJson(const String &url, JsonDocument &doc, JsonDocument *filtro) {
  WiFiClientSecure cliente;
  cliente.setInsecure();  // datos publicos y de solo lectura
  HTTPClient http;
  http.useHTTP10(true);   // sin "chunked": ArduinoJson lee directo del flujo
  // chess.com pide que el user-agent diga como contactar con quien lo hizo (API publica, desde 2023)
  http.setUserAgent("TamaPoke/2026.11 (+https://github.com/Mu-Muu/TamaPoke)");
  if (!http.begin(cliente, url)) return false;
  // en el registro solo el servidor: ni la consulta (la ciudad, las coordenadas) ni la ruta (la de
  // chess.com lleva el usuario)
  int h0 = url.indexOf("://");
  h0 = h0 < 0 ? 0 : h0 + 3;
  int h1 = url.indexOf('/', h0), q = url.indexOf('?', h0);
  if (h1 < 0 || (q >= 0 && q < h1)) h1 = q;
  String sitio = h1 < 0 ? url.substring(h0) : url.substring(h0, h1);
  int code = http.GET();
  bool ok = false;
  if (code == 200) {
    // Se baja entero a PSRAM y luego se analiza: leyendo directo del flujo TLS,
    // ArduinoJson veia el final antes de tiempo (IncompleteInput) en cuanto
    // el trozo siguiente tardaba en llegar.
    int len = http.getSize();  // -1 si el servidor no manda Content-Length
    size_t cap = len > 0 ? (size_t)len : 600000;
    char *buf = (char *)ps_malloc(cap + 1);
    size_t n = 0;
    if (buf) {
      WiFiClient *st = http.getStreamPtr();  // nulo si ya cerraron sin mandar nada
      uint32_t t = millis();
      while (st && (len < 0 || n < (size_t)len) && n < cap && millis() - t < 20000) {
        int a = st->available();
        size_t r = a > 0 ? st->readBytes(buf + n, min((size_t)a, cap - n)) : 0;
        if (r > 0) {
          n += r;
          t = millis();
        } else if (a > 0) {
          delay(5);  // decia que habia algo y no dio nada: sin esto se daba vueltas sin parar
        } else if (!st->connected()) {
          break;
        } else {
          delay(5);
        }
      }
      buf[n] = 0;
      DeserializationError e = filtro
          ? deserializeJson(doc, buf, n, DeserializationOption::Filter(*filtro))
          : deserializeJson(doc, buf, n);
      ok = !e;
      Serial.printf("bajado %u/%d bytes %s%s\n", (unsigned)n, len, sitio.c_str(), e ? " (json mal)" : "");
      if (e) Serial.printf("json: %s\n", e.c_str());
      free(buf);
    }
  } else {
    Serial.printf("http %d %s\n", code, sitio.c_str());
  }
  http.end();
  return ok;
}

// ---------- ajedrez ----------

uint32_t chessSyncDay() {
  Preferences p;
  p.begin("chess", true);
  uint32_t d = p.getUInt("sday", 0);
  p.end();
  return d;
}

static bool chessSyncLocked(Pet &pet, ChessReport &rep) {
  rep = ChessReport();
  char usuario[CHESS_USER_MAX + 1];
  if (!chessAccount(usuario, sizeof(usuario))) {  // ni en el portal ni en config_private.h
    Serial.println("sync: sin usuario de chess.com");
    return false;
  }
  if (!conectar()) {
    Serial.println("sync: sin wi-fi");
    desconectarSiSobra();
    return false;
  }
  time_t utc = ponerEnHora();
  uint32_t pared = rtcEpoch();
  pet.setClock(pared);  // hora corregida, sin aplicar progresion
  ChessState st;
  chessLoad(st);
  bool ok = utc != 0;
  // primera vez (o cuenta nueva): solo se apuntan las marcas y se cuenta desde
  // la medianoche local de hoy
  if (ok && chessPrepare(st, usuario, CHESS_USER, (uint32_t)utc - (pared % 86400)))
    Serial.println("sync: cuenta nueva, se vuelven a tomar las marcas");

  String base = String("https://api.chess.com/pub/player/") + usuario;
  JsonDocument stats;
  ok = ok && bajarJson(base + "/stats", stats, nullptr);
  if (ok) chessScoreStats(stats.as<JsonVariantConst>(), st, rep);

  // archivos mensuales desde el de la marca de agua hasta el actual (UTC)
  if (ok) {
    JsonDocument filtro;
    chessArchiveFilter(filtro);
    time_t desde = st.watermark;
    struct tm a, b;
    gmtime_r(&desde, &a);
    gmtime_r(&utc, &b);
    int y = a.tm_year + 1900, m = a.tm_mon + 1;
    int yFin = b.tm_year + 1900, mFin = b.tm_mon + 1;
    for (int n = 0; n < 3 && ok && (y < yFin || (y == yFin && m <= mFin)); n++) {
      char url[24];
      snprintf(url, sizeof(url), "/games/%04d/%02d", y, m);
      JsonDocument mes;
      ok = bajarJson(base + url, mes, &filtro);
      if (ok) chessScoreArchive(mes.as<JsonVariantConst>(), st, rep);
      if (ok) chessArchiveDone(st, y, m, yFin, mFin);  // mes cerrado: la proxima vez, desde el siguiente
      if (++m > 12) { m = 1; y++; }
    }
  }
  desconectarSiSobra();
  if (!ok) return false;

  chessCommit(st, rep);
  if (rep.total() > 0) {
    pet.chessPtsTotal += rep.total();
    pet.earn(rep.total(), PT_CHESS);  // al libro de puntos como ajedrez
  }
  pet.chessMedals(rep.wins, rep.newBestRapid || rep.newBestDaily || rep.hundreds ||
                                rep.newTactics || rep.newRush);
  // los puntos a la NVS antes que la marca de agua: si se va la luz en medio, como mucho se cuentan
  // otra vez; al reves (marca guardada, puntos aun sin volcar) se perdian para siempre
  pet.flushSave();
  chessSave(st);
  chessSaveReport(rep, pared / 86400);
  Preferences p;
  p.begin("chess", false);
  p.putUInt("sday", pared / 86400);
  p.end();
  Serial.printf("sync: %u partidas, %u victorias, +%ld (records +%ld) -> %ld puntos\n", rep.games,
                rep.wins, (long)rep.total(), (long)rep.bonusPts, (long)pet.points);
  return true;
}

bool chessSync(Pet &pet, ChessReport &rep) {
  netLock();
  bool ok = chessSyncLocked(pet, rep);
  netUnlock();
  return ok;
}

// ---------- hora por NTP (al arrancar y cada hora, junto con el tiempo) ----------
//
// Sin tocar TZ (ver walltime.h): solo se apunta la UTC recibida y el bucle
// principal corrige el reloj (netTimeTake). La sincronizacion del ajedrez pide la
// hora por su cuenta (ponerEnHora); el aviso de hora sirve para las dos.
static portMUX_TYPE ntpMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t ntpUtc = 0, ntpMs = 0, ntpCount = 0;
static uint32_t ntpTaken = 0;

static void horaRecibida(struct timeval *tv) {
  portENTER_CRITICAL(&ntpMux);
  ntpUtc = (uint32_t)tv->tv_sec;
  ntpMs = millis();
  ntpCount++;
  portEXIT_CRITICAL(&ntpMux);
}

// con la red ya conectada: pide la hora y espera la respuesta (hasta 5 s)
static void pedirHora() {
  sntp_set_time_sync_notification_cb(horaRecibida);
  if (!esp_sntp_enabled()) {
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_init();
  } else {
    esp_sntp_restart();
  }
  uint32_t antes = ntpCount, t0 = millis();
  while (ntpCount == antes && millis() - t0 < 5000) delay(50);
}

bool netTimeTake(uint32_t &utc) {
  portENTER_CRITICAL(&ntpMux);
  bool nueva = ntpCount != ntpTaken;
  ntpTaken = ntpCount;
  uint32_t u = ntpUtc, ms = ntpMs;
  portEXIT_CRITICAL(&ntpMux);
  if (!nueva) return false;
  utc = u + (millis() - ms) / 1000;  // lo que ha pasado desde que llego
  return true;
}

// ---------- tiempo ----------

static portMUX_TYPE wxMux = portMUX_INITIALIZER_UNLOCKED;
static Weather gWx;
static uint32_t gWxMs = 0;

Weather weatherNow() {
  portENTER_CRITICAL(&wxMux);
  Weather w = gWx;
  uint32_t at = gWxMs;
  portEXIT_CRITICAL(&wxMux);
  if (w.valid && millis() - at > WX_STALE_MS) w.valid = false;
  return w;
}

void weatherRefresh() { wxNow = true; }

// la ciudad nueva del portal: se busca y se guarda lo encontrado; su desfase llega con el
// tiempo, justo despues. Sin red o si el servicio no contesta, se queda pendiente
static volatile uint8_t lugarAviso = 0;  // para el bucle principal: 1 = puesta, 2 = no se encontro

uint8_t netPlaceNews() {
  uint8_t a = lugarAviso;
  lugarAviso = 0;
  return a;
}

// lo encontrado (p abierta para escribir); el desfase llega con el tiempo, justo despues
static void guardarLugar(Preferences &p, const Place &pl, uint8_t como) {
  p.putString("name", pl.name);
  p.putString("lat", pl.lat);
  p.putString("lon", pl.lon);
  p.putString("tz", pl.tz);
  p.putUChar("auto", como);
  p.remove("off");  // el del lugar de antes ya no vale
  wallClearCityOffset();
}

// el geocodificador de Open-Meteo: -1 = no contesto, 0 = no hay ese sitio, 1 = encontrado
static int8_t geocodificar(const char *nombre, Place &pl) {
  static char enc[560];  // hasta 180 bytes, cada uno como %XX
  urlEncode(nombre, enc, sizeof(enc));
  String url = String("https://geocoding-api.open-meteo.com/v1/search?count=1&format=json&language=") +
               (gLang == LANG_ZH ? "zh" : "en") + "&name=" + enc;
  JsonDocument doc;
  if (!bajarJson(url, doc, nullptr)) return -1;
  return placeParse(doc.as<JsonVariantConst>(), pl) ? 1 : 0;
}

static void buscarLugar() {
  String q = lugarGet("q");
  if (!q.length()) return;
  Place pl;
  int8_t r = geocodificar(q.c_str(), pl);
  if (r < 0) return;
  bool ok = r > 0;
  Preferences p;
  p.begin("place", false);
  p.remove("q");
  if (ok) guardarLugar(p, pl, 0);
  p.end();
  lugarAviso = ok ? 1 : 2;
  Serial.printf("lugar: %s\n", ok ? "puesto" : "no encontrado");  // sin el nombre
}

// Sin lugar propio en config.h (el instalador web) ni ciudad escrita: el de la conexion a
// internet, por la IP (ipapi.co: ciudad, coordenadas y zona; ve la IP, como todos los
// servicios a los que llama la placa). La salida a internet del proveedor puede estar lejos,
// hasta en otra zona (en la prueba, una hora de menos): si la zona no es la del movil, el
// tiempo sale de la ciudad que nombra la zona del movil y la hora, de esa zona. Una vez por
// arranque y otra tras cerrar el portal (puede ser otra red); si falla, con el proximo tiempo
static void buscarPorRed() {
#if WX_DEFAULT_PLACE
  if (redBuscada || lugarGet("q").length()) return;
  Preferences p;
  p.begin("place", true);
  bool escrita = p.isKey("lat") && p.getUChar("auto", 0) == 0;
  p.end();
  if (escrita) {  // la ciudad del portal manda
    redBuscada = true;
    return;
  }
  String ptz = lugarGet("ptz");
  JsonDocument doc;
  Place pl;
  bool hay = bajarJson("https://ipapi.co/json/", doc, nullptr) && placeFromIp(doc.as<JsonVariantConst>(), pl);
  uint8_t como = 1;
  if (ptz.length() && (!hay || ptz != pl.tz)) {
    char ciudad[40];
    zoneCity(ptz.c_str(), ciudad, sizeof(ciudad));
    Place zc;
    if (geocodificar(ciudad, zc) > 0 && ptz == zc.tz) {
      pl = zc;
      hay = true;
      como = 2;
    } else if (hay) {
      snprintf(pl.tz, sizeof(pl.tz), "%s", ptz.c_str());  // el sitio de la conexion, la hora del movil
    }
  }
  if (!hay) {
    Serial.println("lugar: la conexion no dice donde esta");
    return;
  }
  redBuscada = true;
  if (lugarGet("name") == pl.name && lugarGet("tz") == pl.tz) return;  // el de siempre: se queda su desfase
  p.begin("place", false);
  guardarLugar(p, pl, como);
  p.end();
  lugarAviso = 1;
  Serial.printf("lugar: puesto por %s\n", como == 2 ? "la zona del movil" : "la conexion");  // sin el nombre
#endif
}

static bool bajarTiempo() {
  String lat = lugarGet("lat"), lon = lugarGet("lon"), tz = lugarGet("tz");
  bool lugar = lat.length() && lon.length() && tz.length();
  String ptz = WX_DEFAULT_PLACE ? lugarGet("ptz") : String();
  if (!lugar) {  // el de config.h; con la zona del movil, si la hay (la hora, al menos, buena)
    lat = WX_LAT;
    lon = WX_LON;
    tz = ptz.length() ? ptz : String(WX_TZ);
  }
  bool ciudad = lugar || ptz.length();  // la hora sale del desfase que trae el tiempo
  tz.replace("/", "%2F");  // WX_TZ ya viene asi
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=") + lat + "&longitude=" + lon +
               "&current=weather_code,temperature_2m&daily=sunrise,sunset&timezone=" + tz + "&forecast_days=1";
  JsonDocument doc;
  Weather w;
  if (!bajarJson(url, doc, nullptr) || !weatherParse(doc.as<JsonVariantConst>(), w)) return false;
  if (ciudad && w.utcOffset != INT32_MIN) {  // la hora de la ciudad, con su horario de verano de hoy
    wallSetCityOffset(w.utcOffset);
    Preferences p;
    p.begin("place", false);
    if (p.getInt("off", INT32_MIN) != w.utcOffset) p.putInt("off", w.utcOffset);  // solo si cambia (flash)
    p.end();
  }
  portENTER_CRITICAL(&wxMux);
  gWx = w;
  gWxMs = millis();
  portEXIT_CRITICAL(&wxMux);
  Serial.printf("tiempo: %u %dC sol %d-%d\n", w.kind, w.tempC, w.sunriseMin, w.sunsetMin);
  return true;
}

// ---------- Stack-chan (xiaozhi MCP por WebSocket) ----------
//
// El punto de acceso de xiaozhi.me es un WebSocket por el que van mensajes
// JSON-RPC de MCP, uno por trama de texto. La placa hace de servidor MCP:
// contesta initialize, tools/list y tools/call (y ping).

static WebSocketsClient *ws = nullptr;
static volatile bool wsUp = false;
static QueueHandle_t cmdQ = nullptr, resQ = nullptr;
#define NET_TEXT 800  // respuesta para Stack-chan (en chino: 3 bytes por caracter)
struct NetRes {
  uint32_t seq;
  char text[NET_TEXT];
};
static uint32_t cmdSeq = 0;

bool mcpOnline() { return wsUp; }

bool netTakeCommand(NetCmd &c) { return cmdQ && xQueueReceive(cmdQ, &c, 0) == pdTRUE; }

void netGiveResult(uint32_t seq, const char *text) {
  if (!resQ) return;
  NetRes r;
  r.seq = seq;
  strncpy(r.text, text, sizeof(r.text) - 1);
  r.text[sizeof(r.text) - 1] = 0;
  xQueueOverwrite(resQ, &r);
}

// la orden se ejecuta en el bucle principal (el que toca la mascota y la pantalla)
static bool ejecutar(uint8_t tool, uint8_t a, uint8_t b, char *out, size_t cap) {
  NetCmd c = { ++cmdSeq, tool, a, b };
  NetRes r;
  xQueueReset(resQ);
  xQueueOverwrite(cmdQ, &c);
  uint32_t t0 = millis();
  while (millis() - t0 < 3000) {
    if (xQueueReceive(resQ, &r, pdMS_TO_TICKS(100)) == pdTRUE && r.seq == c.seq) {
      strncpy(out, r.text, cap - 1);
      out[cap - 1] = 0;
      return true;
    }
  }
  xQueueReset(cmdQ);
  strncpy(out, "TamaPoke 正忙，等一下再试。", cap - 1);
  out[cap - 1] = 0;
  return false;
}

static void mcpMensaje(const uint8_t *txt, size_t len) {
  JsonDocument in;
  if (deserializeJson(in, (const char *)txt, len)) return;
  JsonVariantConst id = in["id"];
  if (id.isNull()) return;  // notificacion (notifications/initialized...): no se contesta
  const char *method = in["method"] | "";
  JsonDocument out;
  out["jsonrpc"] = "2.0";
  out["id"] = id;
  if (!strcmp(method, "initialize")) {
    JsonObject r = out["result"].to<JsonObject>();
    r["protocolVersion"] = in["params"]["protocolVersion"] | "2024-11-05";
    r["capabilities"]["tools"].to<JsonObject>();
    r["serverInfo"]["name"] = "TamaPoke";
    r["serverInfo"]["version"] = "1.0";
  } else if (!strcmp(method, "ping")) {
    out["result"].to<JsonObject>();
  } else if (!strcmp(method, "tools/list")) {
    stackchanToolList(out["result"]["tools"].to<JsonArray>());
  } else if (!strcmp(method, "tools/call")) {
    const char *name = in["params"]["name"] | "";
    int tool = stackchanFind(name, in["params"]["arguments"]);
    if (tool < 0) {
      out["error"]["code"] = -32602;
      out["error"]["message"] = "unknown tool or action";
    } else {
      uint8_t a, b;
      stackchanArgs((uint8_t)tool, in["params"]["arguments"], a, b);
      static char texto[NET_TEXT];  // en la tarea de red, una llamada cada vez
      bool ok = ejecutar((uint8_t)tool, a, b, texto, sizeof(texto));
      Serial.printf("mcp: %s\n", name);
      JsonObject r = out["result"].to<JsonObject>();
      JsonObject c = r["content"].to<JsonArray>().add<JsonObject>();
      c["type"] = "text";
      c["text"] = texto;
      r["isError"] = !ok;
    }
  } else {
    out["error"]["code"] = -32601;
    out["error"]["message"] = "method not found";
  }
  String s;
  serializeJson(out, s);
  ws->sendTXT(s);
}

static void mcpEvento(WStype_t type, uint8_t *payload, size_t len) {
  if (type == WStype_CONNECTED) {
    wsUp = true;
    Serial.println("mcp: conectado a xiaozhi");
  } else if (type == WStype_DISCONNECTED) {
    if (wsUp) Serial.println("mcp: desconectado");
    wsUp = false;
  } else if (type == WStype_TEXT) {
    mcpMensaje(payload, len);
  }
}

// "wss://api.xiaozhi.me/mcp/?token=..." -> host, puerto, ruta
static bool mcpStart() {
  String url = credGet("mcp");
  bool ssl = url.startsWith("wss://");
  if (!ssl && !url.startsWith("ws://")) return false;
  String rest = url.substring(ssl ? 6 : 5);
  int slash = rest.indexOf('/');
  String hostPort = slash < 0 ? rest : rest.substring(0, slash);
  String path = slash < 0 ? String("/") : rest.substring(slash);
  uint16_t port = ssl ? 443 : 80;
  int colon = hostPort.indexOf(':');
  String host = hostPort;
  if (colon >= 0) {
    host = hostPort.substring(0, colon);
    port = (uint16_t)hostPort.substring(colon + 1).toInt();
  }
  if (host.length() == 0) return false;
  ws = new WebSocketsClient();
  if (ssl) ws->beginSSL(host.c_str(), port, path.c_str(), "", "");  // sin huella: setInsecure
  else ws->begin(host.c_str(), port, path.c_str(), "");
  ws->onEvent(mcpEvento);
  ws->setReconnectInterval(15000);
  ws->enableHeartbeat(30000, 10000, 3);
  Serial.printf("mcp: conectando a %s\n", host.c_str());  // el token no se imprime
  return true;
}

static void mcpStop() {
  if (!ws) return;
  ws->disconnect();
  delete ws;
  ws = nullptr;
  wsUp = false;
}

// ---------- tarea de red ----------

#define NET_STACK 16384  // bytes: TLS (chess.com, el tiempo, el correo, xiaozhi) va en esta tarea
static TaskHandle_t netTaskH = nullptr;

static void netTask(void *) {
  uint32_t nextWx = 0, nextJoin = 0;
  for (;;) {
    if (portalOn || linkOn || !wifiHasCreds()) {
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }
    netLock();
    // otra vez, ya con el cerrojo: el portal o "见面" pueden haberlo cogido mientras se esperaba
    // (miramos los avisos, se pusieron y nos lo soltaron). Sin esto la tarea volvia a conectar a
    // casa justo cuando ellos acababan de dejar la radio a su gusto
    if (portalOn || linkOn) {
      netUnlock();
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }
    uint32_t now = millis();
    bool keep = keepOnline();
    if (keep) {
      if (WiFi.status() != WL_CONNECTED && (int32_t)(now - nextJoin) >= 0) {
        mcpStop();
        phoneStepsStop();
        if (!conectar()) nextJoin = now + 60000;  // la red no esta: otro intento en 1 min
      }
      if (WiFi.status() == WL_CONNECTED) {
        if (!ws) mcpStart();
        if (ws) ws->loop();
        phoneStepsLoop();
        if (mailSyncDue(true)) mailNetSync();  // correo: cada 15 min con USB (o si lo piden)
      }
    } else if (ws || phoneStepsOn() || WiFi.getMode() != WIFI_OFF) {  // sin USB: se apaga la radio (tambien si se quedo encendida sin llegar a conectar)
      mcpStop();
      phoneStepsStop();
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    }
    // con bateria: de madrugada no (la hora ya se puso a medianoche con el ajedrez)
    bool madrugada = false;
    if (!usbPower) {
      time_t u = time(nullptr);
      if (u > 1700000000) madrugada = (wallFromUtc((uint32_t)u) / 3600) % 24 < WX_BATT_QUIET_UNTIL;
    }
    if (!wxNow && madrugada && (int32_t)(now - nextWx) >= 0) nextWx = now + 30UL * 60 * 1000;
    if (!keep && mailSyncWanted()) {  // con bateria y piden el correo ya ("立即收发" o algo que sale)
      if (conectar()) mailNetSync();
      else mailSyncCancel();  // sin red: no se reintenta en cada vuelta; lo que sale espera al tiempo (3 h)
      desconectarSiSobra();
    }
    if (wxNow || (int32_t)(now - nextWx) >= 0) {
      wxNow = false;
      bool con = conectar();
      if (con) buscarLugar();  // ciudad nueva del portal: el tiempo ya sale de ella
      if (con) buscarPorRed();  // o, sin lugar propio, donde esta la conexion
      bool ok = con && bajarTiempo();  // antes de la hora: trae el desfase de la ciudad
      if (con) pedirHora();
      if (con && !keep) mailNetSync();  // con bateria, el correo va con el tiempo (cada 3 h)
      desconectarSiSobra();
      nextWx = millis() + (ok ? (usbPower ? WX_EVERY_MS : WX_EVERY_BATT_MS) : 15UL * 60 * 1000);
    }
    netUnlock();
    vTaskDelay(pdMS_TO_TICKS(ws || phoneStepsOn() ? 10 : 2000));  // sin nada que atender no hay prisa
  }
}

// Cada sesion TLS (chess.com, el tiempo, el correo, xiaozhi) pide unos 47 KB, y el core
// precompilado (CONFIG_MBEDTLS_INTERNAL_MEM_ALLOC) los saca de la RAM interna: con xiaozhi
// conectado (USB) y otra descarga a la vez, la interna llego a bajar a 15 KB. Van a la PSRAM,
// como hace CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC; solo si alli no caben, a la interna.
static void *tlsCalloc(size_t n, size_t size) {
  void *p = heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  return p ? p : heap_caps_calloc(n, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}
static void tlsFree(void *p) { heap_caps_free(p); }

void netBegin() {
  if (netMx) return;
  mbedtls_platform_set_calloc_free(tlsCalloc, tlsFree);  // antes de la primera conexion TLS
  // las credenciales se pasan siempre: que el wifi no las vuelva a escribir en la
  // NVS en cada intento (con varias redes, cada cambio era una escritura en flash)
  WiFi.persistent(false);
  {  // el lugar del portal (o el de la conexion): su ultimo desfase, hasta que llegue el tiempo
    Preferences p;
    p.begin("place", false);
#if !WX_DEFAULT_PLACE  // lo que dejo un firmware sin lugar propio: aqui manda el de config.h
    if (p.getUChar("auto", 0)) p.clear();
    p.remove("ptz");
    if (!p.isKey("lat")) p.remove("off");
#endif
    int32_t off = p.getInt("off", INT32_MIN);
    if (off != INT32_MIN) wallSetCityOffset(off);
    p.end();
  }
  netMx = xSemaphoreCreateMutex();
  cmdQ = xQueueCreate(1, sizeof(NetCmd));
  resQ = xQueueCreate(1, sizeof(NetRes));
  xTaskCreatePinnedToCore(netTask, "net", NET_STACK, nullptr, 1, &netTaskH, 0);
}

uint32_t netStackFree() { return netTaskH ? (uint32_t)uxTaskGetStackHighWaterMark(netTaskH) : 0; }
