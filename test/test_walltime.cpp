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

// la ciudad del portal: manda su desfase (el que da Open-Meteo con el tiempo) y se puede quitar
TEST(walltime, el_desfase_de_la_ciudad_manda_y_se_quita) {
  const uint32_t julio = 1782921600u;  // 2026-07-01 16:00 UTC
  const uint32_t cfg = wallFromUtc(julio);
  wallSetCityOffset(8 * 3600);  // Shanghai
  CHECK_EQ(wallFromUtc(julio), julio + 8 * 3600);
  wallSetCityOffset(-(2 * 3600 + 30 * 60));  // Terranova en verano
  CHECK_EQ(wallFromUtc(julio), julio - (2 * 3600 + 30 * 60));
  wallSetCityOffset(20 * 3600);  // no existe: se queda el de antes
  CHECK_EQ(wallFromUtc(julio), julio - (2 * 3600 + 30 * 60));
  wallClearCityOffset();
  CHECK_EQ(wallFromUtc(julio), cfg);
}

// el reloj en 24 h o en 12 h con AM/PM (issue #15)
TEST(walltime, reloj_de_24_y_de_12_horas) {
  char t[8];
  const uint32_t dia = 20000u * 86400u;  // cualquier dia: solo cuenta la hora
  struct { unsigned h, m; const char *h24, *h12, *ap; } C[] = {
    { 0, 0, "00:00", "12:00", "AM" },  { 0, 5, "00:05", "12:05", "AM" },  { 7, 5, "07:05", "7:05", "AM" },
    { 11, 59, "11:59", "11:59", "AM" }, { 12, 0, "12:00", "12:00", "PM" }, { 12, 30, "12:30", "12:30", "PM" },
    { 15, 30, "15:30", "3:30", "PM" },  { 23, 59, "23:59", "11:59", "PM" },
  };
  for (auto &c : C) {
    uint32_t e = dia + c.h * 3600 + c.m * 60 + 59;  // los segundos no cuentan
    CHECK_STREQ(clockHHMM(e, false, t, sizeof(t)), "");
    CHECK_STREQ(t, c.h24);
    CHECK_STREQ(clockHHMM(e, true, t, sizeof(t)), c.ap);
    CHECK_STREQ(t, c.h12);
  }
}
