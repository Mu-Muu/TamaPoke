#pragma once
#include <stdint.h>
#include <ArduinoJson.h>

// Tiempo real del lugar de config.h (Open-Meteo, sin clave) para el fondo: lluvia,
// nieve, niebla, tormenta, nubes; y la salida/puesta del sol para el dia y la
// noche. Aqui solo lo que no depende del hardware (se prueba en el PC); la
// descarga esta en wifisync.cpp.

#define WX_EVERY_MS (60UL * 60 * 1000)      // se actualiza cada hora (con USB)
#define WX_EVERY_BATT_MS (3UL * 60 * 60 * 1000)  // con bateria, cada 3 h (la radio es lo que mas gasta)
#define WX_BATT_QUIET_UNTIL 6                // con bateria, de 0 a 6 h no se conecta
#define WX_STALE_MS (4UL * 60 * 60 * 1000)  // sin datos de hace 4 h: cielo normal (con bateria se pide cada 3)

enum : uint8_t { WX_UNKNOWN = 0, WX_CLEAR, WX_CLOUDY, WX_FOG, WX_RAIN, WX_SNOW, WX_STORM };

struct Weather {
  bool valid = false;
  uint8_t kind = WX_UNKNOWN;
  int16_t sunriseMin = -1, sunsetMin = -1;  // minuto del dia local, -1 = no se sabe
  int8_t tempC = 0;
};

// codigo WMO de Open-Meteo -> WX_*
uint8_t wxFromWmo(int code);
// "2026-11-06T07:05" -> 425 (minuto del dia); -1 si no se entiende
int16_t wxMinuteOf(const char *iso);
// respuesta de /v1/forecast?current=weather_code,temperature_2m&daily=sunrise,sunset
bool weatherParse(JsonVariantConst doc, Weather &w);

// fase del dia por la hora local: dia, amanecer/atardecer (cerca de la salida o
// la puesta del sol) o noche. Sin datos del sol: sale a las 6:30 y se pone a las 19:30.
enum : uint8_t { PH_DAY = 0, PH_TWILIGHT, PH_NIGHT };
#define TWILIGHT_BEFORE 30  // minutos de "luz de amanecer/atardecer" antes...
#define TWILIGHT_AFTER 40   // ...y despues de la salida (al reves en la puesta)
uint8_t dayPhase(int minuteOfDay, int sunriseMin, int sunsetMin);
