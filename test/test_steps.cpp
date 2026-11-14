// Pasos a partir del acelerometro (steps.h) con senales sinteticas de andar.
#include "framework.h"
#include "../steps.h"

static uint32_t semilla = 1;
static float ruido(float amp) {  // pseudoaleatorio fijo: los tests siempre dan lo mismo
  semilla = semilla * 1103515245u + 12345u;
  return ((float)((semilla >> 16) & 0x7fff) / 32767.0f * 2.0f - 1.0f) * amp;
}

// |a| de una pisada en el instante f (ms desde que empieza): golpe hacia arriba
// de amplitud amp durante 100 ms y rebote hacia abajo durante 150 ms
static float pisada(uint32_t f, float amp) {
  const float PI_F = 3.14159265f;
  if (f < 100) return amp * sinf(PI_F * f / 100.0f);
  if (f < 250) return -0.6f * amp * sinf(PI_F * (f - 100) / 150.0f);
  return 0;
}

// n pasos cada periodo ms (desde t = 1 s), leidos cada dt ms; devuelve los contados
static uint32_t andar(int n, uint32_t periodo, float amp, uint32_t dt, float ruidoAmp = 0.012f) {
  StepDetector d;
  semilla = 1;
  uint32_t fin = 1000 + n * periodo + 2000;
  for (uint32_t t = 0; t < fin; t += dt) {
    float m = 1.0f + ruido(ruidoAmp);
    if (t >= 1000 && (t - 1000) / periodo < (uint32_t)n) m += pisada((t - 1000) % periodo, amp);
    d.feed(m, t);
  }
  return d.total;
}

TEST(steps, cuarenta_pasos_normales_a_62_hz) {
  CHECK_RANGE(andar(40, 550, 0.25f, 16), 39u, 40u);
}

TEST(steps, igual_leyendo_a_25_hz) {
  CHECK_RANGE(andar(40, 550, 0.25f, 40), 38u, 40u);
}

TEST(steps, pisadas_suaves_tambien_cuentan) {
  CHECK_RANGE(andar(40, 550, 0.13f, 16), 36u, 40u);
}

TEST(steps, paso_lento_de_un_segundo) {
  CHECK_RANGE(andar(20, 1000, 0.25f, 16), 19u, 20u);
}

TEST(steps, mas_lento_que_un_paso_no_es_andar) {
  CHECK_EQ(andar(10, 1600, 0.3f, 16), 0u);  // cada golpe vuelve a empezar
}

TEST(steps, en_reposo_con_ruido_no_cuenta) {
  StepDetector d;
  semilla = 7;
  for (uint32_t t = 0; t < 60000; t += 16) d.feed(1.0f + ruido(0.02f), t);
  CHECK_EQ(d.total, 0u);
}

TEST(steps, golpes_sueltos_no_son_pasos) {
  StepDetector d;
  for (uint32_t t = 0; t < 20000; t += 16) {
    float m = 1.0f;
    if (t >= 2000 && (t - 2000) % 3000 < 250) m += pisada((t - 2000) % 3000, 0.6f);  // uno cada 3 s
    d.feed(m, t);
  }
  CHECK_EQ(d.total, 0u);
}

TEST(steps, una_deriva_lenta_no_cuenta) {
  StepDetector d;
  for (uint32_t t = 0; t < 20000; t += 16) d.feed(1.0f + 0.06f * t / 20000.0f, t);  // 1,00 -> 1,06 g
  CHECK_EQ(d.total, 0u);
}

TEST(steps, tras_una_pausa_vuelve_a_contar) {
  StepDetector d;
  for (uint32_t t = 0; t < 22000; t += 16) {
    float m = 1.0f;
    uint32_t ms = t >= 1000 ? t - 1000 : 0;
    // 15 pasos, 5 s quieto, 15 pasos
    if (t >= 1000 && ms < 15 * 550) m += pisada(ms % 550, 0.25f);
    else if (t >= 1000 && ms >= 15 * 550 + 5000 && ms < 30 * 550 + 5000) m += pisada((ms - 5000) % 550, 0.25f);
    d.feed(m, t);
  }
  CHECK_RANGE(d.total, 28u, 30u);
}

TEST(steps, take_recoge_y_vacia) {
  StepDetector d;
  for (uint32_t t = 0; t < 1000 + 10 * 550; t += 16)
    d.feed(1.0f + (t >= 1000 ? pisada((t - 1000) % 550, 0.25f) : 0), t);
  uint16_t n = d.take();
  CHECK_EQ((uint32_t)n, d.total);
  CHECK(n >= 9);
  CHECK_EQ(d.take(), (uint16_t)0);
}

TEST(steps, instantes_que_retroceden_no_rompen_nada) {
  // las muestras de la FIFO llevan instantes reconstruidos: a veces se pisan
  StepDetector d;
  uint32_t t = 0;
  for (int i = 0; i < 4000; i++) {
    t += (i % 50 == 49) ? (uint32_t)-8 : 16u;  // de vez en cuando, 8 ms hacia atras
    float m = 1.0f + (t >= 1000 ? pisada((t - 1000) % 550, 0.25f) : 0);
    d.feed(m, t);
  }
  CHECK(d.total > 90);
  CHECK(d.total < 130);
}
