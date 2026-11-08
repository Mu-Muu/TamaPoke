// Tests de la logica de la mascota (pet.cpp) sobre los shims del host.
#include "framework.h"
#include "shim/Arduino.h"
#include "../pet.h"
#include "../dex.h"
#include "../regions.h"
#include <string.h>
#include <set>

// ---------------------------------------------------------------- helpers
static void advance(Pet &p, uint32_t minutes) {
  for (uint32_t i = 0; i < minutes; i++) {
    mockAdvanceMillis(PET_TICK_MS);
    p.update(millis());
  }
}

// mascota recien eclosionada de la especie pedida, con la NVS limpia
static void makePet(Pet &p, int16_t dex) {
  mockNvsReset();
  mockSetMillis(0);
  p.begin();
  if (p.awaitingStarter()) p.chooseStarter(dex);
  p.eggTap();
  p.eggTap();
  p.eggTap();  // 3 toques = eclosion inmediata
  p.nature = NAT_EASY;  // el caracter se sortea: aqui sin manias, para medir lo de base
}

static void setStats(Pet &p, uint8_t f, uint8_t j, uint8_t e, uint8_t h) {
  p.fullness = f; p.joy = j; p.energy = e; p.hygiene = h;
}

// ---------------------------------------------------------------- huevo
TEST(egg, primera_partida_pide_inicial) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK(p.isEgg());
  CHECK(p.awaitingStarter());
  CHECK_EQ(p.eggCracks(), (uint8_t)0);
  CHECK_EQ(p.registeredCount(), (uint16_t)0);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
}

// regresion v1.3: el huevo eclosionaba solo a los 3 min mientras el jugador
// aun estaba eligiendo inicial, y se perdia la eleccion
TEST(egg, no_eclosiona_mientras_se_elige_inicial) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK(p.awaitingStarter());
  advance(p, 600);  // 10 horas de juego
  CHECK_MSG(p.isEgg(), "el huevo NO debe eclosionar durante la eleccion");
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
  p.chooseStarter(7);
  CHECK(!p.awaitingStarter());
  advance(p, 3);
  CHECK(!p.isEgg());
  CHECK_EQ(p.speciesId, (int16_t)7);
}

TEST(egg, eclosiona_sola_a_los_3_minutos) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.chooseStarter(1);
  advance(p, 2);
  CHECK(p.isEgg());
  advance(p, 1);
  CHECK(!p.isEgg());
  CHECK_EQ(p.speciesId, (int16_t)1);
}

TEST(egg, tres_toques_eclosionan) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.chooseStarter(4);
  p.eggTap();
  CHECK_EQ(p.eggCracks(), (uint8_t)1);
  CHECK(p.isEgg());
  p.eggTap();
  CHECK_EQ(p.eggCracks(), (uint8_t)2);
  CHECK(p.isEgg());
  p.eggTap();
  CHECK(!p.isEgg());
  CHECK_EQ(p.speciesId, (int16_t)4);
}

TEST(egg, eclosionar_sortea_genes_y_resetea_al_individuo) {
  Pet p;
  makePet(p, 4);
  CHECK_RANGE(p.geneAtk, 90, 110);
  CHECK_RANGE(p.geneDef, 90, 110);
  CHECK_RANGE(p.geneSpe, 90, 110);
  CHECK_EQ(p.trAtk, (uint8_t)0);
  CHECK_EQ(p.trDef, (uint8_t)0);
  CHECK_EQ(p.trSpe, (uint8_t)0);
  CHECK_EQ(p.bond, (uint8_t)0);
  CHECK_STREQ(p.nick, "");
  CHECK_MSG(p.isRegistered(4), "criar = registrar en la pokedex");
  CHECK_EQ(p.registeredCount(), (uint16_t)1);
}

TEST(egg, tocar_huevo_no_hace_nada_si_ya_eclosiono) {
  Pet p;
  makePet(p, 4);
  uint8_t cracks = p.eggCracks();
  p.eggTap();
  CHECK_EQ(p.eggCracks(), cracks);
  CHECK_EQ(p.speciesId, (int16_t)4);
}

TEST(egg, rareza_del_huevo_es_valida) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK_RANGE(p.eggRarity(), (uint8_t)R_COMUN, (uint8_t)R_LEGENDARIO);
}

// ---------------------------------------------------------------- comida
// una sola baya, sin gustos: +25 de comida y nada mas
TEST(feed, baya_sube_25_y_topa_en_100) {
  Pet p;
  makePet(p, 4);
  p.fullness = 50;
  p.joy = 50;
  p.feed();
  CHECK_EQ(p.fullness, (uint8_t)75);
  CHECK_EQ(p.joy, (uint8_t)50);
  CHECK(p.eating());
  p.feed();
  CHECK_EQ(p.fullness, (uint8_t)100);  // 100 en vez de 105
}

TEST(feed, chuche_engorda) {
  Pet p;
  makePet(p, 4);
  p.fullness = 50;
  p.joy = 50;
  p.bag[ITEM_CANDY] = 1;
  p.feedCandy();
  CHECK_EQ(p.bag[ITEM_CANDY], (uint16_t)0);
  CHECK_EQ(p.fullness, (uint8_t)60);
  CHECK_EQ(p.joy, (uint8_t)62);
  CHECK_EQ(p.weight, (uint8_t)12);
}

TEST(feed, dormido_o_huevo_no_come) {
  Pet p;
  makePet(p, 4);
  p.fullness = 40;
  p.toggleLight();  // a dormir
  CHECK(p.sleeping);
  p.feed();
  p.feedCandy();
  CHECK_EQ(p.fullness, (uint8_t)40);

  Pet e;
  mockNvsReset();
  e.begin();
  e.chooseStarter(4);
  e.fullness = 40;
  e.feed();
  CHECK_EQ(e.fullness, (uint8_t)40);
}

// ---------------------------------------------------------------- tick
TEST(tick, despierto_baja_comida_2_y_energia_1_por_minuto) {
  Pet p;
  makePet(p, 4);
  setStats(p, 100, 100, 100, 100);
  p.poops = 0;
  mockForceRandom(99);  // random(100) < 15 falso: sin cacas
  advance(p, 10);
  mockClearForcedRandom();
  CHECK_EQ(p.fullness, (uint8_t)80);
  CHECK_EQ(p.energy, (uint8_t)90);
  CHECK_EQ(p.hygiene, (uint8_t)90);
  CHECK_EQ(p.joy, (uint8_t)90);
  CHECK_EQ(p.ageMinutes, (uint32_t)10);
  CHECK_EQ(p.poops, (uint8_t)0);
}

TEST(tick, las_barras_nunca_bajan_de_cero) {
  Pet p;
  makePet(p, 4);
  setStats(p, 5, 3, 2, 1);
  advance(p, 30);
  CHECK_EQ(p.fullness, (uint8_t)0);
  CHECK_EQ(p.joy, (uint8_t)0);
  CHECK_EQ(p.energy, (uint8_t)0);
  CHECK_EQ(p.hygiene, (uint8_t)0);
}

TEST(tick, dormir_recupera_energia_con_suelos) {
  Pet p;
  makePet(p, 4);
  setStats(p, 100, 100, 10, 100);
  p.toggleLight();
  advance(p, 20);
  CHECK_MSG(p.energy == 100, "durmiendo la energia sube 6/min hasta 100");
  CHECK_MSG(p.fullness > 80, "durmiendo el hambre baja ~4x mas lento");
  advance(p, 2000);  // dormir muchisimo: los suelos protegen
  CHECK_MSG(p.fullness >= 30, "suelo de comida durmiendo = 30");
  CHECK_MSG(p.joy >= 35, "suelo de felicidad durmiendo = 35");
  CHECK_MSG(p.hygiene >= 45, "suelo de higiene durmiendo = 45");
  CHECK_EQ(p.careMistakes, (uint8_t)0);
}

TEST(tick, sobrepeso_se_quema_solo) {
  Pet p;
  makePet(p, 4);
  p.weight = 30;
  advance(p, 30);
  CHECK_EQ(p.weight, (uint8_t)20);  // -1 cada 3 minutos
}

TEST(tick, descuido_cuenta_una_vez_por_hora) {
  Pet p;
  makePet(p, 4);
  setStats(p, 0, 0, 0, 0);
  advance(p, 1);
  CHECK_EQ(p.careMistakes, (uint8_t)1);
  advance(p, 50);
  CHECK_MSG(p.careMistakes == 1, "el enfriamiento evita contar el mismo descuido cada minuto");
  advance(p, 11);
  CHECK_EQ(p.careMistakes, (uint8_t)2);
}

// regresion v1.4: el descuido restaba 3 de vinculo cada 30 min y era imposible
// recuperarlo; ahora resta 1 y nunca lo deja en 0
TEST(tick, descuido_enfria_el_vinculo_sin_arrasarlo) {
  Pet p;
  makePet(p, 4);
  p.bond = 50;
  setStats(p, 0, 0, 0, 0);
  advance(p, 1);
  CHECK_EQ(p.bond, (uint8_t)49);
  p.bond = 1;
  advance(p, 61);
  CHECK_MSG(p.bond == 1, "el vinculo no cae por debajo de 1");
}

TEST(tick, buen_cuidado_3h_forja_resistencia) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.trDef, (uint8_t)0);
  for (int h = 0; h < RES_GROW_MIN; h++) {  // mantener todo >= 40 durante 3 h
    setStats(p, 100, 100, 100, 100);
    advance(p, 1);
  }
  CHECK_EQ(p.trDef, (uint8_t)1);
}

// ---------------------------------------------------------------- niveles
TEST(level, un_nivel_por_hora_de_juego) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.level(), (uint8_t)1);
  p.ageMinutes = 59;
  CHECK_EQ(p.level(), (uint8_t)1);
  p.ageMinutes = 60;
  CHECK_EQ(p.level(), (uint8_t)2);
  p.ageMinutes = 24 * 60;
  CHECK_EQ(p.level(), (uint8_t)25);
}

// Arreglado en v1.8: level() era uint8_t y daba la vuelta a 0 a los ~10,6 dias
TEST(level, no_desborda_con_mascotas_longevas) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = 254 * 60;
  uint8_t before = p.level();
  CHECK_EQ(before, (uint8_t)255);
  p.ageMinutes = 255 * 60;  // nivel 256 -> deberia seguir subiendo (o saturar)
  CHECK_MSG(p.level() >= before, "el nivel nunca deberia bajar al envejecer");
}

// ---------------------------------------------------------------- evolucion
TEST(evolve, la_primera_a_los_3_dias_y_con_buen_cuidado) {
  Pet p;
  makePet(p, 4);  // CHARMANDER
  p.ageMinutes = EVO1_AGE_MIN - 1;
  CHECK(!p.canEvolveNow());
  p.ageMinutes = EVO1_AGE_MIN;
  CHECK(p.canEvolveNow());
  p.fullness = 39;  // una barra por debajo de 40 lo bloquea
  CHECK(!p.canEvolveNow());
  p.fullness = 40;
  CHECK(p.canEvolveNow());
}

TEST(evolve, la_segunda_a_los_5_dias) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = EVO1_AGE_MIN;
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)5);
  CHECK_EQ(p.evoCount, (uint8_t)1);
  mockAdvanceMillis(EVOLVE_ANIM_MS + 1);
  p.ageMinutes = EVO2_AGE_MIN - 1;
  CHECK(!p.canEvolveNow());
  p.ageMinutes = EVO2_AGE_MIN;
  CHECK(p.canEvolveNow());
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)6);
  CHECK_EQ(p.evoCount, (uint8_t)2);
}

TEST(evolve, el_que_nace_ya_evolucionado_cuenta_desde_su_primera) {
  Pet p;
  makePet(p, 25);  // PIKACHU salido del huevo: su 1a evolucion, a los 3 dias
  p.ageMinutes = EVO1_AGE_MIN;
  CHECK(p.canEvolveNow());
}

TEST(evolve, cada_descuido_retrasa_dos_horas) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = EVO1_AGE_MIN;
  p.careMistakes = 1;
  CHECK(!p.canEvolveNow());
  p.ageMinutes = EVO1_AGE_MIN + EVO_MISTAKE_MIN;
  CHECK(p.canEvolveNow());
}

TEST(evolve, muchos_descuidos_no_adelantan_la_evolucion) {
  Pet p;
  makePet(p, 4);
  p.careMistakes = 250;
  p.ageMinutes = EVO1_AGE_MIN;
  CHECK_MSG(!p.canEvolveNow(), "con 250 descuidos aun le falta");
}

TEST(evolve, la_pocion_deja_evolucionar_ya_y_no_se_gasta_si_no_sirve) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = 60;
  p.bag[ITEM_POTION_EVO] = 1;
  CHECK(!p.canEvolveNow());
  CHECK(p.useEvoPotion());
  CHECK_EQ(p.bag[ITEM_POTION_EVO], (uint16_t)0);
  CHECK(p.canEvolveNow());
  CHECK(p.wantEvolveButton());
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)5);
  CHECK(!p.evoBoost);  // gastada en esta evolucion
  // la siguiente vuelve a esperar sus 5 dias
  mockAdvanceMillis(EVOLVE_ANIM_MS + 1);
  CHECK(!p.canEvolveNow());

  p.bag[ITEM_POTION_EVO] = 1;
  p.ageMinutes = EVO2_AGE_MIN;  // ya puede: no hace falta
  CHECK(!p.useEvoPotion());
  CHECK_EQ(p.bag[ITEM_POTION_EVO], (uint16_t)1);
  Pet f;
  makePet(f, 6);  // CHARIZARD: forma final
  f.bag[ITEM_POTION_EVO] = 1;
  CHECK(!f.useEvoPotion());
  CHECK_EQ(f.bag[ITEM_POTION_EVO], (uint16_t)1);
}

TEST(evolve, la_cuenta_viaja_con_la_granja) {
  Pet a;
  makePet(a, 4);
  a.evoCount = 1;
  a.evoBoost = true;
  PetRec r;
  a.exportRec(r);
  Pet b;
  makePet(b, 7);
  b.importRec(r);
  CHECK_EQ(b.evoCount, (uint8_t)1);
  CHECK(b.evoBoost);
}

TEST(evolve, no_evoluciona_dormido_ni_de_huevo_ni_en_ceremonia) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = EVO1_AGE_MIN;
  CHECK(p.canEvolveNow());
  p.toggleLight();
  CHECK(!p.canEvolveNow());
  p.toggleLight();
  p.startFarewell();
  CHECK(!p.canEvolveNow());

  Pet e;
  mockNvsReset();
  e.begin();
  CHECK(!e.canEvolveNow());
}

TEST(evolve, transforma_registra_y_anima) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = EVO1_AGE_MIN;
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)5);
  CHECK_EQ(p.prevSpeciesId, (int16_t)4);
  CHECK(p.isRegistered(5));
  CHECK(p.evolving());
  CHECK_RANGE(p.evolveT(), 0.0f, 1.0f);
  mockAdvanceMillis(EVOLVE_ANIM_MS + 1);
  CHECK(!p.evolving());
}

TEST(evolve, forma_final_no_evoluciona) {
  Pet p;
  makePet(p, 6);  // CHARIZARD
  p.ageMinutes = 200 * 60;
  CHECK(!p.canEvolveNow());
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)6);
}

TEST(evolve, eevee_se_ramifica_en_sus_ocho_evoluciones) {
  std::set<int> vistas;
  for (int seed = 1; seed <= 80; seed++) {
    randomSeed(seed);
    Pet p;
    makePet(p, 133);  // EEVEE
    p.ageMinutes = EVO1_AGE_MIN;
    CHECK(p.canEvolveNow());
    p.evolve();
    vistas.insert(p.speciesId);
    CHECK(p.isRegistered(p.speciesId));
  }
  CHECK(vistas == (std::set<int>{ 134, 135, 136, 196, 197, 470, 471, 700 }));
}

TEST(evolve, la_rama_prefiere_la_que_falta_en_la_pokedex) {
  for (int seed = 1; seed <= 12; seed++) {
    randomSeed(seed);
    Pet p;
    makePet(p, 44);  // GLOOM -> VILEPLUME (45) / BELLOSSOM (182)
    p.dexReg[(45 - 1) >> 3] |= (uint8_t)(1 << ((45 - 1) & 7));
    p.ageMinutes = EVO1_AGE_MIN;
    CHECK(p.canEvolveNow());
    p.evolve();
    CHECK_EQ(p.speciesId, (int16_t)182);
  }
}

TEST(evolve, boton_de_evolucion_se_pospone_al_declinar) {
  Pet p;
  makePet(p, 4);
  p.ageMinutes = EVO1_AGE_MIN;
  CHECK(p.wantEvolveButton());
  p.declineEvolve();
  CHECK_MSG(!p.wantEvolveButton(), "'mantener forma' quita el boton");
  CHECK_MSG(p.canEvolveNow(), "pero la evolucion sigue disponible");
  p.ageMinutes = EVO1_AGE_MIN + 60;  // sube de nivel: se re-ofrece
  CHECK(p.wantEvolveButton());
}

// ---------------------------------------------------------------- stats
TEST(stats, formula_base_por_genes_mas_nivel_y_entreno) {
  Pet p;
  makePet(p, 4);  // CHARMANDER: atk 52, def 43, spe 65
  p.geneAtk = p.geneDef = p.geneSpe = 100;
  p.trAtk = p.trDef = p.trSpe = 0;
  p.ageMinutes = 0;  // nivel 1
  CHECK_EQ(p.atkStat(), (uint16_t)(52 + 1));
  CHECK_EQ(p.defStat(), (uint16_t)(43 + 1));
  CHECK_EQ(p.speStat(), (uint16_t)(65 + 1));
  p.ageMinutes = 9 * 60;  // nivel 10
  p.trAtk = 20;
  CHECK_EQ(p.atkStat(), (uint16_t)(52 + 10 + 20));
  p.geneAtk = 110;
  CHECK_EQ(p.atkStat(), (uint16_t)(52 * 110 / 100 + 10 + 20));
}

TEST(stats, el_huevo_no_tiene_stats) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK_EQ(p.atkStat(), (uint16_t)0);
  CHECK_EQ(p.defStat(), (uint16_t)0);
  CHECK_EQ(p.speStat(), (uint16_t)0);
}

TEST(potion, la_pocion_de_fuerza_sube_5_y_se_gasta) {
  Pet p;
  makePet(p, 4);
  p.bag[ITEM_POTION_STR] = 2;
  CHECK(p.usePotion(ITEM_POTION_STR));
  CHECK_EQ(p.trAtk, (uint8_t)POTION_GAIN);
  CHECK_EQ(p.bag[ITEM_POTION_STR], (uint16_t)1);
  CHECK(p.usePotion(ITEM_POTION_STR));
  CHECK_EQ(p.trAtk, (uint8_t)(2 * POTION_GAIN));
  CHECK_MSG(!p.usePotion(ITEM_POTION_STR), "sin pociones no hace nada");
  CHECK_EQ(p.trAtk, (uint8_t)(2 * POTION_GAIN));
}

TEST(potion, la_de_velocidad_sube_la_velocidad) {
  Pet p;
  makePet(p, 4);
  p.bag[ITEM_POTION_SPD] = 1;
  CHECK(p.usePotion(ITEM_POTION_SPD));
  CHECK_EQ(p.trSpe, (uint8_t)POTION_GAIN);
  CHECK_EQ(p.trAtk, (uint8_t)0);
}

TEST(potion, topa_en_100_y_al_tope_no_se_gasta) {
  Pet p;
  makePet(p, 4);
  p.trAtk = 98;
  p.bag[ITEM_POTION_STR] = 2;
  CHECK(p.usePotion(ITEM_POTION_STR));
  CHECK_EQ(p.trAtk, (uint8_t)100);
  CHECK_MSG(!p.usePotion(ITEM_POTION_STR), "al 100 no se gasta otra");
  CHECK_EQ(p.bag[ITEM_POTION_STR], (uint16_t)1);
}

TEST(potion, un_huevo_no_puede_tomar_pociones) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.bag[ITEM_POTION_STR] = 1;
  CHECK(!p.usePotion(ITEM_POTION_STR));
  CHECK_EQ(p.bag[ITEM_POTION_STR], (uint16_t)1);
}

TEST(play, jugar_sube_felicidad_y_cansa) {
  Pet p;
  makePet(p, 4);
  setStats(p, 50, 50, 50, 50);
  p.play();
  CHECK_EQ(p.joy, (uint8_t)75);
  CHECK_EQ(p.energy, (uint8_t)40);
  CHECK_EQ(p.fullness, (uint8_t)45);
  CHECK(p.showHeart());
}

TEST(care, limpiar_quita_cacas_y_deja_higiene_a_tope) {
  Pet p;
  makePet(p, 4);
  p.poops = 3;
  p.hygiene = 10;
  p.clean();
  CHECK_EQ(p.poops, (uint8_t)0);
  CHECK_EQ(p.hygiene, (uint8_t)100);
}

TEST(care, mimar_sube_felicidad_y_vinculo) {
  Pet p;
  makePet(p, 4);
  p.joy = 50;
  p.caress();
  CHECK_EQ(p.joy, (uint8_t)55);
  CHECK_EQ(p.bond, (uint8_t)1);
  CHECK(p.showHeart());
}

TEST(care, el_vinculo_tiene_tope_diario) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 50; i++) p.play();  // +2 de vinculo cada vez
  CHECK_MSG(p.bond <= 20, "tope de 20 puntos de vinculo al dia");
  CHECK_EQ(p.bond, (uint8_t)20);
}

TEST(mood, refleja_el_estado) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 80);
  CHECK_EQ((int)p.mood(), (int)MOOD_HAPPY);
  p.joy = 20;
  CHECK_EQ((int)p.mood(), (int)MOOD_SAD);
  p.joy = 80;
  p.feed();
  CHECK_EQ((int)p.mood(), (int)MOOD_EATING);
  mockAdvanceMillis(EAT_ANIM_MS + 1);
  p.toggleLight();
  CHECK_EQ((int)p.mood(), (int)MOOD_SLEEPING);
}

// ---------------------------------------------------------------- enfermedad
// bien cuidado (limpio, sin sobrepeso, necesidades altas) no enferma nunca
TEST(sick, bien_cuidado_no_enferma) {
  Pet p;
  makePet(p, 4);
  randomSeed(11);
  for (int i = 0; i < 1500; i++) {
    setStats(p, 90, 90, 90, 90);
    p.poops = 0;
    advance(p, 1);
    CHECK_MSG(!p.sick, "enfermo sin motivo");
  }
}

TEST(sick, con_cacas_acaba_enfermando) {
  Pet p;
  makePet(p, 4);
  randomSeed(12);
  for (int i = 0; i < 3000 && !p.sick; i++) {
    setStats(p, 90, 90, 90, 90);
    p.poops = 3;
    advance(p, 1);
  }
  CHECK_MSG(p.sick, "3 cacas durante 50 h y nunca enfermo");
}

TEST(sick, la_medicina_cura_y_amarga) {
  Pet p;
  makePet(p, 4);
  p.sick = true;
  p.sickMinutes = 30;
  p.joy = 60;
  p.bag[ITEM_MEDICINE] = 1;
  p.giveMedicine();
  CHECK(!p.sick);
  CHECK_EQ(p.bag[ITEM_MEDICINE], (uint16_t)0);
  CHECK_EQ(p.sickMinutes, (uint16_t)0);
  CHECK_EQ(p.joy, (uint8_t)55);
}

TEST(sick, la_medicina_a_un_sano_no_hace_nada) {
  Pet p;
  makePet(p, 4);
  p.joy = 60;
  p.bag[ITEM_MEDICINE] = 1;
  p.giveMedicine();
  CHECK(!p.sick);
  CHECK_EQ(p.joy, (uint8_t)60);
  CHECK_EQ(p.bag[ITEM_MEDICINE], (uint16_t)1);  // no se gasta
}

TEST(sick, sin_medicina_en_la_mochila_no_se_cura) {
  Pet p;
  makePet(p, 4);
  p.sick = true;
  p.giveMedicine();
  CHECK(p.sick);
}

TEST(sick, sin_curar_cuenta_un_descuido_cada_2_horas) {
  Pet p;
  makePet(p, 4);
  p.sick = true;
  p.points = 0;  // sin puntos ni medicina: no puede curarse solo
  p.bag[ITEM_MEDICINE] = 0;
  uint8_t antes = p.careMistakes;
  for (int i = 0; i < 2 * SICK_MISTAKE_MIN; i++) {
    setStats(p, 90, 90, 90, 90);  // sin otros descuidos
    p.poops = 0;
    advance(p, 1);
  }
  CHECK(p.sick);  // sin medicina ni puntos no se cura solo
  CHECK_EQ(p.careMistakes, (uint8_t)(antes + 2));
}

// un minuto con la misma semilla, sano y enfermo (el reloj simulado es global:
// cada mascota se crea y avanza por separado)
static void unMinuto(Pet &p, bool enfermo) {
  makePet(p, 4);
  p.sick = enfermo;
  setStats(p, 80, 80, 80, 100);
  p.poops = 0;
  randomSeed(5);
  advance(p, 1);
}

TEST(sick, enfermo_la_comida_y_el_animo_caen_mas_rapido) {
  Pet sano, malo;
  unMinuto(sano, false);
  unMinuto(malo, true);
  CHECK_EQ(malo.fullness, (uint8_t)(sano.fullness - 1));
  CHECK_EQ(malo.joy, (uint8_t)(sano.joy - 1));
}

TEST(sick, dormido_ni_enferma_ni_cuenta_el_tiempo) {
  Pet p;
  makePet(p, 4);
  p.toggleLight();
  CHECK(p.sleeping);
  randomSeed(13);
  for (int i = 0; i < 600; i++) {
    p.poops = 3;
    p.weight = 90;
    advance(p, 1);
  }
  CHECK(!p.sick);
  p.sick = true;
  p.sickMinutes = 10;
  advance(p, 300);
  CHECK_EQ(p.sickMinutes, (uint16_t)10);
}

TEST(sick, apagado_no_enferma) {
  Pet p;
  makePet(p, 4);
  p.poops = 3;
  p.weight = 90;
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 3 * 86400);  // 3 dias apagado
  CHECK(!p.sick);
}

TEST(sick, se_guarda_y_se_recupera) {
  Pet p;
  makePet(p, 4);
  p.sick = true;
  p.sickMinutes = 77;
  p.bag[ITEM_CANDY] = 1;
  p.feedCandy();  // cualquier accion guarda
  Pet q;
  q.begin();
  CHECK(q.sick);
  CHECK_EQ(q.sickMinutes, (uint16_t)77);
}

TEST(sick, un_huevo_nuevo_nace_sano) {
  Pet p;
  makePet(p, 4);
  p.sick = true;
  p.newEgg();
  CHECK(!p.sick);
  CHECK_EQ(p.sickMinutes, (uint16_t)0);
}

// ---------------------------------------------------------------- granja
static void aLaGranja(Pet &p, int16_t dex) {
  p.farmReg[(dex - 1) >> 3] |= (uint8_t)(1 << ((dex - 1) & 7));
}

TEST(farm, un_huevo_nunca_sale_de_una_especie_de_la_granja) {
  Pet p;
  makePet(p, 4);
  // todas las comunes en la granja menos Pidgey (16): de un comun solo sale el
  for (int d = 1; d <= DEX_COUNT; d++)
    if (DEX_TBL[d].rarity == R_COMUN && d != 16) aLaGranja(p, d);
  for (int i = 0; i < 500; i++) {
    randomSeed(i * 7919 + 3);
    int16_t d = p.pickEggSpecies();
    CHECK_MSG(!p.inFarm(d), "salio una especie que ya esta en la granja");
    if (DEX_TBL[d].rarity == R_COMUN) CHECK_EQ(d, (int16_t)16);
  }
}

TEST(farm, no_evoluciona_a_una_especie_de_la_granja) {
  Pet p;
  makePet(p, 4);  // CHARMANDER -> CHARMELEON (5)
  p.ageMinutes = EVO1_AGE_MIN;
  CHECK(p.canEvolveNow());
  aLaGranja(p, 5);
  CHECK_MSG(!p.canEvolveNow(), "Charmeleon esta en la granja");
  CHECK(!p.wantEvolveButton());
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)4);
}

TEST(farm, la_rama_evita_la_de_la_granja_aunque_falte_en_la_pokedex) {
  for (int seed = 1; seed <= 12; seed++) {
    randomSeed(seed);
    Pet p;
    makePet(p, 44);  // GLOOM -> VILEPLUME (45) / BELLOSSOM (182)
    aLaGranja(p, 182);
    p.ageMinutes = EVO1_AGE_MIN;
    p.evolve();
    CHECK_EQ(p.speciesId, (int16_t)45);
  }
}

TEST(farm, con_todas_las_ramas_en_la_granja_no_evoluciona) {
  Pet p;
  makePet(p, 44);
  aLaGranja(p, 45);
  aLaGranja(p, 182);
  p.ageMinutes = 40 * 60;
  CHECK(!p.canEvolveNow());
}

TEST(farm, guardar_y_sacar_conserva_al_individuo) {
  Pet p;
  makePet(p, 25);
  p.shiny = true;
  p.ageMinutes = 1234;
  p.geneAtk = 93; p.trSpe = 17; p.bond = 42; p.careMistakes = 3;
  p.sick = true; p.sickMinutes = 50;
  p.rename("PIKA");
  PetRec r;
  p.exportRec(r);
  CHECK_EQ((int)r.ver, PET_REC_VER);

  Pet q;
  makePet(q, 1);
  q.importRec(r);
  CHECK_EQ(q.speciesId, (int16_t)25);
  CHECK(q.shiny);
  CHECK_EQ(q.ageMinutes, (uint32_t)1234);
  CHECK_EQ(q.geneAtk, (uint8_t)93);
  CHECK_EQ(q.trSpe, (uint8_t)17);
  CHECK_EQ(q.bond, (uint8_t)42);
  CHECK_EQ(q.careMistakes, (uint8_t)3);
  CHECK(q.sick);
  CHECK_EQ(q.sickMinutes, (uint16_t)50);
  CHECK_STREQ(q.nick, "PIKA");
  CHECK(q.fromFarm);
  CHECK(!q.isEgg());
}

TEST(farm, sacado_de_la_granja_no_aplica_tiempo_al_volver) {
  Pet p;
  makePet(p, 25);
  p.ageMinutes = 500;
  PetRec r;
  p.exportRec(r);
  mockAdvanceMillis(100u * PET_TICK_MS);  // tiempo que paso "en la granja"
  Pet q;
  makePet(q, 1);
  mockAdvanceMillis(50u * PET_TICK_MS);
  q.importRec(r);
  q.update(millis());  // sin ticks pendientes
  CHECK_EQ(q.ageMinutes, (uint32_t)500);
}

TEST(farm, recoger_es_un_final_neutro_y_trae_huevo) {
  Pet p;
  makePet(p, 4);
  p.fromFarm = true;
  p.startCollect();
  CHECK_EQ((int)p.ceremony, (int)CER_COLLECT);
  CHECK_EQ((int)p.lastEnd, (int)CER_COLLECT);
  mockAdvanceMillis(CEREMONY_MS + 10);
  p.update(millis());
  CHECK(p.isEgg());
  CHECK(!p.fromFarm);
}

TEST(farm, el_flag_de_la_granja_se_guarda) {
  Pet p;
  makePet(p, 4);
  PetRec r;
  p.exportRec(r);
  p.importRec(r);  // guarda
  Pet q;
  q.begin();
  CHECK(q.fromFarm);
}

// ---------------------------------------------------------------- tienda
TEST(shop, se_empieza_con_10_puntos) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK_EQ(p.points, (int32_t)START_POINTS);
  CHECK_EQ(START_POINTS, 10);
}

TEST(shop, comprar_descuenta_y_llena_la_mochila) {
  Pet p;
  makePet(p, 4);
  p.points = 12;
  CHECK(p.buy(ITEM_EGG_COMMON));
  CHECK_EQ(p.points, (int32_t)2);
  CHECK_EQ(p.bag[ITEM_EGG_COMMON], (uint16_t)1);
  CHECK(p.buy(ITEM_CANDY));
  CHECK_EQ(p.points, (int32_t)0);
  CHECK_MSG(!p.buy(ITEM_CANDY), "sin puntos no se compra");
  CHECK_EQ(p.bag[ITEM_CANDY], (uint16_t)1);
}

TEST(shop, los_precios_son_los_acordados) {
  CHECK_EQ(ITEM_PRICE[ITEM_CANDY], (uint16_t)2);
  CHECK_EQ(ITEM_PRICE[ITEM_MEDICINE], (uint16_t)3);
  CHECK_EQ(ITEM_PRICE[ITEM_POTION_STR], (uint16_t)5);
  CHECK_EQ(ITEM_PRICE[ITEM_POTION_SPD], (uint16_t)5);
  CHECK_EQ(ITEM_PRICE[ITEM_SHINY_CHARM], (uint16_t)50);
  CHECK_EQ(ITEM_PRICE[ITEM_EGG_COMMON], (uint16_t)10);
  CHECK_EQ(ITEM_PRICE[ITEM_EGG_RARE], (uint16_t)30);
  CHECK_EQ(ITEM_PRICE[ITEM_EGG_LEGEND], (uint16_t)100);
}

TEST(shop, puntos_y_mochila_se_guardan) {
  Pet p;
  makePet(p, 4);
  p.points = 77;
  CHECK(p.buy(ITEM_MEDICINE));
  Pet q;
  q.begin();
  CHECK_EQ(q.points, (int32_t)74);
  CHECK_EQ(q.bag[ITEM_MEDICINE], (uint16_t)1);
}

TEST(shop, los_puntos_no_bajan_de_cero) {
  Pet p;
  makePet(p, 4);
  p.points = 3;
  p.addPoints(-10);
  CHECK_EQ(p.points, (int32_t)0);
  p.addPoints(25);
  CHECK_EQ(p.points, (int32_t)25);
}

// tras un final no sale huevo solo: el hueco espera a un huevo de la mochila
static void dejarHueco(Pet &p) {
  makePet(p, 6);
  p.startCollect();
  mockAdvanceMillis(CEREMONY_MS + 1);
  p.update(millis());
}

TEST(egg, sin_huevo_en_la_mochila_el_hueco_sigue_vacio) {
  Pet p;
  dejarHueco(p);
  CHECK(p.vacant);
  uint32_t edad = p.ageMinutes;
  advance(p, 600);
  CHECK(p.vacant);
  CHECK_EQ(p.ageMinutes, edad);  // sin compañero no pasa el tiempo
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_MSG(p.vacant && p.speciesId < 0, "tocar el hueco no hace nada");
  CHECK(!p.hatchEgg(EGG_COMMON));
}

TEST(egg, abrir_un_huevo_de_la_mochila) {
  Pet p;
  dejarHueco(p);
  p.bag[ITEM_EGG_RARE] = 1;
  CHECK(p.hatchEgg(EGG_RARE));
  CHECK(!p.vacant);
  CHECK(p.isEgg());
  CHECK_EQ(p.bag[ITEM_EGG_RARE], (uint16_t)0);
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK(!p.isEgg());
}

TEST(egg, con_compañero_no_se_abren_huevos) {
  Pet p;
  makePet(p, 4);
  p.bag[ITEM_EGG_COMMON] = 1;
  CHECK(!p.hatchEgg(EGG_COMMON));
  CHECK_EQ(p.bag[ITEM_EGG_COMMON], (uint16_t)1);
}

TEST(egg, apagado_el_hueco_no_eclosiona) {
  Pet p;
  dejarHueco(p);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 86400);
  CHECK(p.vacant);
}

TEST(egg, sacar_de_la_granja_ocupa_el_hueco) {
  Pet a;
  makePet(a, 25);
  PetRec r;
  a.exportRec(r);
  Pet p;
  dejarHueco(p);
  p.importRec(r);
  CHECK(!p.vacant);
  CHECK_EQ(p.speciesId, (int16_t)25);
}

// probabilidad de cada tipo de huevo, sin bonus (racha 0, vinculo 0, sin despedida)
static void tiradas(uint8_t kind, int registradas, double &raro, double &leg) {
  Pet p;
  makePet(p, 4);
  memset(p.dexReg, 0, sizeof(p.dexReg));
  for (int d = 1; d <= registradas; d++) p.dexReg[(d - 1) >> 3] |= (uint8_t)(1 << ((d - 1) & 7));
  p.streak = 0;
  p.bond = 0;
  p.lastEnd = CER_NONE;
  int nr = 0, nl = 0, N = 6000;
  for (int i = 0; i < N; i++) {
    randomSeed(i * 2654435761u + kind);
    uint8_t r = DEX_TBL[p.pickEggSpecies(kind)].rarity;
    if (r == R_RARO) nr++;
    if (r == R_LEGENDARIO) nl++;
  }
  raro = 100.0 * nr / N;
  leg = 100.0 * nl / N;
}

TEST(egg, cada_tipo_de_huevo_tiene_su_probabilidad) {
  for (uint8_t k = 0; k < EGG_KINDS; k++) {
    double raro, leg;
    tiradas(k, 30, raro, leg);
    char msg[96];
    snprintf(msg, sizeof(msg), "huevo %d: raro %.1f%% (esperado %d), legendario %.1f%% (esperado %d)",
             k, raro, EGG_RARE_PCT[k], leg, EGG_LEG_PCT[k]);
    CHECK_MSG(raro > EGG_RARE_PCT[k] - 2.5 && raro < EGG_RARE_PCT[k] + 2.5, msg);
    CHECK_MSG(leg > EGG_LEG_PCT[k] - 1.5 && leg < EGG_LEG_PCT[k] + 1.5, msg);
  }
  CHECK_EQ(EGG_RARE_PCT[EGG_COMMON], (uint8_t)10);
  CHECK_EQ(EGG_RARE_PCT[EGG_RARE], (uint8_t)20);
  CHECK_EQ(EGG_RARE_PCT[EGG_LEGEND], (uint8_t)30);
  CHECK_EQ(EGG_LEG_PCT[EGG_COMMON], (uint8_t)3);
  CHECK_EQ(EGG_LEG_PCT[EGG_RARE], (uint8_t)6);
  CHECK_EQ(EGG_LEG_PCT[EGG_LEGEND], (uint8_t)10);
}

TEST(egg, el_huevo_comun_no_da_legendarios_hasta_25_registradas) {
  double raro, leg;
  tiradas(EGG_COMMON, 10, raro, leg);
  CHECK_EQ(leg, 0.0);
  tiradas(EGG_LEGEND, 10, raro, leg);
  CHECK_MSG(leg > 5.0, "el huevo legendario si puede, aunque haya pocas registradas");
}

TEST(egg, el_amuleto_shiny_se_gasta_y_multiplica_el_shiny) {
  int conAmuleto = 0, sin = 0, N = 3000;
  for (int i = 0; i < N; i++) {
    Pet p;
    makePet(p, 4);
    p.streak = 0; p.bond = 0; p.lastEnd = CER_NONE;
    randomSeed(i * 7 + 1);
    p.bag[ITEM_SHINY_CHARM] = 1;
    p.newEgg();
    CHECK_EQ(p.bag[ITEM_SHINY_CHARM], (uint16_t)0);
    p.eggTap(); p.eggTap(); p.eggTap();
    if (p.shiny) conAmuleto++;
    randomSeed(i * 7 + 1);
    p.newEgg();
    p.eggTap(); p.eggTap(); p.eggTap();
    if (p.shiny) sin++;
  }
  // 1/48 sin amuleto (~2%), 1/12 con el (~8%)
  CHECK_MSG(sin * 100 / N <= 4, "sin amuleto ~2%");
  CHECK_MSG(conAmuleto * 100 / N >= 6, "con amuleto ~8%");
}

// ---------------------------------------------------------------- pokedex
TEST(dexreg, bitmap_cubre_toda_la_pokedex) {
  Pet p;
  makePet(p, 4);
  memset(p.dexReg, 0, sizeof(p.dexReg));
  CHECK_EQ(p.registeredCount(), (uint16_t)0);
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    p.dexReg[(d - 1) >> 3] |= (uint8_t)(1 << ((d - 1) & 7));
    CHECK(p.isRegistered(d));
  }
  CHECK_EQ(p.registeredCount(), (uint16_t)DEX_COUNT);
}

TEST(dexreg, fuera_de_rango_no_esta_registrado) {
  Pet p;
  makePet(p, 4);
  memset(p.dexReg, 0xFF, sizeof(p.dexReg));
  CHECK(!p.isRegistered(0));
  CHECK(!p.isRegistered(-1));
  CHECK(!p.isRegistered(DEX_COUNT + 1));
  CHECK(!p.isRegistered(32767));
  CHECK_EQ(p.registeredCount(), (uint16_t)DEX_COUNT);
}

TEST(dexreg, linea_incompleta_se_detecta) {
  Pet p;
  makePet(p, 1);  // BULBASAUR -> IVYSAUR -> VENUSAUR
  CHECK(p.lineHasUnregistered(1));
  p.dexReg[(2 - 1) >> 3] |= (uint8_t)(1 << ((2 - 1) & 7));
  p.dexReg[(3 - 1) >> 3] |= (uint8_t)(1 << ((3 - 1) & 7));
  CHECK_MSG(!p.lineHasUnregistered(1), "linea 1-2-3 completa");
}

TEST(dexreg, la_rama_de_eevee_cuenta_las_ocho) {
  Pet p;
  makePet(p, 133);  // EEVEE
  CHECK(p.lineHasUnregistered(133));
  for (int16_t d : { 134, 135, 136, 196, 197, 470, 471 })
    p.dexReg[(d - 1) >> 3] |= (uint8_t)(1 << ((d - 1) & 7));
  CHECK_MSG(p.lineHasUnregistered(133), "falta SYLVEON (700)");
  p.dexReg[(700 - 1) >> 3] |= (uint8_t)(1 << ((700 - 1) & 7));
  CHECK(!p.lineHasUnregistered(133));
}

TEST(dexreg, una_rama_con_su_propia_evolucion_cuenta_entera) {
  Pet p;
  makePet(p, 265);  // WURMPLE -> SILCOON -> BEAUTIFLY / CASCOON -> DUSTOX
  for (int16_t d : { 265, 266, 267, 268 })
    p.dexReg[(d - 1) >> 3] |= (uint8_t)(1 << ((d - 1) & 7));
  CHECK_MSG(p.lineHasUnregistered(265), "falta DUSTOX (269)");
  p.dexReg[(269 - 1) >> 3] |= (uint8_t)(1 << ((269 - 1) & 7));
  CHECK(!p.lineHasUnregistered(265));
}

TEST(egg, el_sorteo_siempre_da_una_especie_valida) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 400; i++) {
    randomSeed(i * 7919 + 1);
    int16_t d = p.pickEggSpecies();
    CHECK_RANGE(d, (int16_t)1, (int16_t)DEX_COUNT);
    CHECK_MSG(DEX_TBL[d].rarity != R_EVO, "de un huevo nunca sale una forma evolucionada");
    CHECK_MSG(DEX_TBL[d].rarity != R_NONE, "de un huevo nunca sale una especie sin sprite");
  }
}

TEST(egg, el_primer_huevo_es_un_inicial_clasico) {
  for (int seed = 1; seed <= 20; seed++) {
    randomSeed(seed);
    mockNvsReset();
    Pet p;
    p.begin();
    int16_t d = p.pickEggSpecies();
    bool classic = false;
    for (int i = 0; i < NUM_CLASSIC_DEX; i++)
      if (CLASSIC_DEX[i] == d) classic = true;
    CHECK_MSG(classic, "sin pokedex, solo salen iniciales clasicos");
  }
}

TEST(egg, una_escapada_castiga_la_rareza) {
  Pet p;
  makePet(p, 4);
  p.lastEnd = CER_RUNAWAY;
  for (int seed = 1; seed <= 60; seed++) {
    randomSeed(seed);
    int16_t d = p.pickEggSpecies();
    CHECK_MSG(DEX_TBL[d].rarity == R_COMUN, "tras una escapada solo salen comunes");
  }
}

// ---------------------------------------------------------------- racha
TEST(streak, el_primer_cuidado_del_dia_suma) {
  Pet p;
  makePet(p, 4);
  p.setClock(100u * 86400);
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)1);
  CHECK_EQ(p.bestStreak, (uint16_t)1);
  p.caress();
  CHECK_MSG(p.streak == 1, "el segundo cuidado del mismo dia no suma");
  p.setClock(101u * 86400);
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)2);
}

TEST(streak, un_dia_perdido_reinicia) {
  Pet p;
  makePet(p, 4);
  p.setClock(100u * 86400);
  p.caress();
  p.setClock(101u * 86400);
  p.caress();
  p.setClock(105u * 86400);  // hueco de dias
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)1);
  CHECK_MSG(p.bestStreak == 2, "el record de racha se conserva");
}

TEST(streak, sin_reloj_no_hay_racha) {
  Pet p;
  makePet(p, 4);
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)0);
}

TEST(streak, celebra_los_hitos) {
  Pet p;
  makePet(p, 4);
  for (uint32_t d = 100; d < 103; d++) {
    p.setClock(d * 86400);
    p.caress();
  }
  CHECK_EQ(p.streak, (uint16_t)3);
  CHECK_MSG(p.showMilestone(), "hito de 3 dias celebrado");
  CHECK_EQ(p.lastMilestone, (uint16_t)3);
}

TEST(bonus, racha_y_vinculo_mejoran_el_huevo) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.careBonus(), 0);
  p.streak = 30;
  p.bond = 100;
  CHECK_EQ(p.careBonus(), 14);
  p.streak = 300;  // la racha se capa a 30
  CHECK_EQ(p.careBonus(), 14);
}

// ---------------------------------------------------------------- medallas
TEST(medals, nivel_10_da_medalla) {
  Pet p;
  makePet(p, 4);
  CHECK(!p.hasMedal(MED_LV10));
  p.ageMinutes = 9 * 60 - 1;
  advance(p, 1);
  CHECK(p.hasMedal(MED_LV10));
  CHECK(p.showMedal());
}

// sin sobrepeso, nivel 5 y cero descuidos
TEST(medals, en_forma_llega_al_nivel_5) {
  Pet p;
  makePet(p, 4);
  p.weight = 0;
  p.ageMinutes = 4 * 60 - 1;
  advance(p, 1);
  CHECK(p.hasMedal(MED_FIT));
  CHECK_EQ(p.totalMedals, (uint16_t)1);

  Pet q;
  makePet(q, 4);
  q.careMistakes = 1;   // un descuido la deja fuera
  q.ageMinutes = 4 * 60 - 1;
  advance(q, 1);
  CHECK(!q.hasMedal(MED_FIT));
}

TEST(medals, cada_medalla_nueva_suma_al_total) {
  Pet p;
  makePet(p, 4);
  p.weight = 0;
  p.ageMinutes = 9 * 60 - 1;
  advance(p, 1);  // cruza nivel 10: MED_LV10 + MED_FIT de golpe
  CHECK(p.hasMedal(MED_LV10));
  CHECK(p.hasMedal(MED_FIT));
  CHECK_MSG(p.totalMedals == 2, "el total cuenta cada bit ganado, no cada evento");
}

TEST(medals, forma_final_al_nacer) {
  Pet p;
  makePet(p, 143);  // SNORLAX ya es forma final
  CHECK(p.hasMedal(MED_FINAL));
  CHECK_EQ(p.totalMedals, (uint16_t)1);
}

TEST(medals, curarse_con_medicina_da_medalla) {
  Pet p;
  makePet(p, 4);
  p.sick = true;
  p.bag[ITEM_MEDICINE] = 1;
  p.giveMedicine();
  CHECK(!p.sick);
  CHECK(p.hasMedal(MED_CURED));
  CHECK(p.showMedal());
}

TEST(medals, legendario_y_shiny_al_nacer) {
  Pet p;
  makePet(p, 150);  // MEWTWO: legendario y forma final
  CHECK(p.hasMedal(MED_LEGEND));
  CHECK(p.hasMedal(MED_FINAL));
  Pet q;
  makePet(q, 4);
  CHECK(!q.hasMedal(MED_LEGEND));
  q.shiny = true;
  advance(q, 1);
  CHECK(q.hasMedal(MED_SHINY));
}

TEST(medals, pociones_hasta_50_dan_fuerza_y_velocidad) {
  Pet p;
  makePet(p, 4);
  p.bag[ITEM_POTION_STR] = 10;
  p.bag[ITEM_POTION_SPD] = 9;
  for (int i = 0; i < 10; i++) p.usePotion(ITEM_POTION_STR);
  for (int i = 0; i < 9; i++) p.usePotion(ITEM_POTION_SPD);
  advance(p, 1);
  CHECK_EQ(p.trAtk, (uint8_t)MED_TRAIN_MIN);
  CHECK(p.hasMedal(MED_STRONG));
  CHECK_MSG(!p.hasMedal(MED_SWIFT), "45 de velocidad aun no llega");
}

TEST(medals, racha_de_30_dias) {
  Pet p;
  makePet(p, 4);
  p.streak = 29;
  advance(p, 1);
  CHECK(!p.hasMedal(MED_STREAK30));
  p.streak = 30;
  advance(p, 1);
  CHECK(p.hasMedal(MED_STREAK30));
  CHECK(p.hasMedal(MED_STREAK7));
}

TEST(medals, el_ajedrez_da_medallas_al_compañero_presente) {
  Pet p;
  makePet(p, 4);
  p.chessMedals(0, false);  // solo derrotas o tablas: nada
  CHECK(!p.hasMedal(MED_CHESS));
  CHECK(!p.hasMedal(MED_RECORD));
  p.chessMedals(2, false);
  CHECK(p.hasMedal(MED_CHESS));
  CHECK(!p.hasMedal(MED_RECORD));
  p.chessMedals(0, true);
  CHECK(p.hasMedal(MED_RECORD));

  mockNvsReset();
  Pet e;
  e.begin();  // huevo: no hay compañero que la reciba
  e.chessMedals(3, true);
  CHECK_EQ(e.medals, (uint16_t)0);
}

TEST(medals, la_ficha_enseña_15_medallas_sin_el_bit_retirado) {
  uint16_t all = 0;
  for (int i = 0; i < MED_COUNT; i++) {
    uint16_t b = medalBit(i);
    CHECK_MSG(b != MED_RETIRED, "la medalla de la baya ya no se enseña");
    CHECK_MSG(!(all & b), "cada medalla tiene su bit");
    all |= b;
  }
  CHECK_EQ(all, (uint16_t)~MED_RETIRED);
}

// la de la baya (bit 3) de una partida o una granja anteriores se borra
TEST(medals, la_medalla_de_la_baya_vieja_se_borra) {
  Pet a;
  makePet(a, 4);
  a.medals = MED_RETIRED | MED_LV10;
  a.rename("OLD");  // guarda
  Pet b;
  b.begin();
  CHECK_EQ(b.medals, (uint16_t)MED_LV10);

  PetRec r;
  a.exportRec(r);
  r.medals = MED_RETIRED | MED_BOND;
  Pet c;
  makePet(c, 7);
  c.importRec(r);
  CHECK_EQ(c.medals, (uint16_t)MED_BOND);
}

TEST(medals, el_total_acumula_entre_crianzas) {
  Pet p;
  makePet(p, 143);
  CHECK_EQ(p.totalMedals, (uint16_t)1);
  p.newEgg();
  p.chooseStarter(128);  // TAUROS, otra forma final (no legendaria)
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_MSG(p.medals == MED_FINAL, "las medallas del individuo se reinician");
  CHECK_MSG(p.totalMedals == 2, "el total del jugador se conserva");
}

// ---------------------------------------------------------------- ceremonias
TEST(ceremony, despedida_solo_en_forma_final_y_a_los_3_dias) {
  Pet p;
  makePet(p, 6);  // CHARIZARD, forma final
  p.ageMinutes = FAREWELL_AGE_MIN - 1;
  CHECK(!p.canFarewellNow());
  p.ageMinutes = FAREWELL_AGE_MIN;
  CHECK(p.canFarewellNow());

  Pet q;
  makePet(q, 4);  // CHARMANDER no es forma final
  q.ageMinutes = FAREWELL_AGE_MIN * 3;
  CHECK(!q.canFarewellNow());
}

TEST(ceremony, la_despedida_deja_el_hueco_sin_huevo) {
  Pet p;
  makePet(p, 6);
  p.ageMinutes = FAREWELL_AGE_MIN;
  p.startFarewell();
  CHECK_EQ(p.ceremony, (uint8_t)CER_FAREWELL);
  CHECK_RANGE(p.ceremonyT(), 0.0f, 1.0f);
  mockAdvanceMillis(CEREMONY_MS + 1);
  p.update(millis());
  CHECK(p.vacant);  // ya no sale huevo solo: hay que abrir uno de la mochila
  CHECK(p.isEgg());  // isEgg() cubre el hueco: bloquea las acciones igual
  CHECK_EQ(p.ceremony, (uint8_t)CER_NONE);
  CHECK_EQ(p.lastEnd, (uint8_t)CER_FAREWELL);
}

TEST(ceremony, escaparse_pide_una_hora_de_abandono_total) {
  Pet p;
  makePet(p, 4);
  setStats(p, 0, 0, 0, 0);
  CHECK(!p.canRunawayNow());
  advance(p, RUNAWAY_TICKS - 1);
  CHECK(!p.canRunawayNow());
  advance(p, 1);
  CHECK(p.canRunawayNow());
}

TEST(ceremony, un_solo_cuidado_salva_del_abandono) {
  Pet p;
  makePet(p, 4);
  p.dbgRunawayReady();
  CHECK(p.canRunawayNow());
  p.feed();
  advance(p, 1);
  CHECK_MSG(!p.canRunawayNow(), "comer resetea el contador de abandono");
}

TEST(ceremony, durante_la_ceremonia_no_se_puede_interactuar) {
  Pet p;
  makePet(p, 4);
  setStats(p, 50, 50, 50, 50);
  p.poops = 2;
  p.startRunaway();
  p.feed();
  p.feedCandy();
  p.play();
  p.clean();
  p.caress();
  p.toggleLight();
  CHECK_EQ(p.fullness, (uint8_t)50);
  CHECK_EQ(p.joy, (uint8_t)50);
  CHECK_EQ(p.poops, (uint8_t)2);
  CHECK(!p.sleeping);
}

TEST(ceremony, soltar_marca_el_final_como_liberacion) {
  Pet p;
  makePet(p, 4);
  p.release();
  CHECK_EQ(p.ceremony, (uint8_t)CER_RELEASE);
  CHECK_EQ(p.lastEnd, (uint8_t)CER_RELEASE);
  mockAdvanceMillis(CEREMONY_MS + 1);
  p.update(millis());
  CHECK(p.isEgg());
}

TEST(ceremony, un_huevo_no_se_despide_ni_se_suelta) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.startFarewell();
  p.release();
  p.startRunaway();
  CHECK_EQ(p.ceremony, (uint8_t)CER_NONE);
}

// ---------------------------------------------------------------- offline
// apagado cuenta como dormido aunque estuviera despierto: descansa y al
// encender sigue despierto
TEST(offline, apagado_duerme_y_recupera_energia) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 20, 100);
  CHECK(!p.sleeping);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 600 * 60);  // 600 minutos apagado
  CHECK_EQ(p.ageMinutes, (uint32_t)600);
  CHECK_EQ(p.energy, (uint8_t)100);
  CHECK_MSG(p.fullness == 30, "suelo de comida dormido = 30");
  CHECK_MSG(p.joy == 35, "suelo de felicidad dormido = 35");
  CHECK_MSG(p.hygiene == 45, "suelo de higiene dormido = 45");
  CHECK_MSG(p.careMistakes == 0, "en ausencia no se acumulan descuidos");
  CHECK_MSG(p.poops == 0, "durmiendo no ensucia");
  CHECK_MSG(!p.sleeping, "al encender sigue como estaba: despierto");
}

// pantalla apagada con el boton PWR: se duerme, y al encenderla despierta
TEST(sleep, pantalla_apagada_duerme_y_encendida_despierta) {
  Pet p;
  makePet(p, 4);
  CHECK(!p.sleeping);
  p.screenNap(true);
  CHECK(p.sleeping);
  p.energy = 20;
  advance(p, 5);
  CHECK_EQ(p.energy, (uint8_t)50);  // +6 por minuto, como dormido
  p.screenNap(false);
  CHECK(!p.sleeping);
}

TEST(sleep, si_ya_dormia_sigue_dormido_al_encender) {
  Pet p;
  makePet(p, 4);
  p.toggleLight();  // a dormir con la luz
  p.screenNap(true);
  p.screenNap(false);
  CHECK_MSG(p.sleeping, "lo durmio el jugador: no se despierta solo");
  Pet q;
  makePet(q, 4);
  q.screenNap(false);  // encender sin haberlo dormido no cambia nada
  CHECK(!q.sleeping);
}

// apagar la pantalla, reiniciar (corte, reflasheo) y encender: tiene que
// acordarse de que dormia por la pantalla y despertar (antes seguia dormido
// hasta que alguien le diera a la luz, y sin cuidarse solo)
TEST(sleep, tras_reiniciar_la_siesta_de_pantalla_tambien_despierta) {
  Pet p;
  makePet(p, 4);
  p.lastSeenEpoch = 1000u * 86400 + 15 * 3600;  // las 3 de la tarde
  p.screenNap(true);
  CHECK(p.sleeping);
  Pet q;
  q.begin();
  q.lastSeenEpoch = p.lastSeenEpoch;
  CHECK(q.sleeping);
  q.screenNap(false);  // arranca con la pantalla encendida
  CHECK_MSG(!q.sleeping, "dormia solo por la pantalla apagada");
}

TEST(sleep, el_huevo_no_duerme_al_apagar_la_pantalla) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.screenNap(true);
  CHECK(!p.sleeping);
}

TEST(offline, tope_de_dos_semanas) {
  Pet p;
  makePet(p, 4);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 90u * 86400);  // 90 dias
  CHECK_EQ(p.ageMinutes, (uint32_t)(14u * 24 * 60));
}

TEST(offline, menos_de_dos_minutos_no_hace_nada) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 100);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 90);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
  CHECK_EQ(p.fullness, (uint8_t)80);
}

TEST(offline, el_huevo_eclosiona_en_tu_ausencia) {
  Pet p;
  makePet(p, 4);
  p.newEgg();  // ya hay pokedex: no pide inicial
  CHECK(!p.awaitingStarter());
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 600);  // 10 minutos
  CHECK(!p.isEgg());
  CHECK_RANGE(p.speciesId, (int16_t)1, (int16_t)DEX_COUNT);
}

TEST(offline, durmiendo_se_descansa_tambien_apagado) {
  Pet p;
  makePet(p, 4);
  setStats(p, 90, 90, 20, 90);
  p.toggleLight();
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 120 * 60);
  CHECK_EQ(p.energy, (uint8_t)100);
  CHECK_MSG(p.fullness >= 30, "suelos de sueno tambien offline");
  CHECK_MSG(p.poops == 0, "durmiendo no ensucia");
}

TEST(offline, sin_reloj_no_se_aplica_nada) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 100);
  p.syncClock(0);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
  CHECK_EQ(p.fullness, (uint8_t)80);
}

// ---------------------------------------------------------------- guardado
TEST(save, el_estado_sobrevive_a_un_reinicio) {
  Pet a;
  makePet(a, 25);  // PIKACHU
  a.ageMinutes = 1234;
  a.streak = 9;
  a.bestStreak = 11;
  a.bond = 44;
  a.trAtk = 17;
  a.weight = 33;
  a.rename("SPARKY");  // rename persiste todo el estado

  Pet b;
  b.begin();  // mismo "NVS": simula un reinicio
  CHECK_EQ(b.speciesId, (int16_t)25);
  CHECK_EQ(b.ageMinutes, (uint32_t)1234);
  CHECK_EQ(b.streak, (uint16_t)9);
  CHECK_EQ(b.bestStreak, (uint16_t)11);
  CHECK_EQ(b.bond, (uint8_t)44);
  CHECK_EQ(b.trAtk, (uint8_t)17);
  CHECK_EQ(b.weight, (uint8_t)33);
  CHECK_STREQ(b.nick, "SPARKY");
  CHECK(b.isRegistered(25));
  CHECK(!b.awaitingStarter());
}

TEST(save, el_apodo_se_recorta_a_11_caracteres) {
  Pet p;
  makePet(p, 4);
  p.rename("NOMBRE-LARGUISIMO-QUE-NO-CABE");
  CHECK_EQ((int)strlen(p.nick), 11);
  CHECK_STREQ(p.nick, "NOMBRE-LARG");
}

TEST(save, borrado_de_fabrica_deja_todo_como_nuevo) {
  Pet a;
  makePet(a, 4);
  a.factoryReset();
  Pet b;
  b.begin();
  CHECK(b.isEgg());
  CHECK(b.awaitingStarter());
  CHECK_EQ(b.registeredCount(), (uint16_t)0);
}

TEST(save, el_autoguardado_se_marca_y_se_vuelca) {
  Pet p;
  makePet(p, 4);
  CHECK(!p.savePending());
  advance(p, 5);
  CHECK_MSG(p.savePending(), "cada 5 ticks queda un guardado pendiente");
  p.flushSave();
  CHECK(!p.savePending());
}

// ---------------------------------------------------------------- regiones
// registra todo Kanto menos `falta` (como si se hubieran criado antes)
static void casiKanto(Pet &p, int16_t falta) {
  for (int d = 1; d <= 151; d++)
    if (d != falta) p.dexReg[(d - 1) >> 3] |= (1 << ((d - 1) & 7));
}

TEST(region, completar_kanto_da_puntos_y_un_huevo_shiny_legendario) {
  mockNvsReset();
  mockSetMillis(0);
  Pet p;
  p.begin();
  casiKanto(p, 4);
  p.chooseStarter(4);
  int32_t antes = p.points;
  p.eggTap(); p.eggTap(); p.eggTap();  // nace CHARMANDER: el ultimo que faltaba
  CHECK(p.regionDone & 1);
  CHECK(p.regionEggs & 1);
  CHECK_EQ(p.points, antes + REGION_REWARD_PTS);
  CHECK_EQ(p.regionJustDone, (uint8_t)1);
  CHECK(p.showRegionDone());

  // sin compañero se abre: legendario de Kanto y shiny
  p.startCollect();
  mockAdvanceMillis(CEREMONY_MS + 1);
  p.update(millis());
  CHECK(p.vacant);
  CHECK(p.hatchRegionEgg());
  CHECK(!(p.regionEggs & 1));
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_RANGE(p.speciesId, (int16_t)1, (int16_t)151);
  CHECK_EQ(DEX_TBL[p.speciesId].rarity, (uint8_t)R_LEGENDARIO);
  CHECK(p.shiny);
}

TEST(region, solo_se_premia_una_vez) {
  mockNvsReset();
  mockSetMillis(0);
  Pet p;
  p.begin();
  casiKanto(p, 4);
  p.chooseStarter(4);
  p.eggTap(); p.eggTap(); p.eggTap();
  int32_t pts = p.points;
  p.checkRegionOf(4);
  CHECK_EQ(p.points, pts);
}

TEST(region, incompleta_no_premia_y_con_compañero_no_se_abre) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.regionDone, (uint16_t)0);
  p.regionEggs = 1;
  CHECK_MSG(!p.hatchRegionEgg(), "hay compañero: el huevo espera en la mochila");
  CHECK_EQ(p.regionEggs, (uint16_t)1);
}

TEST(region, el_huevo_de_region_respeta_la_granja_y_no_gasta_el_amuleto) {
  mockNvsReset();
  mockSetMillis(0);
  Pet p;
  p.begin();
  // los 5 legendarios de Kanto en la granja: sale uno de fuera de Kanto
  for (int d : { 144, 145, 146, 150, 151 }) p.farmReg[(d - 1) >> 3] |= (1 << ((d - 1) & 7));
  p.chooseStarter(1);
  p.eggTap(); p.eggTap(); p.eggTap();
  p.startCollect();
  mockAdvanceMillis(CEREMONY_MS + 1);
  p.update(millis());
  p.regionEggs = 1;
  p.bag[ITEM_SHINY_CHARM] = 1;
  CHECK(p.hatchRegionEgg());
  CHECK_EQ(p.bag[ITEM_SHINY_CHARM], (uint16_t)1);
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK(!p.inFarm(p.speciesId));
  CHECK_EQ(DEX_TBL[p.speciesId].rarity, (uint8_t)R_LEGENDARIO);
}

TEST(region, se_guarda) {
  Pet a;
  makePet(a, 4);
  a.regionDone = 0x5;
  a.regionEggs = 0x4;
  a.rename("X");
  Pet b;
  b.begin();
  CHECK_EQ(b.regionDone, (uint16_t)0x5);
  CHECK_EQ(b.regionEggs, (uint16_t)0x4);
}

// ---------------------------------------------------------------- tiempo de fuera
// 10 min despiertos con tiempo `pw`: misma semilla en cada pasada, asi las
// cacas al azar salen igual y solo cambia el efecto del tiempo
static uint8_t tras10min(int16_t dex, uint8_t pw, bool energia) {
  Pet p;
  makePet(p, dex);
  p.setWeather(pw);
  setStats(p, 90, 60, 60, 90);
  p.ageMinutes = 100;  // pasa por 105 y 110 (cada 5 min)
  randomSeed(1234);
  advance(p, 10);
  return energia ? p.energy : p.joy;
}

TEST(weather, la_lluvia_alegra_al_de_agua_y_disgusta_al_de_fuego) {
  Pet agua;
  makePet(agua, 7);  // SQUIRTLE (playa)
  agua.setWeather(PW_RAIN);
  CHECK_EQ(agua.weatherFeel(), (int8_t)1);
  Pet fuego;
  makePet(fuego, 4);  // CHARMANDER (volcan)
  fuego.setWeather(PW_RAIN);
  CHECK_EQ(fuego.weatherFeel(), (int8_t)-1);
  CHECK_EQ(tras10min(7, PW_RAIN, false), (uint8_t)(tras10min(7, PW_NONE, false) + 2));
  CHECK_EQ(tras10min(4, PW_RAIN, false), (uint8_t)(tras10min(4, PW_NONE, false) - 2));
}

TEST(weather, la_nieve_da_frio_salvo_a_los_de_hielo) {
  Pet p;
  makePet(p, 1);  // BULBASAUR
  p.setWeather(PW_SNOW);
  CHECK(p.feelsCold());
  CHECK_EQ(tras10min(1, PW_SNOW, true), (uint8_t)(tras10min(1, PW_NONE, true) - 2));
  Pet hielo;
  makePet(hielo, 144);  // ARTICUNO (nieve)
  hielo.setWeather(PW_SNOW);
  CHECK(!hielo.feelsCold());
  CHECK_EQ(hielo.weatherFeel(), (int8_t)1);
}

TEST(weather, el_sol_anima_a_los_de_planta_y_sin_dato_no_pasa_nada) {
  Pet p;
  makePet(p, 1);
  p.setWeather(PW_SUN);
  CHECK_EQ(p.weatherFeel(), (int8_t)1);
  p.setWeather(PW_NONE);
  CHECK_EQ(p.weatherFeel(), (int8_t)0);
  Pet q;
  makePet(q, 4);
  q.setWeather(PW_SUN);
  CHECK_EQ(q.weatherFeel(), (int8_t)0);
}

// ---------------------------------------------------------------- pasos
TEST(steps, un_punto_cada_mil_hasta_cinco_al_dia) {
  Pet p;
  makePet(p, 4);
  p.lastSeenEpoch = 1000u * 86400 + 3600;
  int32_t base = p.points;
  p.addSteps(999);
  CHECK_EQ(p.points, base);
  p.addSteps(1);
  CHECK_EQ(p.points, base + 1);
  p.addSteps(9000);
  CHECK_EQ(p.stepsToday, (uint32_t)10000);
  CHECK_EQ(p.points, base + STEP_PTS_MAX);
  // dia nuevo: vuelve a contar
  p.lastSeenEpoch += 86400;
  p.addSteps(1500);
  CHECK_EQ(p.stepsToday, (uint32_t)1500);
  CHECK_EQ(p.points, base + STEP_PTS_MAX + 1);
}

TEST(steps, el_paseo_alegra_una_vez_al_dia) {
  Pet p;
  makePet(p, 4);
  p.lastSeenEpoch = 1000u * 86400;
  p.joy = 50;
  p.addSteps(STEP_JOY_AT - 1);
  CHECK_EQ(p.joy, (uint8_t)50);
  p.addSteps(1);
  CHECK_EQ(p.joy, (uint8_t)65);
  p.addSteps(5000);
  CHECK_EQ(p.joy, (uint8_t)65);
}

TEST(steps, se_guardan) {
  Pet a;
  makePet(a, 4);
  a.lastSeenEpoch = 1000u * 86400;
  a.addSteps(2345);
  a.rename("X");
  Pet b;
  b.begin();
  CHECK_EQ(b.stepsToday, (uint32_t)2345);
  CHECK_EQ(b.stepPts, (uint8_t)2);
}

// ---------------------------------------------------------------- album
TEST(memoir, cada_final_deja_una_pagina) {
  Pet p;
  makePet(p, 4);
  MemRec r;
  CHECK(!p.takeMemoir(r));
  p.lastSeenEpoch = 1000u * 86400 + 600;
  p.rename("FUEGO");
  p.medals = MED_LV10 | MED_BOND;
  p.release();
  CHECK(p.takeMemoir(r));
  CHECK_EQ(r.endType, (uint8_t)CER_RELEASE);
  CHECK_EQ(r.speciesId, (int16_t)4);
  CHECK_EQ(r.endEpoch, (uint32_t)(1000u * 86400 + 600));
  CHECK_EQ(r.medals, (uint16_t)(MED_LV10 | MED_BOND));
  CHECK_STREQ(r.nick, "FUEGO");
  CHECK_MSG(!p.takeMemoir(r), "solo una vez");
}

TEST(memoir, la_fecha_de_nacimiento_viaja_con_la_granja) {
  mockNvsReset();
  mockSetMillis(0);
  Pet p;
  p.begin();
  p.lastSeenEpoch = 1000u * 86400 + 7200;
  p.chooseStarter(4);
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_EQ(p.bornEpoch, (uint32_t)(1000u * 86400 + 7200));
  PetRec r;
  p.exportRec(r);
  CHECK_EQ(r.ver, (uint8_t)PET_REC_VER);
  Pet q;
  makePet(q, 7);
  q.importRec(r);
  CHECK_EQ(q.bornEpoch, (uint32_t)(1000u * 86400 + 7200));
  r.ver = 1;  // ficha antigua: sin fecha
  q.importRec(r);
  CHECK_EQ(q.bornEpoch, (uint32_t)0);
}

// ---------------------------------------------------------------- caracter
TEST(nature, cada_caracter_tiene_su_mania) {
  Pet p;
  makePet(p, 4);
  CHECK(p.nature < NAT_COUNT);
  p.nature = NAT_GLUTTON;
  p.fullness = 50;
  p.feed();
  CHECK_EQ(p.fullness, (uint8_t)80);  // +30 en vez de +25
  mockAdvanceMillis(EAT_ANIM_MS + 1);
  p.nature = NAT_PLAYFUL;
  p.joy = 50;
  p.play();
  CHECK_EQ(p.joy, (uint8_t)80);
  p.nature = NAT_TIDY;
  p.joy = 50;
  p.clean();
  CHECK_EQ(p.joy, (uint8_t)58);
}

TEST(nature, al_gloton_le_baja_mas_la_comida) {
  auto comida = [](uint8_t nat) {
    Pet p;
    makePet(p, 4);
    p.nature = nat;
    setStats(p, 90, 90, 90, 90);
    p.ageMinutes = 100;
    randomSeed(77);
    advance(p, 8);  // pasa por 104 y 108
    return p.fullness;
  };
  CHECK_EQ(comida(NAT_GLUTTON), (uint8_t)(comida(NAT_EASY) - 2));
}

TEST(nature, viaja_con_la_granja_y_se_guarda) {
  Pet a;
  makePet(a, 4);
  a.nature = NAT_LAZY;
  PetRec r;
  a.exportRec(r);
  Pet b;
  makePet(b, 7);
  b.importRec(r);
  CHECK_EQ(b.nature, (uint8_t)NAT_LAZY);
  b.rename("Z");
  Pet c;
  c.begin();
  CHECK_EQ(c.nature, (uint8_t)NAT_LAZY);
}

// ---------------------------------------------------------------- complementos
TEST(accessory, se_compra_una_vez_y_se_pone_y_quita) {
  Pet p;
  makePet(p, 4);
  p.points = 20;
  CHECK(!p.buyAccessory(ACC_CROWN));  // 80: no llega
  CHECK(p.buyAccessory(ACC_BOW));
  CHECK_EQ(p.points, (int32_t)5);
  CHECK_EQ(p.accWorn, (uint8_t)(ACC_BOW + 1));
  CHECK_MSG(!p.buyAccessory(ACC_BOW), "ya lo tiene");
  p.wearAccessory(ACC_BOW);
  CHECK_EQ(p.accWorn, (uint8_t)0);
  p.wearAccessory(ACC_CAP);  // no lo tiene
  CHECK_EQ(p.accWorn, (uint8_t)0);
  p.rename("Q");
  Pet q;
  q.begin();
  CHECK_EQ(q.accOwned, (uint8_t)(1 << ACC_BOW));
}

// ---------------------------------------------------------------- noche
TEST(night, se_duerme_solo_tarde_y_se_levanta_por_la_manana) {
  Pet p;
  makePet(p, 4);
  p.nightCheck(22 * 60, true);
  CHECK(!p.sleeping);
  p.nightCheck(23 * 60 + 5, false);  // aun la estan usando
  CHECK(!p.sleeping);
  p.nightCheck(23 * 60 + 5, true);
  CHECK(p.sleeping);
  CHECK(p.autoSlept);
  p.nightCheck(3 * 60, true);
  CHECK(p.sleeping);
  p.nightCheck(7 * 60, true);
  CHECK(!p.sleeping);
  CHECK(!p.autoSlept);
}

TEST(night, si_lo_duerme_el_jugador_no_lo_despierta) {
  Pet p;
  makePet(p, 4);
  p.toggleLight();
  p.nightCheck(8 * 60, true);
  CHECK_MSG(p.sleeping, "lo durmio el jugador: sigue durmiendo");
}

TEST(night, de_noche_encender_la_pantalla_no_lo_despierta) {
  Pet p;
  makePet(p, 4);
  p.lastSeenEpoch = 1000u * 86400 + 2 * 3600;  // las 2 de la madrugada
  p.screenNap(true);
  p.screenNap(false);
  CHECK(p.sleeping);
  CHECK(p.autoSlept);
  p.nightCheck(7 * 60 + 1, true);
  CHECK(!p.sleeping);
}

// ---------------------------------------------------------------- crecer en la granja
TEST(farm, en_la_granja_crece_a_un_decimo_y_vuelve_sano) {
  Pet a;
  makePet(a, 4);
  a.lastSeenEpoch = 1000u * 86400;
  a.ageMinutes = 7 * 60;  // Lv.8
  a.sick = true;
  a.fullness = 5;
  a.poops = 3;
  PetRec r;
  a.exportRec(r);
  CHECK_EQ(r.farmEpoch, (uint32_t)(1000u * 86400));
  CHECK_EQ(r.ver, (uint8_t)PET_REC_VER);
  // 3 dias despues: 72 h / 10 = 7,2 h mas de edad
  Pet b;
  makePet(b, 7);
  b.lastSeenEpoch = 1000u * 86400 + 3 * 86400;
  b.importRec(r);
  CHECK_EQ(b.ageMinutes, (uint32_t)(7 * 60 + 3 * 24 * 60 / FARM_GROWTH_DIV));
  CHECK_EQ(b.level(), (uint16_t)15);
  CHECK(!b.sick);
  CHECK(b.fullness >= 80);
  CHECK_EQ(b.poops, (uint8_t)0);
  CHECK_EQ(b.speciesId, (int16_t)4);  // sin evolucionar (CHARMANDER evoluciona al 16)
}

TEST(farm, el_nivel_de_la_granja_se_calcula_con_la_hora) {
  CHECK_EQ(farmAge(600, 0, 2000000), (uint32_t)600);  // sin fecha de entrada: no crece
  CHECK_EQ(farmAge(600, 1000000, 1000000 + 36000), (uint32_t)(600 + 60));  // 10 h -> 1 h
  CHECK_EQ(levelOfAge(farmAge(0, 1000000, 1000000 + 10 * 3600 * 5)), (uint16_t)6);
}

TEST(farm, sacarlo_enseguida_no_lo_cambia) {
  Pet a;
  makePet(a, 4);
  a.lastSeenEpoch = 1000u * 86400;
  a.fullness = 30;
  a.sick = true;
  PetRec r;
  a.exportRec(r);
  Pet b;
  makePet(b, 7);
  b.lastSeenEpoch = 1000u * 86400 + 600;  // 10 min: ni crece ni se cura
  b.importRec(r);
  CHECK_EQ(b.fullness, (uint8_t)30);
  CHECK(b.sick);
}

// ---------------------------------------------------------------- caricia
TEST(stroke, acariciar_alegra_mas_que_tocar) {
  Pet p;
  makePet(p, 4);
  p.joy = 50;
  p.caress();
  CHECK_EQ(p.joy, (uint8_t)55);
  p.stroke();
  CHECK_EQ(p.joy, (uint8_t)63);
  CHECK(p.showHeart());
  p.toggleLight();
  p.stroke();
  CHECK_EQ(p.joy, (uint8_t)63);  // dormido no
}

// ---------------------------------------------------------------- cosecha
TEST(harvest, una_vez_al_dia_desde_las_7_y_poco) {
  Pet p;
  makePet(p, 4);
  uint8_t pts, item;
  int32_t base = p.points;
  p.lastSeenEpoch = 1000u * 86400 + 6 * 3600;  // 6:00: aun no
  CHECK(!p.farmHarvest(12, pts, item));
  p.lastSeenEpoch = 1000u * 86400 + 7 * 3600;
  mockForceRandom(99);  // sin chuche ni medicina
  CHECK(p.farmHarvest(12, pts, item));
  mockClearForcedRandom();
  CHECK_EQ(pts, (uint8_t)2);  // 12 / 5
  CHECK_EQ(item, (uint8_t)0xFF);
  CHECK_EQ(p.points, base + 2);
  CHECK_MSG(!p.farmHarvest(12, pts, item), "solo una vez al dia");
  p.lastSeenEpoch += 86400;
  mockForceRandom(99);
  CHECK(p.farmHarvest(40, pts, item));
  mockClearForcedRandom();
  CHECK_EQ(pts, (uint8_t)HARVEST_MAX);
}

TEST(harvest, con_pocos_no_hay_puntos_pero_a_veces_algo) {
  Pet p;
  makePet(p, 4);
  uint8_t pts, item;
  p.lastSeenEpoch = 1000u * 86400 + 8 * 3600;
  mockForceRandom(3);  // < 15: una chuche
  CHECK(p.farmHarvest(4, pts, item));
  mockClearForcedRandom();
  CHECK_EQ(pts, (uint8_t)0);
  CHECK_EQ(item, (uint8_t)ITEM_CANDY);
  CHECK_EQ(p.bag[ITEM_CANDY], (uint16_t)1);
  p.lastSeenEpoch += 86400;
  CHECK(!p.farmHarvest(0, pts, item));  // granja vacia: nada
}

TEST(gestures, demasiadas_cosquillas_agobian) {
  Pet p;
  makePet(p, 4);
  p.joy = 50;
  p.pester();
  CHECK_EQ(p.joy, (uint8_t)44);
  p.toggleLight();
  p.pester();
  CHECK_EQ(p.joy, (uint8_t)44);  // dormido no se entera
}

// ---------------------------------------------------------------- se cura solo
TEST(selfmed, con_medicina_en_la_mochila_se_la_toma_solo) {
  Pet p;
  makePet(p, 4);
  setStats(p, 90, 90, 90, 90);
  p.sick = true;
  p.sickMinutes = 0;
  p.bag[ITEM_MEDICINE] = 1;
  p.points = 50;
  advance(p, SELF_MED_MIN - 1);
  CHECK(p.sick);  // aun no
  advance(p, 2);
  CHECK(!p.sick);
  CHECK_EQ(p.bag[ITEM_MEDICINE], (uint16_t)0);
  CHECK_EQ(p.points, (int32_t)50);  // no gasto puntos
  CHECK(p.hasMedal(MED_CURED));
  CHECK(p.takeSelfMed());
  CHECK(!p.takeSelfMed());
}

TEST(selfmed, sin_medicina_no_la_compra_con_los_puntos) {
  Pet p;
  makePet(p, 4);
  setStats(p, 90, 90, 90, 90);
  p.sick = true;
  p.bag[ITEM_MEDICINE] = 0;
  p.points = 50;
  advance(p, SELF_MED_MIN + 5);
  CHECK(p.sick);
  CHECK_EQ(p.points, (int32_t)50);
  CHECK_EQ(p.bag[ITEM_MEDICINE], (uint16_t)0);
  CHECK(!p.takeSelfMed());
}

TEST(selfmed, dormido_no_se_la_toma) {
  Pet p;
  makePet(p, 4);
  p.sick = true;
  p.bag[ITEM_MEDICINE] = 1;
  p.toggleLight();
  advance(p, SELF_MED_MIN + 5);
  CHECK(p.sick);
}

TEST(selfcare, siesta_y_juego_solo) {
  Pet p;
  makePet(p, 4);
  p.energy = 50;
  p.joy = 50;
  p.selfRest();
  CHECK_EQ(p.energy, (uint8_t)54);
  p.selfPlay();
  CHECK_EQ(p.joy, (uint8_t)54);
  CHECK_EQ(p.energy, (uint8_t)53);
}

// ---------------------------------------------------------------- se cuida solo
TEST(guardian, sin_corriente_o_atendido_no_hace_nada) {
  Pet p;
  makePet(p, 4);
  setStats(p, 30, 30, 30, 30);
  p.poops = 0;
  mockForceRandom(99);
  advance(p, 1);
  mockClearForcedRandom();
  CHECK(p.fullness < AUTO_CARE_FROM);
}

TEST(guardian, se_cuida_hasta_la_mitad_de_una_en_una) {
  Pet p;
  makePet(p, 4);
  p.setGuardian(true);
  setStats(p, 20, 20, 20, 20);
  p.poops = 0;
  uint16_t racha = p.streak;
  mockForceRandom(99);  // sin cacas nuevas
  advance(p, 1);
  CHECK_EQ(p.fullness, (uint8_t)AUTO_CARE_LEVEL);  // lo primero: comer
  CHECK(p.hygiene < AUTO_CARE_LEVEL);              // lo demas, en los minutos siguientes
  CHECK(p.eating());
  advance(p, 1);
  CHECK(p.hygiene >= AUTO_CARE_LEVEL - 1);
  CHECK_EQ(p.selfBathSeq, (uint16_t)1);
  advance(p, 2);
  mockClearForcedRandom();
  CHECK(p.energy >= AUTO_CARE_LEVEL - 2);
  CHECK(p.joy >= AUTO_CARE_LEVEL - 2);
  CHECK(p.fullness < 100);  // nunca lo llena del todo
  CHECK_EQ(p.streak, racha);  // no es cuidado del jugador
}

TEST(guardian, dos_cacas_las_limpia_aunque_este_limpio) {
  Pet p;
  makePet(p, 4);
  p.setGuardian(true);
  setStats(p, 90, 90, 90, 90);
  p.poops = 2;
  mockForceRandom(99);
  advance(p, 1);
  mockClearForcedRandom();
  CHECK_EQ(p.poops, (uint8_t)0);
  CHECK(p.hygiene > AUTO_CARE_LEVEL);  // no lo baja a la mitad
}

// lo mismo que en sick.con_cacas_acaba_enfermando, pero cuidandose solo: nunca
TEST(guardian, cuidandose_solo_no_enferma) {
  Pet p;
  makePet(p, 4);
  p.setGuardian(true);
  p.weight = 200;  // sobrepeso: eso no lo arregla el solo
  randomSeed(12);
  for (int i = 0; i < 3000; i++) {
    setStats(p, 15, 15, 15, 15);
    p.poops = 3;
    advance(p, 1);
    CHECK_MSG(!p.sick, "enfermo cuidandose solo");
  }
}

// ---------------------------------------------------------------- regiones en orden
static void registrarHuevosDe(Pet &p, int r) {
  for (int d = REGIONS[r].first; d <= REGIONS[r].last; d++)
    if (DEX_TBL[d].rarity == R_COMUN || DEX_TBL[d].rarity == R_RARO)
      p.dexReg[(d - 1) >> 3] |= (1 << ((d - 1) & 7));
}

TEST(unlock, al_principio_solo_kanto) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.unlockedRegions(), (uint8_t)1);
  for (int i = 0; i < 300; i++) {
    randomSeed(i * 31 + 5);
    int16_t d = p.pickEggSpecies(i % 3 == 0 ? EGG_LEGEND : EGG_RARE);
    CHECK_MSG(d >= 1 && d <= 151, "un huevo trajo una especie de una region cerrada");
  }
}

TEST(unlock, las_de_huevo_de_kanto_abren_johto) {
  Pet p;
  makePet(p, 4);
  registrarHuevosDe(p, 0);
  CHECK(p.regionHatchDone(0));
  CHECK_EQ(p.unlockedRegions(), (uint8_t)2);
  bool johto = false;
  for (int i = 0; i < 400 && !johto; i++) {
    randomSeed(i * 17 + 1);
    int16_t d = p.pickEggSpecies(EGG_COMMON);
    CHECK(d <= 251);
    johto = d >= 152;
  }
  CHECK_MSG(johto, "con Johto abierta ya salen de alli");
  // sin las de evolucion ni legendarios no esta completa (sin premio)
  CHECK_EQ(p.regionDone, (uint16_t)0);
}

TEST(unlock, en_orden_aunque_se_registre_algo_de_mas_adelante) {
  Pet p;
  makePet(p, 4);
  registrarHuevosDe(p, 2);  // Hoenn entera, sin Kanto: sigue cerrada
  CHECK_EQ(p.unlockedRegions(), (uint8_t)1);
  registrarHuevosDe(p, 0);
  registrarHuevosDe(p, 1);
  CHECK_EQ(p.unlockedRegions(), (uint8_t)4);  // Kanto, Johto, Hoenn hechas: abre Sinnoh
}

// ---------------------------------------------------------------- capacidades
// la misma mascota con y sin una capacidad, con la misma semilla
static uint8_t trasMinutos(uint8_t tra, uint8_t trs, bool energia, int min) {
  Pet p;
  makePet(p, 4);
  p.trAtk = tra;
  p.trSpe = trs;
  setStats(p, 95, 95, 95, 95);
  p.ageMinutes = 0;
  randomSeed(99);
  mockForceRandom(99);  // sin cacas ni enfermedad
  advance(p, min);
  mockClearForcedRandom();
  return energia ? p.energy : p.joy;
}

TEST(ability, el_aguante_frena_la_energia) {
  // 100 minutos: sin aguante baja 100; con 100 de aguante (50%) baja 50
  CHECK_EQ(trasMinutos(0, 0, true, 60), (uint8_t)35);
  // con 100 de aguante, de los minutos 1..60 no baja en 1..49: baja 11
  CHECK_EQ(trasMinutos(100, 0, true, 60), (uint8_t)(95 - 11));
  Pet p;
  makePet(p, 4);
  p.trAtk = 60;
  CHECK_EQ(p.staminaPct(), (uint8_t)30);
}

TEST(ability, la_vitalidad_frena_el_animo) {
  CHECK(trasMinutos(0, 100, false, 60) > trasMinutos(0, 0, false, 60));
  Pet p;
  makePet(p, 4);
  p.trSpe = 40;
  CHECK_EQ(p.vitalityPct(), (uint8_t)20);
}

TEST(ability, la_resistencia_rebaja_enfermar) {
  Pet p;
  makePet(p, 4);
  p.trDef = 100;
  CHECK_EQ(p.resistPct(), (uint8_t)60);
  // con riesgo maximo (11 por mil) y la tirada justo en el limite rebajado, no enferma
  setStats(p, 15, 15, 15, 15);
  p.poops = 2;
  p.weight = 60;
  mockForceRandom(11 * 40 / 10);  // 11*(100-60)/10 = 44: no es < 44
  advance(p, 1);
  mockClearForcedRandom();
  CHECK(!p.sick);
}

TEST(stats, cuentan_los_nacidos_y_los_pasos_de_siempre) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.raisedCount, (uint16_t)1);
  p.lastSeenEpoch = 1000u * 86400;
  p.addSteps(1200);
  p.lastSeenEpoch += 86400;  // otro dia: los de hoy empiezan de cero, los de siempre no
  p.addSteps(300);
  CHECK_EQ(p.stepsToday, (uint32_t)300);
  CHECK_EQ(p.stepsTotal, (uint32_t)1500);
  p.release();
  mockAdvanceMillis(CEREMONY_MS + 1);
  p.update(millis());
  p.bag[ITEM_EGG_COMMON] = 1;
  CHECK(p.hatchEgg(EGG_COMMON));
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_EQ(p.raisedCount, (uint16_t)2);
  p.rename("S");
  Pet q;
  q.begin();
  CHECK_EQ(q.raisedCount, (uint16_t)2);
  CHECK_EQ(q.stepsTotal, (uint32_t)1500);
}
