// Freno del codigo del movil (phonesteps.h): tras PHONE_CODE_TRIES fallos seguidos, nada durante
// PHONE_CODE_LOCK_MS, ni siquiera el codigo bueno.
#include "framework.h"
#include "../phonesteps.h"

TEST(phonecode, los_aciertos_no_frenan) {
  PhoneCodeGuard g;
  for (int i = 0; i < 50; i++) {
    CHECK(g.open(1000 + i));
    g.result(true, 1000 + i);
  }
  CHECK(g.open(2000));
}

TEST(phonecode, tras_cinco_fallos_seguidos_se_cierra_un_rato) {
  PhoneCodeGuard g;
  uint32_t t = 5000;
  for (int i = 0; i < PHONE_CODE_TRIES - 1; i++) {
    CHECK(g.open(t));
    g.result(false, t);
  }
  CHECK(g.open(t));  // aun no
  g.result(false, t);
  CHECK(!g.open(t));  // el quinto lo cierra
  CHECK(!g.open(t + PHONE_CODE_LOCK_MS - 1));
  CHECK(g.open(t + PHONE_CODE_LOCK_MS));  // pasado el rato, otra vez
}

TEST(phonecode, un_acierto_pone_la_cuenta_a_cero) {
  PhoneCodeGuard g;
  for (int i = 0; i < PHONE_CODE_TRIES - 1; i++) g.result(false, 100);
  g.result(true, 100);  // el bueno, antes del quinto fallo
  for (int i = 0; i < PHONE_CODE_TRIES - 1; i++) g.result(false, 200);
  CHECK(g.open(200));
}

TEST(phonecode, tras_el_cierre_vuelven_a_contar_cinco) {
  PhoneCodeGuard g;
  for (int i = 0; i < PHONE_CODE_TRIES; i++) g.result(false, 0);
  uint32_t t = PHONE_CODE_LOCK_MS;
  CHECK(g.open(t));
  g.result(false, t);
  CHECK(g.open(t));  // un fallo suelto no lo vuelve a cerrar
}

TEST(phonecode, probarlos_todos_lleva_dias) {
  // con el freno, al ritmo mas rapido posible: PHONE_CODE_TRIES intentos por cada cierre
  uint32_t t = 0, probados = 0;
  PhoneCodeGuard g;
  while (probados < 9000) {
    if (!g.open(t)) {
      t += 1000;
      continue;
    }
    g.result(false, t);
    probados++;
  }
  CHECK(t / 1000 / 3600 >= 24);  // mas de un dia (antes: unos minutos)
}

TEST(phonecode, el_reloj_que_da_la_vuelta_no_lo_deja_cerrado) {
  PhoneCodeGuard g;
  uint32_t t = 0xFFFFFFFFu - 1000;  // millis() a punto de dar la vuelta
  for (int i = 0; i < PHONE_CODE_TRIES; i++) g.result(false, t);
  CHECK(!g.open(t + 1000));
  CHECK(g.open(t + PHONE_CODE_LOCK_MS));  // ya dio la vuelta
}
