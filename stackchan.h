#pragma once
#include <stdint.h>
#include <stddef.h>
#include <ArduinoJson.h>
#include "pet.h"

// Herramientas que ve Stack-chan (xiaozhi, por MCP; la conexion esta en
// wifisync.cpp). Cada una hace su accion sobre la mascota y devuelve una frase
// en chino para que el asistente la cuente.
enum : uint8_t {
  MCP_STATUS = 0, MCP_FEED, MCP_PLAY, MCP_BATH, MCP_PET, MCP_SLEEP, MCP_WAKE,
  MCP_CANDY, MCP_MEDICINE, MCP_CHESS, MCP_POKEDEX, MCP_BUY, MCP_FARM, MCP_TOOL_COUNT
};
// por voz solo se compran cosas baratas (un error del reconocimiento no cuesta mucho)
#define MCP_BUY_MAX 5  // unidades por orden

// lo que pone el sketch (lo que pet no sabe)
struct StackchanCtx {
  const char *name = "";          // nombre del compañero (apodo o especie; "" si no hay)
  void (*bath)() = nullptr;       // arranca la animacion del baño
  const char *farmList = nullptr; // MCP_FARM: la lista ya escrita; nullptr = sin SD
};

void stackchanToolList(JsonArray tools);  // para tools/list
int stackchanFind(const char *name);      // indice MCP_*, -1 si no existe
// argumentos de tools/call -> a, b (MCP_BUY: objeto ITEM_* y cantidad)
void stackchanArgs(uint8_t tool, JsonVariantConst args, uint8_t &a, uint8_t &b);
void stackchanTool(Pet &pet, uint8_t tool, uint8_t a, uint8_t b, const StackchanCtx &ctx, char *out,
                   size_t cap);
