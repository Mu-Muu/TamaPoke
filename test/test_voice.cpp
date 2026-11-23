// Voz de los amigos en el correo (voice.h): que grabacion toca y la cabecera WAV.
#include "framework.h"
#include "../voice.h"
#include <cstring>
#include <vector>

TEST(voice, cada_mensaje_tiene_su_grabacion) {
  for (uint8_t f = 0; f < MAIL_PHRASES; f++) CHECK_EQ(voiceClip(MT_CARD, f), (int)f);  // la frase de la postal
  CHECK_EQ(voiceClip(MT_VISIT, 0), VOICE_VISIT);
  CHECK_EQ(voiceClip(MT_GIFT, 5), VOICE_GIFT);  // un regalo: la misma, lleve la frase que lleve
  CHECK_EQ(voiceClip(MT_ACK, 0), -1);           // los acuses no se enseñan
  CHECK_EQ(voiceClip(MT_CARD, MAIL_PHRASES), -1);  // una frase que esta placa no tiene
  CHECK_EQ(VOICE_GIFT + 1, VOICE_CLIPS);
}

TEST(voice, la_ruta_lleva_al_amigo_y_el_numero) {
  char p[32];
  voicePath(p, sizeof(p), 0x1a2b3c4daabbccddull, 3);
  CHECK_STREQ(p, "/voice/1a2b3c4d/03.wav");  // como en MAIL INFO: los 8 primeros hex
  voicePath(p, sizeof(p), 0x0000000100000000ull, VOICE_GIFT);
  CHECK_STREQ(p, "/voice/00000001/13.wav");
}

// cabecera WAV como la que deja ffmpeg (y, si se quiere, un bloque LIST antes de los datos)
static std::vector<uint8_t> wav(uint32_t rate, uint16_t ch, uint16_t bits, uint32_t dataLen, bool list = false,
                                uint16_t fmt = 1) {
  std::vector<uint8_t> b;
  auto u32 = [&](uint32_t v) { for (int i = 0; i < 4; i++) b.push_back((uint8_t)(v >> (8 * i))); };
  auto u16 = [&](uint16_t v) { b.push_back((uint8_t)v); b.push_back((uint8_t)(v >> 8)); };
  auto tag = [&](const char *t) { b.insert(b.end(), t, t + 4); };
  tag("RIFF"); u32(0); tag("WAVE");
  tag("fmt "); u32(16); u16(fmt); u16(ch); u32(rate); u32(rate * ch * bits / 8); u16(ch * bits / 8); u16(bits);
  if (list) { tag("LIST"); u32(5); b.insert(b.end(), {'I', 'N', 'F', 'O', 'x'}); b.push_back(0); }  // impar: relleno
  tag("data"); u32(dataLen);
  return b;
}

TEST(voice, encuentra_los_datos_de_un_wav_16k_mono) {
  auto h = wav(16000, 1, 16, 32000);
  uint32_t off = 0, len = 0;
  CHECK(voiceWav(h.data(), h.size(), (uint32_t)h.size() + 32000, off, len));
  CHECK_EQ(off, (uint32_t)44);
  CHECK_EQ(len, (uint32_t)32000);
  auto h2 = wav(16000, 1, 16, 1000, true);  // con LIST (y su byte de relleno) delante
  CHECK(voiceWav(h2.data(), h2.size(), (uint32_t)h2.size() + 1000, off, len));
  CHECK_EQ(off, (uint32_t)h2.size());
  CHECK_EQ(len, (uint32_t)1000);
}

TEST(voice, solo_acepta_el_formato_que_sabe_tocar) {
  uint32_t off, len;
  auto a = wav(44100, 1, 16, 100);
  CHECK(!voiceWav(a.data(), a.size(), 1000, off, len));  // otra frecuencia
  auto b = wav(16000, 2, 16, 100);
  CHECK(!voiceWav(b.data(), b.size(), 1000, off, len));  // estereo
  auto c = wav(16000, 1, 8, 100);
  CHECK(!voiceWav(c.data(), c.size(), 1000, off, len));  // 8 bits
  auto d = wav(16000, 1, 16, 100, false, 3);
  CHECK(!voiceWav(d.data(), d.size(), 1000, off, len));  // coma flotante
  const char basura[] = "no es un wav, solo texto con mas de doce bytes";
  CHECK(!voiceWav((const uint8_t *)basura, sizeof(basura), sizeof(basura), off, len));
  auto e = wav(16000, 1, 16, 100);
  CHECK(!voiceWav(e.data(), 30, 1000, off, len));  // cabecera cortada antes de "data"
}

TEST(voice, un_largo_mentiroso_se_recorta_al_fichero_y_al_tope) {
  uint32_t off, len;
  auto h = wav(16000, 1, 16, 0xFFFFFFFF);  // dice mas de lo que hay
  CHECK(voiceWav(h.data(), h.size(), (uint32_t)h.size() + 301, off, len));
  CHECK_EQ(len, (uint32_t)300);  // lo que hay de verdad, en muestras enteras (pares de bytes)
  auto g = wav(16000, 1, 16, VOICE_MAX_BYTES * 2);
  CHECK(voiceWav(g.data(), g.size(), (uint32_t)g.size() + VOICE_MAX_BYTES * 2, off, len));
  CHECK_EQ(len, (uint32_t)VOICE_MAX_BYTES);  // una grabacion larguisima se corta (memoria)
  auto z = wav(16000, 1, 16, 0);
  CHECK(!voiceWav(z.data(), z.size(), (uint32_t)z.size(), off, len));  // sin muestras: nada que tocar
}
