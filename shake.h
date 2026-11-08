#pragma once
#include <stdint.h>
#include <math.h>

// Sacudidas a partir del acelerometro (QMI8658), en g. Sin depender del
// hardware: el sketch le pasa cada lectura y aqui se decide.
//
// Un "pico" es salirse de 1 g (la gravedad, en reposo) mas de SHAKE_PEAK_G y
// volver por debajo de SHAKE_RESET_G (histeresis: una sola sacudida no cuenta
// como varias). Mover la placa de un lado a otro da un pico en cada extremo.
//   SHAKE_SOFT: SHAKE_PEAKS picos en SHAKE_WIN_MS (agitar el huevo)
//   SHAKE_HARD: SHAKE_HARD_PEAKS picos fuertes (>= SHAKE_HARD_G) en
//               SHAKE_HARD_WIN_MS (agitarlo con ganas: se marea)
#define SHAKE_PEAK_G 0.8f
#define SHAKE_RESET_G 0.35f
#define SHAKE_HARD_G 1.8f
#define SHAKE_PEAKS 3
#define SHAKE_WIN_MS 1000
#define SHAKE_HARD_PEAKS 4
#define SHAKE_HARD_WIN_MS 1500

enum : uint8_t { SHAKE_NONE = 0, SHAKE_SOFT = 1, SHAKE_HARD = 2 };  // bits

struct ShakeDetector {
  bool inPeak = false;
  float peakMax = 0;
  uint32_t soft[SHAKE_PEAKS] = { 0 };       // instantes de los ultimos picos
  uint32_t hard[SHAKE_HARD_PEAKS] = { 0 };  // y de los ultimos picos fuertes
  uint8_t nSoft = 0, nHard = 0;

  // una lectura (en g) en el instante ms; devuelve SHAKE_* (bits)
  uint8_t feed(float ax, float ay, float az, uint32_t ms) {
    float dev = fabsf(sqrtf(ax * ax + ay * ay + az * az) - 1.0f);
    if (!inPeak) {
      if (dev >= SHAKE_PEAK_G) {
        inPeak = true;
        peakMax = dev;
      }
      return SHAKE_NONE;
    }
    if (dev > peakMax) peakMax = dev;
    if (dev >= SHAKE_RESET_G) return SHAKE_NONE;
    inPeak = false;  // fin del pico: se cuenta
    uint8_t ev = SHAKE_NONE;
    if (push(soft, nSoft, SHAKE_PEAKS, ms, SHAKE_WIN_MS)) {
      ev |= SHAKE_SOFT;
      nSoft = 0;
    }
    if (peakMax >= SHAKE_HARD_G && push(hard, nHard, SHAKE_HARD_PEAKS, ms, SHAKE_HARD_WIN_MS)) {
      ev |= SHAKE_HARD;
      nHard = 0;
    }
    return ev;
  }

private:
  // apunta un pico; true si ya hay n dentro de la ventana
  static bool push(uint32_t *t, uint8_t &cnt, uint8_t n, uint32_t ms, uint32_t win) {
    if (cnt == n) {  // lleno: se descarta el mas viejo
      for (uint8_t i = 1; i < n; i++) t[i - 1] = t[i];
      cnt--;
    }
    t[cnt++] = ms;
    return cnt == n && ms - t[0] <= win;
  }
};
