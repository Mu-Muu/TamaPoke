#pragma once
#include <stdint.h>

// Pasos del movil: un Atajo de iPhone (app Atajos) lee los pasos de hoy de la app
// Salud y llama a http://tamapoke.local/steps?n=PASOS&k=CODIGO. Solo en la red de casa
// y con la placa en linea (USB puesto). CODIGO: 4 cifras que salen en la pagina de
// estadisticas, para que otro aparato de la red no pueda tocar los pasos.
// El servidor vive en la tarea de red (otro nucleo); los pasos pasan al bucle
// principal por phoneStepsTake(), que es quien toca la mascota.
void phoneStepsLoop();     // tarea de red, conectada: arranca el servidor si hace falta y atiende
void phoneStepsStop();     // tarea de red: al desconectar o abrir el portal
bool phoneStepsOn();
bool phoneStepsTake(uint32_t &n);  // bucle principal: los pasos que llegaron (una vez)
// Practica de hockey (SuperDeker): otro Atajo pregunta los minutos y llama a
// http://tamapoke.local/hockey?min=MINUTOS&k=CODIGO (el mismo codigo). Se van sumando
bool phoneHockeyTake(uint16_t &min);  // bucle principal: los minutos que llegaron desde la ultima vez
// Premio de casa: el Atajo del movil de un padre pregunta cuantos puntos y llama a
// http://tamapoke.local/reward?pts=PUNTOS&k=CODIGO (de 1 a REWARD_PTS_REQ, pet.h; el mismo codigo)
bool phoneRewardTake(uint8_t &pts);   // bucle principal: los puntos que llegaron desde la ultima vez
uint16_t phoneStepsCode();         // el codigo (se crea al azar la primera vez, NVS "phone")

// El codigo tiene 4 cifras: sin freno, otro aparato de la red los prueba todos en unos minutos (y
// con /reward se da puntos). Tras PHONE_CODE_TRIES fallos seguidos, nada (ni el bueno) durante
// PHONE_CODE_LOCK_MS: probarlos todos pasa a llevar dias. Un acierto pone la cuenta a cero.
#define PHONE_CODE_TRIES 5
#define PHONE_CODE_LOCK_MS (5UL * 60 * 1000)
struct PhoneCodeGuard {
  uint8_t fails = 0;
  bool locked = false;
  uint32_t until = 0;
  // ¿se puede mirar el codigo ahora? (now = millis)
  bool open(uint32_t now) {
    if (locked && (int32_t)(now - until) < 0) return false;
    locked = false;
    return true;
  }
  // como fue: true si el codigo era el bueno
  void result(bool ok, uint32_t now) {
    if (ok) {
      fails = 0;
      return;
    }
    if (++fails >= PHONE_CODE_TRIES) {
      fails = 0;
      locked = true;
      until = now + PHONE_CODE_LOCK_MS;
    }
  }
};
