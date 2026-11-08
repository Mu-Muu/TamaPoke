#include "weather.h"
#include <string.h>
#include <stdlib.h>

uint8_t wxFromWmo(int c) {
  if (c < 0) return WX_UNKNOWN;
  if (c <= 1) return WX_CLEAR;                       // despejado / casi
  if (c <= 3) return WX_CLOUDY;                      // nubes, cubierto
  if (c == 45 || c == 48) return WX_FOG;
  if (c >= 51 && c <= 67) return WX_RAIN;            // llovizna, lluvia, helada
  if (c >= 71 && c <= 77) return WX_SNOW;
  if (c >= 80 && c <= 82) return WX_RAIN;            // chubascos
  if (c == 85 || c == 86) return WX_SNOW;            // chubascos de nieve
  if (c >= 95 && c <= 99) return WX_STORM;
  return WX_UNKNOWN;
}

int16_t wxMinuteOf(const char *iso) {
  if (!iso) return -1;
  const char *t = strchr(iso, 'T');
  if (!t || strlen(t) < 6 || t[3] != ':') return -1;
  int h = atoi(t + 1), m = atoi(t + 4);
  if (h < 0 || h > 23 || m < 0 || m > 59) return -1;
  return (int16_t)(h * 60 + m);
}

bool weatherParse(JsonVariantConst doc, Weather &w) {
  JsonVariantConst cur = doc["current"];
  if (cur.isNull() || cur["weather_code"].isNull()) return false;
  w.kind = wxFromWmo(cur["weather_code"] | -1);
  float t = cur["temperature_2m"] | 0.0f;
  w.tempC = (int8_t)(t < 0 ? t - 0.5f : t + 0.5f);
  w.sunriseMin = wxMinuteOf(doc["daily"]["sunrise"][0] | (const char *)nullptr);
  w.sunsetMin = wxMinuteOf(doc["daily"]["sunset"][0] | (const char *)nullptr);
  w.valid = true;
  return true;
}

uint8_t dayPhase(int m, int rise, int set) {
  if (rise < 0 || set < 0 || set <= rise) {
    rise = 6 * 60 + 30;
    set = 19 * 60 + 30;
  }
  if (m < rise - TWILIGHT_BEFORE || m >= set + TWILIGHT_BEFORE) return PH_NIGHT;
  if (m < rise + TWILIGHT_AFTER || m >= set - TWILIGHT_AFTER) return PH_TWILIGHT;
  return PH_DAY;
}
