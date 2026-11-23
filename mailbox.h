#pragma once
#include <Arduino.h>
#include "mail.h"

// Correo con amigos lejos (ver mail.h): el estado (mi buzon, el de cada amigo, lo que sale, lo que
// ha llegado y aun no se ha visto) en la NVS "mail". La red la hace la tarea de red (mailNetSync,
// con el Wi-Fi conectado); lo que se ve, el bucle principal (mail_ui.ino). Entre las dos, un cerrojo.
//
// Cada cuanto: con USB cada MAIL_EVERY_USB_MS; con bateria cuando se conecta para el tiempo (cada 3 h)
// o cuando se pide ("立即收发", mailSyncSoon).

#define MAIL_FRIENDS 4
#define MAIL_OUTQ 8
#define MAIL_INQ 8
#define MAIL_EVERY_USB_MS (15UL * 60 * 1000)
#define MAIL_URL_MAX 72
#define MAIL_BOX_MAX 28
#define MAIL_AUTH_MAX 40

struct MailBox {  // donde se le escribe a alguien
  char url[MAIL_URL_MAX];
  char box[MAIL_BOX_MAX];
  char auth[MAIL_AUTH_MAX];  // vacio = sin clave (ntfy.sh)
};

void mailBegin();                                // en setup (despues de la NVS)
void mailTick(uint32_t wallEpoch);               // el bucle principal: la hora de pared (sin I2C en la red)
void mailRefreshFriends();                       // amigos y claves (link.h) para la tarea de red
void mailMyBox(MailBox &b);                      // el mio, para decirselo a un amigo al verse
bool mailBoxFromWire(const uint8_t *b, size_t n, MailBox &out);  // lo que llega por "见面"
size_t mailBoxToWire(const MailBox &b, uint8_t *out, size_t cap);
void mailSetFriendBox(uint64_t pid, const MailBox &b);
bool mailFriendReady(uint64_t pid);              // ya sabemos su buzon
bool mailSend(uint64_t to, MailMsg m);           // a la cola (from, id y horas los pone esto)
uint8_t mailOutCount(uint64_t to);               // postales y regalos aun sin acuse
bool mailPeekArrival(MailMsg &m);                // lo primero que llego y no se ha visto
void mailPopArrival();                           // ya se vio
bool mailFakeArrival(const MailMsg &m);          // consola MAIL FAKE: solo para verlo
bool mailTakeRefund(uint8_t &item);              // un regalo que nunca llego: vuelve a la mochila
void mailSyncSoon();                             // "立即收发"
void mailSyncCancel();                           // no hay red: ya se intentara con el tiempo
bool mailSyncDue(bool usb);                      // tarea de red: ¿toca?
bool mailSyncWanted();                           // tarea de red con bateria: alguien pidio "ya"
void mailNetSync();                              // tarea de red, con el Wi-Fi conectado
uint64_t mailSelf();
void mailInfo();                                 // consola MAIL INFO

// el cerrojo del correo, para leer su NVS ("mail") sin que la tarea de red la cambie a medias
// (la copia de seguridad, backup.cpp)
void mailLock();
void mailUnlock();
