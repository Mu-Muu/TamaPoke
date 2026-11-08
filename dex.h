#pragma once
#include <stdint.h>
#include "i18n.h"  // gLang

// GENERADO por tools/gen_dex.py desde tools/dex_data.py - no editar

#define DEX_COUNT 1025
#define DEX_BITMAP_BYTES 129  // pokedex en bits (dexReg)

// rareza: 0 = solo por evolucion, 1 = comun, 2 = raro, 3 = legendario,
// 4 = no disponible (sin sprite animado: ni huevo ni evolucion)
enum : uint8_t { R_EVO = 0, R_COMUN, R_RARO, R_LEGENDARIO, R_NONE };

struct DexEntry {
  const char *name;
  uint16_t evolvesTo;   // numero de dex, 0 = forma final
  uint8_t evolveLevel;
  uint8_t rarity;       // sale de huevo si es R_COMUN..R_LEGENDARIO
  uint16_t accent;      // color RGB565 del tipo para la UI
  uint8_t bHp, bAtk, bDef, bSpe;  // base stats reales de gen 1
  uint8_t biome;        // 0 pradera 1 playa 2 bosque 3 volcan 4 montana 5 nieve
};

static const DexEntry DEX_TBL[DEX_COUNT + 1] = {
  { "?", 0, 0, 0, 0x2946, 50, 50, 50, 50, 0 },  // 0: sin usar
  { "BULBASAUR", 2, 16, R_COMUN, 0x3C49, 45, 49, 49, 45, 2 },  // 1 planta
  { "IVYSAUR", 3, 32, R_EVO, 0x3C49, 60, 62, 63, 60, 2 },  // 2 planta
  { "VENUSAUR", 0, 0, R_EVO, 0x3C49, 80, 82, 83, 80, 2 },  // 3 planta
  { "CHARMANDER", 5, 16, R_COMUN, 0xEA87, 39, 52, 43, 65, 3 },  // 4 fuego
  { "CHARMELEON", 6, 36, R_EVO, 0xEA87, 58, 64, 58, 80, 3 },  // 5 fuego
  { "CHARIZARD", 0, 0, R_EVO, 0xEA87, 78, 84, 78, 100, 3 },  // 6 fuego
  { "SQUIRTLE", 8, 16, R_COMUN, 0x4C98, 44, 48, 65, 43, 1 },  // 7 agua
  { "WARTORTLE", 9, 36, R_EVO, 0x4C98, 59, 63, 80, 58, 1 },  // 8 agua
  { "BLASTOISE", 0, 0, R_EVO, 0x4C98, 79, 83, 100, 78, 1 },  // 9 agua
  { "CATERPIE", 11, 7, R_COMUN, 0x7CC4, 45, 30, 35, 45, 2 },  // 10 bicho
  { "METAPOD", 12, 10, R_EVO, 0x7CC4, 50, 20, 55, 30, 2 },  // 11 bicho
  { "BUTTERFREE", 0, 0, R_EVO, 0x7CC4, 60, 45, 50, 70, 2 },  // 12 bicho
  { "WEEDLE", 14, 7, R_COMUN, 0x7CC4, 40, 35, 30, 50, 2 },  // 13 bicho
  { "KAKUNA", 15, 10, R_EVO, 0x7CC4, 45, 25, 50, 35, 2 },  // 14 bicho
  { "BEEDRILL", 0, 0, R_EVO, 0x7CC4, 65, 90, 40, 75, 2 },  // 15 bicho
  { "PIDGEY", 17, 18, R_COMUN, 0x8C4D, 40, 45, 40, 56, 0 },  // 16 normal
  { "PIDGEOTTO", 18, 36, R_EVO, 0x8C4D, 63, 60, 55, 71, 0 },  // 17 normal
  { "PIDGEOT", 0, 0, R_EVO, 0x8C4D, 83, 80, 75, 101, 0 },  // 18 normal
  { "RATTATA", 20, 20, R_COMUN, 0x8C4D, 30, 56, 35, 72, 0 },  // 19 normal
  { "RATICATE", 0, 0, R_EVO, 0x8C4D, 55, 81, 60, 97, 0 },  // 20 normal
  { "SPEAROW", 22, 20, R_COMUN, 0x8C4D, 40, 60, 30, 70, 0 },  // 21 normal
  { "FEAROW", 0, 0, R_EVO, 0x8C4D, 65, 90, 65, 100, 0 },  // 22 normal
  { "EKANS", 24, 22, R_COMUN, 0x8A73, 35, 60, 44, 55, 0 },  // 23 veneno
  { "ARBOK", 0, 0, R_EVO, 0x8A73, 60, 95, 69, 80, 0 },  // 24 veneno
  { "PIKACHU", 26, 30, R_EVO, 0xBCA1, 35, 55, 40, 90, 0 },  // 25 electrico
  { "RAICHU", 0, 0, R_EVO, 0xBCA1, 60, 90, 55, 110, 0 },  // 26 electrico
  { "SANDSHREW", 28, 22, R_COMUN, 0xB447, 50, 75, 85, 40, 4 },  // 27 tierra
  { "SANDSLASH", 0, 0, R_EVO, 0xB447, 75, 100, 110, 65, 4 },  // 28 tierra
  { "NIDORAN H", 30, 16, R_COMUN, 0x8A73, 55, 47, 52, 41, 0 },  // 29 veneno
  { "NIDORINA", 31, 30, R_EVO, 0x8A73, 70, 62, 67, 56, 0 },  // 30 veneno
  { "NIDOQUEEN", 0, 0, R_EVO, 0x8A73, 90, 92, 87, 76, 0 },  // 31 veneno
  { "NIDORAN M", 33, 16, R_COMUN, 0x8A73, 46, 57, 40, 50, 0 },  // 32 veneno
  { "NIDORINO", 34, 30, R_EVO, 0x8A73, 61, 72, 57, 65, 0 },  // 33 veneno
  { "NIDOKING", 0, 0, R_EVO, 0x8A73, 81, 102, 77, 85, 0 },  // 34 veneno
  { "CLEFAIRY", 36, 30, R_EVO, 0x8C4D, 70, 45, 48, 35, 0 },  // 35 normal
  { "CLEFABLE", 0, 0, R_EVO, 0x8C4D, 95, 70, 73, 60, 0 },  // 36 normal
  { "VULPIX", 38, 30, R_COMUN, 0xEA87, 38, 41, 40, 65, 3 },  // 37 fuego
  { "NINETALES", 0, 0, R_EVO, 0xEA87, 73, 76, 75, 100, 3 },  // 38 fuego
  { "JIGGLYPUFF", 40, 30, R_EVO, 0x8C4D, 115, 45, 20, 20, 0 },  // 39 normal
  { "WIGGLYTUFF", 0, 0, R_EVO, 0x8C4D, 140, 70, 45, 45, 0 },  // 40 normal
  { "ZUBAT", 42, 22, R_COMUN, 0x8A73, 40, 45, 35, 55, 0 },  // 41 veneno
  { "GOLBAT", 169, 30, R_EVO, 0x8A73, 75, 80, 70, 90, 0 },  // 42 veneno
  { "ODDISH", 44, 21, R_COMUN, 0x3C49, 45, 50, 55, 30, 2 },  // 43 planta
  { "GLOOM", 45, 36, R_EVO, 0x3C49, 60, 65, 70, 40, 2 },  // 44 planta
  { "VILEPLUME", 0, 0, R_EVO, 0x3C49, 75, 80, 85, 50, 2 },  // 45 planta
  { "PARAS", 47, 24, R_COMUN, 0x7CC4, 35, 70, 55, 25, 2 },  // 46 bicho
  { "PARASECT", 0, 0, R_EVO, 0x7CC4, 60, 95, 80, 30, 2 },  // 47 bicho
  { "VENONAT", 49, 31, R_COMUN, 0x7CC4, 60, 55, 50, 45, 2 },  // 48 bicho
  { "VENOMOTH", 0, 0, R_EVO, 0x7CC4, 70, 65, 60, 90, 2 },  // 49 bicho
  { "DIGLETT", 51, 26, R_COMUN, 0xB447, 10, 55, 25, 95, 4 },  // 50 tierra
  { "DUGTRIO", 0, 0, R_EVO, 0xB447, 35, 100, 50, 120, 4 },  // 51 tierra
  { "MEOWTH", 53, 28, R_COMUN, 0x8C4D, 40, 45, 35, 90, 0 },  // 52 normal
  { "PERSIAN", 0, 0, R_EVO, 0x8C4D, 65, 70, 60, 115, 0 },  // 53 normal
  { "PSYDUCK", 55, 33, R_COMUN, 0x4C98, 50, 52, 48, 55, 1 },  // 54 agua
  { "GOLDUCK", 0, 0, R_EVO, 0x4C98, 80, 82, 78, 85, 1 },  // 55 agua
  { "MANKEY", 57, 28, R_COMUN, 0xA2A5, 40, 80, 35, 70, 0 },  // 56 lucha
  { "PRIMEAPE", 979, 33, R_EVO, 0xA2A5, 65, 105, 60, 95, 0 },  // 57 lucha
  { "GROWLITHE", 59, 30, R_RARO, 0xEA87, 55, 70, 45, 60, 3 },  // 58 fuego
  { "ARCANINE", 0, 0, R_EVO, 0xEA87, 90, 110, 80, 95, 3 },  // 59 fuego
  { "POLIWAG", 61, 25, R_COMUN, 0x4C98, 40, 50, 40, 90, 1 },  // 60 agua
  { "POLIWHIRL", 62, 36, R_EVO, 0x4C98, 65, 65, 65, 90, 1 },  // 61 agua
  { "POLIWRATH", 0, 0, R_EVO, 0x4C98, 90, 95, 95, 70, 1 },  // 62 agua
  { "ABRA", 64, 16, R_COMUN, 0xD28F, 25, 20, 15, 90, 0 },  // 63 psiquico
  { "KADABRA", 65, 40, R_EVO, 0xD28F, 40, 35, 30, 105, 0 },  // 64 psiquico
  { "ALAKAZAM", 0, 0, R_EVO, 0xD28F, 55, 50, 45, 120, 0 },  // 65 psiquico
  { "MACHOP", 67, 28, R_COMUN, 0xA2A5, 70, 80, 50, 35, 0 },  // 66 lucha
  { "MACHOKE", 68, 40, R_EVO, 0xA2A5, 80, 100, 70, 45, 0 },  // 67 lucha
  { "MACHAMP", 0, 0, R_EVO, 0xA2A5, 90, 130, 80, 55, 0 },  // 68 lucha
  { "BELLSPROUT", 70, 21, R_COMUN, 0x3C49, 50, 75, 35, 40, 2 },  // 69 planta
  { "WEEPINBELL", 71, 36, R_EVO, 0x3C49, 65, 90, 50, 55, 2 },  // 70 planta
  { "VICTREEBEL", 0, 0, R_EVO, 0x3C49, 80, 105, 65, 70, 2 },  // 71 planta
  { "TENTACOOL", 73, 30, R_COMUN, 0x4C98, 40, 40, 35, 70, 1 },  // 72 agua
  { "TENTACRUEL", 0, 0, R_EVO, 0x4C98, 80, 70, 65, 100, 1 },  // 73 agua
  { "GEODUDE", 75, 25, R_COMUN, 0x9407, 40, 80, 100, 20, 4 },  // 74 roca
  { "GRAVELER", 76, 40, R_EVO, 0x9407, 55, 95, 115, 35, 4 },  // 75 roca
  { "GOLEM", 0, 0, R_EVO, 0x9407, 80, 120, 130, 45, 4 },  // 76 roca
  { "PONYTA", 78, 40, R_RARO, 0xEA87, 50, 85, 55, 90, 3 },  // 77 fuego
  { "RAPIDASH", 0, 0, R_EVO, 0xEA87, 65, 100, 70, 105, 3 },  // 78 fuego
  { "SLOWPOKE", 80, 37, R_COMUN, 0x4C98, 90, 65, 65, 15, 1 },  // 79 agua
  { "SLOWBRO", 0, 0, R_EVO, 0x4C98, 95, 75, 110, 30, 1 },  // 80 agua
  { "MAGNEMITE", 82, 30, R_COMUN, 0xBCA1, 25, 35, 70, 45, 0 },  // 81 electrico
  { "MAGNETON", 462, 35, R_EVO, 0xBCA1, 50, 60, 95, 70, 0 },  // 82 electrico
  { "FARFETCHD", 865, 25, R_RARO, 0x8C4D, 52, 90, 55, 60, 0 },  // 83 normal
  { "DODUO", 85, 31, R_COMUN, 0x8C4D, 35, 85, 45, 75, 0 },  // 84 normal
  { "DODRIO", 0, 0, R_EVO, 0x8C4D, 60, 110, 70, 110, 0 },  // 85 normal
  { "SEEL", 87, 34, R_COMUN, 0x4C98, 65, 45, 55, 45, 1 },  // 86 agua
  { "DEWGONG", 0, 0, R_EVO, 0x4C98, 90, 70, 80, 70, 1 },  // 87 agua
  { "GRIMER", 89, 38, R_RARO, 0x8A73, 80, 80, 50, 25, 0 },  // 88 veneno
  { "MUK", 0, 0, R_EVO, 0x8A73, 105, 105, 75, 50, 0 },  // 89 veneno
  { "SHELLDER", 91, 30, R_COMUN, 0x4C98, 30, 65, 100, 40, 1 },  // 90 agua
  { "CLOYSTER", 0, 0, R_EVO, 0x4C98, 50, 95, 180, 70, 1 },  // 91 agua
  { "GASTLY", 93, 25, R_COMUN, 0x6AD3, 30, 35, 30, 80, 0 },  // 92 fantasma
  { "HAUNTER", 94, 40, R_EVO, 0x6AD3, 45, 50, 45, 95, 0 },  // 93 fantasma
  { "GENGAR", 0, 0, R_EVO, 0x6AD3, 60, 65, 60, 110, 0 },  // 94 fantasma
  { "ONIX", 208, 30, R_RARO, 0x9407, 35, 45, 160, 70, 4 },  // 95 roca
  { "DROWZEE", 97, 26, R_COMUN, 0xD28F, 60, 48, 45, 42, 0 },  // 96 psiquico
  { "HYPNO", 0, 0, R_EVO, 0xD28F, 85, 73, 70, 67, 0 },  // 97 psiquico
  { "KRABBY", 99, 28, R_COMUN, 0x4C98, 30, 105, 90, 50, 1 },  // 98 agua
  { "KINGLER", 0, 0, R_EVO, 0x4C98, 55, 130, 115, 75, 1 },  // 99 agua
  { "VOLTORB", 101, 30, R_COMUN, 0xBCA1, 40, 30, 50, 100, 0 },  // 100 electrico
  { "ELECTRODE", 0, 0, R_EVO, 0xBCA1, 60, 50, 70, 150, 0 },  // 101 electrico
  { "EXEGGCUTE", 103, 30, R_COMUN, 0x3C49, 60, 40, 80, 40, 2 },  // 102 planta
  { "EXEGGUTOR", 0, 0, R_EVO, 0x3C49, 95, 95, 85, 55, 2 },  // 103 planta
  { "CUBONE", 105, 28, R_COMUN, 0xB447, 50, 50, 95, 35, 4 },  // 104 tierra
  { "MAROWAK", 0, 0, R_EVO, 0xB447, 60, 80, 110, 45, 4 },  // 105 tierra
  { "HITMONLEE", 0, 0, R_EVO, 0xA2A5, 50, 120, 53, 87, 0 },  // 106 lucha
  { "HITMONCHAN", 0, 0, R_EVO, 0xA2A5, 50, 105, 79, 76, 0 },  // 107 lucha
  { "LICKITUNG", 463, 25, R_RARO, 0x8C4D, 90, 55, 75, 30, 0 },  // 108 normal
  { "KOFFING", 110, 35, R_COMUN, 0x8A73, 40, 65, 95, 35, 0 },  // 109 veneno
  { "WEEZING", 0, 0, R_EVO, 0x8A73, 65, 90, 120, 60, 0 },  // 110 veneno
  { "RHYHORN", 112, 42, R_RARO, 0xB447, 80, 85, 95, 25, 4 },  // 111 tierra
  { "RHYDON", 464, 47, R_EVO, 0xB447, 105, 130, 120, 40, 4 },  // 112 tierra
  { "CHANSEY", 242, 30, R_EVO, 0x8C4D, 250, 5, 5, 50, 0 },  // 113 normal
  { "TANGELA", 465, 25, R_RARO, 0x3C49, 65, 55, 115, 60, 2 },  // 114 planta
  { "KANGASKHAN", 0, 0, R_RARO, 0x8C4D, 105, 95, 80, 90, 0 },  // 115 normal
  { "HORSEA", 117, 32, R_COMUN, 0x4C98, 30, 40, 70, 60, 1 },  // 116 agua
  { "SEADRA", 230, 42, R_EVO, 0x4C98, 55, 65, 95, 85, 1 },  // 117 agua
  { "GOLDEEN", 119, 33, R_COMUN, 0x4C98, 45, 67, 60, 63, 1 },  // 118 agua
  { "SEAKING", 0, 0, R_EVO, 0x4C98, 80, 92, 65, 68, 1 },  // 119 agua
  { "STARYU", 121, 30, R_COMUN, 0x4C98, 30, 45, 55, 85, 1 },  // 120 agua
  { "STARMIE", 0, 0, R_EVO, 0x4C98, 60, 75, 85, 115, 1 },  // 121 agua
  { "MR. MIME", 0, 0, R_EVO, 0xD28F, 40, 45, 65, 90, 0 },  // 122 psiquico
  { "SCYTHER", 212, 30, R_RARO, 0x7CC4, 70, 110, 80, 105, 2 },  // 123 bicho
  { "JYNX", 0, 0, R_EVO, 0x4DB8, 65, 50, 35, 95, 5 },  // 124 hielo
  { "ELECTABUZZ", 466, 40, R_EVO, 0xBCA1, 65, 83, 57, 105, 0 },  // 125 electrico
  { "MAGMAR", 467, 40, R_EVO, 0xEA87, 65, 95, 57, 93, 3 },  // 126 fuego
  { "PINSIR", 0, 0, R_RARO, 0x7CC4, 65, 125, 100, 85, 2 },  // 127 bicho
  { "TAUROS", 0, 0, R_RARO, 0x8C4D, 75, 100, 95, 110, 0 },  // 128 normal
  { "MAGIKARP", 130, 20, R_COMUN, 0x4C98, 20, 10, 55, 80, 1 },  // 129 agua
  { "GYARADOS", 0, 0, R_EVO, 0x4C98, 95, 125, 79, 81, 1 },  // 130 agua
  { "LAPRAS", 0, 0, R_RARO, 0x4C98, 130, 85, 80, 60, 1 },  // 131 agua
  { "DITTO", 0, 0, R_RARO, 0x8C4D, 48, 48, 48, 48, 0 },  // 132 normal
  { "EEVEE", 134, 30, R_COMUN, 0x8C4D, 55, 55, 50, 55, 0 },  // 133 normal
  { "VAPOREON", 0, 0, R_EVO, 0x4C98, 130, 65, 60, 65, 1 },  // 134 agua
  { "JOLTEON", 0, 0, R_EVO, 0xBCA1, 65, 65, 60, 130, 0 },  // 135 electrico
  { "FLAREON", 0, 0, R_EVO, 0xEA87, 65, 130, 60, 65, 3 },  // 136 fuego
  { "PORYGON", 233, 30, R_RARO, 0x8C4D, 65, 60, 70, 40, 0 },  // 137 normal
  { "OMANYTE", 139, 40, R_RARO, 0x9407, 35, 40, 100, 35, 1 },  // 138 roca
  { "OMASTAR", 0, 0, R_EVO, 0x9407, 70, 60, 125, 55, 1 },  // 139 roca
  { "KABUTO", 141, 40, R_RARO, 0x9407, 30, 80, 90, 55, 1 },  // 140 roca
  { "KABUTOPS", 0, 0, R_EVO, 0x9407, 60, 115, 105, 80, 1 },  // 141 roca
  { "AERODACTYL", 0, 0, R_RARO, 0x9407, 80, 105, 65, 130, 4 },  // 142 roca
  { "SNORLAX", 0, 0, R_EVO, 0x8C4D, 160, 110, 65, 30, 0 },  // 143 normal
  { "ARTICUNO", 0, 0, R_LEGENDARIO, 0x4DB8, 90, 85, 100, 85, 5 },  // 144 hielo
  { "ZAPDOS", 0, 0, R_LEGENDARIO, 0xBCA1, 90, 90, 85, 100, 0 },  // 145 electrico
  { "MOLTRES", 0, 0, R_LEGENDARIO, 0xEA87, 90, 100, 90, 90, 3 },  // 146 fuego
  { "DRATINI", 148, 30, R_RARO, 0x5A98, 41, 64, 45, 50, 1 },  // 147 dragon
  { "DRAGONAIR", 149, 55, R_EVO, 0x5A98, 61, 84, 65, 70, 1 },  // 148 dragon
  { "DRAGONITE", 0, 0, R_EVO, 0x5A98, 91, 134, 95, 80, 1 },  // 149 dragon
  { "MEWTWO", 0, 0, R_LEGENDARIO, 0xD28F, 106, 110, 90, 130, 0 },  // 150 psiquico
  { "MEW", 0, 0, R_LEGENDARIO, 0xD28F, 100, 100, 100, 100, 0 },  // 151 psiquico
  { "CHIKORITA", 153, 16, R_COMUN, 0x3C49, 45, 49, 65, 45, 2 },  // 152 planta
  { "BAYLEEF", 154, 32, R_EVO, 0x3C49, 60, 62, 80, 60, 2 },  // 153 planta
  { "MEGANIUM", 0, 0, R_EVO, 0x3C49, 80, 82, 100, 80, 2 },  // 154 planta
  { "CYNDAQUIL", 156, 14, R_COMUN, 0xEA87, 39, 52, 43, 65, 3 },  // 155 fuego
  { "QUILAVA", 157, 36, R_EVO, 0xEA87, 58, 64, 58, 80, 3 },  // 156 fuego
  { "TYPHLOSION", 0, 0, R_EVO, 0xEA87, 78, 84, 78, 100, 3 },  // 157 fuego
  { "TOTODILE", 159, 18, R_COMUN, 0x4C98, 50, 65, 64, 43, 1 },  // 158 agua
  { "CROCONAW", 160, 30, R_EVO, 0x4C98, 65, 80, 80, 58, 1 },  // 159 agua
  { "FERALIGATR", 0, 0, R_EVO, 0x4C98, 85, 105, 100, 78, 1 },  // 160 agua
  { "SENTRET", 162, 15, R_COMUN, 0x8C4D, 35, 46, 34, 20, 0 },  // 161 normal
  { "FURRET", 0, 0, R_EVO, 0x8C4D, 85, 76, 64, 90, 0 },  // 162 normal
  { "HOOTHOOT", 164, 20, R_COMUN, 0x8C4D, 60, 30, 30, 50, 0 },  // 163 normal
  { "NOCTOWL", 0, 0, R_EVO, 0x8C4D, 100, 50, 50, 70, 0 },  // 164 normal
  { "LEDYBA", 166, 18, R_COMUN, 0x7CC4, 40, 20, 30, 55, 2 },  // 165 bicho
  { "LEDIAN", 0, 0, R_EVO, 0x7CC4, 55, 35, 50, 85, 2 },  // 166 bicho
  { "SPINARAK", 168, 22, R_COMUN, 0x7CC4, 40, 60, 40, 30, 2 },  // 167 bicho
  { "ARIADOS", 0, 0, R_EVO, 0x7CC4, 70, 90, 70, 40, 2 },  // 168 bicho
  { "CROBAT", 0, 0, R_EVO, 0x8A73, 85, 90, 80, 130, 0 },  // 169 veneno
  { "CHINCHOU", 171, 27, R_COMUN, 0x4C98, 75, 38, 38, 67, 1 },  // 170 agua
  { "LANTURN", 0, 0, R_EVO, 0x4C98, 125, 58, 58, 67, 1 },  // 171 agua
  { "PICHU", 25, 10, R_COMUN, 0xBCA1, 20, 40, 15, 60, 0 },  // 172 electrico
  { "CLEFFA", 35, 10, R_COMUN, 0x8C4D, 50, 25, 28, 15, 0 },  // 173 normal
  { "IGGLYBUFF", 39, 10, R_COMUN, 0x8C4D, 90, 30, 15, 15, 0 },  // 174 normal
  { "TOGEPI", 176, 10, R_RARO, 0x8C4D, 35, 20, 65, 20, 0 },  // 175 normal
  { "TOGETIC", 468, 30, R_EVO, 0x8C4D, 55, 40, 85, 40, 0 },  // 176 normal
  { "NATU", 178, 25, R_COMUN, 0xD28F, 40, 50, 45, 70, 0 },  // 177 psiquico
  { "XATU", 0, 0, R_EVO, 0xD28F, 65, 75, 70, 95, 0 },  // 178 psiquico
  { "MAREEP", 180, 15, R_COMUN, 0xBCA1, 55, 40, 40, 35, 0 },  // 179 electrico
  { "FLAAFFY", 181, 30, R_EVO, 0xBCA1, 70, 55, 55, 45, 0 },  // 180 electrico
  { "AMPHAROS", 0, 0, R_EVO, 0xBCA1, 90, 75, 85, 55, 0 },  // 181 electrico
  { "BELLOSSOM", 0, 0, R_EVO, 0x3C49, 75, 80, 95, 50, 2 },  // 182 planta
  { "MARILL", 184, 18, R_EVO, 0x4C98, 70, 20, 50, 40, 1 },  // 183 agua
  { "AZUMARILL", 0, 0, R_EVO, 0x4C98, 100, 50, 80, 50, 1 },  // 184 agua
  { "SUDOWOODO", 0, 0, R_EVO, 0x9407, 70, 100, 115, 30, 4 },  // 185 roca
  { "POLITOED", 0, 0, R_EVO, 0x4C98, 90, 75, 75, 70, 1 },  // 186 agua
  { "HOPPIP", 188, 18, R_COMUN, 0x3C49, 35, 35, 40, 50, 2 },  // 187 planta
  { "SKIPLOOM", 189, 27, R_EVO, 0x3C49, 55, 45, 50, 80, 2 },  // 188 planta
  { "JUMPLUFF", 0, 0, R_EVO, 0x3C49, 75, 55, 70, 110, 2 },  // 189 planta
  { "AIPOM", 424, 25, R_COMUN, 0x8C4D, 55, 70, 55, 85, 0 },  // 190 normal
  { "SUNKERN", 192, 20, R_COMUN, 0x3C49, 30, 30, 30, 30, 2 },  // 191 planta
  { "SUNFLORA", 0, 0, R_EVO, 0x3C49, 75, 75, 55, 30, 2 },  // 192 planta
  { "YANMA", 469, 25, R_RARO, 0x7CC4, 65, 65, 45, 95, 2 },  // 193 bicho
  { "WOOPER", 195, 20, R_COMUN, 0x4C98, 55, 45, 45, 15, 1 },  // 194 agua
  { "QUAGSIRE", 0, 0, R_EVO, 0x4C98, 95, 85, 85, 35, 1 },  // 195 agua
  { "ESPEON", 0, 0, R_EVO, 0xD28F, 65, 65, 60, 110, 0 },  // 196 psiquico
  { "UMBREON", 0, 0, R_EVO, 0x5A6E, 95, 65, 110, 65, 0 },  // 197 siniestro
  { "MURKROW", 430, 30, R_RARO, 0x5A6E, 60, 85, 42, 91, 0 },  // 198 siniestro
  { "SLOWKING", 0, 0, R_EVO, 0x4C98, 95, 75, 80, 30, 1 },  // 199 agua
  { "MISDREAVUS", 429, 30, R_RARO, 0x6AD3, 60, 60, 60, 85, 0 },  // 200 fantasma
  { "UNOWN", 0, 0, R_RARO, 0xD28F, 48, 72, 48, 48, 0 },  // 201 psiquico
  { "WOBBUFFET", 0, 0, R_EVO, 0xD28F, 190, 33, 58, 33, 0 },  // 202 psiquico
  { "GIRAFARIG", 981, 25, R_COMUN, 0x8C4D, 70, 80, 65, 85, 0 },  // 203 normal
  { "PINECO", 205, 31, R_COMUN, 0x7CC4, 50, 65, 90, 15, 2 },  // 204 bicho
  { "FORRETRESS", 0, 0, R_EVO, 0x7CC4, 75, 90, 140, 40, 2 },  // 205 bicho
  { "DUNSPARCE", 982, 25, R_RARO, 0x8C4D, 100, 70, 70, 45, 0 },  // 206 normal
  { "GLIGAR", 472, 25, R_COMUN, 0xB447, 65, 75, 105, 85, 4 },  // 207 tierra
  { "STEELIX", 0, 0, R_EVO, 0xB447, 75, 85, 200, 30, 4 },  // 208 tierra
  { "SNUBBULL", 210, 23, R_COMUN, 0x8C4D, 60, 80, 50, 30, 0 },  // 209 normal
  { "GRANBULL", 0, 0, R_EVO, 0x8C4D, 90, 120, 75, 45, 0 },  // 210 normal
  { "QWILFISH", 904, 25, R_COMUN, 0x4C98, 65, 95, 85, 85, 1 },  // 211 agua
  { "SCIZOR", 0, 0, R_EVO, 0x7CC4, 70, 130, 100, 65, 2 },  // 212 bicho
  { "SHUCKLE", 0, 0, R_RARO, 0x7CC4, 20, 10, 230, 5, 2 },  // 213 bicho
  { "HERACROSS", 0, 0, R_RARO, 0x7CC4, 80, 125, 75, 85, 2 },  // 214 bicho
  { "SNEASEL", 461, 25, R_RARO, 0x5A6E, 55, 95, 55, 115, 0 },  // 215 siniestro
  { "TEDDIURSA", 217, 30, R_COMUN, 0x8C4D, 60, 80, 50, 40, 0 },  // 216 normal
  { "URSARING", 901, 35, R_EVO, 0x8C4D, 90, 130, 75, 55, 0 },  // 217 normal
  { "SLUGMA", 219, 38, R_COMUN, 0xEA87, 40, 40, 40, 20, 3 },  // 218 fuego
  { "MAGCARGO", 0, 0, R_EVO, 0xEA87, 60, 50, 120, 30, 3 },  // 219 fuego
  { "SWINUB", 221, 33, R_COMUN, 0x4DB8, 50, 50, 40, 50, 5 },  // 220 hielo
  { "PILOSWINE", 473, 38, R_EVO, 0x4DB8, 100, 100, 80, 50, 5 },  // 221 hielo
  { "CORSOLA", 864, 38, R_RARO, 0x4C98, 65, 55, 95, 35, 1 },  // 222 agua
  { "REMORAID", 224, 25, R_COMUN, 0x4C98, 35, 65, 35, 65, 1 },  // 223 agua
  { "OCTILLERY", 0, 0, R_EVO, 0x4C98, 75, 105, 75, 45, 1 },  // 224 agua
  { "DELIBIRD", 0, 0, R_RARO, 0x4DB8, 45, 55, 45, 75, 5 },  // 225 hielo
  { "MANTINE", 0, 0, R_EVO, 0x4C98, 85, 40, 70, 70, 1 },  // 226 agua
  { "SKARMORY", 0, 0, R_RARO, 0x7C73, 65, 80, 140, 70, 4 },  // 227 acero
  { "HOUNDOUR", 229, 24, R_COMUN, 0x5A6E, 45, 60, 30, 65, 0 },  // 228 siniestro
  { "HOUNDOOM", 0, 0, R_EVO, 0x5A6E, 75, 90, 50, 95, 0 },  // 229 siniestro
  { "KINGDRA", 0, 0, R_EVO, 0x4C98, 75, 95, 95, 85, 1 },  // 230 agua
  { "PHANPY", 232, 25, R_COMUN, 0xB447, 90, 60, 60, 40, 4 },  // 231 tierra
  { "DONPHAN", 0, 0, R_EVO, 0xB447, 90, 120, 120, 50, 4 },  // 232 tierra
  { "PORYGON2", 474, 40, R_EVO, 0x8C4D, 85, 80, 90, 60, 0 },  // 233 normal
  { "STANTLER", 899, 25, R_RARO, 0x8C4D, 73, 95, 62, 85, 0 },  // 234 normal
  { "SMEARGLE", 0, 0, R_RARO, 0x8C4D, 55, 20, 35, 75, 0 },  // 235 normal
  { "TYROGUE", 106, 20, R_RARO, 0xA2A5, 35, 35, 35, 35, 0 },  // 236 lucha
  { "HITMONTOP", 0, 0, R_EVO, 0xA2A5, 50, 95, 95, 70, 0 },  // 237 lucha
  { "SMOOCHUM", 124, 30, R_RARO, 0x4DB8, 45, 30, 15, 65, 5 },  // 238 hielo
  { "ELEKID", 125, 30, R_RARO, 0xBCA1, 45, 63, 37, 95, 0 },  // 239 electrico
  { "MAGBY", 126, 30, R_RARO, 0xEA87, 45, 75, 37, 83, 3 },  // 240 fuego
  { "MILTANK", 0, 0, R_RARO, 0x8C4D, 95, 80, 105, 100, 0 },  // 241 normal
  { "BLISSEY", 0, 0, R_EVO, 0x8C4D, 255, 10, 10, 55, 0 },  // 242 normal
  { "RAIKOU", 0, 0, R_LEGENDARIO, 0xBCA1, 90, 85, 75, 115, 0 },  // 243 electrico
  { "ENTEI", 0, 0, R_LEGENDARIO, 0xEA87, 115, 115, 85, 100, 3 },  // 244 fuego
  { "SUICUNE", 0, 0, R_LEGENDARIO, 0x4C98, 100, 75, 115, 85, 1 },  // 245 agua
  { "LARVITAR", 247, 30, R_RARO, 0x9407, 50, 64, 50, 41, 4 },  // 246 roca
  { "PUPITAR", 248, 55, R_EVO, 0x9407, 70, 84, 70, 51, 4 },  // 247 roca
  { "TYRANITAR", 0, 0, R_EVO, 0x9407, 100, 134, 110, 61, 4 },  // 248 roca
  { "LUGIA", 0, 0, R_LEGENDARIO, 0xD28F, 106, 90, 130, 110, 0 },  // 249 psiquico
  { "HO_OH", 0, 0, R_LEGENDARIO, 0xEA87, 106, 130, 90, 90, 3 },  // 250 fuego
  { "CELEBI", 0, 0, R_LEGENDARIO, 0xD28F, 100, 100, 100, 100, 0 },  // 251 psiquico
  { "TREECKO", 253, 16, R_COMUN, 0x3C49, 40, 45, 35, 70, 2 },  // 252 planta
  { "GROVYLE", 254, 36, R_EVO, 0x3C49, 50, 65, 45, 95, 2 },  // 253 planta
  { "SCEPTILE", 0, 0, R_EVO, 0x3C49, 70, 85, 65, 120, 2 },  // 254 planta
  { "TORCHIC", 256, 16, R_COMUN, 0xEA87, 45, 60, 40, 45, 3 },  // 255 fuego
  { "COMBUSKEN", 257, 36, R_EVO, 0xEA87, 60, 85, 60, 55, 3 },  // 256 fuego
  { "BLAZIKEN", 0, 0, R_EVO, 0xEA87, 80, 120, 70, 80, 3 },  // 257 fuego
  { "MUDKIP", 259, 16, R_COMUN, 0x4C98, 50, 70, 50, 40, 1 },  // 258 agua
  { "MARSHTOMP", 260, 36, R_EVO, 0x4C98, 70, 85, 70, 50, 1 },  // 259 agua
  { "SWAMPERT", 0, 0, R_EVO, 0x4C98, 100, 110, 90, 60, 1 },  // 260 agua
  { "POOCHYENA", 262, 18, R_COMUN, 0x5A6E, 35, 55, 35, 35, 0 },  // 261 siniestro
  { "MIGHTYENA", 0, 0, R_EVO, 0x5A6E, 70, 90, 70, 70, 0 },  // 262 siniestro
  { "ZIGZAGOON", 264, 20, R_COMUN, 0x8C4D, 38, 30, 41, 60, 0 },  // 263 normal
  { "LINOONE", 862, 35, R_EVO, 0x8C4D, 78, 70, 61, 100, 0 },  // 264 normal
  { "WURMPLE", 266, 7, R_COMUN, 0x7CC4, 45, 45, 35, 20, 2 },  // 265 bicho
  { "SILCOON", 267, 10, R_EVO, 0x7CC4, 50, 35, 55, 15, 2 },  // 266 bicho
  { "BEAUTIFLY", 0, 0, R_EVO, 0x7CC4, 60, 70, 50, 65, 2 },  // 267 bicho
  { "CASCOON", 269, 10, R_EVO, 0x7CC4, 50, 35, 55, 15, 2 },  // 268 bicho
  { "DUSTOX", 0, 0, R_EVO, 0x7CC4, 60, 50, 70, 65, 2 },  // 269 bicho
  { "LOTAD", 271, 14, R_COMUN, 0x4C98, 40, 30, 30, 30, 1 },  // 270 agua
  { "LOMBRE", 272, 30, R_EVO, 0x4C98, 60, 50, 50, 50, 1 },  // 271 agua
  { "LUDICOLO", 0, 0, R_EVO, 0x4C98, 80, 70, 70, 70, 1 },  // 272 agua
  { "SEEDOT", 274, 14, R_COMUN, 0x3C49, 40, 40, 50, 30, 2 },  // 273 planta
  { "NUZLEAF", 275, 30, R_EVO, 0x3C49, 70, 70, 40, 60, 2 },  // 274 planta
  { "SHIFTRY", 0, 0, R_EVO, 0x3C49, 90, 100, 60, 80, 2 },  // 275 planta
  { "TAILLOW", 277, 22, R_COMUN, 0x8C4D, 40, 55, 30, 85, 0 },  // 276 normal
  { "SWELLOW", 0, 0, R_EVO, 0x8C4D, 60, 85, 60, 125, 0 },  // 277 normal
  { "WINGULL", 279, 25, R_COMUN, 0x4C98, 40, 30, 30, 85, 1 },  // 278 agua
  { "PELIPPER", 0, 0, R_EVO, 0x4C98, 60, 50, 100, 65, 1 },  // 279 agua
  { "RALTS", 281, 20, R_COMUN, 0xD28F, 28, 25, 25, 40, 0 },  // 280 psiquico
  { "KIRLIA", 282, 30, R_EVO, 0xD28F, 38, 35, 35, 50, 0 },  // 281 psiquico
  { "GARDEVOIR", 0, 0, R_EVO, 0xD28F, 68, 65, 65, 80, 0 },  // 282 psiquico
  { "SURSKIT", 284, 22, R_COMUN, 0x7CC4, 40, 30, 32, 65, 2 },  // 283 bicho
  { "MASQUERAIN", 0, 0, R_EVO, 0x7CC4, 70, 60, 62, 80, 2 },  // 284 bicho
  { "SHROOMISH", 286, 23, R_COMUN, 0x3C49, 60, 40, 60, 35, 2 },  // 285 planta
  { "BRELOOM", 0, 0, R_EVO, 0x3C49, 60, 130, 80, 70, 2 },  // 286 planta
  { "SLAKOTH", 288, 18, R_COMUN, 0x8C4D, 60, 60, 60, 30, 0 },  // 287 normal
  { "VIGOROTH", 289, 36, R_EVO, 0x8C4D, 80, 80, 80, 90, 0 },  // 288 normal
  { "SLAKING", 0, 0, R_EVO, 0x8C4D, 150, 160, 100, 100, 0 },  // 289 normal
  { "NINCADA", 291, 20, R_COMUN, 0x7CC4, 31, 45, 90, 40, 2 },  // 290 bicho
  { "NINJASK", 0, 0, R_EVO, 0x7CC4, 61, 90, 45, 160, 2 },  // 291 bicho
  { "SHEDINJA", 0, 0, R_EVO, 0x7CC4, 1, 90, 45, 40, 2 },  // 292 bicho
  { "WHISMUR", 294, 20, R_COMUN, 0x8C4D, 64, 51, 23, 28, 0 },  // 293 normal
  { "LOUDRED", 295, 40, R_EVO, 0x8C4D, 84, 71, 43, 48, 0 },  // 294 normal
  { "EXPLOUD", 0, 0, R_EVO, 0x8C4D, 104, 91, 63, 68, 0 },  // 295 normal
  { "MAKUHITA", 297, 24, R_COMUN, 0xA2A5, 72, 60, 30, 25, 0 },  // 296 lucha
  { "HARIYAMA", 0, 0, R_EVO, 0xA2A5, 144, 120, 60, 50, 0 },  // 297 lucha
  { "AZURILL", 183, 10, R_COMUN, 0x8C4D, 50, 20, 40, 20, 0 },  // 298 normal
  { "NOSEPASS", 476, 25, R_COMUN, 0x9407, 30, 45, 135, 30, 4 },  // 299 roca
  { "SKITTY", 301, 30, R_COMUN, 0x8C4D, 50, 45, 45, 50, 0 },  // 300 normal
  { "DELCATTY", 0, 0, R_EVO, 0x8C4D, 70, 65, 65, 90, 0 },  // 301 normal
  { "SABLEYE", 0, 0, R_RARO, 0x5A6E, 50, 75, 75, 50, 0 },  // 302 siniestro
  { "MAWILE", 0, 0, R_RARO, 0x7C73, 50, 85, 85, 50, 4 },  // 303 acero
  { "ARON", 305, 32, R_COMUN, 0x7C73, 50, 70, 100, 30, 4 },  // 304 acero
  { "LAIRON", 306, 42, R_EVO, 0x7C73, 60, 90, 140, 40, 4 },  // 305 acero
  { "AGGRON", 0, 0, R_EVO, 0x7C73, 70, 110, 180, 50, 4 },  // 306 acero
  { "MEDITITE", 308, 37, R_COMUN, 0xA2A5, 30, 40, 55, 60, 0 },  // 307 lucha
  { "MEDICHAM", 0, 0, R_EVO, 0xA2A5, 60, 60, 75, 80, 0 },  // 308 lucha
  { "ELECTRIKE", 310, 26, R_COMUN, 0xBCA1, 40, 45, 40, 65, 0 },  // 309 electrico
  { "MANECTRIC", 0, 0, R_EVO, 0xBCA1, 70, 75, 60, 105, 0 },  // 310 electrico
  { "PLUSLE", 0, 0, R_COMUN, 0xBCA1, 60, 50, 40, 95, 0 },  // 311 electrico
  { "MINUN", 0, 0, R_COMUN, 0xBCA1, 60, 40, 50, 95, 0 },  // 312 electrico
  { "VOLBEAT", 0, 0, R_COMUN, 0x7CC4, 65, 73, 75, 85, 2 },  // 313 bicho
  { "ILLUMISE", 0, 0, R_COMUN, 0x7CC4, 65, 47, 75, 85, 2 },  // 314 bicho
  { "ROSELIA", 407, 30, R_EVO, 0x3C49, 50, 60, 45, 65, 2 },  // 315 planta
  { "GULPIN", 317, 26, R_COMUN, 0x8A73, 70, 43, 53, 40, 0 },  // 316 veneno
  { "SWALOT", 0, 0, R_EVO, 0x8A73, 100, 73, 83, 55, 0 },  // 317 veneno
  { "CARVANHA", 319, 30, R_COMUN, 0x4C98, 45, 90, 20, 65, 1 },  // 318 agua
  { "SHARPEDO", 0, 0, R_EVO, 0x4C98, 70, 120, 40, 95, 1 },  // 319 agua
  { "WAILMER", 321, 40, R_COMUN, 0x4C98, 130, 70, 35, 60, 1 },  // 320 agua
  { "WAILORD", 0, 0, R_EVO, 0x4C98, 170, 90, 45, 60, 1 },  // 321 agua
  { "NUMEL", 323, 33, R_COMUN, 0xEA87, 60, 60, 40, 35, 3 },  // 322 fuego
  { "CAMERUPT", 0, 0, R_EVO, 0xEA87, 70, 100, 70, 40, 3 },  // 323 fuego
  { "TORKOAL", 0, 0, R_RARO, 0xEA87, 70, 85, 140, 20, 3 },  // 324 fuego
  { "SPOINK", 326, 32, R_COMUN, 0xD28F, 60, 25, 35, 60, 0 },  // 325 psiquico
  { "GRUMPIG", 0, 0, R_EVO, 0xD28F, 80, 45, 65, 80, 0 },  // 326 psiquico
  { "SPINDA", 0, 0, R_RARO, 0x8C4D, 60, 60, 60, 60, 0 },  // 327 normal
  { "TRAPINCH", 329, 35, R_COMUN, 0xB447, 45, 100, 45, 10, 4 },  // 328 tierra
  { "VIBRAVA", 330, 45, R_EVO, 0xB447, 50, 70, 50, 70, 4 },  // 329 tierra
  { "FLYGON", 0, 0, R_EVO, 0xB447, 80, 100, 80, 100, 4 },  // 330 tierra
  { "CACNEA", 332, 32, R_COMUN, 0x3C49, 50, 85, 40, 35, 2 },  // 331 planta
  { "CACTURNE", 0, 0, R_EVO, 0x3C49, 70, 115, 60, 55, 2 },  // 332 planta
  { "SWABLU", 334, 35, R_COMUN, 0x8C4D, 45, 40, 60, 50, 0 },  // 333 normal
  { "ALTARIA", 0, 0, R_EVO, 0x5A98, 75, 70, 90, 80, 1 },  // 334 dragon
  { "ZANGOOSE", 0, 0, R_RARO, 0x8C4D, 73, 115, 60, 90, 0 },  // 335 normal
  { "SEVIPER", 0, 0, R_RARO, 0x8A73, 73, 100, 60, 65, 0 },  // 336 veneno
  { "LUNATONE", 0, 0, R_RARO, 0x9407, 90, 55, 65, 70, 4 },  // 337 roca
  { "SOLROCK", 0, 0, R_RARO, 0x9407, 90, 95, 85, 70, 4 },  // 338 roca
  { "BARBOACH", 340, 30, R_COMUN, 0x4C98, 50, 48, 43, 60, 1 },  // 339 agua
  { "WHISCASH", 0, 0, R_EVO, 0x4C98, 110, 78, 73, 60, 1 },  // 340 agua
  { "CORPHISH", 342, 30, R_COMUN, 0x4C98, 43, 80, 65, 35, 1 },  // 341 agua
  { "CRAWDAUNT", 0, 0, R_EVO, 0x4C98, 63, 120, 85, 55, 1 },  // 342 agua
  { "BALTOY", 344, 36, R_COMUN, 0xB447, 40, 40, 55, 55, 4 },  // 343 tierra
  { "CLAYDOL", 0, 0, R_EVO, 0xB447, 60, 70, 105, 75, 4 },  // 344 tierra
  { "LILEEP", 346, 40, R_RARO, 0x9407, 66, 41, 77, 23, 4 },  // 345 roca
  { "CRADILY", 0, 0, R_EVO, 0x9407, 86, 81, 97, 43, 4 },  // 346 roca
  { "ANORITH", 348, 40, R_RARO, 0x9407, 45, 95, 50, 75, 4 },  // 347 roca
  { "ARMALDO", 0, 0, R_EVO, 0x9407, 75, 125, 100, 45, 4 },  // 348 roca
  { "FEEBAS", 350, 30, R_RARO, 0x4C98, 20, 15, 20, 80, 1 },  // 349 agua
  { "MILOTIC", 0, 0, R_EVO, 0x4C98, 95, 60, 79, 81, 1 },  // 350 agua
  { "CASTFORM", 0, 0, R_COMUN, 0x8C4D, 70, 70, 70, 70, 0 },  // 351 normal
  { "KECLEON", 0, 0, R_RARO, 0x8C4D, 60, 90, 70, 40, 0 },  // 352 normal
  { "SHUPPET", 354, 37, R_COMUN, 0x6AD3, 44, 75, 35, 45, 0 },  // 353 fantasma
  { "BANETTE", 0, 0, R_EVO, 0x6AD3, 64, 115, 65, 65, 0 },  // 354 fantasma
  { "DUSKULL", 356, 37, R_COMUN, 0x6AD3, 20, 40, 90, 25, 0 },  // 355 fantasma
  { "DUSCLOPS", 477, 42, R_EVO, 0x6AD3, 40, 70, 130, 25, 0 },  // 356 fantasma
  { "TROPIUS", 0, 0, R_RARO, 0x3C49, 99, 68, 83, 51, 2 },  // 357 planta
  { "CHIMECHO", 0, 0, R_EVO, 0xD28F, 75, 50, 80, 65, 0 },  // 358 psiquico
  { "ABSOL", 0, 0, R_RARO, 0x5A6E, 65, 130, 60, 75, 0 },  // 359 siniestro
  { "WYNAUT", 202, 15, R_COMUN, 0xD28F, 95, 23, 48, 23, 0 },  // 360 psiquico
  { "SNORUNT", 362, 42, R_RARO, 0x4DB8, 50, 50, 50, 50, 5 },  // 361 hielo
  { "GLALIE", 0, 0, R_EVO, 0x4DB8, 80, 80, 80, 80, 5 },  // 362 hielo
  { "SPHEAL", 364, 32, R_COMUN, 0x4DB8, 70, 40, 50, 25, 5 },  // 363 hielo
  { "SEALEO", 365, 44, R_EVO, 0x4DB8, 90, 60, 70, 45, 5 },  // 364 hielo
  { "WALREIN", 0, 0, R_EVO, 0x4DB8, 110, 80, 90, 65, 5 },  // 365 hielo
  { "CLAMPERL", 367, 40, R_COMUN, 0x4C98, 35, 64, 85, 32, 1 },  // 366 agua
  { "HUNTAIL", 0, 0, R_EVO, 0x4C98, 55, 104, 105, 52, 1 },  // 367 agua
  { "GOREBYSS", 0, 0, R_EVO, 0x4C98, 55, 84, 105, 52, 1 },  // 368 agua
  { "RELICANTH", 0, 0, R_RARO, 0x4C98, 100, 90, 130, 55, 1 },  // 369 agua
  { "LUVDISC", 0, 0, R_RARO, 0x4C98, 43, 30, 55, 97, 1 },  // 370 agua
  { "BAGON", 372, 30, R_RARO, 0x5A98, 45, 75, 60, 50, 1 },  // 371 dragon
  { "SHELGON", 373, 50, R_EVO, 0x5A98, 65, 95, 100, 50, 1 },  // 372 dragon
  { "SALAMENCE", 0, 0, R_EVO, 0x5A98, 95, 135, 80, 100, 1 },  // 373 dragon
  { "BELDUM", 375, 20, R_RARO, 0x7C73, 40, 55, 80, 30, 4 },  // 374 acero
  { "METANG", 376, 45, R_EVO, 0x7C73, 60, 75, 100, 50, 4 },  // 375 acero
  { "METAGROSS", 0, 0, R_EVO, 0x7C73, 80, 135, 130, 70, 4 },  // 376 acero
  { "REGIROCK", 0, 0, R_LEGENDARIO, 0x9407, 80, 100, 200, 50, 4 },  // 377 roca
  { "REGICE", 0, 0, R_LEGENDARIO, 0x4DB8, 80, 50, 100, 50, 5 },  // 378 hielo
  { "REGISTEEL", 0, 0, R_LEGENDARIO, 0x7C73, 80, 75, 150, 50, 4 },  // 379 acero
  { "LATIAS", 0, 0, R_LEGENDARIO, 0x5A98, 80, 80, 90, 110, 1 },  // 380 dragon
  { "LATIOS", 0, 0, R_LEGENDARIO, 0x5A98, 80, 90, 80, 110, 1 },  // 381 dragon
  { "KYOGRE", 0, 0, R_LEGENDARIO, 0x4C98, 100, 100, 90, 90, 1 },  // 382 agua
  { "GROUDON", 0, 0, R_LEGENDARIO, 0xB447, 100, 150, 140, 90, 4 },  // 383 tierra
  { "RAYQUAZA", 0, 0, R_LEGENDARIO, 0x5A98, 105, 150, 90, 95, 1 },  // 384 dragon
  { "JIRACHI", 0, 0, R_LEGENDARIO, 0x7C73, 100, 100, 100, 100, 4 },  // 385 acero
  { "DEOXYS", 0, 0, R_LEGENDARIO, 0xD28F, 50, 150, 50, 150, 0 },  // 386 psiquico
  { "TURTWIG", 388, 18, R_RARO, 0x3C49, 55, 68, 64, 31, 2 },  // 387 planta
  { "GROTLE", 389, 32, R_EVO, 0x3C49, 75, 89, 85, 36, 2 },  // 388 planta
  { "TORTERRA", 0, 0, R_EVO, 0x3C49, 95, 109, 105, 56, 2 },  // 389 planta
  { "CHIMCHAR", 391, 14, R_RARO, 0xEA87, 44, 58, 44, 61, 3 },  // 390 fuego
  { "MONFERNO", 392, 36, R_EVO, 0xEA87, 64, 78, 52, 81, 3 },  // 391 fuego
  { "INFERNAPE", 0, 0, R_EVO, 0xEA87, 76, 104, 71, 108, 3 },  // 392 fuego
  { "PIPLUP", 394, 16, R_RARO, 0x4C98, 53, 51, 53, 40, 1 },  // 393 agua
  { "PRINPLUP", 395, 36, R_EVO, 0x4C98, 64, 66, 68, 50, 1 },  // 394 agua
  { "EMPOLEON", 0, 0, R_EVO, 0x4C98, 84, 86, 88, 60, 1 },  // 395 agua
  { "STARLY", 397, 14, R_COMUN, 0x8C4D, 40, 55, 30, 60, 0 },  // 396 normal
  { "STARAVIA", 398, 34, R_EVO, 0x8C4D, 55, 75, 50, 80, 0 },  // 397 normal
  { "STARAPTOR", 0, 0, R_EVO, 0x8C4D, 85, 120, 70, 100, 0 },  // 398 normal
  { "BIDOOF", 400, 15, R_COMUN, 0x8C4D, 59, 45, 40, 31, 0 },  // 399 normal
  { "BIBAREL", 0, 0, R_EVO, 0x8C4D, 79, 85, 60, 71, 0 },  // 400 normal
  { "KRICKETOT", 402, 10, R_COMUN, 0x7CC4, 37, 25, 41, 25, 2 },  // 401 bicho
  { "KRICKETUNE", 0, 0, R_EVO, 0x7CC4, 77, 85, 51, 65, 2 },  // 402 bicho
  { "SHINX", 404, 15, R_RARO, 0xBCA1, 45, 65, 34, 45, 0 },  // 403 electrico
  { "LUXIO", 405, 30, R_EVO, 0xBCA1, 60, 85, 49, 60, 0 },  // 404 electrico
  { "LUXRAY", 0, 0, R_EVO, 0xBCA1, 80, 120, 79, 70, 0 },  // 405 electrico
  { "BUDEW", 315, 25, R_RARO, 0x3C49, 40, 30, 35, 55, 2 },  // 406 planta
  { "ROSERADE", 0, 0, R_EVO, 0x3C49, 60, 70, 65, 90, 2 },  // 407 planta
  { "CRANIDOS", 409, 30, R_COMUN, 0x9407, 67, 125, 40, 58, 4 },  // 408 roca
  { "RAMPARDOS", 0, 0, R_EVO, 0x9407, 97, 165, 60, 58, 4 },  // 409 roca
  { "SHIELDON", 411, 30, R_COMUN, 0x9407, 30, 42, 118, 30, 4 },  // 410 roca
  { "BASTIODON", 0, 0, R_EVO, 0x9407, 60, 52, 168, 30, 4 },  // 411 roca
  { "BURMY", 413, 20, R_COMUN, 0x7CC4, 40, 29, 45, 36, 2 },  // 412 bicho
  { "WORMADAM", 0, 0, R_EVO, 0x7CC4, 60, 59, 85, 36, 2 },  // 413 bicho
  { "MOTHIM", 0, 0, R_EVO, 0x7CC4, 70, 94, 50, 66, 2 },  // 414 bicho
  { "COMBEE", 416, 21, R_COMUN, 0x7CC4, 30, 30, 42, 70, 2 },  // 415 bicho
  { "VESPIQUEN", 0, 0, R_EVO, 0x7CC4, 70, 80, 102, 40, 2 },  // 416 bicho
  { "PACHIRISU", 0, 0, R_COMUN, 0xBCA1, 60, 45, 70, 95, 0 },  // 417 electrico
  { "BUIZEL", 419, 26, R_COMUN, 0x4C98, 55, 65, 35, 85, 1 },  // 418 agua
  { "FLOATZEL", 0, 0, R_EVO, 0x4C98, 85, 105, 55, 115, 1 },  // 419 agua
  { "CHERUBI", 421, 25, R_COMUN, 0x3C49, 45, 35, 45, 35, 2 },  // 420 planta
  { "CHERRIM", 0, 0, R_EVO, 0x3C49, 70, 60, 70, 85, 2 },  // 421 planta
  { "SHELLOS", 423, 30, R_COMUN, 0x4C98, 76, 48, 48, 34, 1 },  // 422 agua
  { "GASTRODON", 0, 0, R_EVO, 0x4C98, 111, 83, 68, 39, 1 },  // 423 agua
  { "AMBIPOM", 0, 0, R_EVO, 0x8C4D, 75, 100, 66, 115, 0 },  // 424 normal
  { "DRIFLOON", 426, 28, R_COMUN, 0x6AD3, 90, 50, 34, 70, 0 },  // 425 fantasma
  { "DRIFBLIM", 0, 0, R_EVO, 0x6AD3, 150, 80, 44, 80, 0 },  // 426 fantasma
  { "BUNEARY", 428, 25, R_COMUN, 0x8C4D, 55, 66, 44, 85, 0 },  // 427 normal
  { "LOPUNNY", 0, 0, R_EVO, 0x8C4D, 65, 76, 84, 105, 0 },  // 428 normal
  { "MISMAGIUS", 0, 0, R_EVO, 0x6AD3, 60, 60, 60, 105, 0 },  // 429 fantasma
  { "HONCHKROW", 0, 0, R_EVO, 0x5A6E, 100, 125, 52, 71, 0 },  // 430 siniestro
  { "GLAMEOW", 432, 38, R_COMUN, 0x8C4D, 49, 55, 42, 85, 0 },  // 431 normal
  { "PURUGLY", 0, 0, R_EVO, 0x8C4D, 71, 82, 64, 112, 0 },  // 432 normal
  { "CHINGLING", 358, 25, R_COMUN, 0xD28F, 45, 30, 50, 45, 0 },  // 433 psiquico
  { "STUNKY", 435, 34, R_COMUN, 0x8A73, 63, 63, 47, 74, 0 },  // 434 veneno
  { "SKUNTANK", 0, 0, R_EVO, 0x8A73, 103, 93, 67, 84, 0 },  // 435 veneno
  { "BRONZOR", 437, 33, R_COMUN, 0x7C73, 57, 24, 86, 23, 4 },  // 436 acero
  { "BRONZONG", 0, 0, R_EVO, 0x7C73, 67, 89, 116, 33, 4 },  // 437 acero
  { "BONSLY", 185, 25, R_COMUN, 0x9407, 50, 80, 95, 10, 4 },  // 438 roca
  { "MIME JR.", 122, 25, R_COMUN, 0xD28F, 20, 25, 45, 60, 0 },  // 439 psiquico
  { "HAPPINY", 113, 25, R_RARO, 0x8C4D, 100, 5, 5, 30, 0 },  // 440 normal
  { "CHATOT", 0, 0, R_COMUN, 0x8C4D, 76, 65, 45, 91, 0 },  // 441 normal
  { "SPIRITOMB", 0, 0, R_COMUN, 0x6AD3, 50, 92, 108, 35, 0 },  // 442 fantasma
  { "GIBLE", 444, 24, R_RARO, 0x5A98, 58, 70, 45, 42, 1 },  // 443 dragon
  { "GABITE", 445, 48, R_EVO, 0x5A98, 68, 90, 65, 82, 1 },  // 444 dragon
  { "GARCHOMP", 0, 0, R_EVO, 0x5A98, 108, 130, 95, 102, 1 },  // 445 dragon
  { "MUNCHLAX", 143, 25, R_RARO, 0x8C4D, 135, 85, 40, 5, 0 },  // 446 normal
  { "RIOLU", 448, 25, R_RARO, 0xA2A5, 40, 70, 40, 60, 0 },  // 447 lucha
  { "LUCARIO", 0, 0, R_EVO, 0xA2A5, 70, 110, 70, 90, 0 },  // 448 lucha
  { "HIPPOPOTAS", 450, 34, R_RARO, 0xB447, 68, 72, 78, 32, 4 },  // 449 tierra
  { "HIPPOWDON", 0, 0, R_EVO, 0xB447, 108, 112, 118, 47, 4 },  // 450 tierra
  { "SKORUPI", 452, 40, R_COMUN, 0x8A73, 40, 50, 90, 65, 0 },  // 451 veneno
  { "DRAPION", 0, 0, R_EVO, 0x8A73, 70, 90, 110, 95, 0 },  // 452 veneno
  { "CROAGUNK", 454, 37, R_COMUN, 0x8A73, 48, 61, 40, 50, 0 },  // 453 veneno
  { "TOXICROAK", 0, 0, R_EVO, 0x8A73, 83, 106, 65, 85, 0 },  // 454 veneno
  { "CARNIVINE", 0, 0, R_COMUN, 0x3C49, 74, 100, 72, 46, 2 },  // 455 planta
  { "FINNEON", 457, 31, R_COMUN, 0x4C98, 49, 49, 56, 66, 1 },  // 456 agua
  { "LUMINEON", 0, 0, R_EVO, 0x4C98, 69, 69, 76, 91, 1 },  // 457 agua
  { "MANTYKE", 226, 25, R_COMUN, 0x4C98, 45, 20, 50, 50, 1 },  // 458 agua
  { "SNOVER", 460, 40, R_COMUN, 0x3C49, 60, 62, 50, 40, 2 },  // 459 planta
  { "ABOMASNOW", 0, 0, R_EVO, 0x3C49, 90, 92, 75, 60, 2 },  // 460 planta
  { "WEAVILE", 0, 0, R_EVO, 0x5A6E, 70, 120, 65, 125, 0 },  // 461 siniestro
  { "MAGNEZONE", 0, 0, R_EVO, 0xBCA1, 70, 70, 115, 60, 0 },  // 462 electrico
  { "LICKILICKY", 0, 0, R_EVO, 0x8C4D, 110, 85, 95, 50, 0 },  // 463 normal
  { "RHYPERIOR", 0, 0, R_EVO, 0xB447, 115, 140, 130, 40, 4 },  // 464 tierra
  { "TANGROWTH", 0, 0, R_EVO, 0x3C49, 100, 100, 125, 50, 2 },  // 465 planta
  { "ELECTIVIRE", 0, 0, R_EVO, 0xBCA1, 75, 123, 67, 95, 0 },  // 466 electrico
  { "MAGMORTAR", 0, 0, R_EVO, 0xEA87, 75, 95, 67, 83, 3 },  // 467 fuego
  { "TOGEKISS", 0, 0, R_EVO, 0xD435, 85, 50, 95, 80, 0 },  // 468 hada
  { "YANMEGA", 0, 0, R_EVO, 0x7CC4, 86, 76, 86, 95, 2 },  // 469 bicho
  { "LEAFEON", 0, 0, R_EVO, 0x3C49, 65, 110, 130, 95, 2 },  // 470 planta
  { "GLACEON", 0, 0, R_EVO, 0x4DB8, 65, 60, 110, 65, 5 },  // 471 hielo
  { "GLISCOR", 0, 0, R_EVO, 0xB447, 75, 95, 125, 95, 4 },  // 472 tierra
  { "MAMOSWINE", 0, 0, R_EVO, 0x4DB8, 110, 130, 80, 80, 5 },  // 473 hielo
  { "PORYGON-Z", 0, 0, R_EVO, 0x8C4D, 85, 80, 70, 90, 0 },  // 474 normal
  { "GALLADE", 0, 0, R_EVO, 0xD28F, 68, 125, 65, 80, 0 },  // 475 psiquico
  { "PROBOPASS", 0, 0, R_EVO, 0x9407, 60, 55, 145, 40, 4 },  // 476 roca
  { "DUSKNOIR", 0, 0, R_EVO, 0x6AD3, 45, 100, 135, 45, 0 },  // 477 fantasma
  { "FROSLASS", 0, 0, R_EVO, 0x4DB8, 70, 80, 70, 110, 5 },  // 478 hielo
  { "ROTOM", 0, 0, R_COMUN, 0xBCA1, 50, 50, 77, 91, 0 },  // 479 electrico
  { "UXIE", 0, 0, R_LEGENDARIO, 0xD28F, 75, 75, 130, 95, 0 },  // 480 psiquico
  { "MESPRIT", 0, 0, R_LEGENDARIO, 0xD28F, 80, 105, 105, 80, 0 },  // 481 psiquico
  { "AZELF", 0, 0, R_LEGENDARIO, 0xD28F, 75, 125, 70, 115, 0 },  // 482 psiquico
  { "DIALGA", 0, 0, R_LEGENDARIO, 0x7C73, 100, 120, 120, 90, 4 },  // 483 acero
  { "PALKIA", 0, 0, R_LEGENDARIO, 0x4C98, 90, 120, 100, 100, 1 },  // 484 agua
  { "HEATRAN", 0, 0, R_LEGENDARIO, 0xEA87, 91, 90, 106, 77, 3 },  // 485 fuego
  { "REGIGIGAS", 0, 0, R_LEGENDARIO, 0x8C4D, 110, 160, 110, 100, 0 },  // 486 normal
  { "GIRATINA", 0, 0, R_LEGENDARIO, 0x6AD3, 150, 100, 120, 90, 0 },  // 487 fantasma
  { "CRESSELIA", 0, 0, R_LEGENDARIO, 0xD28F, 120, 70, 110, 85, 0 },  // 488 psiquico
  { "PHIONE", 0, 0, R_LEGENDARIO, 0x4C98, 80, 80, 80, 80, 1 },  // 489 agua
  { "MANAPHY", 0, 0, R_LEGENDARIO, 0x4C98, 100, 100, 100, 100, 1 },  // 490 agua
  { "DARKRAI", 0, 0, R_LEGENDARIO, 0x5A6E, 70, 90, 90, 125, 0 },  // 491 siniestro
  { "SHAYMIN", 0, 0, R_LEGENDARIO, 0x3C49, 100, 100, 100, 100, 2 },  // 492 planta
  { "ARCEUS", 0, 0, R_LEGENDARIO, 0x8C4D, 120, 120, 120, 120, 0 },  // 493 normal
  { "VICTINI", 0, 0, R_LEGENDARIO, 0xD28F, 100, 100, 100, 100, 0 },  // 494 psiquico
  { "SNIVY", 496, 17, R_RARO, 0x3C49, 45, 45, 55, 63, 2 },  // 495 planta
  { "SERVINE", 497, 36, R_EVO, 0x3C49, 60, 60, 75, 83, 2 },  // 496 planta
  { "SERPERIOR", 0, 0, R_EVO, 0x3C49, 75, 75, 95, 113, 2 },  // 497 planta
  { "TEPIG", 499, 17, R_RARO, 0xEA87, 65, 63, 45, 45, 3 },  // 498 fuego
  { "PIGNITE", 500, 36, R_EVO, 0xEA87, 90, 93, 55, 55, 3 },  // 499 fuego
  { "EMBOAR", 0, 0, R_EVO, 0xEA87, 110, 123, 65, 65, 3 },  // 500 fuego
  { "OSHAWOTT", 502, 17, R_RARO, 0x4C98, 55, 55, 45, 45, 1 },  // 501 agua
  { "DEWOTT", 503, 36, R_EVO, 0x4C98, 75, 75, 60, 60, 1 },  // 502 agua
  { "SAMUROTT", 0, 0, R_EVO, 0x4C98, 95, 100, 85, 70, 1 },  // 503 agua
  { "PATRAT", 505, 20, R_COMUN, 0x8C4D, 45, 55, 39, 42, 0 },  // 504 normal
  { "WATCHOG", 0, 0, R_EVO, 0x8C4D, 60, 85, 69, 77, 0 },  // 505 normal
  { "LILLIPUP", 507, 16, R_COMUN, 0x8C4D, 45, 60, 45, 55, 0 },  // 506 normal
  { "HERDIER", 508, 32, R_EVO, 0x8C4D, 65, 80, 65, 60, 0 },  // 507 normal
  { "STOUTLAND", 0, 0, R_EVO, 0x8C4D, 85, 110, 90, 80, 0 },  // 508 normal
  { "PURRLOIN", 510, 20, R_COMUN, 0x5A6E, 41, 50, 37, 66, 0 },  // 509 siniestro
  { "LIEPARD", 0, 0, R_EVO, 0x5A6E, 64, 88, 50, 106, 0 },  // 510 siniestro
  { "PANSAGE", 512, 30, R_COMUN, 0x3C49, 50, 53, 48, 64, 2 },  // 511 planta
  { "SIMISAGE", 0, 0, R_EVO, 0x3C49, 75, 98, 63, 101, 2 },  // 512 planta
  { "PANSEAR", 0, 0, R_COMUN, 0xEA87, 50, 53, 48, 64, 3 },  // 513 fuego
  { "SIMISEAR", 0, 0, R_NONE, 0xEA87, 75, 98, 63, 101, 3 },  // 514 fuego
  { "PANPOUR", 0, 0, R_COMUN, 0x4C98, 50, 53, 48, 64, 1 },  // 515 agua
  { "SIMIPOUR", 0, 0, R_NONE, 0x4C98, 75, 98, 63, 101, 1 },  // 516 agua
  { "MUNNA", 518, 30, R_COMUN, 0xD28F, 76, 25, 45, 24, 0 },  // 517 psiquico
  { "MUSHARNA", 0, 0, R_EVO, 0xD28F, 116, 55, 85, 29, 0 },  // 518 psiquico
  { "PIDOVE", 521, 32, R_COMUN, 0x8C4D, 50, 55, 50, 43, 0 },  // 519 normal
  { "TRANQUILL", 0, 0, R_NONE, 0x8C4D, 62, 77, 62, 65, 0 },  // 520 normal
  { "UNFEZANT", 0, 0, R_EVO, 0x8C4D, 80, 115, 80, 93, 0 },  // 521 normal
  { "BLITZLE", 0, 0, R_NONE, 0xBCA1, 45, 60, 32, 76, 0 },  // 522 electrico
  { "ZEBSTRIKA", 0, 0, R_NONE, 0xBCA1, 75, 100, 63, 116, 0 },  // 523 electrico
  { "ROGGENROLA", 525, 25, R_RARO, 0x9407, 55, 75, 85, 15, 4 },  // 524 roca
  { "BOLDORE", 526, 40, R_EVO, 0x9407, 70, 105, 105, 20, 4 },  // 525 roca
  { "GIGALITH", 0, 0, R_EVO, 0x9407, 85, 135, 130, 25, 4 },  // 526 roca
  { "WOOBAT", 528, 25, R_COMUN, 0xD28F, 65, 45, 43, 72, 0 },  // 527 psiquico
  { "SWOOBAT", 0, 0, R_EVO, 0xD28F, 67, 57, 55, 114, 0 },  // 528 psiquico
  { "DRILBUR", 530, 31, R_COMUN, 0xB447, 60, 85, 40, 68, 4 },  // 529 tierra
  { "EXCADRILL", 0, 0, R_EVO, 0xB447, 110, 135, 60, 88, 4 },  // 530 tierra
  { "AUDINO", 0, 0, R_COMUN, 0x8C4D, 103, 60, 86, 50, 0 },  // 531 normal
  { "TIMBURR", 533, 25, R_COMUN, 0xA2A5, 75, 80, 55, 35, 0 },  // 532 lucha
  { "GURDURR", 534, 40, R_EVO, 0xA2A5, 85, 105, 85, 40, 0 },  // 533 lucha
  { "CONKELDURR", 0, 0, R_EVO, 0xA2A5, 105, 140, 95, 45, 0 },  // 534 lucha
  { "TYMPOLE", 536, 25, R_COMUN, 0x4C98, 50, 50, 40, 64, 1 },  // 535 agua
  { "PALPITOAD", 537, 36, R_EVO, 0x4C98, 75, 65, 55, 69, 1 },  // 536 agua
  { "SEISMITOAD", 0, 0, R_EVO, 0x4C98, 105, 95, 75, 74, 1 },  // 537 agua
  { "THROH", 0, 0, R_NONE, 0xA2A5, 120, 100, 85, 45, 0 },  // 538 lucha
  { "SAWK", 0, 0, R_COMUN, 0xA2A5, 75, 125, 75, 85, 0 },  // 539 lucha
  { "SEWADDLE", 541, 20, R_COMUN, 0x7CC4, 45, 53, 70, 42, 2 },  // 540 bicho
  { "SWADLOON", 542, 25, R_EVO, 0x7CC4, 55, 63, 90, 42, 2 },  // 541 bicho
  { "LEAVANNY", 0, 0, R_EVO, 0x7CC4, 75, 103, 80, 92, 2 },  // 542 bicho
  { "VENIPEDE", 544, 22, R_COMUN, 0x7CC4, 30, 45, 59, 57, 2 },  // 543 bicho
  { "WHIRLIPEDE", 545, 30, R_EVO, 0x7CC4, 40, 55, 99, 47, 2 },  // 544 bicho
  { "SCOLIPEDE", 0, 0, R_EVO, 0x7CC4, 60, 100, 89, 112, 2 },  // 545 bicho
  { "COTTONEE", 547, 30, R_COMUN, 0x3C49, 40, 27, 60, 66, 2 },  // 546 planta
  { "WHIMSICOTT", 0, 0, R_EVO, 0x3C49, 60, 67, 85, 116, 2 },  // 547 planta
  { "PETILIL", 549, 30, R_COMUN, 0x3C49, 45, 35, 50, 30, 2 },  // 548 planta
  { "LILLIGANT", 0, 0, R_EVO, 0x3C49, 70, 60, 75, 90, 2 },  // 549 planta
  { "BASCULIN", 902, 25, R_RARO, 0x4C98, 70, 92, 65, 98, 1 },  // 550 agua
  { "SANDILE", 552, 29, R_RARO, 0xB447, 50, 72, 35, 65, 4 },  // 551 tierra
  { "KROKOROK", 553, 40, R_EVO, 0xB447, 60, 82, 45, 74, 4 },  // 552 tierra
  { "KROOKODILE", 0, 0, R_EVO, 0xB447, 95, 117, 80, 92, 4 },  // 553 tierra
  { "DARUMAKA", 555, 35, R_COMUN, 0xEA87, 70, 90, 45, 50, 3 },  // 554 fuego
  { "DARMANITAN", 0, 0, R_EVO, 0xEA87, 105, 140, 55, 95, 3 },  // 555 fuego
  { "MARACTUS", 0, 0, R_COMUN, 0x3C49, 75, 86, 67, 60, 2 },  // 556 planta
  { "DWEBBLE", 0, 0, R_COMUN, 0x7CC4, 50, 65, 85, 55, 2 },  // 557 bicho
  { "CRUSTLE", 0, 0, R_NONE, 0x7CC4, 70, 105, 125, 45, 2 },  // 558 bicho
  { "SCRAGGY", 560, 39, R_COMUN, 0x5A6E, 50, 75, 70, 48, 0 },  // 559 siniestro
  { "SCRAFTY", 0, 0, R_EVO, 0x5A6E, 65, 90, 115, 58, 0 },  // 560 siniestro
  { "SIGILYPH", 0, 0, R_COMUN, 0xD28F, 72, 58, 80, 97, 0 },  // 561 psiquico
  { "YAMASK", 563, 34, R_COMUN, 0x6AD3, 38, 30, 85, 30, 0 },  // 562 fantasma
  { "COFAGRIGUS", 0, 0, R_EVO, 0x6AD3, 58, 50, 145, 30, 0 },  // 563 fantasma
  { "TIRTOUGA", 0, 0, R_NONE, 0x4C98, 54, 78, 103, 22, 1 },  // 564 agua
  { "CARRACOSTA", 0, 0, R_NONE, 0x4C98, 74, 108, 133, 32, 1 },  // 565 agua
  { "ARCHEN", 567, 37, R_RARO, 0x9407, 55, 112, 45, 70, 4 },  // 566 roca
  { "ARCHEOPS", 0, 0, R_EVO, 0x9407, 75, 140, 65, 110, 4 },  // 567 roca
  { "TRUBBISH", 569, 36, R_COMUN, 0x8A73, 50, 50, 62, 65, 0 },  // 568 veneno
  { "GARBODOR", 0, 0, R_EVO, 0x8A73, 80, 95, 82, 75, 0 },  // 569 veneno
  { "ZORUA", 571, 30, R_RARO, 0x5A6E, 40, 65, 40, 65, 0 },  // 570 siniestro
  { "ZOROARK", 0, 0, R_EVO, 0x5A6E, 60, 105, 60, 105, 0 },  // 571 siniestro
  { "MINCCINO", 573, 30, R_COMUN, 0x8C4D, 55, 50, 40, 75, 0 },  // 572 normal
  { "CINCCINO", 0, 0, R_EVO, 0x8C4D, 75, 95, 60, 115, 0 },  // 573 normal
  { "GOTHITA", 575, 32, R_COMUN, 0xD28F, 45, 30, 50, 45, 0 },  // 574 psiquico
  { "GOTHORITA", 576, 41, R_EVO, 0xD28F, 60, 45, 70, 55, 0 },  // 575 psiquico
  { "GOTHITELLE", 0, 0, R_EVO, 0xD28F, 70, 55, 95, 65, 0 },  // 576 psiquico
  { "SOLOSIS", 578, 32, R_COMUN, 0xD28F, 45, 30, 40, 20, 0 },  // 577 psiquico
  { "DUOSION", 579, 41, R_EVO, 0xD28F, 65, 40, 50, 30, 0 },  // 578 psiquico
  { "REUNICLUS", 0, 0, R_EVO, 0xD28F, 110, 65, 75, 30, 0 },  // 579 psiquico
  { "DUCKLETT", 581, 35, R_COMUN, 0x4C98, 62, 44, 50, 55, 1 },  // 580 agua
  { "SWANNA", 0, 0, R_EVO, 0x4C98, 75, 87, 63, 98, 1 },  // 581 agua
  { "VANILLITE", 583, 35, R_RARO, 0x4DB8, 36, 50, 50, 44, 5 },  // 582 hielo
  { "VANILLISH", 584, 47, R_EVO, 0x4DB8, 51, 65, 65, 59, 5 },  // 583 hielo
  { "VANILLUXE", 0, 0, R_EVO, 0x4DB8, 71, 95, 85, 79, 5 },  // 584 hielo
  { "DEERLING", 586, 34, R_COMUN, 0x8C4D, 60, 60, 50, 75, 0 },  // 585 normal
  { "SAWSBUCK", 0, 0, R_EVO, 0x8C4D, 80, 100, 70, 95, 0 },  // 586 normal
  { "EMOLGA", 0, 0, R_COMUN, 0xBCA1, 55, 75, 60, 103, 0 },  // 587 electrico
  { "KARRABLAST", 589, 40, R_COMUN, 0x7CC4, 50, 75, 45, 60, 2 },  // 588 bicho
  { "ESCAVALIER", 0, 0, R_EVO, 0x7CC4, 70, 135, 105, 20, 2 },  // 589 bicho
  { "FOONGUS", 0, 0, R_COMUN, 0x3C49, 69, 55, 45, 15, 2 },  // 590 planta
  { "AMOONGUSS", 0, 0, R_NONE, 0x3C49, 114, 85, 70, 30, 2 },  // 591 planta
  { "FRILLISH", 0, 0, R_NONE, 0x4C98, 55, 40, 50, 40, 1 },  // 592 agua
  { "JELLICENT", 0, 0, R_COMUN, 0x4C98, 100, 60, 70, 60, 1 },  // 593 agua
  { "ALOMOMOLA", 0, 0, R_COMUN, 0x4C98, 165, 75, 80, 65, 1 },  // 594 agua
  { "JOLTIK", 596, 36, R_COMUN, 0x7CC4, 50, 47, 50, 65, 2 },  // 595 bicho
  { "GALVANTULA", 0, 0, R_EVO, 0x7CC4, 70, 77, 60, 108, 2 },  // 596 bicho
  { "FERROSEED", 598, 40, R_COMUN, 0x3C49, 44, 50, 91, 10, 2 },  // 597 planta
  { "FERROTHORN", 0, 0, R_EVO, 0x3C49, 74, 94, 131, 20, 2 },  // 598 planta
  { "KLINK", 600, 38, R_RARO, 0x7C73, 40, 55, 70, 30, 4 },  // 599 acero
  { "KLANG", 601, 49, R_EVO, 0x7C73, 60, 80, 95, 50, 4 },  // 600 acero
  { "KLINKLANG", 0, 0, R_EVO, 0x7C73, 60, 100, 115, 90, 4 },  // 601 acero
  { "TYNAMO", 603, 39, R_RARO, 0xBCA1, 35, 55, 40, 60, 0 },  // 602 electrico
  { "EELEKTRIK", 604, 44, R_EVO, 0xBCA1, 65, 85, 70, 40, 0 },  // 603 electrico
  { "EELEKTROSS", 0, 0, R_EVO, 0xBCA1, 85, 115, 80, 50, 0 },  // 604 electrico
  { "ELGYEM", 606, 42, R_COMUN, 0xD28F, 55, 55, 55, 30, 0 },  // 605 psiquico
  { "BEHEEYEM", 0, 0, R_EVO, 0xD28F, 75, 75, 75, 40, 0 },  // 606 psiquico
  { "LITWICK", 608, 41, R_RARO, 0x6AD3, 50, 30, 55, 20, 0 },  // 607 fantasma
  { "LAMPENT", 609, 46, R_EVO, 0x6AD3, 60, 40, 60, 55, 0 },  // 608 fantasma
  { "CHANDELURE", 0, 0, R_EVO, 0x6AD3, 60, 55, 90, 80, 0 },  // 609 fantasma
  { "AXEW", 611, 38, R_RARO, 0x5A98, 46, 87, 60, 57, 1 },  // 610 dragon
  { "FRAXURE", 612, 48, R_EVO, 0x5A98, 66, 117, 70, 67, 1 },  // 611 dragon
  { "HAXORUS", 0, 0, R_EVO, 0x5A98, 76, 147, 90, 97, 1 },  // 612 dragon
  { "CUBCHOO", 614, 37, R_COMUN, 0x4DB8, 55, 70, 40, 40, 5 },  // 613 hielo
  { "BEARTIC", 0, 0, R_EVO, 0x4DB8, 95, 130, 80, 50, 5 },  // 614 hielo
  { "CRYOGONAL", 0, 0, R_RARO, 0x4DB8, 80, 50, 50, 105, 5 },  // 615 hielo
  { "SHELMET", 0, 0, R_NONE, 0x7CC4, 50, 40, 85, 25, 2 },  // 616 bicho
  { "ACCELGOR", 0, 0, R_COMUN, 0x7CC4, 80, 70, 40, 145, 2 },  // 617 bicho
  { "STUNFISK", 0, 0, R_COMUN, 0xB447, 109, 66, 84, 32, 4 },  // 618 tierra
  { "MIENFOO", 620, 50, R_RARO, 0xA2A5, 45, 85, 50, 65, 0 },  // 619 lucha
  { "MIENSHAO", 0, 0, R_EVO, 0xA2A5, 65, 125, 60, 105, 0 },  // 620 lucha
  { "DRUDDIGON", 0, 0, R_COMUN, 0x5A98, 77, 120, 90, 48, 1 },  // 621 dragon
  { "GOLETT", 623, 43, R_COMUN, 0xB447, 59, 74, 50, 35, 4 },  // 622 tierra
  { "GOLURK", 0, 0, R_EVO, 0xB447, 89, 124, 80, 55, 4 },  // 623 tierra
  { "PAWNIARD", 625, 52, R_RARO, 0x5A6E, 45, 85, 70, 60, 0 },  // 624 siniestro
  { "BISHARP", 983, 57, R_EVO, 0x5A6E, 65, 125, 100, 70, 0 },  // 625 siniestro
  { "BOUFFALANT", 0, 0, R_NONE, 0x8C4D, 95, 110, 95, 55, 0 },  // 626 normal
  { "RUFFLET", 628, 54, R_RARO, 0x8C4D, 70, 83, 50, 60, 0 },  // 627 normal
  { "BRAVIARY", 0, 0, R_EVO, 0x8C4D, 100, 123, 75, 80, 0 },  // 628 normal
  { "VULLABY", 630, 54, R_RARO, 0x5A6E, 70, 55, 75, 60, 0 },  // 629 siniestro
  { "MANDIBUZZ", 0, 0, R_EVO, 0x5A6E, 110, 65, 105, 80, 0 },  // 630 siniestro
  { "HEATMOR", 0, 0, R_COMUN, 0xEA87, 85, 97, 66, 65, 3 },  // 631 fuego
  { "DURANT", 0, 0, R_COMUN, 0x7CC4, 58, 109, 112, 109, 2 },  // 632 bicho
  { "DEINO", 634, 50, R_RARO, 0x5A6E, 52, 65, 50, 38, 0 },  // 633 siniestro
  { "ZWEILOUS", 635, 64, R_EVO, 0x5A6E, 72, 85, 70, 58, 0 },  // 634 siniestro
  { "HYDREIGON", 0, 0, R_EVO, 0x5A6E, 92, 105, 90, 98, 0 },  // 635 siniestro
  { "LARVESTA", 637, 59, R_RARO, 0x7CC4, 55, 85, 55, 60, 2 },  // 636 bicho
  { "VOLCARONA", 0, 0, R_EVO, 0x7CC4, 85, 60, 65, 100, 2 },  // 637 bicho
  { "COBALION", 0, 0, R_LEGENDARIO, 0x7C73, 91, 90, 129, 108, 4 },  // 638 acero
  { "TERRAKION", 0, 0, R_LEGENDARIO, 0x9407, 91, 129, 90, 108, 4 },  // 639 roca
  { "VIRIZION", 0, 0, R_LEGENDARIO, 0x3C49, 91, 90, 72, 108, 2 },  // 640 planta
  { "TORNADUS", 0, 0, R_LEGENDARIO, 0x7C78, 79, 115, 70, 111, 4 },  // 641 volador
  { "THUNDURUS", 0, 0, R_LEGENDARIO, 0xBCA1, 79, 115, 70, 111, 0 },  // 642 electrico
  { "RESHIRAM", 0, 0, R_LEGENDARIO, 0x5A98, 100, 120, 100, 90, 1 },  // 643 dragon
  { "ZEKROM", 0, 0, R_LEGENDARIO, 0x5A98, 100, 150, 120, 90, 1 },  // 644 dragon
  { "LANDORUS", 0, 0, R_LEGENDARIO, 0xB447, 89, 125, 90, 101, 4 },  // 645 tierra
  { "KYUREM", 0, 0, R_LEGENDARIO, 0x5A98, 125, 130, 90, 95, 1 },  // 646 dragon
  { "KELDEO", 0, 0, R_LEGENDARIO, 0x4C98, 91, 72, 90, 108, 1 },  // 647 agua
  { "MELOETTA", 0, 0, R_LEGENDARIO, 0x8C4D, 100, 77, 77, 90, 0 },  // 648 normal
  { "GENESECT", 0, 0, R_LEGENDARIO, 0x7CC4, 71, 120, 95, 99, 2 },  // 649 bicho
  { "CHESPIN", 651, 16, R_RARO, 0x3C49, 56, 61, 65, 38, 2 },  // 650 planta
  { "QUILLADIN", 652, 36, R_EVO, 0x3C49, 61, 78, 95, 57, 2 },  // 651 planta
  { "CHESNAUGHT", 0, 0, R_EVO, 0x3C49, 88, 107, 122, 64, 2 },  // 652 planta
  { "FENNEKIN", 654, 16, R_RARO, 0xEA87, 40, 45, 40, 60, 3 },  // 653 fuego
  { "BRAIXEN", 655, 36, R_EVO, 0xEA87, 59, 59, 58, 73, 3 },  // 654 fuego
  { "DELPHOX", 0, 0, R_EVO, 0xEA87, 75, 69, 72, 104, 3 },  // 655 fuego
  { "FROAKIE", 657, 16, R_RARO, 0x4C98, 41, 56, 40, 71, 1 },  // 656 agua
  { "FROGADIER", 658, 36, R_EVO, 0x4C98, 54, 63, 52, 97, 1 },  // 657 agua
  { "GRENINJA", 0, 0, R_EVO, 0x4C98, 72, 95, 67, 122, 1 },  // 658 agua
  { "BUNNELBY", 660, 20, R_COMUN, 0x8C4D, 38, 36, 38, 57, 0 },  // 659 normal
  { "DIGGERSBY", 0, 0, R_EVO, 0x8C4D, 85, 56, 77, 78, 0 },  // 660 normal
  { "FLETCHLING", 662, 17, R_COMUN, 0x8C4D, 45, 50, 43, 62, 0 },  // 661 normal
  { "FLETCHINDER", 663, 35, R_EVO, 0xEA87, 62, 73, 55, 84, 3 },  // 662 fuego
  { "TALONFLAME", 0, 0, R_EVO, 0xEA87, 78, 81, 71, 126, 3 },  // 663 fuego
  { "SCATTERBUG", 665, 9, R_COMUN, 0x7CC4, 38, 35, 40, 35, 2 },  // 664 bicho
  { "SPEWPA", 666, 12, R_EVO, 0x7CC4, 45, 22, 60, 29, 2 },  // 665 bicho
  { "VIVILLON", 0, 0, R_EVO, 0x7CC4, 80, 52, 50, 89, 2 },  // 666 bicho
  { "LITLEO", 0, 0, R_COMUN, 0xEA87, 62, 50, 58, 72, 3 },  // 667 fuego
  { "PYROAR", 0, 0, R_NONE, 0xEA87, 86, 68, 72, 106, 3 },  // 668 fuego
  { "FLABEBE", 670, 19, R_RARO, 0xD435, 44, 38, 39, 42, 0 },  // 669 hada
  { "FLOETTE", 671, 30, R_EVO, 0xD435, 54, 45, 47, 52, 0 },  // 670 hada
  { "FLORGES", 0, 0, R_EVO, 0xD435, 78, 65, 68, 75, 0 },  // 671 hada
  { "SKIDDO", 673, 32, R_RARO, 0x3C49, 66, 65, 48, 52, 2 },  // 672 planta
  { "GOGOAT", 0, 0, R_EVO, 0x3C49, 123, 100, 62, 68, 2 },  // 673 planta
  { "PANCHAM", 675, 32, R_COMUN, 0xA2A5, 67, 82, 62, 43, 0 },  // 674 lucha
  { "PANGORO", 0, 0, R_EVO, 0xA2A5, 95, 124, 78, 58, 0 },  // 675 lucha
  { "FURFROU", 0, 0, R_COMUN, 0x8C4D, 75, 80, 60, 102, 0 },  // 676 normal
  { "ESPURR", 678, 25, R_COMUN, 0xD28F, 62, 48, 54, 68, 0 },  // 677 psiquico
  { "MEOWSTIC", 0, 0, R_EVO, 0xD28F, 74, 48, 76, 104, 0 },  // 678 psiquico
  { "HONEDGE", 680, 35, R_COMUN, 0x7C73, 45, 80, 100, 28, 4 },  // 679 acero
  { "DOUBLADE", 681, 40, R_EVO, 0x7C73, 59, 110, 150, 35, 4 },  // 680 acero
  { "AEGISLASH", 0, 0, R_EVO, 0x7C73, 60, 50, 140, 60, 4 },  // 681 acero
  { "SPRITZEE", 683, 40, R_COMUN, 0xD435, 78, 52, 60, 23, 0 },  // 682 hada
  { "AROMATISSE", 0, 0, R_EVO, 0xD435, 101, 72, 72, 29, 0 },  // 683 hada
  { "SWIRLIX", 685, 40, R_COMUN, 0xD435, 62, 48, 66, 49, 0 },  // 684 hada
  { "SLURPUFF", 0, 0, R_EVO, 0xD435, 82, 80, 86, 72, 0 },  // 685 hada
  { "INKAY", 687, 30, R_COMUN, 0x5A6E, 53, 54, 53, 45, 0 },  // 686 siniestro
  { "MALAMAR", 0, 0, R_EVO, 0x5A6E, 86, 92, 88, 73, 0 },  // 687 siniestro
  { "BINACLE", 689, 39, R_COMUN, 0x9407, 42, 52, 67, 50, 4 },  // 688 roca
  { "BARBARACLE", 0, 0, R_EVO, 0x9407, 72, 105, 115, 68, 4 },  // 689 roca
  { "SKRELP", 691, 48, R_COMUN, 0x8A73, 50, 60, 60, 30, 0 },  // 690 veneno
  { "DRAGALGE", 0, 0, R_EVO, 0x8A73, 65, 75, 90, 44, 0 },  // 691 veneno
  { "CLAUNCHER", 693, 37, R_COMUN, 0x4C98, 50, 53, 62, 44, 1 },  // 692 agua
  { "CLAWITZER", 0, 0, R_EVO, 0x4C98, 71, 73, 88, 59, 1 },  // 693 agua
  { "HELIOPTILE", 695, 30, R_COMUN, 0xBCA1, 44, 38, 33, 70, 0 },  // 694 electrico
  { "HELIOLISK", 0, 0, R_EVO, 0xBCA1, 62, 55, 52, 109, 0 },  // 695 electrico
  { "TYRUNT", 697, 39, R_RARO, 0x9407, 58, 89, 77, 48, 4 },  // 696 roca
  { "TYRANTRUM", 0, 0, R_EVO, 0x9407, 82, 121, 119, 71, 4 },  // 697 roca
  { "AMAURA", 699, 39, R_RARO, 0x9407, 77, 59, 50, 46, 4 },  // 698 roca
  { "AURORUS", 0, 0, R_EVO, 0x9407, 123, 77, 72, 58, 4 },  // 699 roca
  { "SYLVEON", 0, 0, R_EVO, 0xD435, 95, 65, 65, 60, 0 },  // 700 hada
  { "HAWLUCHA", 0, 0, R_COMUN, 0xA2A5, 78, 92, 75, 118, 0 },  // 701 lucha
  { "DEDENNE", 0, 0, R_COMUN, 0xBCA1, 67, 58, 57, 101, 0 },  // 702 electrico
  { "CARBINK", 0, 0, R_COMUN, 0x9407, 50, 50, 150, 50, 4 },  // 703 roca
  { "GOOMY", 705, 40, R_RARO, 0x5A98, 45, 50, 35, 40, 1 },  // 704 dragon
  { "SLIGGOO", 706, 50, R_EVO, 0x5A98, 68, 75, 53, 60, 1 },  // 705 dragon
  { "GOODRA", 0, 0, R_EVO, 0x5A98, 90, 100, 70, 80, 1 },  // 706 dragon
  { "KLEFKI", 0, 0, R_COMUN, 0x7C73, 57, 80, 91, 75, 4 },  // 707 acero
  { "PHANTUMP", 709, 40, R_COMUN, 0x6AD3, 43, 70, 48, 38, 0 },  // 708 fantasma
  { "TREVENANT", 0, 0, R_EVO, 0x6AD3, 85, 110, 76, 56, 0 },  // 709 fantasma
  { "PUMPKABOO", 711, 40, R_COMUN, 0x6AD3, 49, 66, 70, 51, 0 },  // 710 fantasma
  { "GOURGEIST", 0, 0, R_EVO, 0x6AD3, 65, 90, 122, 84, 0 },  // 711 fantasma
  { "BERGMITE", 713, 37, R_RARO, 0x4DB8, 55, 69, 85, 28, 5 },  // 712 hielo
  { "AVALUGG", 0, 0, R_EVO, 0x4DB8, 95, 117, 184, 28, 5 },  // 713 hielo
  { "NOIBAT", 715, 48, R_RARO, 0x7C78, 40, 30, 35, 55, 4 },  // 714 volador
  { "NOIVERN", 0, 0, R_EVO, 0x7C78, 85, 70, 80, 123, 4 },  // 715 volador
  { "XERNEAS", 0, 0, R_LEGENDARIO, 0xD435, 126, 131, 95, 99, 0 },  // 716 hada
  { "YVELTAL", 0, 0, R_LEGENDARIO, 0x5A6E, 126, 131, 95, 99, 0 },  // 717 siniestro
  { "ZYGARDE", 0, 0, R_LEGENDARIO, 0x5A98, 108, 100, 121, 95, 1 },  // 718 dragon
  { "DIANCIE", 0, 0, R_LEGENDARIO, 0x9407, 50, 100, 150, 50, 4 },  // 719 roca
  { "HOOPA", 0, 0, R_LEGENDARIO, 0xD28F, 80, 110, 60, 70, 0 },  // 720 psiquico
  { "VOLCANION", 0, 0, R_LEGENDARIO, 0xEA87, 80, 110, 120, 70, 3 },  // 721 fuego
  { "ROWLET", 723, 17, R_RARO, 0x3C49, 68, 55, 55, 42, 2 },  // 722 planta
  { "DARTRIX", 724, 34, R_EVO, 0x3C49, 78, 75, 75, 52, 2 },  // 723 planta
  { "DECIDUEYE", 0, 0, R_EVO, 0x3C49, 78, 107, 75, 70, 2 },  // 724 planta
  { "LITTEN", 726, 17, R_RARO, 0xEA87, 45, 65, 40, 70, 3 },  // 725 fuego
  { "TORRACAT", 727, 34, R_EVO, 0xEA87, 65, 85, 50, 90, 3 },  // 726 fuego
  { "INCINEROAR", 0, 0, R_EVO, 0xEA87, 95, 115, 90, 60, 3 },  // 727 fuego
  { "POPPLIO", 729, 17, R_RARO, 0x4C98, 50, 54, 54, 40, 1 },  // 728 agua
  { "BRIONNE", 730, 34, R_EVO, 0x4C98, 60, 69, 69, 50, 1 },  // 729 agua
  { "PRIMARINA", 0, 0, R_EVO, 0x4C98, 80, 74, 74, 60, 1 },  // 730 agua
  { "PIKIPEK", 733, 28, R_COMUN, 0x8C4D, 35, 75, 30, 65, 0 },  // 731 normal
  { "TRUMBEAK", 0, 0, R_NONE, 0x8C4D, 55, 85, 50, 75, 0 },  // 732 normal
  { "TOUCANNON", 0, 0, R_EVO, 0x8C4D, 80, 120, 75, 60, 0 },  // 733 normal
  { "YUNGOOS", 0, 0, R_COMUN, 0x8C4D, 48, 70, 30, 45, 0 },  // 734 normal
  { "GUMSHOOS", 0, 0, R_NONE, 0x8C4D, 88, 110, 60, 45, 0 },  // 735 normal
  { "GRUBBIN", 737, 20, R_COMUN, 0x7CC4, 47, 62, 45, 46, 2 },  // 736 bicho
  { "CHARJABUG", 738, 25, R_EVO, 0x7CC4, 57, 82, 95, 36, 2 },  // 737 bicho
  { "VIKAVOLT", 0, 0, R_EVO, 0x7CC4, 77, 70, 90, 43, 2 },  // 738 bicho
  { "CRABRAWLER", 740, 25, R_COMUN, 0xA2A5, 47, 82, 57, 63, 0 },  // 739 lucha
  { "CRABOMINABLE", 0, 0, R_EVO, 0xA2A5, 97, 132, 77, 43, 0 },  // 740 lucha
  { "ORICORIO", 0, 0, R_COMUN, 0xEA87, 75, 70, 70, 93, 3 },  // 741 fuego
  { "CUTIEFLY", 743, 25, R_COMUN, 0x7CC4, 40, 45, 40, 84, 2 },  // 742 bicho
  { "RIBOMBEE", 0, 0, R_EVO, 0x7CC4, 60, 55, 60, 124, 2 },  // 743 bicho
  { "ROCKRUFF", 745, 25, R_COMUN, 0x9407, 45, 65, 40, 60, 4 },  // 744 roca
  { "LYCANROC", 0, 0, R_EVO, 0x9407, 75, 115, 65, 112, 4 },  // 745 roca
  { "WISHIWASHI", 0, 0, R_COMUN, 0x4C98, 45, 20, 20, 40, 1 },  // 746 agua
  { "MAREANIE", 748, 38, R_COMUN, 0x8A73, 50, 53, 62, 45, 0 },  // 747 veneno
  { "TOXAPEX", 0, 0, R_EVO, 0x8A73, 50, 63, 152, 35, 0 },  // 748 veneno
  { "MUDBRAY", 750, 30, R_COMUN, 0xB447, 70, 100, 70, 45, 4 },  // 749 tierra
  { "MUDSDALE", 0, 0, R_EVO, 0xB447, 100, 125, 100, 35, 4 },  // 750 tierra
  { "DEWPIDER", 752, 22, R_COMUN, 0x4C98, 38, 40, 52, 27, 1 },  // 751 agua
  { "ARAQUANID", 0, 0, R_EVO, 0x4C98, 68, 70, 92, 42, 1 },  // 752 agua
  { "FOMANTIS", 754, 34, R_COMUN, 0x3C49, 40, 55, 35, 35, 2 },  // 753 planta
  { "LURANTIS", 0, 0, R_EVO, 0x3C49, 70, 105, 90, 45, 2 },  // 754 planta
  { "MORELULL", 0, 0, R_COMUN, 0x3C49, 40, 35, 55, 15, 2 },  // 755 planta
  { "SHIINOTIC", 0, 0, R_NONE, 0x3C49, 60, 45, 80, 30, 2 },  // 756 planta
  { "SALANDIT", 758, 33, R_COMUN, 0x8A73, 48, 44, 40, 77, 0 },  // 757 veneno
  { "SALAZZLE", 0, 0, R_EVO, 0x8A73, 68, 64, 60, 117, 0 },  // 758 veneno
  { "STUFFUL", 760, 27, R_COMUN, 0x8C4D, 70, 75, 50, 50, 0 },  // 759 normal
  { "BEWEAR", 0, 0, R_EVO, 0x8C4D, 120, 125, 80, 60, 0 },  // 760 normal
  { "BOUNSWEET", 762, 18, R_RARO, 0x3C49, 42, 30, 38, 32, 2 },  // 761 planta
  { "STEENEE", 763, 25, R_EVO, 0x3C49, 52, 40, 48, 62, 2 },  // 762 planta
  { "TSAREENA", 0, 0, R_EVO, 0x3C49, 72, 120, 98, 72, 2 },  // 763 planta
  { "COMFEY", 0, 0, R_COMUN, 0xD435, 51, 52, 90, 100, 0 },  // 764 hada
  { "ORANGURU", 0, 0, R_NONE, 0x8C4D, 90, 60, 80, 60, 0 },  // 765 normal
  { "PASSIMIAN", 0, 0, R_COMUN, 0xA2A5, 100, 120, 90, 80, 0 },  // 766 lucha
  { "WIMPOD", 768, 30, R_RARO, 0x7CC4, 25, 35, 40, 80, 2 },  // 767 bicho
  { "GOLISOPOD", 0, 0, R_EVO, 0x7CC4, 75, 125, 140, 40, 2 },  // 768 bicho
  { "SANDYGAST", 770, 42, R_COMUN, 0x6AD3, 55, 55, 80, 15, 0 },  // 769 fantasma
  { "PALOSSAND", 0, 0, R_EVO, 0x6AD3, 85, 75, 110, 35, 0 },  // 770 fantasma
  { "PYUKUMUKU", 0, 0, R_COMUN, 0x4C98, 55, 60, 130, 5, 1 },  // 771 agua
  { "TYPE: NULL", 773, 25, R_LEGENDARIO, 0x8C4D, 95, 95, 95, 59, 0 },  // 772 normal
  { "SILVALLY", 0, 0, R_EVO, 0x8C4D, 95, 95, 95, 95, 0 },  // 773 normal
  { "MINIOR", 0, 0, R_COMUN, 0x9407, 60, 60, 100, 60, 4 },  // 774 roca
  { "KOMALA", 0, 0, R_COMUN, 0x8C4D, 65, 115, 65, 65, 0 },  // 775 normal
  { "TURTONATOR", 0, 0, R_COMUN, 0xEA87, 60, 78, 135, 36, 3 },  // 776 fuego
  { "TOGEDEMARU", 0, 0, R_COMUN, 0xBCA1, 65, 98, 63, 96, 0 },  // 777 electrico
  { "MIMIKYU", 0, 0, R_COMUN, 0x6AD3, 55, 90, 80, 96, 0 },  // 778 fantasma
  { "BRUXISH", 0, 0, R_COMUN, 0x4C98, 68, 105, 70, 92, 1 },  // 779 agua
  { "DRAMPA", 0, 0, R_COMUN, 0x8C4D, 78, 60, 85, 36, 0 },  // 780 normal
  { "DHELMISE", 0, 0, R_RARO, 0x6AD3, 70, 131, 100, 40, 0 },  // 781 fantasma
  { "JANGMO-O", 783, 35, R_RARO, 0x5A98, 45, 55, 65, 45, 1 },  // 782 dragon
  { "HAKAMO-O", 784, 45, R_EVO, 0x5A98, 55, 75, 90, 65, 1 },  // 783 dragon
  { "KOMMO-O", 0, 0, R_EVO, 0x5A98, 75, 110, 125, 85, 1 },  // 784 dragon
  { "TAPU KOKO", 0, 0, R_LEGENDARIO, 0xBCA1, 70, 115, 85, 130, 0 },  // 785 electrico
  { "TAPU LELE", 0, 0, R_LEGENDARIO, 0xD28F, 70, 85, 75, 95, 0 },  // 786 psiquico
  { "TAPU BULU", 0, 0, R_LEGENDARIO, 0x3C49, 70, 130, 115, 75, 2 },  // 787 planta
  { "TAPU FINI", 0, 0, R_LEGENDARIO, 0x4C98, 70, 75, 115, 85, 1 },  // 788 agua
  { "COSMOG", 790, 43, R_LEGENDARIO, 0xD28F, 43, 29, 31, 37, 0 },  // 789 psiquico
  { "COSMOEM", 791, 53, R_EVO, 0xD28F, 43, 29, 131, 37, 0 },  // 790 psiquico
  { "SOLGALEO", 0, 0, R_EVO, 0xD28F, 137, 137, 107, 97, 0 },  // 791 psiquico
  { "LUNALA", 0, 0, R_EVO, 0xD28F, 137, 113, 89, 97, 0 },  // 792 psiquico
  { "NIHILEGO", 0, 0, R_RARO, 0x9407, 109, 53, 47, 103, 4 },  // 793 roca
  { "BUZZWOLE", 0, 0, R_RARO, 0x7CC4, 107, 139, 139, 79, 2 },  // 794 bicho
  { "PHEROMOSA", 0, 0, R_RARO, 0x7CC4, 71, 137, 37, 151, 2 },  // 795 bicho
  { "XURKITREE", 0, 0, R_RARO, 0xBCA1, 83, 89, 71, 83, 0 },  // 796 electrico
  { "CELESTEELA", 0, 0, R_RARO, 0x7C73, 97, 101, 103, 61, 4 },  // 797 acero
  { "KARTANA", 0, 0, R_RARO, 0x3C49, 59, 181, 131, 109, 2 },  // 798 planta
  { "GUZZLORD", 0, 0, R_RARO, 0x5A6E, 223, 101, 53, 43, 0 },  // 799 siniestro
  { "NECROZMA", 0, 0, R_LEGENDARIO, 0xD28F, 97, 107, 101, 79, 0 },  // 800 psiquico
  { "MAGEARNA", 0, 0, R_LEGENDARIO, 0x7C73, 80, 95, 115, 65, 4 },  // 801 acero
  { "MARSHADOW", 0, 0, R_LEGENDARIO, 0xA2A5, 90, 125, 80, 125, 0 },  // 802 lucha
  { "POIPOLE", 804, 25, R_RARO, 0x8A73, 67, 73, 67, 73, 0 },  // 803 veneno
  { "NAGANADEL", 0, 0, R_EVO, 0x8A73, 73, 73, 73, 121, 0 },  // 804 veneno
  { "STAKATAKA", 0, 0, R_RARO, 0x9407, 61, 131, 211, 13, 4 },  // 805 roca
  { "BLACEPHALON", 0, 0, R_RARO, 0xEA87, 53, 127, 53, 107, 3 },  // 806 fuego
  { "ZERAORA", 0, 0, R_LEGENDARIO, 0xBCA1, 88, 112, 75, 143, 0 },  // 807 electrico
  { "MELTAN", 809, 25, R_LEGENDARIO, 0x7C73, 46, 65, 65, 34, 4 },  // 808 acero
  { "MELMETAL", 0, 0, R_EVO, 0x7C73, 135, 143, 143, 34, 4 },  // 809 acero
  { "GROOKEY", 811, 16, R_RARO, 0x3C49, 50, 65, 50, 65, 2 },  // 810 planta
  { "THWACKEY", 812, 35, R_EVO, 0x3C49, 70, 85, 70, 80, 2 },  // 811 planta
  { "RILLABOOM", 0, 0, R_EVO, 0x3C49, 100, 125, 90, 85, 2 },  // 812 planta
  { "SCORBUNNY", 814, 16, R_RARO, 0xEA87, 50, 71, 40, 69, 3 },  // 813 fuego
  { "RABOOT", 815, 35, R_EVO, 0xEA87, 65, 86, 60, 94, 3 },  // 814 fuego
  { "CINDERACE", 0, 0, R_EVO, 0xEA87, 80, 116, 75, 119, 3 },  // 815 fuego
  { "SOBBLE", 817, 16, R_RARO, 0x4C98, 50, 40, 40, 70, 1 },  // 816 agua
  { "DRIZZILE", 818, 35, R_EVO, 0x4C98, 65, 60, 55, 90, 1 },  // 817 agua
  { "INTELEON", 0, 0, R_EVO, 0x4C98, 70, 85, 65, 120, 1 },  // 818 agua
  { "SKWOVET", 820, 24, R_COMUN, 0x8C4D, 70, 55, 55, 25, 0 },  // 819 normal
  { "GREEDENT", 0, 0, R_EVO, 0x8C4D, 120, 95, 95, 20, 0 },  // 820 normal
  { "ROOKIDEE", 822, 18, R_COMUN, 0x7C78, 38, 47, 35, 57, 4 },  // 821 volador
  { "CORVISQUIRE", 823, 38, R_EVO, 0x7C78, 68, 67, 55, 77, 4 },  // 822 volador
  { "CORVIKNIGHT", 0, 0, R_EVO, 0x7C78, 98, 87, 105, 67, 4 },  // 823 volador
  { "BLIPBUG", 825, 10, R_COMUN, 0x7CC4, 25, 20, 20, 45, 2 },  // 824 bicho
  { "DOTTLER", 826, 30, R_EVO, 0x7CC4, 50, 35, 80, 30, 2 },  // 825 bicho
  { "ORBEETLE", 0, 0, R_EVO, 0x7CC4, 60, 45, 110, 90, 2 },  // 826 bicho
  { "NICKIT", 828, 18, R_COMUN, 0x5A6E, 40, 28, 28, 50, 0 },  // 827 siniestro
  { "THIEVUL", 0, 0, R_EVO, 0x5A6E, 70, 58, 58, 90, 0 },  // 828 siniestro
  { "GOSSIFLEUR", 830, 20, R_COMUN, 0x3C49, 40, 40, 60, 10, 2 },  // 829 planta
  { "ELDEGOSS", 0, 0, R_EVO, 0x3C49, 60, 50, 90, 60, 2 },  // 830 planta
  { "WOOLOO", 832, 24, R_COMUN, 0x8C4D, 42, 40, 55, 48, 0 },  // 831 normal
  { "DUBWOOL", 0, 0, R_EVO, 0x8C4D, 72, 80, 100, 88, 0 },  // 832 normal
  { "CHEWTLE", 834, 22, R_COMUN, 0x4C98, 50, 64, 50, 44, 1 },  // 833 agua
  { "DREDNAW", 0, 0, R_EVO, 0x4C98, 90, 115, 90, 74, 1 },  // 834 agua
  { "YAMPER", 836, 25, R_COMUN, 0xBCA1, 59, 45, 50, 26, 0 },  // 835 electrico
  { "BOLTUND", 0, 0, R_EVO, 0xBCA1, 69, 90, 60, 121, 0 },  // 836 electrico
  { "ROLYCOLY", 0, 0, R_COMUN, 0x9407, 30, 40, 50, 30, 4 },  // 837 roca
  { "CARKOL", 0, 0, R_NONE, 0x9407, 80, 60, 90, 50, 4 },  // 838 roca
  { "COALOSSAL", 0, 0, R_NONE, 0x9407, 110, 80, 120, 30, 4 },  // 839 roca
  { "APPLIN", 841, 30, R_RARO, 0x3C49, 40, 40, 80, 20, 2 },  // 840 planta
  { "FLAPPLE", 0, 0, R_EVO, 0x3C49, 70, 110, 80, 70, 2 },  // 841 planta
  { "APPLETUN", 0, 0, R_EVO, 0x3C49, 110, 85, 80, 30, 2 },  // 842 planta
  { "SILICOBRA", 844, 36, R_RARO, 0xB447, 52, 57, 75, 46, 4 },  // 843 tierra
  { "SANDACONDA", 0, 0, R_EVO, 0xB447, 72, 107, 125, 71, 4 },  // 844 tierra
  { "CRAMORANT", 0, 0, R_COMUN, 0x7C78, 70, 85, 55, 85, 4 },  // 845 volador
  { "ARROKUDA", 847, 26, R_COMUN, 0x4C98, 41, 63, 40, 66, 1 },  // 846 agua
  { "BARRASKEWDA", 0, 0, R_EVO, 0x4C98, 61, 123, 60, 136, 1 },  // 847 agua
  { "TOXEL", 849, 30, R_COMUN, 0xBCA1, 40, 38, 35, 40, 0 },  // 848 electrico
  { "TOXTRICITY", 0, 0, R_EVO, 0xBCA1, 75, 98, 70, 75, 0 },  // 849 electrico
  { "SIZZLIPEDE", 851, 28, R_RARO, 0xEA87, 50, 65, 45, 45, 3 },  // 850 fuego
  { "CENTISKORCH", 0, 0, R_EVO, 0xEA87, 100, 115, 65, 65, 3 },  // 851 fuego
  { "CLOBBOPUS", 853, 25, R_COMUN, 0xA2A5, 50, 68, 60, 32, 0 },  // 852 lucha
  { "GRAPPLOCT", 0, 0, R_EVO, 0xA2A5, 80, 118, 90, 42, 0 },  // 853 lucha
  { "SINISTEA", 855, 30, R_COMUN, 0x6AD3, 40, 45, 45, 50, 0 },  // 854 fantasma
  { "POLTEAGEIST", 0, 0, R_EVO, 0x6AD3, 60, 65, 65, 70, 0 },  // 855 fantasma
  { "HATENNA", 857, 32, R_RARO, 0xD28F, 42, 30, 45, 39, 0 },  // 856 psiquico
  { "HATTREM", 858, 42, R_EVO, 0xD28F, 57, 40, 65, 49, 0 },  // 857 psiquico
  { "HATTERENE", 0, 0, R_EVO, 0xD28F, 57, 90, 95, 29, 0 },  // 858 psiquico
  { "IMPIDIMP", 860, 32, R_RARO, 0x5A6E, 45, 45, 30, 50, 0 },  // 859 siniestro
  { "MORGREM", 861, 42, R_EVO, 0x5A6E, 65, 60, 45, 70, 0 },  // 860 siniestro
  { "GRIMMSNARL", 0, 0, R_EVO, 0x5A6E, 95, 120, 65, 60, 0 },  // 861 siniestro
  { "OBSTAGOON", 0, 0, R_EVO, 0x5A6E, 93, 90, 101, 95, 0 },  // 862 siniestro
  { "PERRSERKER", 0, 0, R_EVO, 0x7C73, 70, 110, 100, 50, 4 },  // 863 acero
  { "CURSOLA", 0, 0, R_EVO, 0x6AD3, 60, 95, 50, 30, 0 },  // 864 fantasma
  { "SIRFETCH'D", 0, 0, R_EVO, 0xA2A5, 62, 135, 95, 65, 0 },  // 865 lucha
  { "MR. RIME", 0, 0, R_NONE, 0x4DB8, 80, 85, 75, 70, 5 },  // 866 hielo
  { "RUNERIGUS", 0, 0, R_EVO, 0xB447, 58, 95, 145, 30, 4 },  // 867 tierra
  { "MILCERY", 869, 25, R_COMUN, 0xD435, 45, 40, 40, 34, 0 },  // 868 hada
  { "ALCREMIE", 0, 0, R_EVO, 0xD435, 65, 60, 75, 64, 0 },  // 869 hada
  { "FALINKS", 0, 0, R_NONE, 0xA2A5, 65, 100, 100, 75, 0 },  // 870 lucha
  { "PINCURCHIN", 0, 0, R_COMUN, 0xBCA1, 48, 101, 95, 15, 0 },  // 871 electrico
  { "SNOM", 873, 25, R_COMUN, 0x4DB8, 30, 25, 35, 20, 5 },  // 872 hielo
  { "FROSMOTH", 0, 0, R_EVO, 0x4DB8, 70, 65, 60, 65, 5 },  // 873 hielo
  { "STONJOURNER", 0, 0, R_COMUN, 0x9407, 100, 125, 135, 70, 4 },  // 874 roca
  { "EISCUE", 0, 0, R_COMUN, 0x4DB8, 75, 80, 110, 50, 5 },  // 875 hielo
  { "INDEEDEE", 0, 0, R_COMUN, 0xD28F, 60, 65, 55, 95, 0 },  // 876 psiquico
  { "MORPEKO", 0, 0, R_COMUN, 0xBCA1, 58, 95, 58, 97, 0 },  // 877 electrico
  { "CUFANT", 879, 34, R_COMUN, 0x7C73, 72, 80, 49, 40, 4 },  // 878 acero
  { "COPPERAJAH", 0, 0, R_EVO, 0x7C73, 122, 130, 69, 30, 4 },  // 879 acero
  { "DRACOZOLT", 0, 0, R_COMUN, 0xBCA1, 90, 100, 90, 75, 0 },  // 880 electrico
  { "ARCTOZOLT", 0, 0, R_COMUN, 0xBCA1, 90, 100, 90, 55, 0 },  // 881 electrico
  { "DRACOVISH", 0, 0, R_COMUN, 0x4C98, 90, 90, 100, 75, 1 },  // 882 agua
  { "ARCTOVISH", 0, 0, R_COMUN, 0x4C98, 90, 90, 100, 55, 1 },  // 883 agua
  { "DURALUDON", 1018, 30, R_RARO, 0x7C73, 70, 95, 115, 85, 4 },  // 884 acero
  { "DREEPY", 886, 50, R_RARO, 0x5A98, 28, 60, 30, 82, 1 },  // 885 dragon
  { "DRAKLOAK", 887, 60, R_EVO, 0x5A98, 68, 80, 50, 102, 1 },  // 886 dragon
  { "DRAGAPULT", 0, 0, R_EVO, 0x5A98, 88, 120, 75, 142, 1 },  // 887 dragon
  { "ZACIAN", 0, 0, R_LEGENDARIO, 0xD435, 92, 120, 115, 138, 0 },  // 888 hada
  { "ZAMAZENTA", 0, 0, R_LEGENDARIO, 0xA2A5, 92, 120, 115, 138, 0 },  // 889 lucha
  { "ETERNATUS", 0, 0, R_LEGENDARIO, 0x8A73, 140, 85, 95, 130, 0 },  // 890 veneno
  { "KUBFU", 892, 25, R_LEGENDARIO, 0xA2A5, 60, 90, 60, 72, 0 },  // 891 lucha
  { "URSHIFU", 0, 0, R_EVO, 0xA2A5, 100, 130, 100, 97, 0 },  // 892 lucha
  { "ZARUDE", 0, 0, R_NONE, 0x5A6E, 105, 120, 105, 105, 0 },  // 893 siniestro
  { "REGIELEKI", 0, 0, R_LEGENDARIO, 0xBCA1, 80, 100, 50, 200, 0 },  // 894 electrico
  { "REGIDRAGO", 0, 0, R_LEGENDARIO, 0x5A98, 200, 100, 50, 80, 1 },  // 895 dragon
  { "GLASTRIER", 0, 0, R_LEGENDARIO, 0x4DB8, 100, 145, 130, 30, 5 },  // 896 hielo
  { "SPECTRIER", 0, 0, R_LEGENDARIO, 0x6AD3, 100, 65, 60, 130, 0 },  // 897 fantasma
  { "CALYREX", 0, 0, R_LEGENDARIO, 0xD28F, 100, 80, 80, 80, 0 },  // 898 psiquico
  { "WYRDEER", 0, 0, R_EVO, 0x8C4D, 103, 105, 72, 65, 0 },  // 899 normal
  { "KLEAVOR", 0, 0, R_EVO, 0x7CC4, 70, 135, 95, 85, 2 },  // 900 bicho
  { "URSALUNA", 0, 0, R_EVO, 0xB447, 130, 140, 105, 50, 4 },  // 901 tierra
  { "BASCULEGION", 0, 0, R_EVO, 0x4C98, 120, 112, 65, 78, 1 },  // 902 agua
  { "SNEASLER", 0, 0, R_EVO, 0xA2A5, 80, 130, 60, 120, 0 },  // 903 lucha
  { "OVERQWIL", 0, 0, R_EVO, 0x5A6E, 85, 115, 95, 85, 0 },  // 904 siniestro
  { "ENAMORUS", 0, 0, R_LEGENDARIO, 0xD435, 74, 115, 70, 106, 0 },  // 905 hada
  { "SPRIGATITO", 907, 16, R_RARO, 0x3C49, 40, 61, 54, 65, 2 },  // 906 planta
  { "FLORAGATO", 908, 36, R_EVO, 0x3C49, 61, 80, 63, 83, 2 },  // 907 planta
  { "MEOWSCARADA", 0, 0, R_EVO, 0x3C49, 76, 110, 70, 123, 2 },  // 908 planta
  { "FUECOCO", 910, 16, R_RARO, 0xEA87, 67, 45, 59, 36, 3 },  // 909 fuego
  { "CROCALOR", 911, 36, R_EVO, 0xEA87, 81, 55, 78, 49, 3 },  // 910 fuego
  { "SKELEDIRGE", 0, 0, R_EVO, 0xEA87, 104, 75, 100, 66, 3 },  // 911 fuego
  { "QUAXLY", 913, 16, R_RARO, 0x4C98, 55, 65, 45, 50, 1 },  // 912 agua
  { "QUAXWELL", 914, 36, R_EVO, 0x4C98, 70, 85, 65, 65, 1 },  // 913 agua
  { "QUAQUAVAL", 0, 0, R_EVO, 0x4C98, 85, 120, 80, 85, 1 },  // 914 agua
  { "LECHONK", 916, 18, R_COMUN, 0x8C4D, 54, 45, 40, 35, 0 },  // 915 normal
  { "OINKOLOGNE", 0, 0, R_EVO, 0x8C4D, 110, 100, 75, 65, 0 },  // 916 normal
  { "TAROUNTULA", 918, 15, R_COMUN, 0x7CC4, 35, 41, 45, 20, 2 },  // 917 bicho
  { "SPIDOPS", 0, 0, R_EVO, 0x7CC4, 60, 79, 92, 35, 2 },  // 918 bicho
  { "NYMBLE", 920, 24, R_COMUN, 0x7CC4, 33, 46, 40, 45, 2 },  // 919 bicho
  { "LOKIX", 0, 0, R_EVO, 0x7CC4, 71, 102, 78, 92, 2 },  // 920 bicho
  { "PAWMI", 922, 18, R_COMUN, 0xBCA1, 45, 50, 20, 60, 0 },  // 921 electrico
  { "PAWMO", 923, 25, R_EVO, 0xBCA1, 60, 75, 40, 85, 0 },  // 922 electrico
  { "PAWMOT", 0, 0, R_EVO, 0xBCA1, 70, 115, 70, 105, 0 },  // 923 electrico
  { "TANDEMAUS", 925, 25, R_COMUN, 0x8C4D, 50, 50, 45, 75, 0 },  // 924 normal
  { "MAUSHOLD", 0, 0, R_EVO, 0x8C4D, 74, 75, 70, 111, 0 },  // 925 normal
  { "FIDOUGH", 927, 26, R_COMUN, 0xD435, 37, 55, 70, 65, 0 },  // 926 hada
  { "DACHSBUN", 0, 0, R_EVO, 0xD435, 57, 80, 115, 95, 0 },  // 927 hada
  { "SMOLIV", 929, 25, R_RARO, 0x3C49, 41, 35, 45, 30, 2 },  // 928 planta
  { "DOLLIV", 930, 35, R_EVO, 0x3C49, 52, 53, 60, 33, 2 },  // 929 planta
  { "ARBOLIVA", 0, 0, R_EVO, 0x3C49, 78, 69, 90, 39, 2 },  // 930 planta
  { "SQUAWKABILLY", 0, 0, R_NONE, 0x8C4D, 82, 96, 51, 92, 0 },  // 931 normal
  { "NACLI", 933, 24, R_COMUN, 0x9407, 55, 55, 75, 25, 4 },  // 932 roca
  { "NACLSTACK", 934, 38, R_EVO, 0x9407, 60, 60, 100, 35, 4 },  // 933 roca
  { "GARGANACL", 0, 0, R_EVO, 0x9407, 100, 100, 130, 35, 4 },  // 934 roca
  { "CHARCADET", 936, 30, R_RARO, 0xEA87, 40, 50, 40, 35, 3 },  // 935 fuego
  { "ARMAROUGE", 0, 0, R_EVO, 0xEA87, 85, 60, 100, 75, 3 },  // 936 fuego
  { "CERULEDGE", 0, 0, R_EVO, 0xEA87, 75, 125, 80, 85, 3 },  // 937 fuego
  { "TADBULB", 939, 30, R_COMUN, 0xBCA1, 61, 31, 41, 45, 0 },  // 938 electrico
  { "BELLIBOLT", 0, 0, R_EVO, 0xBCA1, 109, 64, 91, 45, 0 },  // 939 electrico
  { "WATTREL", 941, 25, R_COMUN, 0xBCA1, 40, 40, 35, 70, 0 },  // 940 electrico
  { "KILOWATTREL", 0, 0, R_EVO, 0xBCA1, 70, 70, 60, 125, 0 },  // 941 electrico
  { "MASCHIFF", 0, 0, R_NONE, 0x5A6E, 60, 78, 60, 51, 0 },  // 942 siniestro
  { "MABOSSTIFF", 0, 0, R_NONE, 0x5A6E, 80, 120, 90, 85, 0 },  // 943 siniestro
  { "SHROODLE", 0, 0, R_NONE, 0x8A73, 40, 65, 35, 75, 0 },  // 944 veneno
  { "GRAFAIAI", 0, 0, R_COMUN, 0x8A73, 63, 95, 65, 110, 0 },  // 945 veneno
  { "BRAMBLIN", 0, 0, R_COMUN, 0x3C49, 40, 65, 30, 60, 2 },  // 946 planta
  { "BRAMBLEGHAST", 0, 0, R_NONE, 0x3C49, 55, 115, 70, 90, 2 },  // 947 planta
  { "TOEDSCOOL", 0, 0, R_COMUN, 0xB447, 40, 40, 35, 70, 4 },  // 948 tierra
  { "TOEDSCRUEL", 0, 0, R_NONE, 0xB447, 80, 70, 65, 100, 4 },  // 949 tierra
  { "KLAWF", 0, 0, R_NONE, 0x9407, 70, 100, 115, 75, 4 },  // 950 roca
  { "CAPSAKID", 952, 30, R_COMUN, 0x3C49, 50, 62, 40, 50, 2 },  // 951 planta
  { "SCOVILLAIN", 0, 0, R_EVO, 0x3C49, 65, 108, 65, 75, 2 },  // 952 planta
  { "RELLOR", 0, 0, R_COMUN, 0x7CC4, 41, 50, 60, 30, 2 },  // 953 bicho
  { "RABSCA", 0, 0, R_NONE, 0x7CC4, 75, 50, 85, 45, 2 },  // 954 bicho
  { "FLITTLE", 0, 0, R_COMUN, 0xD28F, 30, 35, 30, 75, 0 },  // 955 psiquico
  { "ESPATHRA", 0, 0, R_NONE, 0xD28F, 95, 60, 60, 105, 0 },  // 956 psiquico
  { "TINKATINK", 958, 24, R_COMUN, 0xD435, 50, 45, 45, 58, 0 },  // 957 hada
  { "TINKATUFF", 959, 38, R_EVO, 0xD435, 65, 55, 55, 78, 0 },  // 958 hada
  { "TINKATON", 0, 0, R_EVO, 0xD435, 85, 75, 77, 94, 0 },  // 959 hada
  { "WIGLETT", 961, 26, R_COMUN, 0x4C98, 10, 55, 25, 95, 1 },  // 960 agua
  { "WUGTRIO", 0, 0, R_EVO, 0x4C98, 35, 100, 50, 120, 1 },  // 961 agua
  { "BOMBIRDIER", 0, 0, R_COMUN, 0x7C78, 70, 103, 85, 82, 4 },  // 962 volador
  { "FINIZEN", 964, 38, R_COMUN, 0x4C98, 70, 45, 40, 75, 1 },  // 963 agua
  { "PALAFIN", 0, 0, R_EVO, 0x4C98, 100, 70, 72, 100, 1 },  // 964 agua
  { "VAROOM", 966, 40, R_COMUN, 0x7C73, 45, 70, 63, 47, 4 },  // 965 acero
  { "REVAVROOM", 0, 0, R_EVO, 0x7C73, 80, 119, 90, 90, 4 },  // 966 acero
  { "CYCLIZAR", 0, 0, R_COMUN, 0x5A98, 70, 95, 65, 121, 1 },  // 967 dragon
  { "ORTHWORM", 0, 0, R_COMUN, 0x7C73, 70, 85, 145, 65, 4 },  // 968 acero
  { "GLIMMET", 970, 35, R_RARO, 0x9407, 48, 35, 42, 60, 4 },  // 969 roca
  { "GLIMMORA", 0, 0, R_EVO, 0x9407, 83, 55, 90, 86, 4 },  // 970 roca
  { "GREAVARD", 972, 30, R_COMUN, 0x6AD3, 50, 61, 60, 34, 0 },  // 971 fantasma
  { "HOUNDSTONE", 0, 0, R_EVO, 0x6AD3, 72, 101, 100, 68, 0 },  // 972 fantasma
  { "FLAMIGO", 0, 0, R_NONE, 0x7C78, 82, 115, 74, 90, 4 },  // 973 volador
  { "CETODDLE", 975, 30, R_RARO, 0x4DB8, 108, 68, 45, 43, 5 },  // 974 hielo
  { "CETITAN", 0, 0, R_EVO, 0x4DB8, 170, 113, 65, 73, 5 },  // 975 hielo
  { "VELUZA", 0, 0, R_COMUN, 0x4C98, 90, 102, 73, 70, 1 },  // 976 agua
  { "DONDOZO", 0, 0, R_RARO, 0x4C98, 150, 100, 115, 35, 1 },  // 977 agua
  { "TATSUGIRI", 0, 0, R_COMUN, 0x5A98, 68, 50, 60, 82, 1 },  // 978 dragon
  { "ANNIHILAPE", 0, 0, R_EVO, 0xA2A5, 110, 115, 80, 90, 0 },  // 979 lucha
  { "CLODSIRE", 0, 0, R_EVO, 0x8A73, 130, 75, 60, 20, 0 },  // 980 veneno
  { "FARIGIRAF", 0, 0, R_EVO, 0x8C4D, 120, 90, 70, 60, 0 },  // 981 normal
  { "DUDUNSPARCE", 0, 0, R_EVO, 0x8C4D, 125, 100, 80, 55, 0 },  // 982 normal
  { "KINGAMBIT", 0, 0, R_EVO, 0x5A6E, 100, 135, 120, 50, 0 },  // 983 siniestro
  { "GREAT TUSK", 0, 0, R_RARO, 0xB447, 115, 131, 131, 87, 4 },  // 984 tierra
  { "SCREAM TAIL", 0, 0, R_RARO, 0xD435, 115, 65, 99, 111, 0 },  // 985 hada
  { "BRUTE BONNET", 0, 0, R_NONE, 0x3C49, 111, 127, 99, 55, 2 },  // 986 planta
  { "FLUTTER MANE", 0, 0, R_RARO, 0x6AD3, 55, 55, 55, 135, 0 },  // 987 fantasma
  { "SLITHER WING", 0, 0, R_RARO, 0x7CC4, 85, 135, 79, 81, 2 },  // 988 bicho
  { "SANDY SHOCKS", 0, 0, R_RARO, 0xBCA1, 85, 81, 97, 101, 0 },  // 989 electrico
  { "IRON TREADS", 0, 0, R_RARO, 0xB447, 90, 112, 120, 106, 4 },  // 990 tierra
  { "IRON BUNDLE", 0, 0, R_RARO, 0x4DB8, 56, 80, 114, 136, 5 },  // 991 hielo
  { "IRON HANDS", 0, 0, R_RARO, 0xA2A5, 154, 140, 108, 50, 0 },  // 992 lucha
  { "IRON JUGULIS", 0, 0, R_NONE, 0x5A6E, 94, 80, 86, 108, 0 },  // 993 siniestro
  { "IRON MOTH", 0, 0, R_RARO, 0xEA87, 80, 70, 60, 110, 3 },  // 994 fuego
  { "IRON THORNS", 0, 0, R_RARO, 0x9407, 100, 134, 110, 72, 4 },  // 995 roca
  { "FRIGIBAX", 997, 35, R_RARO, 0x5A98, 65, 75, 45, 55, 1 },  // 996 dragon
  { "ARCTIBAX", 998, 54, R_EVO, 0x5A98, 90, 95, 66, 62, 1 },  // 997 dragon
  { "BAXCALIBUR", 0, 0, R_EVO, 0x5A98, 115, 145, 92, 87, 1 },  // 998 dragon
  { "GIMMIGHOUL", 0, 0, R_NONE, 0x6AD3, 45, 30, 70, 10, 0 },  // 999 fantasma
  { "GHOLDENGO", 0, 0, R_RARO, 0x7C73, 87, 60, 95, 84, 4 },  // 1000 acero
  { "WO-CHIEN", 0, 0, R_NONE, 0x5A6E, 85, 85, 100, 70, 0 },  // 1001 siniestro
  { "CHIEN-PAO", 0, 0, R_NONE, 0x5A6E, 80, 120, 80, 135, 0 },  // 1002 siniestro
  { "TING-LU", 0, 0, R_LEGENDARIO, 0x5A6E, 155, 110, 125, 45, 0 },  // 1003 siniestro
  { "CHI-YU", 0, 0, R_LEGENDARIO, 0x5A6E, 55, 80, 80, 100, 0 },  // 1004 siniestro
  { "ROARING MOON", 0, 0, R_RARO, 0x5A98, 105, 139, 71, 119, 1 },  // 1005 dragon
  { "IRON VALIANT", 0, 0, R_RARO, 0xD435, 74, 130, 90, 116, 0 },  // 1006 hada
  { "KORAIDON", 0, 0, R_LEGENDARIO, 0xA2A5, 100, 135, 115, 135, 0 },  // 1007 lucha
  { "MIRAIDON", 0, 0, R_NONE, 0xBCA1, 100, 85, 100, 135, 0 },  // 1008 electrico
  { "WALKING WAKE", 0, 0, R_RARO, 0x4C98, 99, 83, 91, 109, 1 },  // 1009 agua
  { "IRON LEAVES", 0, 0, R_RARO, 0x3C49, 90, 130, 88, 104, 2 },  // 1010 planta
  { "DIPPLIN", 1019, 35, R_EVO, 0x3C49, 80, 80, 110, 40, 2 },  // 1011 planta
  { "POLTCHAGEIST", 1013, 30, R_COMUN, 0x3C49, 40, 45, 45, 50, 2 },  // 1012 planta
  { "SINISTCHA", 0, 0, R_EVO, 0x3C49, 71, 60, 106, 70, 2 },  // 1013 planta
  { "OKIDOGI", 0, 0, R_NONE, 0x8A73, 88, 128, 115, 80, 0 },  // 1014 veneno
  { "MUNKIDORI", 0, 0, R_LEGENDARIO, 0x8A73, 88, 75, 66, 106, 0 },  // 1015 veneno
  { "FEZANDIPITI", 0, 0, R_LEGENDARIO, 0x8A73, 88, 91, 82, 99, 0 },  // 1016 veneno
  { "OGERPON", 0, 0, R_LEGENDARIO, 0x3C49, 80, 120, 84, 110, 2 },  // 1017 planta
  { "ARCHALUDON", 0, 0, R_EVO, 0x7C73, 90, 105, 130, 85, 4 },  // 1018 acero
  { "HYDRAPPLE", 0, 0, R_EVO, 0x3C49, 106, 80, 110, 44, 2 },  // 1019 planta
  { "GOUGING FIRE", 0, 0, R_RARO, 0xEA87, 105, 115, 121, 91, 3 },  // 1020 fuego
  { "RAGING BOLT", 0, 0, R_RARO, 0xBCA1, 125, 73, 91, 75, 0 },  // 1021 electrico
  { "IRON BOULDER", 0, 0, R_NONE, 0x9407, 90, 120, 80, 124, 4 },  // 1022 roca
  { "IRON CROWN", 0, 0, R_NONE, 0x7C73, 90, 72, 100, 98, 4 },  // 1023 acero
  { "TERAPAGOS", 0, 0, R_LEGENDARIO, 0x8C4D, 90, 65, 85, 60, 0 },  // 1024 normal
  { "PECHARUNT", 0, 0, R_LEGENDARIO, 0x8A73, 88, 88, 160, 88, 0 },  // 1025 veneno
};

// Nombres oficiales en chino simplificado, en UTF-8 (se pintan con la
// fuente U8g2). nullptr = sin nombre propio.
static const char *const DEX_NAME_ZH[DEX_COUNT + 1] = {
  nullptr, "妙蛙种子", "妙蛙草", "妙蛙花",
  "小火龙", "火恐龙", "喷火龙", "杰尼龟",
  "卡咪龟", "水箭龟", "绿毛虫", "铁甲蛹",
  "巴大蝶", "独角虫", "铁壳蛹", "大针蜂",
  "波波", "比比鸟", "大比鸟", "小拉达",
  "拉达", "烈雀", "大嘴雀", "阿柏蛇",
  "阿柏怪", "皮卡丘", "雷丘", "穿山鼠",
  "穿山王", "尼多兰", "尼多娜", "尼多后",
  "尼多朗", "尼多力诺", "尼多王", "皮皮",
  "皮可西", "六尾", "九尾", "胖丁",
  "胖可丁", "超音蝠", "大嘴蝠", "走路草",
  "臭臭花", "霸王花", "派拉斯", "派拉斯特",
  "毛球", "摩鲁蛾", "地鼠", "三地鼠",
  "喵喵", "猫老大", "可达鸭", "哥达鸭",
  "猴怪", "火暴猴", "卡蒂狗", "风速狗",
  "蚊香蝌蚪", "蚊香君", "蚊香泳士", "凯西",
  "勇基拉", "胡地", "腕力", "豪力",
  "怪力", "喇叭芽", "口呆花", "大食花",
  "玛瑙水母", "毒刺水母", "小拳石", "隆隆石",
  "隆隆岩", "小火马", "烈焰马", "呆呆兽",
  "呆壳兽", "小磁怪", "三合一磁怪", "大葱鸭",
  "嘟嘟", "嘟嘟利", "小海狮", "白海狮",
  "臭泥", "臭臭泥", "大舌贝", "刺甲贝",
  "鬼斯", "鬼斯通", "耿鬼", "大岩蛇",
  "催眠貘", "引梦貘人", "大钳蟹", "巨钳蟹",
  "霹雳电球", "顽皮雷弹", "蛋蛋", "椰蛋树",
  "卡拉卡拉", "嘎啦嘎啦", "飞腿郎", "快拳郎",
  "大舌头", "瓦斯弹", "双弹瓦斯", "独角犀牛",
  "钻角犀兽", "吉利蛋", "蔓藤怪", "袋兽",
  "墨海马", "海刺龙", "角金鱼", "金鱼王",
  "海星星", "宝石海星", "魔墙人偶", "飞天螳螂",
  "迷唇姐", "电击兽", "鸭嘴火兽", "凯罗斯",
  "肯泰罗", "鲤鱼王", "暴鲤龙", "拉普拉斯",
  "百变怪", "伊布", "水伊布", "雷伊布",
  "火伊布", "多边兽", "菊石兽", "多刺菊石兽",
  "化石盔", "镰刀盔", "化石翼龙", "卡比兽",
  "急冻鸟", "闪电鸟", "火焰鸟", "迷你龙",
  "哈克龙", "快龙", "超梦", "梦幻",
  "菊草叶", "月桂叶", "大竺葵", "火球鼠",
  "火岩鼠", "火暴兽", "小锯鳄", "蓝鳄",
  "大力鳄", "尾立", "大尾立", "咕咕",
  "猫头夜鹰", "芭瓢虫", "安瓢虫", "圆丝蛛",
  "阿利多斯", "叉字蝠", "灯笼鱼", "电灯怪",
  "皮丘", "皮宝宝", "宝宝丁", "波克比",
  "波克基古", "天然雀", "天然鸟", "咩利羊",
  "茸茸羊", "电龙", "美丽花", "玛力露",
  "玛力露丽", "树才怪", "蚊香蛙皇", "毽子草",
  "毽子花", "毽子棉", "长尾怪手", "向日种子",
  "向日花怪", "蜻蜻蜓", "乌波", "沼王",
  "太阳伊布", "月亮伊布", "黑暗鸦", "呆呆王",
  "梦妖", "未知图腾", "果然翁", "麒麟奇",
  "榛果球", "佛烈托斯", "土龙弟弟", "天蝎",
  "大钢蛇", "布鲁", "布鲁皇", "千针鱼",
  "巨钳螳螂", "壶壶", "赫拉克罗斯", "狃拉",
  "熊宝宝", "圈圈熊", "熔岩虫", "熔岩蜗牛",
  "小山猪", "长毛猪", "太阳珊瑚", "铁炮鱼",
  "章鱼桶", "信使鸟", "巨翅飞鱼", "盔甲鸟",
  "戴鲁比", "黑鲁加", "刺龙王", "小小象",
  "顿甲", "多边兽２型", "惊角鹿", "图图犬",
  "无畏小子", "战舞郎", "迷唇娃", "电击怪",
  "鸭嘴宝宝", "大奶罐", "幸福蛋", "雷公",
  "炎帝", "水君", "幼基拉斯", "沙基拉斯",
  "班基拉斯", "洛奇亚", "凤王", "时拉比",
  "木守宫", "森林蜥蜴", "蜥蜴王", "火稚鸡",
  "力壮鸡", "火焰鸡", "水跃鱼", "沼跃鱼",
  "巨沼怪", "土狼犬", "大狼犬", "蛇纹熊",
  "直冲熊", "刺尾虫", "甲壳茧", "狩猎凤蝶",
  "盾甲茧", "毒粉蛾", "莲叶童子", "莲帽小童",
  "乐天河童", "橡实果", "长鼻叶", "狡猾天狗",
  "傲骨燕", "大王燕", "长翅鸥", "大嘴鸥",
  "拉鲁拉丝", "奇鲁莉安", "沙奈朵", "溜溜糖球",
  "雨翅蛾", "蘑蘑菇", "斗笠菇", "懒人獭",
  "过动猿", "请假王", "土居忍士", "铁面忍者",
  "脱壳忍者", "咕妞妞", "吼爆弹", "爆音怪",
  "幕下力士", "铁掌力士", "露力丽", "朝北鼻",
  "向尾喵", "优雅猫", "勾魂眼", "大嘴娃",
  "可可多拉", "可多拉", "波士可多拉", "玛沙那",
  "恰雷姆", "落雷兽", "雷电兽", "正电拍拍",
  "负电拍拍", "电萤虫", "甜甜萤", "毒蔷薇",
  "溶食兽", "吞食兽", "利牙鱼", "巨牙鲨",
  "吼吼鲸", "吼鲸王", "呆火驼", "喷火驼",
  "煤炭龟", "跳跳猪", "噗噗猪", "晃晃斑",
  "大颚蚁", "超音波幼虫", "沙漠蜻蜓", "刺球仙人掌",
  "梦歌仙人掌", "青绵鸟", "七夕青鸟", "猫鼬斩",
  "饭匙蛇", "月石", "太阳岩", "泥泥鳅",
  "鲶鱼王", "龙虾小兵", "铁螯龙虾", "天秤偶",
  "念力土偶", "触手百合", "摇篮百合", "太古羽虫",
  "太古盔甲", "丑丑鱼", "美纳斯", "飘浮泡泡",
  "变隐龙", "怨影娃娃", "诅咒娃娃", "夜巡灵",
  "彷徨夜灵", "热带龙", "风铃铃", "阿勃梭鲁",
  "小果然", "雪童子", "冰鬼护", "海豹球",
  "海魔狮", "帝牙海狮", "珍珠贝", "猎斑鱼",
  "樱花鱼", "古空棘鱼", "爱心鱼", "宝贝龙",
  "甲壳龙", "暴飞龙", "铁哑铃", "金属怪",
  "巨金怪", "雷吉洛克", "雷吉艾斯", "雷吉斯奇鲁",
  "拉帝亚斯", "拉帝欧斯", "盖欧卡", "固拉多",
  "烈空坐", "基拉祈", "代欧奇希斯", "草苗龟",
  "树林龟", "土台龟", "小火焰猴", "猛火猴",
  "烈焰猴", "波加曼", "波皇子", "帝王拿波",
  "姆克儿", "姆克鸟", "姆克鹰", "大牙狸",
  "大尾狸", "圆法师", "音箱蟀", "小猫怪",
  "勒克猫", "伦琴猫", "含羞苞", "罗丝雷朵",
  "头盖龙", "战槌龙", "盾甲龙", "护城龙",
  "结草儿", "结草贵妇", "绅士蛾", "三蜜蜂",
  "蜂女王", "帕奇利兹", "泳圈鼬", "浮潜鼬",
  "樱花宝", "樱花儿", "无壳海兔", "海兔兽",
  "双尾怪手", "飘飘球", "随风球", "卷卷耳",
  "长耳兔", "梦妖魔", "乌鸦头头", "魅力喵",
  "东施喵", "铃铛响", "臭鼬噗", "坦克臭鼬",
  "铜镜怪", "青铜钟", "盆才怪", "魔尼尼",
  "小福蛋", "聒噪鸟", "花岩怪", "圆陆鲨",
  "尖牙陆鲨", "烈咬陆鲨", "小卡比兽", "利欧路",
  "路卡利欧", "沙河马", "河马兽", "钳尾蝎",
  "龙王蝎", "不良蛙", "毒骷蛙", "尖牙笼",
  "荧光鱼", "霓虹鱼", "小球飞鱼", "雪笠怪",
  "暴雪王", "玛狃拉", "自爆磁怪", "大舌舔",
  "超甲狂犀", "巨蔓藤", "电击魔兽", "鸭嘴炎兽",
  "波克基斯", "远古巨蜓", "叶伊布", "冰伊布",
  "天蝎王", "象牙猪", "多边兽乙型", "艾路雷朵",
  "大朝北鼻", "黑夜魔灵", "雪妖女", "洛托姆",
  "由克希", "艾姆利多", "亚克诺姆", "帝牙卢卡",
  "帕路奇亚", "席多蓝恩", "雷吉奇卡斯", "骑拉帝纳",
  "克雷色利亚", "霏欧纳", "玛纳霏", "达克莱伊",
  "谢米", "阿尔宙斯", "比克提尼", "藤藤蛇",
  "青藤蛇", "君主蛇", "暖暖猪", "炒炒猪",
  "炎武王", "水水獭", "双刃丸", "大剑鬼",
  "探探鼠", "步哨鼠", "小约克", "哈约克",
  "长毛狗", "扒手猫", "酷豹", "花椰猴",
  "花椰猿", "爆香猴", "爆香猿", "冷水猴",
  "冷水猿", "食梦梦", "梦梦蚀", "豆豆鸽",
  "咕咕鸽", "高傲雉鸡", "斑斑马", "雷电斑马",
  "石丸子", "地幔岩", "庞岩怪", "滚滚蝙蝠",
  "心蝙蝠", "螺钉地鼠", "龙头地鼠", "差不多娃娃",
  "搬运小匠", "铁骨土人", "修建老匠", "圆蝌蚪",
  "蓝蟾蜍", "蟾蜍王", "投摔鬼", "打击鬼",
  "虫宝包", "宝包茧", "保姆虫", "百足蜈蚣",
  "车轮球", "蜈蚣王", "木棉球", "风妖精",
  "百合根娃娃", "裙儿小姐", "野蛮鲈鱼", "黑眼鳄",
  "混混鳄", "流氓鳄", "火红不倒翁", "达摩狒狒",
  "沙铃仙人掌", "石居蟹", "岩殿居蟹", "滑滑小子",
  "头巾混混", "象征鸟", "哭哭面具", "迭失棺",
  "原盖海龟", "肋骨海龟", "始祖小鸟", "始祖大鸟",
  "破破袋", "灰尘山", "索罗亚", "索罗亚克",
  "泡沫栗鼠", "奇诺栗鼠", "哥德宝宝", "哥德小童",
  "哥德小姐", "单卵细胞球", "双卵细胞球", "人造细胞卵",
  "鸭宝宝", "舞天鹅", "迷你冰", "多多冰",
  "双倍多多冰", "四季鹿", "萌芽鹿", "电飞鼠",
  "盖盖虫", "骑士蜗牛", "哎呀球菇", "败露球菇",
  "轻飘飘", "胖嘟嘟", "保姆曼波", "电电虫",
  "电蜘蛛", "种子铁球", "坚果哑铃", "齿轮儿",
  "齿轮组", "齿轮怪", "麻麻小鱼", "麻麻鳗",
  "麻麻鳗鱼王", "小灰怪", "大宇怪", "烛光灵",
  "灯火幽灵", "水晶灯火灵", "牙牙", "斧牙龙",
  "双斧战龙", "喷嚏熊", "冻原熊", "几何雪花",
  "小嘴蜗", "敏捷虫", "泥巴鱼", "功夫鼬",
  "师父鼬", "赤面龙", "泥偶小人", "泥偶巨人",
  "驹刀小兵", "劈斩司令", "爆炸头水牛", "毛头小鹰",
  "勇士雄鹰", "秃鹰丫头", "秃鹰娜", "熔蚁兽",
  "铁蚁", "单首龙", "双首暴龙", "三首恶龙",
  "燃烧虫", "火神蛾", "勾帕路翁", "代拉基翁",
  "毕力吉翁", "龙卷云", "雷电云", "莱希拉姆",
  "捷克罗姆", "土地云", "酋雷姆", "凯路迪欧",
  "美洛耶塔", "盖诺赛克特", "哈力栗", "胖胖哈力",
  "布里卡隆", "火狐狸", "长尾火狐", "妖火红狐",
  "呱呱泡蛙", "呱头蛙", "甲贺忍蛙", "掘掘兔",
  "掘地兔", "小箭雀", "火箭雀", "烈箭鹰",
  "粉蝶虫", "粉蝶蛹", "彩粉蝶", "小狮狮",
  "火炎狮", "花蓓蓓", "花叶蒂", "花洁夫人",
  "坐骑小羊", "坐骑山羊", "顽皮熊猫", "霸道熊猫",
  "多丽米亚", "妙喵", "超能妙喵", "独剑鞘",
  "双剑鞘", "坚盾剑怪", "粉香香", "芳香精",
  "绵绵泡芙", "胖甜妮", "好啦鱿", "乌贼王",
  "龟脚脚", "龟足巨铠", "垃垃藻", "毒藻龙",
  "铁臂枪虾", "钢炮臂虾", "伞电蜥", "光电伞蜥",
  "宝宝暴龙", "怪颚龙", "冰雪龙", "冰雪巨龙",
  "仙子伊布", "摔角鹰人", "咚咚鼠", "小碎钻",
  "黏黏宝", "黏美儿", "黏美龙", "钥圈儿",
  "小木灵", "朽木妖", "南瓜精", "南瓜怪人",
  "冰宝", "冰岩怪", "嗡蝠", "音波龙",
  "哲尔尼亚斯", "伊裴尔塔尔", "基格尔德", "蒂安希",
  "胡帕", "波尔凯尼恩", "木木枭", "投羽枭",
  "狙射树枭", "火斑喵", "炎热喵", "炽焰咆哮虎",
  "球球海狮", "花漾海狮", "西狮海壬", "小笃儿",
  "喇叭啄鸟", "铳嘴大鸟", "猫鼬少", "猫鼬探长",
  "强颚鸡母虫", "虫电宝", "锹农炮虫", "好胜蟹",
  "好胜毛蟹", "花舞鸟", "萌虻", "蝶结萌虻",
  "岩狗狗", "鬃岩狼人", "弱丁鱼", "好坏星",
  "超坏星", "泥驴仔", "重泥挽马", "滴蛛",
  "滴蛛霸", "伪螳草", "兰螳花", "睡睡菇",
  "灯罩夜菇", "夜盗火蜥", "焰后蜥", "童偶熊",
  "穿着熊", "甜竹竹", "甜舞妮", "甜冷美后",
  "花疗环环", "智挥猩", "投掷猴", "胆小虫",
  "具甲武者", "沙丘娃", "噬沙堡爷", "拳海参",
  "属性：空", "银伴战兽", "小陨星", "树枕尾熊",
  "爆焰龟兽", "托戈德玛尔", "谜拟丘", "磨牙彩皮鱼",
  "老翁龙", "破破舵轮", "心鳞宝", "鳞甲龙",
  "杖尾鳞甲龙", "卡璞・鸣鸣", "卡璞・蝶蝶", "卡璞・哞哞",
  "卡璞・鳍鳍", "科斯莫古", "科斯莫姆", "索尔迦雷欧",
  "露奈雅拉", "虚吾伊德", "爆肌蚊", "费洛美螂",
  "电束木", "铁火辉夜", "纸御剑", "恶食大王",
  "奈克洛兹玛", "玛机雅娜", "玛夏多", "毒贝比",
  "四颚针龙", "垒磊石", "砰头小丑", "捷拉奥拉",
  "美录坦", "美录梅塔", "敲音猴", "啪咚猴",
  "轰擂金刚猩", "炎兔儿", "腾蹴小将", "闪焰王牌",
  "泪眼蜥", "变涩蜥", "千面避役", "贪心栗鼠",
  "藏饱栗鼠", "稚山雀", "蓝鸦", "钢铠鸦",
  "索侦虫", "天罩虫", "以欧路普", "狡小狐",
  "猾大狐", "幼棉棉", "白蓬蓬", "毛辫羊",
  "毛毛角羊", "咬咬龟", "暴噬龟", "来电汪",
  "逐电犬", "小炭仔", "大炭车", "巨炭山",
  "啃果虫", "苹裹龙", "丰蜜龙", "沙包蛇",
  "沙螺蟒", "古月鸟", "刺梭鱼", "戽斗尖梭",
  "电音婴", "颤弦蝾螈", "烧火蚣", "焚焰蚣",
  "拳拳蛸", "八爪武师", "来悲茶", "怖思壶",
  "迷布莉姆", "提布莉姆", "布莉姆温", "捣蛋小妖",
  "诈唬魔", "长毛巨魔", "堵拦熊", "喵头目",
  "魔灵珊瑚", "葱游兵", "踏冰人偶", "迭失板",
  "小仙奶", "霜奶仙", "列阵兵", "啪嚓海胆",
  "雪吞虫", "雪绒蛾", "巨石丁", "冰砌鹅",
  "爱管侍", "莫鲁贝可", "铜象", "大王铜象",
  "雷鸟龙", "雷鸟海兽", "鳃鱼龙", "鳃鱼海兽",
  "铝钢龙", "多龙梅西亚", "多龙奇", "多龙巴鲁托",
  "苍响", "藏玛然特", "无极汰那", "熊徒弟",
  "武道熊师", "萨戮德", "雷吉艾勒奇", "雷吉铎拉戈",
  "雪暴马", "灵幽马", "蕾冠王", "诡角鹿",
  "劈斧螳螂", "月月熊", "幽尾玄鱼", "大狃拉",
  "万针鱼", "眷恋云", "新叶喵", "蒂蕾喵",
  "魔幻假面喵", "呆火鳄", "炙烫鳄", "骨纹巨声鳄",
  "润水鸭", "涌跃鸭", "狂欢浪舞鸭", "爱吃豚",
  "飘香豚", "团珠蛛", "操陷蛛", "豆蟋蟀",
  "烈腿蝗", "布拨", "布土拨", "巴布土拨",
  "一对鼠", "一家鼠", "狗仔包", "麻花犬",
  "迷你芙", "奥利纽", "奥利瓦", "怒鹦哥",
  "盐石宝", "盐石垒", "盐石巨灵", "炭小侍",
  "红莲铠骑", "苍炎刃鬼", "光蚪仔", "电肚蛙",
  "电海燕", "大电海燕", "偶叫獒", "獒教父",
  "滋汁鼹", "涂标客", "纳噬草", "怖纳噬草",
  "原野水母", "陆地水母", "毛崖蟹", "热辣娃",
  "狠辣椒", "虫滚泥", "虫甲圣", "飘飘雏",
  "超能艳鸵", "小锻匠", "巧锻匠", "巨锻匠",
  "海地鼠", "三海地鼠", "下石鸟", "波普海豚",
  "海豚侠", "噗隆隆", "普隆隆姆", "摩托蜥",
  "拖拖蚓", "晶光芽", "晶光花", "墓仔狗",
  "墓扬犬", "缠红鹤", "走鲸", "浩大鲸",
  "轻身鳕", "吃吼霸", "米立龙", "弃世猴",
  "土王", "奇麒麟", "土龙节节", "仆刀将军",
  "雄伟牙", "吼叫尾", "猛恶菇", "振翼发",
  "爬地翅", "沙铁皮", "铁辙迹", "铁包袱",
  "铁臂膀", "铁脖颈", "铁毒蛾", "铁荆棘",
  "凉脊龙", "冻脊龙", "戟脊龙", "索财灵",
  "赛富豪", "古简蜗", "古剑豹", "古鼎鹿",
  "古玉鱼", "轰鸣月", "铁武者", "故勒顿",
  "密勒顿", "波荡水", "铁斑叶", "裹蜜虫",
  "斯魔茶", "来悲粗茶", "够赞狗", "愿增猿",
  "吉雉鸡", "厄诡椪", "铝钢桥龙", "蜜集大蛇",
  "破空焰", "猛雷鼓", "铁磐岩", "铁头壳",
  "太乐巴戈斯", "桃歹郎",
};

// Nombre de la especie en el idioma activo (cae al de DEX_TBL si ese
// idioma no tiene nombre propio para ella).
static inline const char *dexName(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return DEX_TBL[0].name;
  const char *n = (gLang == LANG_ZH) ? DEX_NAME_ZH[dex] : nullptr;
  return n ? n : DEX_TBL[dex].name;
}

// destinos extra de las especies que se ramifican (ademas de evolvesTo);
// Pet::evolve() elige al azar entre todos
struct DexBranch { uint16_t from, to; };
static const DexBranch DEX_BRANCHES[] = {
  { 44, 182 },
  { 52, 863 },
  { 61, 186 },
  { 79, 199 },
  { 123, 900 },
  { 133, 135 }, { 133, 136 }, { 133, 196 }, { 133, 197 }, { 133, 470 }, { 133, 471 }, { 133, 700 },
  { 194, 980 },
  { 215, 903 },
  { 236, 107 }, { 236, 237 },
  { 265, 268 },
  { 281, 475 },
  { 290, 292 },
  { 361, 478 },
  { 366, 368 },
  { 412, 414 },
  { 562, 867 },
  { 790, 792 },
  { 840, 842 }, { 840, 1011 },
  { 935, 937 },
};
#define NUM_DEX_BRANCHES 27

// el primer huevo de la partida: iniciales clasicos
static const int16_t CLASSIC_DEX[] = { 1, 4, 7, 172, 133 };
#define NUM_CLASSIC_DEX 5
