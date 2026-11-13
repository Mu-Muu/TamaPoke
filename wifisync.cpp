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

static WebServer *server = nullptr;
static DNSServer *dns = nullptr;
static bool guardado = false;
static uint32_t portalHasta = 0;
static volatile bool portalOn = false;
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

bool wifiHasCreds() { return credGet("ssid").length() > 0; }
bool mcpConfigured() { return credGet("mcp").length() > 0; }

// ---------- pagina de configuracion ----------

static const char PAGINA[] PROGMEM = R"(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>TamaPoke</title>
<style>body{font-family:sans-serif;max-width:420px;margin:24px auto;padding:0 16px}
input{width:100%;padding:10px;margin:6px 0 14px;font-size:16px;box-sizing:border-box}
button{width:100%;padding:12px;font-size:17px}small{color:#666}</style></head><body>
<h2>TamaPoke Wi-Fi</h2><p>填家里的 Wi-Fi，用来每晚同步 chess.com 的积分和更新天气。</p>
<form method="post" action="/save"><label>Wi-Fi 名称</label><input name="ssid">
<label>密码</label><input name="pass" type="password">
<label>chess.com 用户名（可不填）</label>
<input name="chess" maxlength="25" autocapitalize="none" autocorrect="off" spellcheck="false">
<small>用来把 chess.com 上的对局换成积分。第一次同步（或换了用户名后）只记录当前成绩，不补发以前的分。</small><br><br>
<label>小智 MCP 接入点（可选，给 Stack-chan 用）</label>
<input name="mcp" placeholder="wss://api.xiaozhi.me/mcp/?token=...">
<small>不改的项留空就行（会保留原来的）。要删掉 chess.com 用户名或 MCP 接入点，填一个减号 -</small><br><br>
<button>保存</button></form></body></html>)";

static void paginaGuardado() {
  String ssid = server->arg("ssid");
  String pass = server->arg("pass");
  String mcp = server->arg("mcp");
  String chess = server->arg("chess");
  ssid.trim();
  mcp.trim();
  chess.trim();
  char usuario[CHESS_USER_MAX + 1] = "";
  if (chess.length() && chess != "-" && !chessNormUser(chess.c_str(), usuario, sizeof(usuario))) {
    server->send(400, "text/html; charset=utf-8",
                 "<meta charset=utf-8><p>chess.com 用户名只能有字母、数字、_ 和 -（3 到 25 个），请返回重填。</p>");
    return;
  }
  bool hayRed = wifiHasCreds();
  if ((ssid.length() == 0 && !hayRed) || ssid.length() > 32 || pass.length() > 64 || mcp.length() > 400 ||
      (mcp.length() > 1 && !mcp.startsWith("wss://") && !mcp.startsWith("ws://"))) {
    server->send(400, "text/html; charset=utf-8", "<meta charset=utf-8><p>填的不对，请返回重填。</p>");
    return;
  }
  Preferences p;
  p.begin("wifi", false);
  if (ssid.length()) {
    p.putString("ssid", ssid);
    p.putString("pass", pass);
  }
  if (mcp == "-") p.remove("mcp");
  else if (mcp.length()) p.putString("mcp", mcp);
  p.end();
  if (chess == "-") chessSaveUser("");  // vuelve a valer la de config_private.h (si la hay)
  else if (usuario[0]) chessSaveUser(usuario);
  server->send(200, "text/html; charset=utf-8",
               "<meta charset=utf-8><h2>已保存</h2><p>可以断开这个热点了。</p>");
  guardado = true;
}

static void portalCleanup() {
  if (server) { server->stop(); delete server; server = nullptr; }
  if (dns) { dns->stop(); delete dns; dns = nullptr; }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

static void mcpStop();

void wifiPortalStart() {
  netLock();  // espera a que la tarea de red suelte la radio
  portalOn = true;
  mcpStop();
  WiFi.disconnect(true);
  netUnlock();
  portalCleanup();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_NAME);
  dns = new DNSServer();
  dns->start(53, "*", WiFi.softAPIP());  // cualquier dominio -> la pagina (portal cautivo)
  server = new WebServer(80);
  server->on("/save", HTTP_POST, paginaGuardado);
  server->onNotFound([]() { server->send_P(200, "text/html; charset=utf-8", PAGINA); });
  server->begin();
  guardado = false;
  portalHasta = millis() + PORTAL_MS;
}

bool wifiPortalActive() { return server != nullptr; }

bool wifiPortalLoop() {
  if (!server) return false;
  dns->processNextRequest();
  server->handleClient();
  if ((int32_t)(millis() - portalHasta) > 0) wifiPortalStop();
  return guardado;
}

static volatile bool wxNow = true;  // pedir el tiempo en la proxima vuelta

void wifiPortalStop() {
  portalCleanup();
  portalOn = false;
  wxNow = true;  // red nueva (o la misma): tiempo al momento
}

// ---------- conexion ----------

static volatile bool usbPower = false;
void netSetUsb(bool usb) { usbPower = usb; }
// con USB y punto MCP, la radio se queda encendida (Stack-chan puede hablar
// en cualquier momento); con bateria solo se conecta un momento
static bool keepOnline() { return usbPower && mcpConfigured(); }

static bool conectar() {
  if (WiFi.status() == WL_CONNECTED) return true;
  String ssid = credGet("ssid"), pass = credGet("pass");
  if (ssid.length() == 0) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  for (int i = 0; i < 150 && WiFi.status() != WL_CONNECTED; i++) delay(100);  // 15 s
  return WiFi.status() == WL_CONNECTED;
}

static void desconectarSiSobra() {
  if (keepOnline()) return;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// hora real por NTP. Devuelve el UTC y deja el RTC en hora local (el firmware
// guarda la hora local "de pared" como epoch, ver rtcbat.cpp)
static time_t ponerEnHora() {
  configTzTime(LOCAL_TZ, "pool.ntp.org", "time.google.com");
  time_t utc = 0;
  for (int i = 0; i < 100 && (utc = time(nullptr)) < 1700000000; i++) delay(100);
  if (utc < 1700000000) return 0;
  struct tm lt;
  localtime_r(&utc, &lt);
  // rtcEpoch()/rtcSetEpoch() cuentan con TZ = UTC: se vuelve a dejar asi y la
  // hora local se convierte en epoch "de pared"
  setenv("TZ", "UTC0", 1);
  tzset();
  uint32_t pared = (uint32_t)mktime(&lt);
  rtcSetEpoch(pared);
  return utc;
}

static bool bajarJson(const String &url, JsonDocument &doc, JsonDocument *filtro) {
  WiFiClientSecure cliente;
  cliente.setInsecure();  // datos publicos y de solo lectura
  HTTPClient http;
  http.useHTTP10(true);   // sin "chunked": ArduinoJson lee directo del flujo
  http.setUserAgent("TamaPoke/1.0 (personal project)");
  if (!http.begin(cliente, url)) return false;
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
      WiFiClient *st = http.getStreamPtr();
      uint32_t t = millis();
      while ((len < 0 || n < (size_t)len) && n < cap && millis() - t < 20000) {
        int a = st->available();
        if (a > 0) {
          n += st->readBytes(buf + n, min((size_t)a, cap - n));
          t = millis();
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
      Serial.printf("bajado %u/%d bytes %s%s\n", (unsigned)n, len, url.c_str(), e ? " (json mal)" : "");
      if (e) Serial.printf("json: %s\n", e.c_str());
      free(buf);
    }
  } else {
    Serial.printf("http %d %s\n", code, url.c_str());
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
      if (++m > 12) { m = 1; y++; }
    }
  }
  desconectarSiSobra();
  if (!ok) return false;

  chessCommit(st, rep);
  chessSave(st);
  chessSaveReport(rep, pared / 86400);
  if (rep.total() > 0) {
    pet.chessPtsTotal += rep.total();
    pet.addPoints(rep.total());
  }
  pet.chessMedals(rep.wins, rep.newBestRapid || rep.newBestDaily || rep.hundreds ||
                                rep.newTactics || rep.newRush);
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
// principal corrige el reloj (netTimeTake). La sincronizacion del ajedrez sigue
// usando configTzTime por su cuenta; el aviso de hora sirve para las dos.
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

static bool bajarTiempo() {
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=") + WX_LAT + "&longitude=" + WX_LON +
               "&current=weather_code,temperature_2m&daily=sunrise,sunset&timezone=" WX_TZ "&forecast_days=1";
  JsonDocument doc;
  Weather w;
  if (!bajarJson(url, doc, nullptr) || !weatherParse(doc.as<JsonVariantConst>(), w)) return false;
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
    int tool = stackchanFind(name);
    if (tool < 0) {
      out["error"]["code"] = -32602;
      out["error"]["message"] = "unknown tool";
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

static void netTask(void *) {
  uint32_t nextWx = 0, nextJoin = 0;
  for (;;) {
    if (portalOn || !wifiHasCreds()) {
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }
    netLock();
    uint32_t now = millis();
    bool keep = keepOnline();
    if (keep) {
      if (WiFi.status() != WL_CONNECTED && (int32_t)(now - nextJoin) >= 0) {
        mcpStop();
        if (!conectar()) nextJoin = now + 60000;  // la red no esta: otro intento en 1 min
      }
      if (WiFi.status() == WL_CONNECTED) {
        if (!ws) mcpStart();
        if (ws) ws->loop();
      }
    } else if (ws) {  // sin USB (o sin punto MCP): se apaga la radio
      mcpStop();
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
    if (wxNow || (int32_t)(now - nextWx) >= 0) {
      wxNow = false;
      bool con = conectar();
      if (con) pedirHora();
      bool ok = con && bajarTiempo();
      desconectarSiSobra();
      nextWx = millis() + (ok ? (usbPower ? WX_EVERY_MS : WX_EVERY_BATT_MS) : 15UL * 60 * 1000);
    }
    netUnlock();
    vTaskDelay(pdMS_TO_TICKS(ws ? 10 : 2000));  // sin xiaozhi no hay prisa: despertar menos
  }
}

void netBegin() {
  if (netMx) return;
  netMx = xSemaphoreCreateMutex();
  cmdQ = xQueueCreate(1, sizeof(NetCmd));
  resQ = xQueueCreate(1, sizeof(NetRes));
  xTaskCreatePinnedToCore(netTask, "net", 16384, nullptr, 1, nullptr, 0);
}
