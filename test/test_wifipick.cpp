// Que red Wi-Fi guardada probar primero (wifipick.h).
#include "framework.h"
#include "../wifipick.h"

TEST(wifipick, se_prueba_primero_la_de_mas_senal) {
  const char *saved[WIFI_SLOTS] = { "Casa", "Cole", "" };
  const char *seen[] = { "Vecino", "Cole", "Casa" };
  const int16_t rssi[] = { -40, -55, -72 };
  uint8_t out[WIFI_SLOTS];
  CHECK_EQ(wifiOrder(saved, seen, rssi, 3, -1, out), (uint8_t)2);
  CHECK_EQ(out[0], (uint8_t)1);  // Cole, -55
  CHECK_EQ(out[1], (uint8_t)0);  // Casa, -72
}

TEST(wifipick, las_que_no_se_ven_no_se_prueban) {
  const char *saved[WIFI_SLOTS] = { "Casa", "Cole", "Movil" };
  const char *seen[] = { "Movil", "Otra" };
  const int16_t rssi[] = { -60, -30 };
  uint8_t out[WIFI_SLOTS];
  CHECK_EQ(wifiOrder(saved, seen, rssi, 2, 0, out), (uint8_t)1);
  CHECK_EQ(out[0], (uint8_t)2);
}

TEST(wifipick, un_nombre_repetido_cuenta_con_su_mejor_senal) {
  const char *saved[WIFI_SLOTS] = { "Casa", "Cole", nullptr };
  const char *seen[] = { "Casa", "Cole", "Casa" };  // dos puntos de acceso de Casa
  const int16_t rssi[] = { -80, -60, -45 };
  uint8_t out[WIFI_SLOTS];
  CHECK_EQ(wifiOrder(saved, seen, rssi, 3, -1, out), (uint8_t)2);
  CHECK_EQ(out[0], (uint8_t)0);
  CHECK_EQ(out[1], (uint8_t)1);
}

TEST(wifipick, sin_ver_ninguna_se_prueba_la_ultima_buena) {
  const char *saved[WIFI_SLOTS] = { "Casa", "Oculta", "" };
  uint8_t out[WIFI_SLOTS];
  CHECK_EQ(wifiOrder(saved, nullptr, nullptr, 0, 1, out), (uint8_t)1);
  CHECK_EQ(out[0], (uint8_t)1);  // puede ser una red oculta
  CHECK_EQ(wifiOrder(saved, nullptr, nullptr, 0, -1, out), (uint8_t)1);
  CHECK_EQ(out[0], (uint8_t)0);  // sin ultima buena: la primera guardada
}

TEST(wifipick, la_ultima_buena_vaciada_no_se_prueba) {
  const char *saved[WIFI_SLOTS] = { "", "Cole", "" };
  uint8_t out[WIFI_SLOTS];
  CHECK_EQ(wifiOrder(saved, nullptr, nullptr, 0, 0, out), (uint8_t)1);
  CHECK_EQ(out[0], (uint8_t)1);
}

TEST(wifipick, sin_redes_guardadas_nada) {
  const char *saved[WIFI_SLOTS] = { "", nullptr, "" };
  const char *seen[] = { "Casa" };
  const int16_t rssi[] = { -50 };
  uint8_t out[WIFI_SLOTS];
  CHECK_EQ(wifiOrder(saved, seen, rssi, 1, 0, out), (uint8_t)0);
}

TEST(wifipick, los_nombres_distinguen_mayusculas) {
  const char *saved[WIFI_SLOTS] = { "casa", "", "" };
  const char *seen[] = { "Casa" };
  const int16_t rssi[] = { -50 };
  uint8_t out[WIFI_SLOTS];
  CHECK_EQ(wifiOrder(saved, seen, rssi, 1, -1, out), (uint8_t)1);  // no se ve: a ciegas
  CHECK_EQ(out[0], (uint8_t)0);
}
