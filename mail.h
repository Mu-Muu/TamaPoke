#pragma once
#include <stdint.h>
#include <stddef.h>

// Correo con amigos lejos (mailbox.cpp, mail_ui.ino): un buzon en internet (ntfy.sh en la version
// publica, o un Worker propio con la misma API: tools/mailbox-worker). Cada placa tiene su buzon
// (un nombre al azar) y se lo dice a sus amigos al verse ("见面", cifrado); para mandar algo se deja
// en el buzon del amigo. Todo va cifrado y firmado con la clave de la pareja (la de linkproto.h):
// el buzon solo ve texto sin sentido. Aqui lo que no necesita la placa, para probarlo en el PC.
//
// Lo que se manda: un compañero de visita (MT_VISIT), una postal (MT_CARD: frase y pegatina) o un
// regalo (MT_GIFT: un objeto de la mochila), las dos ultimas con un mensajero (un pokemon del que
// manda) y con acuse (MT_ACK). Nada da puntos.

#define MAIL_VER 1
enum : uint8_t { MT_VISIT = 1, MT_CARD = 2, MT_GIFT = 3, MT_ACK = 4 };
#define MAIL_PHRASES 12
#define MAIL_STICKERS 8

struct MailMsg {
  uint8_t type = 0;
  uint32_t id = 0;        // al azar: acuse y repetidos
  uint64_t from = 0;      // numero de jugador de quien lo manda
  int16_t species = 0;    // el que lo lleva (o el que va de visita)
  uint8_t shiny = 0;
  uint16_t level = 0;
  uint8_t phrase = 0, sticker = 0;  // postal
  uint8_t item = 0, qty = 0;        // regalo
  uint32_t ackOf = 0;               // acuse: el id que se recibio
};
#define MAIL_MSG_LEN 28
size_t mailEncode(uint8_t *b, const MailMsg &m);
bool mailDecode(const uint8_t *b, size_t n, MailMsg &m);  // false: version o tipo que no se conoce
// ¿se puede enseñar tal cual? (un objeto o una frase que esta placa no tiene, de un amigo con un
// firmware mas nuevo, o un mensajero que no existe). Si no, ni se guarda ni se confirma: quien lo
// manda lo repite y, a los 14 dias, un regalo vuelve a su mochila en vez de perderse
bool mailMsgUsable(const MailMsg &m, uint16_t itemCount, int16_t dexMax);

// sellar: nonce (12) | cifrado | etiqueta (16). Cifrado: SHA-256 en modo contador con una clave
// sacada de la de la pareja; etiqueta: HMAC-SHA256 (otra clave) de nonce y cifrado
#define MAIL_NONCE 12
#define MAIL_TAG 16
#define MAIL_SEALED_MAX (MAIL_NONCE + 64 + MAIL_TAG)
size_t mailSeal(const uint8_t key[16], const uint8_t nonce[MAIL_NONCE], const uint8_t *in, size_t n, uint8_t *out);
bool mailOpen(const uint8_t key[16], const uint8_t *in, size_t n, uint8_t *out, size_t &outN);
void mailHmac(const uint8_t *key, size_t kn, const uint8_t *msg, size_t n, uint8_t out[32]);

size_t b64Encode(const uint8_t *in, size_t n, char *out, size_t cap);  // largo (sin el 0); 0 si no cabe
size_t b64Decode(const char *in, uint8_t *out, size_t cap);            // bytes; 0 si no vale

// un nombre de buzon valido para ntfy y el Worker: letras, numeros, - y _, de 8 a 40
bool mailBoxValid(const char *box);

// salida: postales y regalos se repiten hasta el acuse; una visita sale una vez. A los 14 dias
// sin salir o sin acuse se deja (un regalo vuelve a la mochila); tambien una visita o un acuse que
// no pudo salir (un amigo olvidado, un buzon que no contesta): si no, ocupaban la cola para siempre.
// Tiempos en segundos (hora de pared del RTC)
#define MAIL_RESEND_S (4UL * 3600)   // ntfy.sh guarda 12 h: se repite antes
#define MAIL_GIVEUP_S (14UL * 86400)
struct MailOut {
  bool on = false;
  uint64_t to = 0;
  uint32_t firstAt = 0, nextAt = 0;
  uint8_t tries = 0;
  MailMsg msg;
};
bool mailOutDue(const MailOut &o, uint32_t now);
// la hora de pared no siempre avanza: 0 si aun no se sabia (RTC sin pila) y hacia atras si el NTP
// corrige un reloj adelantado o se cambia de ciudad. Un plazo desde 0, o "now - firstAt" en
// negativo (sin signo da la vuelta), daba el regalo por perdido al momento y volvia a la mochila
// mientras aun salia: regalo repetido. Esto lo recoloca a "now"; true si cambio algo
bool mailOutFixClock(MailOut &o, uint32_t now);
bool mailOutExpired(const MailOut &o, uint32_t now);
void mailOutSent(MailOut &o, uint32_t now);  // ya salio: la visita acaba; lo demas espera el acuse
bool mailOutAcked(MailOut &o, uint32_t ackOf);
// pone en la cola q[n] el acuse de `ackOf` para `to`, si no hay ya uno esperando (cada repeticion
// del amigo, cada 4 h, añadia otro, y con su buzon sin contestar llenaban la cola: "信太多了").
// 1: puesto; 0: ya habia uno; -1: cola llena
int mailQueueAck(MailOut *q, int n, uint64_t to, uint64_t from, uint32_t ackOf, uint32_t id, uint32_t now);

// ids ya recibidos (un repetido se vuelve a confirmar pero no se enseña otra vez)
#define MAIL_SEEN 32
struct MailSeen {
  uint32_t ids[MAIL_SEEN] = {};
  uint8_t n = 0, pos = 0;
  bool seen(uint32_t id) const;
  void add(uint32_t id);
  bool repair();  // leido de la NVS: n o pos fuera de rango (estropeado) leerian o escribirian fuera; true si se vacio
};

// una linea de la respuesta de ntfy (o del Worker): {"id":..,"time":..,"event":"message","message":"..."}.
// true si es un mensaje (time y message, recortado a cap)
bool mailParseLine(const char *line, uint32_t &time, char *message, size_t cap);
