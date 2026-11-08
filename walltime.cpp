#include "walltime.h"
#include "config.h"
#include "holiday.h"  // holWeekday
#include <time.h>

// dias desde 1970-01-01 (H. Hinnant, days_from_civil)
static int64_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097LL + (int64_t)doe - 719468;
}

static uint32_t utcAt(int y, int m, int d, int h) { return (uint32_t)(daysFromCivil(y, m, d) * 86400 + h * 3600); }

bool localIsDst(uint32_t utc) {
#if TZ_DST_NA
  time_t t = utc;
  struct tm tm;
  gmtime_r(&t, &tm);
  int y = tm.tm_year + 1900;
  int ini = 8 + (7 - holWeekday(y, 3, 8)) % 7;   // 2o domingo de marzo (el primero >= 8)
  int fin = 1 + (7 - holWeekday(y, 11, 1)) % 7;  // 1er domingo de noviembre
  // a las 2:00 locales: en UTC, 2 - desfase normal al empezar y 2 - (desfase + 1) al acabar
  return utc >= utcAt(y, 3, ini, 2 - TZ_STD_OFFSET_H) && utc < utcAt(y, 11, fin, 1 - TZ_STD_OFFSET_H);
#else
  (void)utc;
  return false;
#endif
}

uint32_t wallFromUtc(uint32_t utc) {
  return utc + (uint32_t)((TZ_STD_OFFSET_H + (localIsDst(utc) ? 1 : 0)) * 3600);
}
