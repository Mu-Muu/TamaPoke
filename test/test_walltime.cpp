// Hora local desde la UTC (walltime.cpp), con la zona de config.h.
#include "framework.h"
#include "../walltime.h"
#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const long OFF = TZ_STD_OFFSET_H * 3600L;  // desfase normal en segundos

#if TZ_DST_NA
TEST(walltime, cambios_de_hora_de_2026_y_2027) {
  // empieza a las 2:00 locales del 2o domingo de marzo y acaba a las 2:00 (de
  // verano) del 1er domingo de noviembre: en UTC, 2:00 - desfase y 1:00 - desfase
  const uint32_t ini26 = 1772928000u + 2 * 3600 - OFF;  // 2026-03-08
  const uint32_t fin26 = 1793491200u + 1 * 3600 - OFF;  // 2026-11-01
  const uint32_t ini27 = 1804982400u + 2 * 3600 - OFF;  // 2027-03-14
  const uint32_t fin27 = 1825545600u + 1 * 3600 - OFF;  // 2027-11-07
  CHECK(!localIsDst(ini26 - 1));
  CHECK(localIsDst(ini26));
  CHECK(localIsDst(fin26 - 1));
  CHECK(!localIsDst(fin26));
  CHECK(!localIsDst(ini27 - 1));
  CHECK(localIsDst(ini27));
  CHECK(localIsDst(fin27 - 1));
  CHECK(!localIsDst(fin27));
}
#endif

TEST(walltime, hora_de_pared) {
  // 2026-11-06 12:00 UTC: hora normal
  CHECK_EQ(wallFromUtc(1793966400u), (uint32_t)(1793966400u + OFF));
  // 2026-07-01 16:00 UTC: en verano, una hora mas si hay horario de verano
  CHECK_EQ(wallFromUtc(1782921600u), (uint32_t)(1782921600u + OFF + (TZ_DST_NA ? 3600 : 0)));
}

// contra la libc del PC con la cadena LOCAL_TZ (asi tambien se comprueba que
// TZ_STD_OFFSET_H y TZ_DST_NA cuadran con ella): cada hora alrededor de los
// cambios de 2026-2045 y una fecha por semana
TEST(walltime, coincide_con_la_libc_y_local_tz) {
  const char *antes = getenv("TZ");
  char guardada[64] = "";
  if (antes) snprintf(guardada, sizeof(guardada), "%s", antes);
  setenv("TZ", LOCAL_TZ, 1);
  tzset();
  int fallos = 0;
  for (uint32_t u = 1767225600u; u < 2398377600u && fallos < 5; u += 3600) {  // 2026-01-01 .. 2046-01-01
    struct tm tm;
    time_t t = u;
    localtime_r(&t, &tm);
    bool cerca = (tm.tm_mon == 2 || tm.tm_mon == 10) && tm.tm_mday <= 15;  // marzo o noviembre, hasta el 15
    if (!cerca && (u / 3600) % (24 * 7) != 0) continue;
    uint32_t esperado = (uint32_t)timegm(&tm);
    if (wallFromUtc(u) != esperado) {
      char m[48];
      snprintf(m, sizeof(m), "utc=%lu", (unsigned long)u);
      CHECK_MSG(false, m);
      fallos++;
    }
  }
  if (antes) setenv("TZ", guardada, 1);
  else unsetenv("TZ");
  tzset();
}
