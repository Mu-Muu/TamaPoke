#include "batcurve.h"
#include "rtcbat.h"
#include "power.h"
#include "pin_config.h"  // define XPOWERS_CHIP_AXP2101
#include <Wire.h>
#include <time.h>
#include <SensorPCF85063.hpp>
#include <XPowersLib.h>
#include <Preferences.h>
#include <esp_system.h>

static SensorPCF85063 rtc;
static XPowersPMU pmu;
static bool rtcOk = false;
static bool pmuOk = false;

bool rtcBegin() {
  rtcOk = rtc.begin(Wire, IIC_SDA, IIC_SCL);
  if (!rtcOk) Serial.println("PCF85063 no detectado");
  return rtcOk;
}

uint32_t rtcEpoch() {
  if (!rtcOk) return 0;
  RTC_DateTime t = rtc.getDateTime();
  if (t.getYear() < 2025 || t.getYear() > 2120) return 0;  // sin hora valida
  struct tm tmv = {};
  tmv.tm_year = t.getYear() - 1900;
  tmv.tm_mon = t.getMonth() - 1;
  tmv.tm_mday = t.getDay();
  tmv.tm_hour = t.getHour();
  tmv.tm_min = t.getMinute();
  tmv.tm_sec = t.getSecond();
  time_t e = mktime(&tmv);  // TZ por defecto = UTC, consistente con gmtime_r
  return e > 0 ? (uint32_t)e : 0;
}

void rtcSetEpoch(uint32_t e) {
  if (!rtcOk) return;
  time_t tt = e;
  struct tm tmv;
  gmtime_r(&tt, &tmv);
  rtc.setDateTime(RTC_DateTime(tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                               tmv.tm_hour, tmv.tm_min, tmv.tm_sec));
}

bool batBegin() {
  pmuOk = pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
  if (!pmuOk) { Serial.println("AXP2101 no detectado"); return false; }
  pmu.enableBattVoltageMeasure();
  pmu.enableVbusVoltageMeasure();  // POWER: un cargador flojo se ve en la tension de VBUS
  pmu.setSysPowerDownVoltage(BAT_HW_CUTOFF_MV);  // respaldo: el PMU corta solo si el firmware no llega a tiempo
  return true;
}

// Enciende la alimentacion de la AMOLED. En la Waveshare 1.75 el panel (OLED VDD)
// cuelga del rail BLDO1 a 3.3V del AXP2101. El firmware daba por hecho que estaba
// encendido; si el PMU se resetea (drenaje total), BLDO1 queda OFF y la pantalla
// se ve negra aunque el resto funcione. Hay que llamarla ANTES de gfx->begin().
void pmuEnablePanel() {
  if (!pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    Serial.println("AXP2101 no detectado (pmuEnablePanel)");
    return;
  }
  pmu.setBLDO1Voltage(3300);   // OLED VDD
  pmu.enableBLDO1();
}

// el estado de energia (I2C) se cachea ~2 s: leerlo en cada frame del loop
// metia trafico I2C inutil y podia oscilar (parpadeo de brillo)
static uint32_t powerCacheT = 0;
static int cachedPct = -1;
static bool cachedCharging = false, cachedUsb = true;

static BatMonitor mon;
static bool monDischarging = false;
static uint32_t pollMs = 2000;
void batSetPollMs(uint32_t ms) { pollMs = ms; }

static void refreshPower() {
  uint32_t now = millis();
  if (powerCacheT && now - powerCacheT < pollMs) return;
  powerCacheT = now ? now : 1;
  if (!pmuOk) { cachedPct = -1; cachedCharging = false; cachedUsb = true; return; }
  bool conn = pmu.isBatteryConnect();
  cachedCharging = pmu.isCharging();
  cachedUsb = pmu.isVbusIn();
  // En descarga el porcentaje sale de la curva de una celda LiPo (batcurve.h); con USB o cargando
  // la tension sube por la carga y no vale, y se usa la del PMU.
  bool discharging = conn && !cachedUsb && !cachedCharging;
  if (discharging != monDischarging) {
    mon.reset();
    mon.seed(cachedPct);  // al desenchufar el porcentaje no sube (la tension sigue inflada por la carga)
    monDischarging = discharging;
  }
  if (discharging) cachedPct = mon.sample((int)pmu.getBattVoltage(), now);
  else cachedPct = conn ? pmu.getBatteryPercent() : -1;
  if (cachedPct < 0 && conn) cachedPct = pmu.getBatteryPercent();  // sin lectura de tension
}

int batPercent() { refreshPower(); return cachedPct; }
bool batCharging() { refreshPower(); return cachedCharging; }
bool batShouldShutdown() { refreshPower(); return monDischarging && mon.shutdownNeeded(millis()); }
void batPowerOff() {
  if (pmuOk) pmu.shutdown();
  mon.reset();  // si sigue vivo (p. ej. se enchufo USB justo ahora) tiene que volver a cumplirse todo el tiempo
}
int batMillivolts() { return pmuOk ? (int)pmu.getBattVoltage() : 0; }
bool usbPresent() { refreshPower(); return cachedUsb; }
bool usbPresentNow() { return pmuOk ? pmu.isVbusIn() : true; }

void pwrSetup() {
  if (!pmuOk) return;
  pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
  pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ);
  pmu.clearIrqStatus();
}

bool pwrShortPressed() {
  if (!pmuOk) return false;
  // el registro se lee a mano, no con getIrqStatus(): ver pwrShortFromIrq (power.h)
  bool hit = pwrShortFromIrq(pmu.readRegister(XPOWERS_AXP2101_INTSTS2), XPOWERS_AXP2101_PKEY_SHORT_IRQ >> 8);
  if (hit) pmu.clearIrqStatus();
  return hit;
}

// ---------- diagnostico: apagones y carga ----------
// Por que arranco el ESP32 (esp_reset_reason) y por que se encendio y se apago el PMU la ultima vez
// (registros 20h y 21h del AXP2101: guardan la causa hasta el siguiente encendido/apagado). Los
// ultimos arranques se apuntan en la NVS ("diag"/"boots") para verlos con POWER cuando la placa
// vuelve a casa; no cambian nada del juego.
struct BootRec {
  uint32_t epoch;
  uint16_t mv;
  uint8_t rst, pwrOn, pwrOff, usb;
};
#define BOOT_RECS 8

void powerBootLog(uint32_t epoch) {
  BootRec r = { epoch, (uint16_t)batMillivolts(), (uint8_t)esp_reset_reason(), 0, 0, 0 };
  if (pmuOk) {
    r.pwrOn = (uint8_t)pmu.getPowerOnSource();
    r.pwrOff = (uint8_t)pmu.getPowerOffSource();
    r.usb = pmu.isVbusIn();
  }
  BootRec v[BOOT_RECS] = {};
  Preferences p;
  p.begin("diag", false);
  p.getBytes("boots", v, sizeof(v));
  memmove(&v[1], &v[0], sizeof(BootRec) * (BOOT_RECS - 1));  // el mas reciente, el primero
  v[0] = r;
  p.putBytes("boots", v, sizeof(v));
  p.end();
  Serial.printf("arranque: reset %u, PMU encendido 0x%02x apagado 0x%02x, bateria %u mV, usb %u\n", r.rst,
                r.pwrOn, r.pwrOff, r.mv, r.usb);
}

// bits del registro, con nombre
static void bits(uint8_t v, const char *const *nom, int n) {
  bool alguno = false;
  for (int i = 0; i < n; i++)
    if (v & (1 << i)) {
      Serial.printf("%s%s", alguno ? "+" : "", nom[i]);
      alguno = true;
    }
  if (!alguno) Serial.print("-");
}

void powerReport() {
  static const char *const RST[] = { "?",       "encendido", "externo", "software", "panico",
                                     "wdt int", "wdt tarea", "wdt",     "sueño",    "caida de tension",
                                     "sdio",    "usb",       "jtag",    "efuse",    "glitch",
                                     "cpu" };
  static const char *const ON[] = { "PWR", "IRQ", "USB", "bat>3.3V", "bateria" };
  static const char *const OFF[] = { "PWR largo", "software", "PWR fijo", "Vsys baja",
                                     "VBUS alta", "DCDC baja", "DCDC alta", "temperatura" };
  static const char *const CHG[] = { "goteo", "precarga", "corriente", "tension", "cargada", "parada" };
  static const uint16_t LIM[] = { 100, 500, 900, 1000, 1500, 2000 };
  if (!pmuOk) {
    Serial.println("POWER: sin PMU");
    return;
  }
  uint8_t st = (uint8_t)pmu.getChargerStatus(), icc = pmu.getChargerConstantCurr(), lim = pmu.getVbusCurrentLimit();
  Serial.printf("ahora: usb %d (%u mV), bateria %u mV %d%%, cargando %d (%s), corriente de carga %u mA, "
                "limite USB %u mA\n",
                pmu.isVbusIn(), pmu.getVbusVoltage(), pmu.getBattVoltage(), pmu.getBatteryPercent(),
                pmu.isCharging(), st < 6 ? CHG[st] : "?", icc <= 8 ? icc * 25 : 200 + (icc - 8) * 100,
                lim < 6 ? LIM[lim] : 0);
  BootRec v[BOOT_RECS] = {};
  Preferences p;
  p.begin("diag", true);
  p.getBytes("boots", v, sizeof(v));
  p.end();
  Serial.println("arranques (el ultimo primero):");
  for (int i = 0; i < BOOT_RECS && v[i].epoch; i++) {
    time_t t = v[i].epoch;
    struct tm tm;
    gmtime_r(&t, &tm);  // hora de pared (como el RTC)
    Serial.printf("  %02d-%02d %02d:%02d reset %s | PMU encendido ", tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
                  tm.tm_min, v[i].rst < 16 ? RST[v[i].rst] : "?");
    bits(v[i].pwrOn, ON, 5);
    Serial.print(", apagado antes ");
    bits(v[i].pwrOff, OFF, 8);
    Serial.printf(" | bateria %u mV, usb %u\n", v[i].mv, v[i].usb);
  }
}
