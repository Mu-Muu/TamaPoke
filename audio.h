#pragma once
#include <stdint.h>

// Efectos de sonido del juego (cola, no bloqueante). El orden coincide con la
// tabla SFX de audio.cpp.
enum Sfx : uint8_t {
  SFX_TAP = 0,  // tocar / boton
  SFX_EAT,      // comer
  SFX_PLAY,     // punto del minijuego / golpe
  SFX_HEART,    // le gusta / mimo
  SFX_HATCH,    // eclosion
  SFX_EVOLVE,   // evolucion
  SFX_MEDAL,    // medalla / hito
  SFX_DENY,     // accion no permitida
  SFX_BYE,      // despedida
  SFX_LEVEL,    // sube de nivel
  SFX_COUNT
};

void audioBegin();          // init ES8311 + I2S + amplificador + tarea de audio
void sfxPlay(uint8_t id);   // encola un efecto (no bloquea el loop)
void audioSetEnabled(bool on);
bool audioEnabled();
// niveles de volumen de ajustes (issue #9, como el #21 del original): 0 = alto (el de siempre),
// 1 = medio, 2 = bajo
#define VOL_LEVELS 3
void audioSetVolume(uint8_t level);  // guarda el nivel en NVS
uint8_t audioVolume();
// amplitud de la onda cuadrada para cada nivel: unos -8 dB y -17 dB respecto al alto
inline int16_t sfxAmp(uint8_t level) { return level == 1 ? 2000 : level >= 2 ? 700 : 5000; }
// el boton de sonido cicla alto, medio, bajo y apagado: el siguiente (VOL_LEVELS = apagado)
inline uint8_t volCycle(uint8_t cur) { return cur >= VOL_LEVELS ? 0 : cur + 1; }
void audioSetSleeping(bool sleeping);  // dormida: amplificador apagado
bool audioBusy();                      // un efecto pedido en reposo (o una voz) aun puede estar sonando: no dormir la CPU
void audioSetIdle(bool idle);          // sin uso reciente: amplificador apagado (un efecto lo enciende 4 s)
// voz de un amigo (voice.h): n muestras PCM mono a 16 kHz en PSRAM (ps_malloc). Suena despues de los
// efectos pendientes, con el volumen de ajustes, tambien con la mascota dormida (la pide quien abre
// el correo), y la tarea de audio la libera al acabar. false si ahora no puede sonar (sin sonido,
// cola llena): entonces la libera quien llama
bool voicePlay(int16_t *pcm, uint32_t n);
