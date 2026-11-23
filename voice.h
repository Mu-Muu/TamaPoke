#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "mail.h"  // MT_*, MAIL_PHRASES

// La voz de un amigo en el correo: al abrir lo que manda, suena su grabacion (si la hay). Las
// grabaciones de cada amigo van en la SD de quien las recibe, una por mensaje:
//   /voice/<amigo>/NN.wav   <amigo> = los 8 primeros hex de su numero de jugador (como en MAIL INFO)
//   NN = 00-11 la frase de la postal (MAIL_PHRASES, en su orden), 12 una visita, 13 un regalo
// WAV PCM a 16 kHz, 16 bits, mono (tools/voice.py las convierte y send_sd.py --dir las sube).
// Logica pura, con tests en test/test_voice.cpp.

#define VOICE_CLIPS 14
#define VOICE_VISIT 12
#define VOICE_GIFT 13
#define VOICE_RATE 16000
#define VOICE_MAX_BYTES (12UL * VOICE_RATE * 2)  // 12 s: en PSRAM mientras suena

// que grabacion va con un mensaje: -1 ninguna
inline int voiceClip(uint8_t type, uint8_t phrase) {
  if (type == MT_CARD) return phrase < MAIL_PHRASES ? phrase : -1;
  if (type == MT_VISIT) return VOICE_VISIT;
  if (type == MT_GIFT) return VOICE_GIFT;
  return -1;
}

inline void voicePath(char *out, size_t cap, uint64_t friendPid, int clip) {
  snprintf(out, cap, "/voice/%08lx/%02d.wav", (unsigned long)(friendPid >> 32), clip);
}

static inline uint32_t voiceLe32(const uint8_t *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static inline uint16_t voiceLe16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }

// En la cabecera (los primeros n bytes de un fichero de fileSize bytes), donde empiezan las muestras
// y cuantos bytes son. Solo PCM 16 kHz, 16 bits, mono: lo demas, false. El largo se recorta a lo que
// cabe en el fichero y a VOICE_MAX_BYTES, en muestras enteras.
inline bool voiceWav(const uint8_t *h, size_t n, uint32_t fileSize, uint32_t &off, uint32_t &len) {
  if (n < 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) return false;
  bool fmtOk = false;
  size_t p = 12;
  while (p + 8 <= n) {
    uint32_t sz = voiceLe32(h + p + 4);
    if (!memcmp(h + p, "fmt ", 4)) {
      if (sz < 16 || p + 8 + 16 > n) return false;
      const uint8_t *f = h + p + 8;
      fmtOk = voiceLe16(f) == 1 && voiceLe16(f + 2) == 1 && voiceLe32(f + 4) == VOICE_RATE && voiceLe16(f + 14) == 16;
      if (!fmtOk) return false;
    } else if (!memcmp(h + p, "data", 4)) {
      if (!fmtOk) return false;
      off = (uint32_t)(p + 8);
      if (off >= fileSize) return false;
      uint32_t hay = fileSize - off;
      len = sz < hay ? sz : hay;
      if (len > VOICE_MAX_BYTES) len = VOICE_MAX_BYTES;
      len &= ~1u;
      return len > 0;
    }
    if (sz > n) return false;  // un bloque mas largo que la cabecera leida: no es lo que se espera
    p += 8 + sz + (sz & 1);    // los bloques se rellenan hasta un numero par
  }
  return false;
}
