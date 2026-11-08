// Deteccion de sacudidas (shake.h) con lecturas sinteticas del acelerometro.
#include "framework.h"
#include "../shake.h"

// una sacudida de amplitud amp (g de desvio) a lo largo del eje x, con picos
// cada periodMs: entre pico y pico, reposo (1 g en z). 20 ms por lectura.
static uint8_t shakeFor(ShakeDetector &d, float amp, uint32_t periodMs, int peaks, uint32_t &t) {
  uint8_t ev = 0;
  for (int p = 0; p < peaks; p++) {
    for (uint32_t k = 0; k < periodMs; k += 20, t += 20) {
      bool pico = k < 60;  // 60 ms fuera de 1 g
      ev |= d.feed(pico ? amp : 0.0f, 0.0f, pico ? 1.0f + amp : 1.0f, t);
    }
  }
  return ev;
}

TEST(shake, en_reposo_no_pasa_nada) {
  ShakeDetector d;
  uint8_t ev = 0;
  for (uint32_t t = 0; t < 5000; t += 20) ev |= d.feed(0.02f, -0.01f, 0.99f, t);
  CHECK_EQ(ev, (uint8_t)SHAKE_NONE);
}

TEST(shake, tres_picos_en_un_segundo_es_agitar) {
  ShakeDetector d;
  uint32_t t = 0;
  CHECK_EQ(shakeFor(d, 1.0f, 300, 3, t), (uint8_t)SHAKE_SOFT);
}

TEST(shake, picos_lentos_no_cuentan) {
  ShakeDetector d;
  uint32_t t = 0;
  CHECK_EQ(shakeFor(d, 1.0f, 700, 6, t), (uint8_t)SHAKE_NONE);  // 3 picos ocupan 1,4 s
}

TEST(shake, temblor_pequeno_no_cuenta) {
  ShakeDetector d;
  uint32_t t = 0;
  CHECK_EQ(shakeFor(d, 0.4f, 200, 10, t), (uint8_t)SHAKE_NONE);
}

TEST(shake, agitar_fuerte_marea) {
  ShakeDetector d;
  uint32_t t = 0;
  uint8_t ev = shakeFor(d, 2.5f, 250, 4, t);
  CHECK(ev & SHAKE_HARD);
  CHECK(ev & SHAKE_SOFT);  // tambien cuenta como agitar
}

TEST(shake, agitar_suave_no_marea) {
  ShakeDetector d;
  uint32_t t = 0;
  CHECK(!(shakeFor(d, 1.2f, 250, 8, t) & SHAKE_HARD));
}

TEST(shake, tras_avisar_vuelve_a_contar_desde_cero) {
  ShakeDetector d;
  uint32_t t = 0;
  CHECK_EQ(shakeFor(d, 1.0f, 300, 3, t), (uint8_t)SHAKE_SOFT);
  CHECK_EQ(shakeFor(d, 1.0f, 300, 2, t), (uint8_t)SHAKE_NONE);  // 2 picos mas no bastan
  CHECK_EQ(shakeFor(d, 1.0f, 300, 1, t), (uint8_t)SHAKE_SOFT);
}
