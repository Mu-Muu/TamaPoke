#include "holiday.h"

// [año - HOL_LUNAR_FIRST] = { mes, dia } del Año Nuevo chino, el Duanwu y el
// Medio Otoño (sacados del calendario lunar con la libreria lunardate)
static const uint8_t LUNAR[HOL_LUNAR_YEARS][3][2] = {
  { { 2, 17 }, { 6, 19 }, { 9, 25 } },  // 2026
  { { 2, 6 }, { 6, 9 }, { 9, 15 } },    // 2027
  { { 1, 26 }, { 5, 28 }, { 10, 3 } },  // 2028
  { { 2, 13 }, { 6, 16 }, { 9, 22 } },  // 2029
  { { 2, 3 }, { 6, 5 }, { 9, 12 } },    // 2030
  { { 1, 23 }, { 6, 24 }, { 10, 1 } },  // 2031
  { { 2, 11 }, { 6, 12 }, { 9, 19 } },  // 2032
  { { 1, 31 }, { 6, 1 }, { 9, 8 } },    // 2033
  { { 2, 19 }, { 6, 20 }, { 9, 27 } },  // 2034
  { { 2, 8 }, { 6, 10 }, { 9, 16 } },   // 2035
  { { 1, 28 }, { 5, 30 }, { 10, 4 } },  // 2036
  { { 2, 15 }, { 6, 18 }, { 9, 24 } },  // 2037
  { { 2, 4 }, { 6, 7 }, { 9, 13 } },    // 2038
  { { 1, 24 }, { 5, 27 }, { 10, 2 } },  // 2039
  { { 2, 12 }, { 6, 14 }, { 9, 20 } },  // 2040
  { { 2, 1 }, { 6, 3 }, { 9, 10 } },    // 2041
  { { 1, 22 }, { 6, 22 }, { 9, 28 } },  // 2042
  { { 2, 10 }, { 6, 11 }, { 9, 17 } },  // 2043
  { { 1, 30 }, { 5, 31 }, { 10, 5 } },  // 2044
  { { 2, 17 }, { 6, 19 }, { 9, 25 } },  // 2045
};

uint8_t seasonOf(int m) {
  if (m >= 4 && m <= 5) return SEASON_SPRING;
  if (m >= 6 && m <= 8) return SEASON_SUMMER;
  if (m >= 9 && m <= 11) return SEASON_AUTUMN;
  return SEASON_WINTER;  // diciembre - marzo
}

int holWeekday(int y, int m, int d) {  // Sakamoto
  static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

void holEaster(int y, int *m, int *d) {  // algoritmo anonimo gregoriano
  int a = y % 19, b = y / 100, c = y % 100, dd = b / 4, e = b % 4;
  int f = (b + 8) / 25, g = (b - f + 1) / 3, h = (19 * a + b - dd - g + 15) % 30;
  int i = c / 4, k = c % 4, l = (32 + 2 * e + 2 * i - h - k) % 7;
  int mm = (a + 11 * h + 22 * l) / 451;
  *m = (h + l - 7 * mm + 114) / 31;
  *d = ((h + l - 7 * mm + 114) % 31) + 1;
}

// dias desde una fecha fija (para restar fechas sin liarse con los meses)
static long dayNum(int y, int m, int d) {
  if (m < 3) { y--; m += 12; }
  return 365L * y + y / 4 - y / 100 + y / 400 + (153 * (m - 3) + 2) / 5 + d;
}

// dia del n-esimo `wd` (0 = domingo) del mes
static int nthWeekday(int y, int m, int wd, int n) {
  int first = holWeekday(y, m, 1);
  return 1 + (wd - first + 7) % 7 + (n - 1) * 7;
}

uint8_t holidayOn(int y, int m, int d, int bdayM, int bdayD) {
  if (bdayM && m == bdayM && d == bdayD) return HOL_BIRTHDAY;
  long today = dayNum(y, m, d);
  int ly = y - HOL_LUNAR_FIRST;
  if (ly >= 0 && ly < HOL_LUNAR_YEARS) {
    long cny = dayNum(y, LUNAR[ly][0][0], LUNAR[ly][0][1]);
    if (today == cny + 14) return HOL_LANTERN;
    if (today == dayNum(y, LUNAR[ly][1][0], LUNAR[ly][1][1])) return HOL_DRAGONBOAT;
    long ma = dayNum(y, LUNAR[ly][2][0], LUNAR[ly][2][1]);
    if (today >= ma - 1 && today <= ma + 1) return HOL_MIDAUTUMN;
    if (today >= cny - 1 && today <= cny + 6) return HOL_CNY;
  }
  if (m == 2 && d == 14) return HOL_VALENTINE;
  int em, ed;
  holEaster(y, &em, &ed);
  long easter = dayNum(y, em, ed);
  if (today >= easter - 2 && today <= easter + 1) return HOL_EASTER;
  if (m == 5 && d == nthWeekday(y, 5, 0, 2)) return HOL_MOTHER;
  if (m == 6 && d == nthWeekday(y, 6, 0, 3)) return HOL_FATHER;
  if (m == 7 && d == 1) return HOL_CANADA;
  if (m == 10) {
    int tg = nthWeekday(y, 10, 1, 2);  // 2o lunes
    if (d >= tg - 2 && d <= tg) return HOL_THANKSGIVING;
    if (d >= 25) return HOL_HALLOWEEN;
  }
  if (m == 12 && d >= 20 && d <= 26) return HOL_CHRISTMAS;
  if ((m == 12 && d == 31) || (m == 1 && d == 1)) return HOL_NEWYEAR;
  return HOL_NONE;
}
