#include "phonesteps.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "config.h"  // MDNS_HOST
#include "pet.h"     // REWARD_PTS_REQ

static WebServer *srv = nullptr;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static uint32_t pendSteps = 0;
static bool pendNew = false;
static uint16_t pendHockey = 0;  // minutos de hockey que aun no ha recogido el bucle principal
static uint8_t pendReward = 0;   // puntos del premio de casa que aun no ha recogido
static uint16_t code = 0;

uint16_t phoneStepsCode() {
  if (code) return code;
  Preferences p;
  p.begin("phone", false);
  code = p.getUShort("code", 0);
  if (code < 1000 || code > 9999) {
    code = 1000 + esp_random() % 9000;
    p.putUShort("code", code);
  }
  p.end();
  return code;
}

static PhoneCodeGuard guard;  // solo la tarea de red (los tres manejadores)

// el codigo de la peticion; si no vale (o hay que esperar), ya contesta el error
static bool codigoBueno() {
  uint32_t now = millis();
  if (!guard.open(now)) {
    srv->send(429, "application/json", "{\"ok\":false,\"error\":\"wait\"}");
    return false;
  }
  bool ok = srv->hasArg("k") && srv->arg("k").toInt() == phoneStepsCode();
  guard.result(ok, now);
  if (!ok) {
    srv->send(403, "application/json", "{\"ok\":false,\"error\":\"code\"}");
    if (!guard.open(now)) Serial.println("pasos del movil: demasiados codigos malos, 5 min sin aceptar ninguno");
  }
  return ok;
}

// GET /steps?n=8123&k=1234 (n puede venir con decimales: la suma de Salud)
static void pasos() {
  if (!codigoBueno()) return;
  float n = srv->hasArg("n") ? srv->arg("n").toFloat() : -1;
  if (n < 0 || n > 200000) {
    srv->send(400, "application/json", "{\"ok\":false,\"error\":\"n\"}");
    return;
  }
  portENTER_CRITICAL(&mux);
  pendSteps = (uint32_t)(n + 0.5f);
  pendNew = true;
  portEXIT_CRITICAL(&mux);
  char r[48];
  snprintf(r, sizeof(r), "{\"ok\":true,\"steps\":%lu}", (unsigned long)(uint32_t)(n + 0.5f));
  srv->send(200, "application/json", r);
}

// GET /hockey?min=15&k=1234: minutos de practica de hockey (SuperDeker)
static void hockey() {
  if (!codigoBueno()) return;
  long m = srv->hasArg("min") ? srv->arg("min").toInt() : 0;
  if (m < 1 || m > 180) {
    srv->send(400, "application/json", "{\"ok\":false,\"error\":\"min\"}");
    return;
  }
  portENTER_CRITICAL(&mux);
  pendHockey = pendHockey + m > 600 ? 600 : pendHockey + m;
  portEXIT_CRITICAL(&mux);
  char r[40];
  snprintf(r, sizeof(r), "{\"ok\":true,\"min\":%ld}", m);
  srv->send(200, "application/json", r);
}

// GET /reward?pts=5&k=1234: premio de casa (un padre, por lo que quiera); el tope del dia lo
// pone la mascota (Pet::reward)
static void premio() {
  if (!codigoBueno()) return;
  long n = srv->hasArg("pts") ? srv->arg("pts").toInt() : 0;
  if (n < 1 || n > REWARD_PTS_REQ) {
    srv->send(400, "application/json", "{\"ok\":false,\"error\":\"pts\"}");
    return;
  }
  portENTER_CRITICAL(&mux);
  pendReward = pendReward + n > 100 ? 100 : pendReward + n;
  portEXIT_CRITICAL(&mux);
  char r[40];
  snprintf(r, sizeof(r), "{\"ok\":true,\"pts\":%ld}", n);
  srv->send(200, "application/json", r);
}

void phoneStepsLoop() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!srv) {
    phoneStepsCode();
    srv = new WebServer(80);
    srv->on("/steps", HTTP_GET, pasos);
    srv->on("/hockey", HTTP_GET, hockey);
    srv->on("/reward", HTTP_GET, premio);
    srv->onNotFound([] { srv->send(200, "text/plain", "TamaPoke"); });
    srv->begin();
    if (MDNS.begin(MDNS_HOST)) MDNS.addService("http", "tcp", 80);
    Serial.printf("pasos del movil: http://%s.local/steps (%s)\n", MDNS_HOST, WiFi.localIP().toString().c_str());
  }
  srv->handleClient();
}

void phoneStepsStop() {
  if (!srv) return;
  srv->stop();
  delete srv;
  srv = nullptr;
  MDNS.end();
}

bool phoneStepsOn() { return srv != nullptr; }

bool phoneHockeyTake(uint16_t &min) {
  portENTER_CRITICAL(&mux);
  min = pendHockey;
  pendHockey = 0;
  portEXIT_CRITICAL(&mux);
  return min > 0;
}

bool phoneRewardTake(uint8_t &pts) {
  portENTER_CRITICAL(&mux);
  pts = pendReward;
  pendReward = 0;
  portEXIT_CRITICAL(&mux);
  return pts > 0;
}

bool phoneStepsTake(uint32_t &n) {
  bool hay;
  portENTER_CRITICAL(&mux);
  hay = pendNew;
  n = pendSteps;
  pendNew = false;
  portEXIT_CRITICAL(&mux);
  return hay;
}
