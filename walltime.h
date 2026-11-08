#pragma once
#include <stdint.h>

// Hora local a partir de la UTC (la del NTP), sin tocar la variable TZ: el
// resto del firmware cuenta con TZ = UTC para pasar la hora "de pared" a epoch
// (ver rtcbat.cpp), y cambiarla desde la tarea de red la estropearia.
// La zona va en config.h: TZ_STD_OFFSET_H (desfase normal) y, si TZ_DST_NA,
// una hora mas desde el 2o domingo de marzo a las 2:00 hasta el 1er domingo de
// noviembre a las 2:00 (la regla de EE. UU. y Canada, la misma que LOCAL_TZ).
bool localIsDst(uint32_t utc);
uint32_t wallFromUtc(uint32_t utc);  // hora local "de pared" como epoch
