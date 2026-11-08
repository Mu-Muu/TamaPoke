#pragma once
#include <stdint.h>

// Copia de seguridad de la partida en la SD (/backup.bin): los espacios de la
// NVS "tamapoke" (mascota, pokedex, puntos, mochila, idioma...), "chess" y
// "fest". La red y su contraseña ("wifi") NO se copian. Solo hay una: cada copia
// sustituye a la anterior.
bool backupSave(uint32_t wallEpoch);   // hora local de la copia
bool backupInfo(uint32_t &wallEpoch);  // ¿hay copia? y de cuando
// vuelca la copia en la NVS (borra antes esos espacios). Despues hay que
// reiniciar sin guardar nada mas (la mascota en RAM es la de antes).
bool backupRestore();
