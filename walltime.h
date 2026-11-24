#pragma once
#include <stddef.h>
#include <stdint.h>

// Hora local a partir de la UTC (la del NTP), sin tocar la variable TZ: el
// resto del firmware cuenta con TZ = UTC para pasar la hora "de pared" a epoch
// (ver rtcbat.cpp), y cambiarla desde la tarea de red la estropearia.
// La zona va en config.h: TZ_STD_OFFSET_H (desfase normal) y, si TZ_DST_NA,
// una hora mas desde el 2o domingo de marzo a las 2:00 hasta el 1er domingo de
// noviembre a las 2:00 (la regla de EE. UU. y Canada, la misma que LOCAL_TZ).
// Con una ciudad en el portal Wi-Fi manda su desfase de ahora (con el horario de verano ya
// dentro), que da Open-Meteo con cada tiempo: el reloj se corrige con el NTP que va justo
// detras, asi que un cambio de hora llega a la vez que llegaba con la regla de config.h.
bool localIsDst(uint32_t utc);
uint32_t wallFromUtc(uint32_t utc);  // hora local "de pared" como epoch
void wallSetCityOffset(int32_t secs);  // segundos al este de UTC; fuera de +-14 h no se hace caso
void wallClearCityOffset();            // vuelve la zona de config.h

// la hora de un reloj, para enseñarla (issue #15): con 24 h "07:05" y devuelve ""; con 12 h "7:05"
// y devuelve "AM" o "PM" (las 0:05 son las 12:05 AM y las 12:05, las 12:05 PM)
const char *clockHHMM(uint32_t wall, bool h12, char *out, size_t n);
