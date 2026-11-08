// Fiestas por fecha (holiday.cpp).
#include "framework.h"
#include "../holiday.h"

TEST(holiday, dia_de_la_semana) {
  CHECK_EQ(holWeekday(2026, 11, 6), 5);  // viernes
  CHECK_EQ(holWeekday(2024, 2, 29), 4);  // jueves (bisiesto)
  CHECK_EQ(holWeekday(2000, 1, 1), 6);   // sabado
}

TEST(holiday, pascua) {
  int m, d;
  holEaster(2026, &m, &d);
  CHECK_EQ(m, 4); CHECK_EQ(d, 5);
  holEaster(2027, &m, &d);
  CHECK_EQ(m, 3); CHECK_EQ(d, 28);
  holEaster(2025, &m, &d);
  CHECK_EQ(m, 4); CHECK_EQ(d, 20);
}

TEST(holiday, fiestas_fijas) {
  CHECK_EQ(holidayOn(2026, 12, 25, 0, 0), (uint8_t)HOL_CHRISTMAS);
  CHECK_EQ(holidayOn(2026, 12, 20, 0, 0), (uint8_t)HOL_CHRISTMAS);
  CHECK_EQ(holidayOn(2026, 12, 27, 0, 0), (uint8_t)HOL_NONE);
  CHECK_EQ(holidayOn(2026, 12, 31, 0, 0), (uint8_t)HOL_NEWYEAR);
  CHECK_EQ(holidayOn(2027, 1, 1, 0, 0), (uint8_t)HOL_NEWYEAR);
  CHECK_EQ(holidayOn(2026, 10, 31, 0, 0), (uint8_t)HOL_HALLOWEEN);
  CHECK_EQ(holidayOn(2026, 10, 25, 0, 0), (uint8_t)HOL_HALLOWEEN);
  CHECK_EQ(holidayOn(2027, 2, 14, 0, 0), (uint8_t)HOL_VALENTINE);
  CHECK_EQ(holidayOn(2027, 7, 1, 0, 0), (uint8_t)HOL_CANADA);
  CHECK_EQ(holidayOn(2026, 11, 6, 0, 0), (uint8_t)HOL_NONE);
}

TEST(holiday, fiestas_moviles) {
  CHECK_EQ(holidayOn(2026, 4, 3, 0, 0), (uint8_t)HOL_EASTER);   // viernes santo
  CHECK_EQ(holidayOn(2026, 4, 6, 0, 0), (uint8_t)HOL_EASTER);   // lunes de pascua
  CHECK_EQ(holidayOn(2026, 4, 7, 0, 0), (uint8_t)HOL_NONE);
  CHECK_EQ(holidayOn(2027, 5, 9, 0, 0), (uint8_t)HOL_MOTHER);   // 2o domingo de mayo
  CHECK_EQ(holidayOn(2027, 6, 20, 0, 0), (uint8_t)HOL_FATHER);  // 3er domingo de junio
  CHECK_EQ(holidayOn(2027, 10, 11, 0, 0), (uint8_t)HOL_THANKSGIVING);  // 2o lunes de octubre
  CHECK_EQ(holidayOn(2027, 10, 9, 0, 0), (uint8_t)HOL_THANKSGIVING);   // ese sabado
  CHECK_EQ(holidayOn(2027, 10, 12, 0, 0), (uint8_t)HOL_NONE);
}

TEST(holiday, fiestas_lunares) {
  CHECK_EQ(holidayOn(2027, 2, 5, 0, 0), (uint8_t)HOL_CNY);   // nochevieja
  CHECK_EQ(holidayOn(2027, 2, 6, 0, 0), (uint8_t)HOL_CNY);   // año nuevo
  CHECK_EQ(holidayOn(2027, 2, 12, 0, 0), (uint8_t)HOL_CNY);  // 7o dia
  CHECK_EQ(holidayOn(2027, 2, 13, 0, 0), (uint8_t)HOL_NONE);
  CHECK_EQ(holidayOn(2027, 2, 20, 0, 0), (uint8_t)HOL_LANTERN);  // 6 feb + 14
  CHECK_EQ(holidayOn(2027, 6, 9, 0, 0), (uint8_t)HOL_DRAGONBOAT);
  CHECK_EQ(holidayOn(2027, 9, 15, 0, 0), (uint8_t)HOL_MIDAUTUMN);
  CHECK_EQ(holidayOn(2027, 9, 16, 0, 0), (uint8_t)HOL_MIDAUTUMN);
  CHECK_EQ(holidayOn(2050, 2, 1, 0, 0), (uint8_t)HOL_NONE);  // fuera de la tabla
}

TEST(holiday, el_cumpleanos_manda) {
  CHECK_EQ(holidayOn(2026, 12, 25, 12, 25), (uint8_t)HOL_BIRTHDAY);
  CHECK_EQ(holidayOn(2027, 5, 20, 5, 20), (uint8_t)HOL_BIRTHDAY);
  CHECK_EQ(holidayOn(2027, 5, 21, 5, 20), (uint8_t)HOL_NONE);
}

TEST(holiday, estaciones) {
  CHECK_EQ(seasonOf(1), (uint8_t)SEASON_WINTER);
  CHECK_EQ(seasonOf(3), (uint8_t)SEASON_WINTER);
  CHECK_EQ(seasonOf(4), (uint8_t)SEASON_SPRING);
  CHECK_EQ(seasonOf(7), (uint8_t)SEASON_SUMMER);
  CHECK_EQ(seasonOf(10), (uint8_t)SEASON_AUTUMN);
  CHECK_EQ(seasonOf(12), (uint8_t)SEASON_WINTER);
}
