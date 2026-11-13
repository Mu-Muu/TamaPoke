// Tiempo y fase del dia (weather.cpp) con respuestas como las de Open-Meteo.
#include "framework.h"
#include "../weather.h"

TEST(weather, codigos_wmo) {
  CHECK_EQ(wxFromWmo(0), (uint8_t)WX_CLEAR);
  CHECK_EQ(wxFromWmo(2), (uint8_t)WX_CLOUDY);
  CHECK_EQ(wxFromWmo(3), (uint8_t)WX_CLOUDY);
  CHECK_EQ(wxFromWmo(45), (uint8_t)WX_FOG);
  CHECK_EQ(wxFromWmo(53), (uint8_t)WX_RAIN);
  CHECK_EQ(wxFromWmo(66), (uint8_t)WX_RAIN);   // lluvia helada
  CHECK_EQ(wxFromWmo(73), (uint8_t)WX_SNOW);
  CHECK_EQ(wxFromWmo(81), (uint8_t)WX_RAIN);
  CHECK_EQ(wxFromWmo(86), (uint8_t)WX_SNOW);
  CHECK_EQ(wxFromWmo(95), (uint8_t)WX_STORM);
  CHECK_EQ(wxFromWmo(-1), (uint8_t)WX_UNKNOWN);
  CHECK_EQ(wxFromWmo(42), (uint8_t)WX_UNKNOWN);
}

TEST(weather, minuto_del_dia) {
  CHECK_EQ(wxMinuteOf("2026-11-06T07:05"), (int16_t)425);
  CHECK_EQ(wxMinuteOf("2026-11-06T16:48"), (int16_t)1008);
  CHECK_EQ(wxMinuteOf("2026-11-06"), (int16_t)-1);
  CHECK_EQ(wxMinuteOf(nullptr), (int16_t)-1);
}

TEST(weather, respuesta_de_open_meteo) {
  JsonDocument doc;
  deserializeJson(doc, R"({"current":{"time":"2026-11-06T14:00","temperature_2m":-3.6,"weather_code":71},
    "daily":{"time":["2026-11-06"],"sunrise":["2026-11-06T07:05"],"sunset":["2026-11-06T16:48"]}})");
  Weather w;
  CHECK(weatherParse(doc.as<JsonVariantConst>(), w));
  CHECK(w.valid);
  CHECK_EQ(w.kind, (uint8_t)WX_SNOW);
  CHECK_EQ(w.tempC, (int8_t)-4);
  CHECK_EQ(w.tempF, (int16_t)26);  // -3.6 C = 25.5 F
  CHECK_EQ(w.sunriseMin, (int16_t)425);
  CHECK_EQ(w.sunsetMin, (int16_t)1008);
}

// grados F para enseñar: desde los decimales de Open-Meteo, redondeando una sola vez
static int16_t enF(const char *temp) {
  char json[160];
  snprintf(json, sizeof(json), R"({"current":{"temperature_2m":%s,"weather_code":0}})", temp);
  JsonDocument doc;
  deserializeJson(doc, json);
  Weather w;
  weatherParse(doc.as<JsonVariantConst>(), w);
  return w.tempF;
}

TEST(weather, la_temperatura_tambien_en_fahrenheit) {
  CHECK_EQ(enF("0"), (int16_t)32);
  CHECK_EQ(enF("-40"), (int16_t)-40);    // donde las dos escalas coinciden
  CHECK_EQ(enF("100"), (int16_t)212);
  CHECK_EQ(enF("21.4"), (int16_t)71);    // 70.5: desde 21 redondeado saldria 70
  CHECK_EQ(enF("-20"), (int16_t)-4);
  CHECK_EQ(enF("-17.9"), (int16_t)0);    // -0.2 F
  CHECK_EQ(enF("-18.3"), (int16_t)-1);   // -0.9 F: bajo cero redondea hacia abajo
  Weather sin;                           // sin datos (o forzado por consola): 0 C y 32 F
  CHECK_EQ(sin.tempF, (int16_t)32);
}

TEST(weather, respuesta_rota_no_vale) {
  JsonDocument doc;
  deserializeJson(doc, R"({"error":true,"reason":"bad"})");
  Weather w;
  CHECK(!weatherParse(doc.as<JsonVariantConst>(), w));
  CHECK(!w.valid);
}

TEST(weather, fases_del_dia_con_el_sol_de_hoy) {
  int rise = 425, set = 1008;  // 7:05 y 16:48
  CHECK_EQ(dayPhase(6 * 60, rise, set), (uint8_t)PH_NIGHT);
  CHECK_EQ(dayPhase(rise - 10, rise, set), (uint8_t)PH_TWILIGHT);  // clarea
  CHECK_EQ(dayPhase(rise + 30, rise, set), (uint8_t)PH_TWILIGHT);
  CHECK_EQ(dayPhase(12 * 60, rise, set), (uint8_t)PH_DAY);
  CHECK_EQ(dayPhase(set - 20, rise, set), (uint8_t)PH_TWILIGHT);  // atardece
  CHECK_EQ(dayPhase(set + 20, rise, set), (uint8_t)PH_TWILIGHT);
  CHECK_EQ(dayPhase(set + 40, rise, set), (uint8_t)PH_NIGHT);
  CHECK_EQ(dayPhase(23 * 60, rise, set), (uint8_t)PH_NIGHT);
}

TEST(weather, sin_datos_del_sol_usa_6_30_y_19_30) {
  CHECK_EQ(dayPhase(5 * 60, -1, -1), (uint8_t)PH_NIGHT);
  CHECK_EQ(dayPhase(6 * 60 + 30, -1, -1), (uint8_t)PH_TWILIGHT);
  CHECK_EQ(dayPhase(13 * 60, -1, -1), (uint8_t)PH_DAY);
  CHECK_EQ(dayPhase(21 * 60, -1, -1), (uint8_t)PH_NIGHT);
}
