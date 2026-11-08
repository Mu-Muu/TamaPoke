#pragma once
#include <Arduino.h>
#include "pet.h"
#include "chess.h"
#include "weather.h"
#include "stackchan.h"

// Wi-Fi: la sincronizacion diaria del ajedrez (y la hora por NTP), el tiempo
// real para el fondo y, con USB, el control por voz desde Stack-chan.
//
// Configuracion: la placa abre un punto de acceso "TamaPoke" con una pagina
// (192.168.4.1) donde se escriben la red, la contraseña y (opcional) el punto
// de acceso MCP de xiaozhi.me desde el movil. Se guardan en la NVS y la placa
// no las enseña nunca.
//
// Sincronizacion: conecta, pone el reloj en hora (zona de config.h, con
// horario de verano), baja /stats y los archivos mensuales necesarios de
// chess.com, suma los puntos y desconecta.
//
// Tarea de red (nucleo 0, no congela la pantalla): el tiempo cada hora; con
// USB y punto MCP configurado se queda conectada y atiende a xiaozhi (MCP por
// WebSocket): Stack-chan ve unas herramientas (estado, comer, jugar...) y las
// ordenes se ejecutan en el bucle principal (netTakeCommand/netGiveResult).

#define WIFI_AP_NAME "TamaPoke"
// la zona horaria (LOCAL_TZ) esta en config.h

bool wifiHasCreds();
void wifiPortalStart();
bool wifiPortalActive();
bool wifiPortalLoop();  // atiende la pagina; true justo cuando se guardo una red
void wifiPortalStop();

// sincroniza ya: true si fue bien (rep = lo ganado, ya sumado a p.points)
bool chessSync(Pet &p, ChessReport &rep);
// dia local (rtcEpoch/86400) de la ultima sincronizacion buena
uint32_t chessSyncDay();

void netBegin();            // arranca la tarea de red (al final de setup)
void netSetUsb(bool usb);   // con USB se queda conectada para Stack-chan
Weather weatherNow();       // el ultimo tiempo (valid = false si no hay o es viejo)
void weatherRefresh();      // pedir el tiempo ya (consola WX)
// hora UTC recibida del NTP desde la ultima llamada (al arrancar y cada hora,
// con el tiempo): el bucle principal corrige el reloj con ella
bool netTimeTake(uint32_t &utc);
bool mcpConfigured();
bool mcpOnline();           // xiaozhi conectado ahora mismo

// orden de Stack-chan para el bucle principal: tool = MCP_* (stackchan.h)
struct NetCmd {
  uint32_t seq;
  uint8_t tool, a, b;  // a, b: argumentos (stackchanArgs)
};
bool netTakeCommand(NetCmd &c);                      // no bloquea
void netGiveResult(uint32_t seq, const char *text);  // respuesta para Stack-chan
