#pragma once
#include <Arduino.h>
#include "pet.h"

// Granja: los compañeros guardados en la SD (alli siguen creciendo, sanos y a
// 1/FARM_GROWTH_DIV de velocidad, sin evolucionar). Un fichero por
// individuo, /farm/fNNNN.bin con su PetRec, nombrado por especie: como no puede
// haber dos de la misma especie, el nombre ya garantiza que no se repiten.
// Sin SD no hay granja (farmReady() false).

struct FarmEntry {
  int16_t speciesId;
  bool shiny;
  uint32_t ageMinutes, farmEpoch;  // edad al entrar y cuando entro (crece alli, ver farmAge)
  char nick[12];
};
// nivel de hoy (a la hora local `now`): en la granja tambien sube, mas despacio
static inline uint16_t farmLevel(const FarmEntry &e, uint32_t now) { return levelOfAge(farmAge(e.ageMinutes, e.farmEpoch, now)); }

bool farmLoad(Pet &p);                 // lee el indice de la SD y rellena p.farmReg
bool farmReady();                      // hay SD (y por tanto granja)
int farmCount();
const FarmEntry &farmAt(int i);        // ordenada por especie
bool farmPut(Pet &p, const PetRec &r); // guarda (o reemplaza) y lo marca en p.farmReg
bool farmTake(Pet &p, int16_t dex, PetRec &out);  // lo lee, lo borra y lo desmarca
