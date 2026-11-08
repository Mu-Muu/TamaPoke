#pragma once
#include <stdint.h>

// Fiestas por la fecha local del RTC: el fondo se decora y el bicho saluda.
// Las del calendario lunar (Año Nuevo chino, Linternas, Barcas del Dragon,
// Medio Otoño) van en una tabla (HOL_LUNAR_FIRST..+N-1); fuera de ella no salen.
enum : uint8_t {
  HOL_NONE = 0,
  HOL_NEWYEAR,      // 31 dic - 1 ene
  HOL_CNY,          // nochevieja china .. 7o dia
  HOL_LANTERN,      // 15 del 1er mes lunar
  HOL_VALENTINE,    // 14 feb
  HOL_EASTER,       // viernes santo .. lunes de pascua
  HOL_MOTHER,       // 2o domingo de mayo
  HOL_DRAGONBOAT,   // 5 del 5o mes lunar
  HOL_FATHER,       // 3er domingo de junio
  HOL_CANADA,       // 1 jul
  HOL_MIDAUTUMN,    // 15 del 8o mes lunar (y la vispera y el dia despues)
  HOL_THANKSGIVING, // Canada: sabado..lunes del 2o lunes de octubre
  HOL_HALLOWEEN,    // 25 - 31 oct
  HOL_CHRISTMAS,    // 20 - 26 dic
  HOL_BIRTHDAY,     // cumpleaños del dueño (si se ha puesto la fecha)
  HOL_COUNT
};

#define HOL_LUNAR_FIRST 2026
#define HOL_LUNAR_YEARS 20

// fiesta del dia (y, m 1-12, d 1-31); bdayM/bdayD = 0 si no hay cumpleaños
uint8_t holidayOn(int y, int m, int d, int bdayM, int bdayD);
// estacion (meteorologica, para un sitio de inviernos largos: el invierno va
// de diciembre a marzo, que es cuando hay nieve)
enum : uint8_t { SEASON_SPRING = 0, SEASON_SUMMER, SEASON_AUTUMN, SEASON_WINTER };
uint8_t seasonOf(int m);  // mes 1-12
// dia de la semana (0 = domingo)
int holWeekday(int y, int m, int d);
// domingo de Pascua de ese año: *m, *d
void holEaster(int y, int *m, int *d);
