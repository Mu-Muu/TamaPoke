#pragma once
#include <stdint.h>
#include <string.h>

// Hasta tres redes Wi-Fi guardadas (casa, colegio, el movil...). Con mas de una
// se escanea y se prueban las que se ven, de mas a menos senal. Si no se ve
// ninguna, se prueba una a ciegas (la ultima con la que se conecto, o la
// primera guardada): puede ser una red oculta, que no sale en el escaneo.
// Sin hardware, para probarlo en el PC; el escaneo y la conexion estan en
// wifisync.cpp.
#define WIFI_SLOTS 3

// saved: nombre de cada hueco ("" o nullptr = vacio); seen/rssi: lo escaneado.
// Deja en out los huecos a probar, en orden, y devuelve cuantos son.
inline uint8_t wifiOrder(const char *const saved[WIFI_SLOTS], const char *const *seen, const int16_t *rssi,
                         int nSeen, int8_t lastGood, uint8_t out[WIFI_SLOTS]) {
  int16_t best[WIFI_SLOTS];
  bool visible[WIFI_SLOTS] = { false };
  for (uint8_t i = 0; i < WIFI_SLOTS; i++) {
    best[i] = INT16_MIN;
    if (!saved[i] || !saved[i][0]) continue;
    for (int j = 0; j < nSeen; j++) {  // un mismo nombre puede salir varias veces (varios puntos de acceso)
      if (!seen[j] || strcmp(seen[j], saved[i]) != 0) continue;
      visible[i] = true;
      if (rssi[j] > best[i]) best[i] = rssi[j];
    }
  }
  uint8_t n = 0;
  for (;;) {  // las que se ven, la de mas senal primero (a igualdad, el hueco mas bajo)
    int8_t m = -1;
    for (uint8_t i = 0; i < WIFI_SLOTS; i++)
      if (visible[i] && (m < 0 || best[i] > best[m])) m = (int8_t)i;
    if (m < 0) break;
    out[n++] = (uint8_t)m;
    visible[m] = false;
  }
  if (n) return n;
  if (lastGood >= 0 && lastGood < WIFI_SLOTS && saved[lastGood] && saved[lastGood][0]) {
    out[0] = (uint8_t)lastGood;
    return 1;
  }
  for (uint8_t i = 0; i < WIFI_SLOTS; i++) {
    if (saved[i] && saved[i][0]) {
      out[0] = i;
      return 1;
    }
  }
  return 0;
}
