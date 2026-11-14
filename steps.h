#pragma once
#include <stdint.h>
#include <math.h>

// Pasos a partir del acelerometro (|a| en g), sin depender del hardware: el
// sketch le pasa cada muestra de la FIFO del QMI8658 con su instante.
//
// El podometro del propio QMI8658 no se usa: en esta placa se activa bien pero
// nunca cuenta (probado con cinco configuraciones, entre ellas las de los
// ejemplos de SensorLib; ShadowEnemyx/TamaPoke llego a lo mismo). La deteccion
// es la de su imu.cpp (MIT): una linea base lenta de |a|; un paso es un impulso
// positivo de mas de STEP_PEAK_G que luego vuelve por debajo de STEP_RESET_G
// (el rebote del mismo paso no cuenta otra vez), con STEP_MIN_GAP_MS a
// STEP_MAX_GAP_MS entre pasos. Hasta ver dos seguidos con ese ritmo no cuenta
// nada (un golpe suelto no es andar); entonces cuentan los dos.
// Aqui la linea base va por tiempo y no por muestra, para que de igual leer a
// 25 Hz que a 62,5 Hz (alli se leia cada ~40 ms).
#define STEP_PEAK_G 0.085f
#define STEP_RESET_G 0.025f
#define STEP_CALM_G 0.08f          // por debajo, la linea base sigue deprisa a |a|
#define STEP_TAU_CALM_MS 310.0f    // constantes de tiempo de la linea base
#define STEP_TAU_MOVE_MS 1580.0f
#define STEP_MIN_GAP_MS 280
#define STEP_MAX_GAP_MS 1400
#define STEP_SETTLE_MS 400         // al empezar, solo se ajusta la linea base

struct StepDetector {
  float base = 0;
  bool inPeak = false;
  bool started = false;
  uint32_t first = 0, lastT = 0, lastPeak = 0;
  uint8_t cadence = 0;     // pasos seguidos con ritmo (1 = solo el primero)
  uint32_t total = 0;      // pasos contados desde el arranque (diagnostico)
  uint16_t pending = 0;    // contados y aun no recogidos con take()

  // una muestra (|a| en g) en el instante ms; devuelve los pasos nuevos (0, 1 o 2)
  uint16_t feed(float mag, uint32_t ms) {
    if (!started) {
      started = true;
      base = mag;
      first = lastT = ms;
      return 0;
    }
    int32_t dt = (int32_t)(ms - lastT);
    if (dt < 0) dt = 0;  // instantes reconstruidos que se pisan: sin avance
    lastT = ms;
    float signal = mag - base;
    float tau = fabsf(signal) < STEP_CALM_G ? STEP_TAU_CALM_MS : STEP_TAU_MOVE_MS;
    base += (mag - base) * (1.0f - expf(-(float)dt / tau));
    if (ms - first < STEP_SETTLE_MS) return 0;

    if (inPeak) {  // el impulso tiene que apagarse antes del siguiente
      if (signal <= STEP_RESET_G) inPeak = false;
      return 0;
    }
    if (signal < STEP_PEAK_G) return 0;  // solo el golpe positivo, no el rebote
    inPeak = true;

    uint32_t gap = lastPeak ? ms - lastPeak : 0;
    if (lastPeak && gap < STEP_MIN_GAP_MS) return 0;
    lastPeak = ms;
    if (!gap || gap > STEP_MAX_GAP_MS) {  // primero de una tanda: aun no es andar
      cadence = 1;
      return 0;
    }
    uint16_t n = cadence == 1 ? 2 : 1;  // al confirmarse, el primero cuenta tambien
    if (cadence < 255) cadence++;
    total += n;
    pending = pending + n > 60000 ? 60000 : (uint16_t)(pending + n);
    return n;
  }

  // los pasos contados desde la ultima vez
  uint16_t take() {
    uint16_t n = pending;
    pending = 0;
    return n;
  }
};
