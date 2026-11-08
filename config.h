#pragma once

// Datos personales del dueño: nombre, cuenta de chess.com, lugar del tiempo y
// zona horaria. Los de verdad van en config_private.h, que NO se sube al
// repositorio (esta en .gitignore): copia config_private.example.h como
// config_private.h y rellenalo. Sin ese archivo se compila con estos valores de
// ejemplo.
#if __has_include("config_private.h")
#include "config_private.h"
#endif

#ifndef OWNER_NAME
#define OWNER_NAME "Trainer"  // en algunas frases ("I love you, ...!")
#endif
static_assert(sizeof(OWNER_NAME) <= 17, "OWNER_NAME: 16 bytes como mucho (cabe en los bocadillos)");

#ifndef CHESS_USER
#define CHESS_USER ""  // usuario de chess.com en minusculas; vacio = sin ajedrez
#endif

// tiempo de Open-Meteo: coordenadas y zona IANA (con "/" escrito %2F)
#ifndef WX_LAT
#define WX_LAT "40.71"
#endif
#ifndef WX_LON
#define WX_LON "-74.01"
#endif
#ifndef WX_TZ
#define WX_TZ "America%2FNew_York"
#endif

// hora local: la cadena POSIX para el NTP y, para pasar la UTC a hora de pared
// sin tocar TZ (walltime.cpp), el desfase normal y si hay horario de verano con
// la regla de EE. UU. y Canada (2o domingo de marzo - 1er domingo de noviembre)
#ifndef LOCAL_TZ
#define LOCAL_TZ "EST5EDT,M3.2.0,M11.1.0"
#endif
#ifndef TZ_STD_OFFSET_H
#define TZ_STD_OFFSET_H (-5)
#endif
#ifndef TZ_DST_NA
#define TZ_DST_NA 1
#endif
