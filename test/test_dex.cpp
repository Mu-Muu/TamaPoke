// Integridad de la tabla de la Pokedex (dex.h, generada por tools/gen_dex.py).
#include "framework.h"
#include "shim/Arduino.h"
#include "../dex.h"
#include "../i18n.h"
#include "../pet.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <set>

static const int N = DEX_COUNT;

TEST(dex, la_tabla_tiene_una_entrada_por_especie_mas_el_cero) {
  CHECK_EQ((int)(sizeof(DEX_TBL) / sizeof(DEX_TBL[0])), N + 1);
  CHECK_EQ((int)(sizeof(DEX_NAME_ZH) / sizeof(DEX_NAME_ZH[0])), N + 1);
}

TEST(dex, todos_los_nombres_son_ascii_en_mayusculas) {
  for (int d = 1; d <= N; d++) {
    const char *n = DEX_TBL[d].name;
    CHECK_MSG(n != NULL && n[0] != 0, "entrada sin nombre");
    if (!n || !n[0]) continue;
    for (const char *c = n; *c; c++) {
      // la fuente GFX del firmware es ASCII y sin minusculas
      CHECK_MSG((unsigned char)*c < 128, std::string("caracter no ASCII en ") + n);
      CHECK_MSG(!islower((unsigned char)*c), std::string("minuscula en ") + n);
    }
  }
}

TEST(dex, todas_las_especies_tienen_nombre_chino) {
  for (int d = 1; d <= N; d++)
    CHECK_MSG(DEX_NAME_ZH[d] && DEX_NAME_ZH[d][0], std::string("sin nombre chino: ") + DEX_TBL[d].name);
}

// la cabecera de la ficha es char[26] con "%s%s Lv.%u": ningun nombre en ningun
// idioma puede desbordarla (el nombre se cortaria en pantalla)
TEST(dex, ningun_nombre_desborda_la_cabecera) {
  char head[26];
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    for (int d = 1; d <= N; d++) {
      int need = snprintf(head, sizeof(head), T(S_NAME_FMT), "*", dexName(d), 255);
      CHECK_MSG(need >= 0 && need < (int)sizeof(head),
                std::string("cabecera cortada: ") + dexName(d));
    }
  }
  gLang = LANG_DEFAULT;
}

TEST(dex, las_evoluciones_apuntan_a_entradas_validas) {
  for (int d = 1; d <= N; d++) {
    uint16_t to = DEX_TBL[d].evolvesTo;
    CHECK_MSG(to <= N, std::string("evolvesTo fuera de rango en ") + DEX_TBL[d].name);
    CHECK_MSG(to != d, std::string("se evoluciona a si mismo: ") + DEX_TBL[d].name);
  }
}

TEST(dex, evolucionar_y_nivel_van_de_la_mano) {
  for (int d = 1; d <= N; d++) {
    bool evolves = DEX_TBL[d].evolvesTo != 0;
    bool hasLevel = DEX_TBL[d].evolveLevel > 0;
    CHECK_MSG(evolves == hasLevel,
              std::string("evolvesTo y evolveLevel no cuadran en ") + DEX_TBL[d].name);
  }
}

TEST(dex, ninguna_cadena_evolutiva_hace_bucle) {
  for (int d = 1; d <= N; d++) {
    std::set<int> seen;
    int cur = d;
    int guard = 0;
    while (cur >= 1 && cur <= N && DEX_TBL[cur].evolvesTo != 0 && guard++ < N) {
      CHECK_MSG(seen.insert(cur).second, std::string("bucle evolutivo en ") + DEX_TBL[d].name);
      if (!seen.count(cur)) break;
      cur = DEX_TBL[cur].evolvesTo;
    }
    CHECK_MSG(guard < 6, std::string("cadena demasiado larga desde ") + DEX_TBL[d].name);
  }
}

TEST(dex, los_niveles_de_evolucion_suben_a_lo_largo_de_la_cadena) {
  for (int d = 1; d <= N; d++) {
    int cur = d, prev = 0, guard = 0;
    while (DEX_TBL[cur].evolvesTo != 0 && guard++ < 6) {
      CHECK_MSG(DEX_TBL[cur].evolveLevel > prev,
                std::string("nivel de evolucion no creciente en ") + DEX_TBL[cur].name);
      prev = DEX_TBL[cur].evolveLevel;
      cur = DEX_TBL[cur].evolvesTo;
    }
  }
}

// una especie que solo sale por evolucion tiene que ser alcanzable desde algun
// huevo, o seria imposible de registrar en la pokedex
TEST(dex, toda_especie_es_alcanzable) {
  bool reach[DEX_COUNT + 1] = { false };
  for (int d = 1; d <= N; d++) {
    if (DEX_TBL[d].rarity == R_EVO || DEX_TBL[d].rarity == R_NONE) continue;  // no sale de huevo
    int cur = d, guard = 0;
    while (cur >= 1 && cur <= N && guard++ < 6) {
      reach[cur] = true;
      for (int b = 0; b < NUM_DEX_BRANCHES; b++)  // ramas: Eevee, Gloom...
        if (DEX_BRANCHES[b].from == cur) {
          int t = DEX_BRANCHES[b].to;
          reach[t] = true;
          if (DEX_TBL[t].evolvesTo) reach[DEX_TBL[t].evolvesTo] = true;  // Cascoon -> Dustox
        }
      if (DEX_TBL[cur].evolvesTo == 0) break;
      cur = DEX_TBL[cur].evolvesTo;
    }
  }
  for (int d = 1; d <= N; d++)
    if (DEX_TBL[d].rarity != R_NONE)  // sin sprite: inalcanzable a proposito
      CHECK_MSG(reach[d], std::string("inalcanzable: ") + DEX_TBL[d].name);
}

// con la despedida a los 3 dias (nivel 72), toda linea tiene que poder llegar a
// su forma final antes de que la mascota cumpla el ciclo
TEST(dex, toda_linea_se_completa_antes_de_la_despedida) {
  const uint32_t maxLevel = FAREWELL_AGE_MIN / MINUTES_PER_LEVEL;
  for (int d = 1; d <= N; d++) {
    if (DEX_TBL[d].rarity == R_EVO || DEX_TBL[d].rarity == R_NONE) continue;
    int cur = d, guard = 0;
    uint32_t need = 0;
    while (DEX_TBL[cur].evolvesTo != 0 && guard++ < 6) {
      if (DEX_TBL[cur].evolveLevel > need) need = DEX_TBL[cur].evolveLevel;
      cur = DEX_TBL[cur].evolvesTo;
    }
    CHECK_MSG(need <= maxLevel,
              std::string("la linea de ") + DEX_TBL[d].name + " no llega a forma final a tiempo");
  }
}

TEST(dex, las_stats_base_son_plausibles) {
  for (int d = 1; d <= N; d++) {
    const DexEntry &e = DEX_TBL[d];
    CHECK_MSG(e.bHp > 0 && e.bAtk > 0 && e.bDef > 0 && e.bSpe > 0,
              std::string("stat base a cero en ") + e.name);
    CHECK_MSG(e.biome <= 5, std::string("bioma desconocido en ") + e.name);
    CHECK_MSG(e.rarity <= R_NONE, std::string("rareza desconocida en ") + e.name);
  }
}

TEST(dex, las_ramas_son_coherentes) {
  for (int b = 0; b < NUM_DEX_BRANCHES; b++) {
    const DexBranch &r = DEX_BRANCHES[b];
    CHECK_RANGE((int)r.from, 1, N);
    CHECK_RANGE((int)r.to, 1, N);
    // una rama cuelga de una especie que ya evoluciona, y es otro destino
    CHECK_MSG(DEX_TBL[r.from].evolvesTo != 0, std::string("rama sin evolucion principal: ") + DEX_TBL[r.from].name);
    CHECK_MSG(DEX_TBL[r.from].evolvesTo != r.to, std::string("rama repetida: ") + DEX_TBL[r.to].name);
    CHECK_EQ((int)DEX_TBL[r.to].rarity, (int)R_EVO);  // no sale de huevo
  }
  // Eevee (133) tiene sus cinco evoluciones de gen 1-2
  std::set<int> eevee = { DEX_TBL[133].evolvesTo };
  for (int b = 0; b < NUM_DEX_BRANCHES; b++)
    if (DEX_BRANCHES[b].from == 133) eevee.insert(DEX_BRANCHES[b].to);
  CHECK(eevee == (std::set<int>{ 134, 135, 136, 196, 197, 470, 471, 700 }));
  CHECK_EQ((int)DEX_TBL[133].rarity, (int)R_COMUN);
}

TEST(dex, los_iniciales_clasicos_son_validos) {
  CHECK_EQ((int)(sizeof(CLASSIC_DEX) / sizeof(CLASSIC_DEX[0])), NUM_CLASSIC_DEX);
  for (int i = 0; i < NUM_CLASSIC_DEX; i++) {
    int16_t d = CLASSIC_DEX[i];
    CHECK_RANGE(d, (int16_t)1, (int16_t)DEX_COUNT);
    CHECK_MSG(DEX_TBL[d].rarity != R_EVO, "un inicial no puede ser una forma evolucionada");
  }
}

// las especies sin sprite no pueden aparecer: ni como evolucion ni como rama
TEST(dex, ninguna_evolucion_lleva_a_una_especie_sin_sprite) {
  for (int d = 1; d <= N; d++)
    if (DEX_TBL[d].evolvesTo)
      CHECK_MSG(DEX_TBL[DEX_TBL[d].evolvesTo].rarity != R_NONE,
                std::string("evoluciona a una especie sin sprite: ") + DEX_TBL[d].name);
  for (int b = 0; b < NUM_DEX_BRANCHES; b++)
    CHECK_MSG(DEX_TBL[DEX_BRANCHES[b].to].rarity != R_NONE,
              std::string("rama a una especie sin sprite: ") + DEX_TBL[DEX_BRANCHES[b].to].name);
}

TEST(dex, hay_al_menos_una_especie_de_cada_rareza) {
  int count[R_NONE + 1] = { 0 };
  for (int d = 1; d <= N; d++) count[DEX_TBL[d].rarity]++;
  for (int r = 0; r <= R_LEGENDARIO; r++)
    CHECK_MSG(count[r] > 0, "hay una rareza sin especies: el sorteo del huevo se quedaria sin candidatos");
}

// pickEggSpecies sorteaba entre los 80 primeros candidatos de un tier; con
// gen 2-3 el comun pasa de 100, asi que el array mide DEX_COUNT. Si este test
// falla es que el sorteo de huevos ya no ve todas las especies del tier. Con
// las regiones en orden, al empezar solo esta abierta Kanto (1-151).
TEST(dex, el_sorteo_ve_todas_las_especies_de_cada_tier) {
  for (int r = R_COMUN; r <= R_LEGENDARIO; r++) {
    std::set<int> salen;
    for (int i = 0; i < 20000; i++) {
      randomSeed(i * 2654435761u + r);
      Pet p;
      p.dexReg[0] = 1;  // ya hay pokedex: no sale el inicial clasico
      int16_t d = p.pickEggSpecies();
      if (DEX_TBL[d].rarity == r) salen.insert(d);
    }
    int n = 0;
    for (int d = 1; d <= 151; d++)
      if (DEX_TBL[d].rarity == r) n++;
    if (r == R_LEGENDARIO) continue;  // solo con 25+ registradas
    CHECK_MSG((int)salen.size() == n, "hay especies del tier que nunca salen del huevo");
  }
}

TEST(dex, dexName_cae_al_nombre_base_si_no_hay_traduccion) {
  gLang = LANG_EN;
  CHECK_STREQ(dexName(1), DEX_TBL[1].name);
  gLang = LANG_ZH;
  for (int d = 1; d <= N; d++) {
    const char *want = DEX_NAME_ZH[d] ? DEX_NAME_ZH[d] : DEX_TBL[d].name;
    CHECK_STREQ(dexName(d), want);
  }
  gLang = LANG_DEFAULT;
}

TEST(dex, dexName_fuera_de_rango_no_revienta) {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    CHECK_STREQ(dexName(0), DEX_TBL[0].name);
    CHECK_STREQ(dexName(-1), DEX_TBL[0].name);
    CHECK_STREQ(dexName(DEX_COUNT + 1), DEX_TBL[0].name);
    CHECK_STREQ(dexName(30000), DEX_TBL[0].name);
  }
  gLang = LANG_DEFAULT;
}
