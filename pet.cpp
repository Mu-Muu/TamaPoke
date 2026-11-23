#include "pet.h"
#include "dex.h"
#include "regions.h"  // rangos de cada region (los paisajes no se usan aqui: el compilador los quita)
#include "audio.h"
#include "mega.h"
#include "fun.h"

// libro de puntos (abajo, con el ocio)
static void ptAdd(uint16_t &v, int32_t n);
static void ptLog(Pet &p, uint8_t src, int32_t n);
#include <string.h>
#include <time.h>

static_assert(PET_DEX_MAX == DEX_COUNT, "PET_DEX_MAX (pet.h) no coincide con DEX_COUNT (dex.h)");
static_assert(PET_DEX_BYTES == DEX_BITMAP_BYTES, "bitmap de la pokedex desalineado");

// destinos de una especie como mucho (Eevee: 8)
#define MAX_EVO_TARGETS 12

// destinos de evolucion de una especie: evolvesTo + sus ramas. Devuelve cuantos.
static int evoTargets(int16_t dex, int16_t *out, int max) {
  int n = 0;
  if (dex < 1 || dex > DEX_COUNT || DEX_TBL[dex].evolvesTo == 0) return 0;
  out[n++] = DEX_TBL[dex].evolvesTo;
  for (int i = 0; i < NUM_DEX_BRANCHES && n < max; i++)
    if (DEX_BRANCHES[i].from == dex) out[n++] = DEX_BRANCHES[i].to;
  return n;
}

// destinos que no estan ya en la granja (no puede haber dos de la misma especie)
static int freeTargets(const Pet &p, int16_t dex, int16_t *out) {
  int16_t all[MAX_EVO_TARGETS];
  int nAll = evoTargets(dex, all, MAX_EVO_TARGETS), n = 0;
  for (int i = 0; i < nAll; i++)
    if (!p.inFarm(all[i])) out[n++] = all[i];
  return n;
}

void Pet::begin() {
  prefs.begin("tamapoke", false);
  if (!prefs.getBool("init", false)) {
    prefs.putBool("init", true);
    newEgg();
  } else {
    load();
  }
  lastTick = millis();
}

void Pet::newEgg(uint8_t kind) {
  if (kind >= EGG_KINDS) kind = EGG_COMMON;
  lvFloor = 0;
  vacant = false;
  ceremony = CER_NONE;
  neglectTicks = 0;
  weight = 0;
  speciesId = -1;
  prevSpeciesId = -1;
  eggTarget = pickEggSpecies(kind);  // especie oculta segun rareza y pokedex
  starterPick = (registeredCount() == 0);  // primera partida: el jugador elige inicial
  // sorteo shiny: 1/48 base, mejor con despedida y con racha/vinculo altos;
  // un amuleto shiny de la mochila (se gasta aqui) lo divide por 4
  int shinyBase = (lastEnd == CER_FAREWELL ? 24 : 48) - careBonus();
  if (shinyBase < 8) shinyBase = 8;
  if (bag[ITEM_SHINY_CHARM] > 0) {
    bag[ITEM_SHINY_CHARM]--;
    shinyBase /= 4;
    if (shinyBase < 2) shinyBase = 2;
  }
  eggShiny = (random(shinyBase) == 0);
  eggTaps = 0;
  fullness = 80;
  joy = 80;
  energy = 80;
  hygiene = 100;
  poops = 0;
  ageMinutes = 0;
  careMistakes = 0;
  mistakeCooldown = 0;
  sleeping = false;
  sick = false;
  sickMinutes = 0;
  fromFarm = false;
  save();
}

// suelo: v baja d, pero nunca por debajo de fl (si ya esta por debajo, no baja)
static uint8_t dropTo(uint8_t v, uint8_t d, uint8_t fl) {
  if (v <= fl) return v;
  return (v - fl > d) ? v - d : fl;
}

void Pet::setClock(uint32_t nowEpoch) {
  lastSeenEpoch = nowEpoch;
  if (nowEpoch) save();  // persiste ya: un corte de luz no pierde la referencia
}

void Pet::syncClock(uint32_t nowEpoch) {
  uint32_t seen = prefs.getUInt("seen", 0);
  lastSeenEpoch = nowEpoch;
  if (nowEpoch == 0) return;
  uint32_t mins = (seen && nowEpoch > seen) ? (nowEpoch - seen) / 60 : 0;
  if (mins < 2 || ceremony != CER_NONE || starterPick || vacant) {
    save();  // primera vez, sin tiempo que aplicar o aun eligiendo inicial
    return;
  }
  if (mins > 14UL * 24 * 60) mins = 14UL * 24 * 60;  // tope: 2 semanas

  // apagado = dormido: recupera energia y las necesidades bajan lento y con
  // suelo, como dormido en vivo (sin cacas, descuidos, enfermedad ni
  // escapadas). Al encender sigue como estaba: si estaba despierto, ya desperto.
  // La evolucion NO se aplica offline: queda lista y la dispara el usuario
  // tocando al bicho cuando vuelve (para que vea la transformacion).
  for (uint32_t i = 0; i < mins; i++) {
    ageMinutes++;
    if (isEgg()) {
      if (ageMinutes >= 3) hatch();  // eclosiona en tu ausencia
      continue;
    }
    energy = clamp100(energy + 6);
    if (ageMinutes % 2 == 0) {
      fullness = dropTo(fullness, 1, 30);
      joy = dropTo(joy, 1, 35);
    }
    if (ageMinutes % 3 == 0) hygiene = dropTo(hygiene, 1, 45);
  }
  Serial.printf("offline: %u min aplicados (nv.%u)\n", mins, level());
  save();
}

void Pet::update(uint32_t nowMs) {
  // fin de ceremonia: la criatura se va y NO sale huevo solo; queda el hueco
  // hasta que se abra uno de la mochila o se saque uno de la granja
  if (ceremony != CER_NONE && !timeLeft(ceremonyUntil)) {
    becomeVacant();
    return;
  }
  while (nowMs - lastTick >= PET_TICK_MS) {
    lastTick += PET_TICK_MS;
    tick();
  }
}

void Pet::tick() {
  if (megaUntil && lastSeenEpoch >= megaUntil) {  // paso el dia: vuelve a su forma
    megaUntil = 0;
    megaEvent = 2;
    pendingSave = true;
  }
  if (ceremony != CER_NONE) return;  // el tiempo se detiene en la despedida
  if (vacant) return;                // sin compañero no pasa nada
  if (starterPick) return;  // la partida no empieza hasta elegir inicial: si el
                            // tiempo corriera aqui, el huevo eclosionaria solo a
                            // los 3 min con la especie sorteada y se perderia la
                            // eleccion del jugador
  const uint16_t lvAntes = level();  // para el sonido de subir de nivel
  ageMinutes++;

  if (isEgg()) {
    if (ageMinutes >= 3) hatch();  // si no lo tocas, eclosiona solo a los 3 min
    return;
  }
  // de expedicion (ocio): esta fuera pasandolo bien; ni bajan sus necesidades ni enferma
  // (en la principal no se le puede atender). Crece igual.
  if (questAway()) return;

  // el sueño es descanso: la energia se recupera y las necesidades bajan MUCHO
  // mas lento que despierto y con suelo (amanece pidiendo algo de mimo, no a
  // cero, sin descuidos ni escapadas). despierto: comida -2/min, hig/joy -1/min.
  // El peso aun se quema. La racha de buen cuidado (goodTicks) queda en pausa, salvo en la
  // siesta de pantalla apagada de dia (issue #8: con bateria la pantalla se apaga sola a los
  // 5 min y la resistencia casi no subia); de noche o con la luz apagada es dormir de verdad.
  if (sleeping) {
    energy = clamp100(energy + (nature == NAT_LAZY ? 8 : 6));  // al perezoso le sienta mejor
    if (weight > 0 && ageMinutes % 3 == 0) weight--;
    if (ageMinutes % 2 == 0) {                 // ~4x mas lento que despierto
      fullness = dropTo(fullness, 1, 30);
      joy = dropTo(joy, 1, 35);
    }
    if (ageMinutes % 3 == 0) hygiene = dropTo(hygiene, 1, 45);
    if (napping && !(lastSeenEpoch && isNightMinute((int)((lastSeenEpoch / 60) % 1440)))) forgeEndurance();
    checkMedals();  // aun puede cruzar un nivel por edad mientras duerme
    if (++ticksSinceSave >= 5) pendingSave = true;
    return;
  }

  if (level() > lvAntes) sfxPlay(SFX_LEVEL);  // subio de nivel (despierto)

  fullness = clamp100(fullness - 2);
  // aguante: algunos minutos la energia no baja (staminaPct de cada 100)
  if ((int)(ageMinutes % 100) >= staminaPct()) energy = clamp100(energy - 1);
  if (fullness > 40 && poops < 3 && random(100) < 15) poops++;

  hygiene = clamp100(hygiene - 1 - 4 * poops);
  // el sobrepeso da pereza: la energia cae el doble
  if (weight > 50) energy = clamp100(energy - 1);
  if (weight > 0 && ageMinutes % 3 == 0) weight--;

  forgeEndurance();

  // enfermedad: el mal cuidado da una probabilidad por mil y por minuto de
  // ponerse malo. Enfermo, la comida y el animo caen mas rapido y cada 2 h sin
  // medicina cuentan como un descuido (retrasa la evolucion). Mientras se cuida
  // solo (guardian) no enferma.
  if (!sick) {
    int riesgo = (poops >= 2 ? 4 : 0) + (weight > 50 ? 3 : 0) + (lowestStat() <= 20 ? 4 : 0);
    // resistencia: menos probabilidad (en milesimas x10 para no perder la rebaja)
    if (!guardian && riesgo && (int)random(10000) < riesgo * (100 - resistPct()) / 10) {
      sick = true;
      sickMinutes = 0;
    }
  } else {
    fullness = clamp100(fullness - 1);
    joy = clamp100(joy - 1);
    // al rato se toma una medicina el solo, si hay en la mochila. Nunca gasta
    // puntos: comprarla es cosa del jugador
    if (sickMinutes >= SELF_MED_MIN && bag[ITEM_MEDICINE] > 0) {
      bag[ITEM_MEDICINE]--;  // como giveMedicine, pero no cuenta como cuidado del jugador
      sick = false;
      sickMinutes = 0;
      joy = clamp100(joy - 5);
      checkMedals(MED_CURED);
      selfMed = true;
      pendingSave = true;
    }
    if (sick && ++sickMinutes % SICK_MISTAKE_MIN == 0) {
      if (careMistakes < 255) careMistakes++;  // uint8_t: con tope, que no vuelva a 0
      if (bond > 1) bond--;
    }
  }

  // vitalidad: algunos minutos el animo no baja por si solo
  int dJoy = (int)(ageMinutes % 100) >= vitalityPct() ? -1 : 0;
  if (fullness < 30) dJoy -= 2;
  if (hygiene < 30) dJoy -= 2;
  joy = clamp100(joy + dJoy);

  if (guardian) autoCare();

  // caracter: su necesidad baja 1 mas cada 4 min
  if (ageMinutes % 4 == 0) {
    if (nature == NAT_GLUTTON) fullness = clamp100(fullness - 1);
    else if (nature == NAT_PLAYFUL) joy = clamp100(joy - 1);
    else if (nature == NAT_TIDY) hygiene = clamp100(hygiene - 1);
    else if (nature == NAT_LAZY) energy = clamp100(energy - 1);
  }

  // el tiempo de fuera, un poco cada 5 min: lluvia para los de agua, nieve para
  // los de hielo y sol para los de planta les gusta; la lluvia no a los de
  // fuego y la nieve da frio al resto (cansa)
  if (ageMinutes % 5 == 0) {
    int8_t f = weatherFeel();
    if (f > 0 && weather == PW_SUN) energy = clamp100(energy + 1);
    else if (f > 0) joy = clamp100(joy + 1);
    else if (f < 0 && feelsCold()) energy = clamp100(energy - 1);
    else if (f < 0) joy = clamp100(joy - 1);
  }

  // Descuido: dejar una estadistica por los suelos cuenta como error de
  // cuidado (con enfriamiento para no contar el mismo descuido cada minuto)
  if (mistakeCooldown > 0) mistakeCooldown--;
  if (lowestStat() <= 10 && mistakeCooldown == 0) {
    if (careMistakes < 255) careMistakes++;
    mistakeCooldown = 60;
    if (bond > 1) bond--;  // el descuido enfria el vinculo, pero sin arrasarlo:
                           // a -3 cada 30 min se perdia mucho mas de lo que se
                           // podia ganar en todo un dia y el vinculo se atascaba
  }

  checkMedals();  // la evolucion la dispara el usuario (canEvolveNow + tap), no el tick

  // abandono total: con TODO a cero durante una hora queda lista para escaparse;
  // NO se va sola, la dispara el usuario con el boton (final triste, lo presencia)
  if (fullness == 0 && joy == 0 && energy == 0 && hygiene == 0) {
    if (neglectTicks < RUNAWAY_TICKS) neglectTicks++;
  } else {
    neglectTicks = 0;  // un solo cuidado la salva
  }

  // ciclo completo (forma final + 7 dias): la despedida NO salta sola; queda
  // lista (canFarewellNow) y la dispara el usuario con el boton, para que la vea

  // autoguardado periodico: NO escribir a flash aqui (corre dentro del loop,
  // mientras se anima); solo marcar y dejar que el loop lo vuelque al atenuar
  if (++ticksSinceSave >= 5) pendingSave = true;
}

// vuelca el guardado periodico pendiente (lo llama el loop en un momento sin
// animacion para que el paron de la escritura a flash no se vea)
void Pet::flushSave() {
  if (pendingSave) save();
}

// quedan miembros sin registrar en la linea evolutiva de esta base?
// (recorre todas las ramas: Wurmple -> Silcoon/Cascoon -> Beautifly/Dustox)
static bool lineHasUnreg(const Pet &p, int16_t dex, int depth) {
  if (dex < 1 || dex > DEX_COUNT || depth > 6) return false;
  if (!p.isRegistered(dex)) return true;
  int16_t next[MAX_EVO_TARGETS];
  int n = evoTargets(dex, next, MAX_EVO_TARGETS);
  for (int i = 0; i < n; i++)
    if (lineHasUnreg(p, next[i], depth + 1)) return true;
  return false;
}

bool Pet::lineHasUnregistered(int16_t base) const {
  return lineHasUnreg(*this, base, 0);
}

uint8_t Pet::eggRarity() const {
  return (eggTarget >= 1 && eggTarget <= DEX_COUNT) ? DEX_TBL[eggTarget].rarity : R_COMUN;
}

// elige la especie del huevo: tirada de rareza (mejorada por una despedida
// completa, castigada por una escapada) y sesgo hacia lineas incompletas
int16_t Pet::pickEggSpecies(uint8_t kind) {
  // primera partida: inicial clasico
  if (registeredCount() == 0) {
    return CLASSIC_DEX[random(NUM_CLASSIC_DEX)];
  }
  if (kind >= EGG_KINDS) kind = EGG_COMMON;
  int16_t maxDex = REGIONS[unlockedRegions() - 1].last;
  if (kind == EGG_FOSSIL) {  // un fosil de las regiones abiertas (mejor uno sin registrar)
    static const int16_t FOSIL[] = { 138, 140, 142, 345, 347, 408, 410, 564, 566, 696, 698, 880, 881, 882, 883 };
    for (int pass = 0; pass < 2; pass++) {
      int16_t cand[sizeof(FOSIL) / sizeof(FOSIL[0])];
      int n = 0;
      for (int16_t d : FOSIL)
        if (d <= maxDex && !inFarm(d) && (pass == 1 || lineHasUnregistered(d))) cand[n++] = d;
      if (n > 0) return cand[random(n)];
    }
    return 138;
  }

  // la base la pone el tipo de huevo; el cuidado (racha + vinculo) y una
  // despedida la mejoran. Una escapada quita esas mejoras (antes forzaba
  // comun, pero un huevo comprado conserva su base). En el huevo comun el
  // legendario solo sale con 25+ registradas.
  bool blessed = (lastEnd == CER_FAREWELL);
  int bonus = (lastEnd == CER_RUNAWAY) ? 0 : careBonus();
  int rare = EGG_RARE_PCT[kind] + bonus + (blessed ? 10 : 0);
  int leg = (kind == EGG_COMMON && registeredCount() < 25)
                ? 0 : EGG_LEG_PCT[kind] + bonus / 3 + (blessed ? 3 : 0);
  uint8_t tier = R_COMUN;
  int r = random(100);
  if (r < leg) tier = R_LEGENDARIO;
  else if (r < leg + rare) tier = R_RARO;
  // solo de las regiones abiertas (son rangos seguidos de la pokedex)

  // candidatos del tier con linea incompleta; si no hay, baja de tier;
  // si la pokedex del tier esta completa, vale cualquiera del tier
  for (int pass = 0; pass < 2; pass++) {
    for (int t = tier; t >= R_COMUN; t--) {
      static int16_t cand[DEX_COUNT];  // static: 2 KB fuera de la pila
      int n = 0;
      for (int16_t d = 1; d <= maxDex; d++) {
        if (DEX_TBL[d].rarity != t) continue;
        if (kind == EGG_WATER && typeOf(d) != TY_WATER) continue;  // el de la pesca: de agua
        if (inFarm(d)) continue;  // ya hay uno en la granja (vale para el shiny)
        if (pass == 0 && !lineHasUnregistered(d)) continue;
        cand[n++] = d;
      }
      if (n > 0) return cand[random(n)];
    }
  }
  return CLASSIC_DEX[random(NUM_CLASSIC_DEX)];  // inalcanzable, por si acaso
}

bool Pet::regionHatchDone(int r) const {
  if (r < 0 || r >= REGION_COUNT) return false;
  if (easyRegions) {  // regiones faciles: basta con unas cuantas, de cualquier rareza
    int n = 0;
    for (int d = REGIONS[r].first; d <= REGIONS[r].last; d++)
      if (isRegistered(d) && ++n >= EASY_REGION_REG) return true;
    return false;
  }
  for (int d = REGIONS[r].first; d <= REGIONS[r].last; d++) {
    uint8_t ra = DEX_TBL[d].rarity;
    if ((ra == R_COMUN || ra == R_RARO) && !isRegistered(d)) return false;
  }
  return true;
}

uint8_t Pet::unlockedRegions() const {
  uint8_t n = regionsOpen;  // 1, salvo si se quitaron las regiones faciles: desde la ultima abierta
  while (n < REGION_COUNT && regionHatchDone(n - 1)) n++;
  return n;
}

void Pet::setEasyRegions(bool on) {
  if (on == easyRegions) return;
  uint8_t n = unlockedRegions();  // las abiertas hasta ahora no se cierran al quitarlas
  if (n > regionsOpen) regionsOpen = n;
  easyRegions = on;
  save();
}

void Pet::registerSpecies(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return;
  bool nueva = !isRegistered(dex);
  dexReg[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
  if (shiny) dexShinyReg[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
  if (nueva) checkRegionOf(dex);
}

// region completa (todas sus especies registradas): una sola vez por region
void Pet::checkRegionOf(int16_t dex) {
  for (int r = 0; r < REGION_COUNT; r++) {
    const Region &rg = REGIONS[r];
    if (dex < rg.first || dex > rg.last) continue;
    if (regionDone & (1 << r)) return;
    for (int d = rg.first; d <= rg.last; d++)  // las que no tienen sprite (R_NONE) no se pueden conseguir
      if (!isRegistered(d) && DEX_TBL[d].rarity != R_NONE) return;
    regionDone |= (1 << r);
    regionEggs |= (1 << r);
    earn(REGION_REWARD_PTS, PT_OTHER);
    regionJustDone = r + 1;
    regionUntil = millis() + 6000;
    sfxPlay(SFX_MEDAL);
    save();
    return;
  }
}

// legendario de la region r para su huevo: uno que no este en la granja (la
// granja manda); si no queda ninguno, cualquier legendario fuera de la granja
static int16_t regionLegend(const Pet &p, int r) {
  int16_t opts[64];
  int n = 0;
  for (int pass = 0; pass < 3 && n == 0; pass++) {
    int lo = pass == 0 ? REGIONS[r].first : 1, hi = pass == 0 ? REGIONS[r].last : DEX_COUNT;
    for (int d = lo; d <= hi && n < 64; d++) {
      if (p.inFarm(d)) continue;
      if (pass < 2 && DEX_TBL[d].rarity != R_LEGENDARIO) continue;
      if (pass == 2 && (d < REGIONS[r].first || d > REGIONS[r].last)) continue;  // ultimo recurso: de la region
      opts[n++] = d;
    }
  }
  return n ? opts[random(n)] : REGIONS[r].first;
}

bool Pet::hatchRegionEgg() {
  if (!vacant || !regionEggs) return false;
  int r = 0;
  while (!(regionEggs & (1 << r))) r++;
  regionEggs &= ~(1 << r);
  uint16_t charm = bag[ITEM_SHINY_CHARM];
  newEgg(EGG_LEGEND);
  bag[ITEM_SHINY_CHARM] = charm;  // ya sale shiny: el amuleto no se gasta
  eggTarget = regionLegend(*this, r);
  eggShiny = true;
  save();
  return true;
}

// la racha y el vinculo mejoran el sorteo del huevo (0..~14)
int Pet::careBonus() const {
  int s = streak > 30 ? 30 : streak;
  return s / 3 + bond / 25;
}

// primer cuidado del dia: avanza la racha y afianza el vinculo
void Pet::registerCare() {
  if (isEgg() || ceremony != CER_NONE) return;
  uint32_t d = today();
  // sin reloj, o ya conto hoy. Un dia antes tambien: la hora volvio atras (NTP corrigio un reloj que iba
  // adelantado cerca de medianoche) y "hoy" ya se conto; sin esto la racha volvia a 1
  if (d == 0 || d == lastCareDay || d + 1 == lastCareDay) return;
  if (lastCareDay == 0 || d == lastCareDay + 1) {
    streak++;
  } else {
    streak = 1;        // hubo un hueco de dias
    lastMilestone = 0;
  }
  lastCareDay = d;
  bondToday = 0;
  if (streak > bestStreak) bestStreak = streak;
  bond = clamp100(bond + 4);
  uint16_t ms = (streak >= 100) ? 100 : (streak >= 30) ? 30
              : (streak >= 7)   ? 7   : (streak >= 3)  ? 3 : 0;
  if (ms > lastMilestone) {
    lastMilestone = ms;
    milestoneUntil = millis() + 4500;
  }
  checkMedals();
  save();
}

void Pet::addBond(uint8_t amt) {
  if (bondToday >= 20) return;  // tope diario: el vinculo no se farmea
  bond = clamp100(bond + amt);
  bondToday += amt;
}

void Pet::checkMedals(uint16_t earned) {
  if (isEgg()) return;
  uint16_t before = medals;
  medals |= earned;
  if (level() >= 10) medals |= MED_LV10;
  if (level() >= 20) medals |= MED_LV20;
  if (level() >= 30) medals |= MED_LV30;
  if (streak >= 7) medals |= MED_STREAK7;
  if (streak >= 30) medals |= MED_STREAK30;
  if (bond >= 100) medals |= MED_BOND;
  if (DEX_TBL[speciesId].evolvesTo == 0) medals |= MED_FINAL;
  if (weight == 0 && level() >= 5 && careMistakes == 0) medals |= MED_FIT;
  if (shiny) medals |= MED_SHINY;
  if (DEX_TBL[speciesId].rarity == R_LEGENDARIO) medals |= MED_LEGEND;
  if (trAtk >= MED_TRAIN_MIN) medals |= MED_STRONG;
  if (trSpe >= MED_TRAIN_MIN) medals |= MED_SWIFT;
  uint16_t gained = medals & ~before;
  if (gained) {
    for (uint16_t m = gained; m; m &= (m - 1)) totalMedals++;
    newMedal = gained;
    medalUntil = millis() + 4000;
    if (!sleeping) sfxPlay(SFX_MEDAL);
    save();
  }
}

void Pet::rename(const char *name) {
  strncpy(nick, name, sizeof(nick) - 1);
  nick[sizeof(nick) - 1] = 0;
  save();
}

// ---------- nivel ----------
// forma anterior de una especie (0 si es la primera): por evolvesTo o por una rama
static int16_t prevForm(int16_t dex) {
  for (int16_t p = 1; p <= DEX_COUNT; p++)
    if (DEX_TBL[p].evolvesTo == dex) return p;
  for (int i = 0; i < NUM_DEX_BRANCHES; i++)
    if (DEX_BRANCHES[i].to == dex) return (int16_t)DEX_BRANCHES[i].from;
  return 0;
}

// nivel de evolucion de la forma anterior de la ultima especie pedida (buscarla recorre la
// pokedex). En una sola palabra (especie | nivel << 11) porque el nivel lo leen la tarea de
// red y el loop.
static volatile uint32_t gAnterior = 0;

uint16_t levelOf(int16_t dex, uint8_t evoCount, uint32_t age) {
  if (dex < 1 || dex > DEX_COUNT) return 1;
  uint32_t c = gAnterior;
  if ((int16_t)(c & 0x7FF) != dex) {
    int16_t p = prevForm(dex);
    c = (uint32_t)dex | (uint32_t)(p ? DEX_TBL[p].evolveLevel : 0) << 11;
    gAnterior = c;
  }
  // etapa del individuo y nivel con el que empieza: 1 si es la primera (salio asi del huevo),
  // si no el de evolucion de la forma anterior
  uint32_t etapa = evoCount > 2 ? 2 : evoCount, desde = etapa ? (c >> 11) & 0x7F : 1;
  if (desde < 1) desde = 1;
  static const uint32_t T[4] = { 0, EVO1_AGE_MIN, EVO2_AGE_MIN, EVO3_AGE_MIN };
  uint32_t t0 = T[etapa], t1 = T[etapa + 1];
  uint32_t hasta = DEX_TBL[dex].evolvesTo ? DEX_TBL[dex].evolveLevel : 0;  // su nivel de evolucion
  uint32_t lv;
  if (age <= t0) lv = desde;  // evoluciono antes de tiempo (pocion): empieza en su nivel
  else if (hasta > desde && age < t1) lv = desde + (uint32_t)((uint64_t)(age - t0) * (hasta - desde) / (t1 - t0));
  else if (hasta > desde) lv = hasta + (age - t1) / LEVEL_SLOW_MIN;  // ya le tocaba evolucionar
  else lv = desde + (age - t0) / LEVEL_SLOW_MIN;                      // forma final
  return lv > LEVEL_MAX ? LEVEL_MAX : (uint16_t)lv;
}

uint32_t ageForLevel(int16_t dex, uint8_t evoCount, uint16_t level) {
  uint32_t lo = 0, hi = 60u * 24 * 60;
  if (levelOf(dex, evoCount, hi) < level) return UINT32_MAX;
  while (lo < hi) {  // el nivel nunca baja con la edad: busqueda binaria
    uint32_t mid = lo + (hi - lo) / 2;
    if (levelOf(dex, evoCount, mid) >= level) hi = mid;
    else lo = mid + 1;
  }
  return lo;
}

void Pet::levelSpan(uint32_t &into, uint32_t &span) const {
  into = span = 0;
  uint16_t lv = level();
  if (isEgg() || lv >= LEVEL_MAX) return;
  uint32_t a0 = ageForLevel(speciesId, evoCount, lv), a1 = ageForLevel(speciesId, evoCount, lv + 1);
  if (a1 == UINT32_MAX || a1 <= ageMinutes) return;
  if (a0 > ageMinutes) a0 = ageMinutes;  // sujeto por lvFloor: la barra empieza ahora
  into = ageMinutes - a0;
  span = a1 - a0;
}

static uint16_t calcStat(uint8_t base, uint8_t gene, uint16_t lvl, uint8_t tr) {
  return (uint16_t)base * gene / 100 + lvl + tr;
}

uint16_t Pet::atkStat() const {
  return isEgg() ? 0 : calcStat(DEX_TBL[speciesId].bAtk, geneAtk, level(), trAtk);
}
uint16_t Pet::defStat() const {
  return isEgg() ? 0 : calcStat(DEX_TBL[speciesId].bDef, geneDef, level(), trDef);
}
uint16_t Pet::speStat() const {
  return isEgg() ? 0 : calcStat(DEX_TBL[speciesId].bSpe, geneSpe, level(), trSpe);
}

uint16_t Pet::registeredCount() const {
  uint16_t n = 0;
  for (int i = 1; i <= DEX_COUNT; i++)
    if (isRegistered(i)) n++;
  return n;
}

// forma final que ya cumplio su ciclo (7 dias): lista para despedirse. La
// despedida la dispara el usuario con el boton (no salta sola, para que la vea)
bool Pet::canFarewellNow() const {
  return !isEgg() && !sleeping && ceremony == CER_NONE &&
         DEX_TBL[speciesId].evolvesTo == 0 && ageMinutes >= FAREWELL_AGE_MIN;
}

// abandono total durante 1h: lista para escaparse. La dispara el usuario con el
// boton (final triste); cuidarla un solo tick la salva (neglectTicks se resetea)
bool Pet::canRunawayNow() const {
  return !isEgg() && !sleeping && ceremony == CER_NONE && neglectTicks >= RUNAWAY_TICKS;
}

int8_t Pet::weatherFeel() const {
  if (isEgg() || weather == PW_NONE) return 0;
  uint8_t b = DEX_TBL[speciesId].biome;  // 1 playa (agua), 2 bosque, 3 volcan, 5 nieve
  if (weather == PW_RAIN) return b == 1 ? 1 : b == 3 ? -1 : 0;
  if (weather == PW_SNOW) return b == 5 ? 1 : -1;
  if (weather == PW_SUN) return b == 2 ? 1 : 0;
  return 0;
}

void Pet::addSteps(uint32_t n) {
  uint32_t d = today();
  if (d && d != stepsDay) {  // dia nuevo: se empieza de cero
    stepsDay = d;
    stepsToday = 0;
    stepPts = 0;
    stepJoy = false;
    pendingSave = true;
  }
  if (!n) return;
  stepsToday += n;
  stepsTotal += n;
  // stepPts = lo dado hoy: hasta STEP_PTS_MAX por los miles y STEP_BONUS_PTS mas a los diez mil
  uint32_t ganados = stepsToday / STEP_PTS_EVERY;
  if (ganados > STEP_PTS_MAX) ganados = STEP_PTS_MAX;
  if (stepsToday >= STEP_BONUS_AT) ganados += STEP_BONUS_PTS;
  if (ganados > stepPts) {
    if (stepPts <= STEP_PTS_MAX && ganados > STEP_PTS_MAX) stepBonus = true;
    earn(ganados - stepPts, PT_STEPS);
    stepPts = (uint8_t)ganados;
  }
  if (!stepJoy && stepsToday >= STEP_JOY_AT && !isEgg() && ceremony == CER_NONE) {
    stepJoy = true;  // ha salido de paseo: contento
    joy = clamp100(joy + 15);
    addBond(2);
    heartUntil = millis() + HEART_MS;
  }
  pendingSave = true;
}

void Pet::phoneSteps(uint32_t n) {
  addSteps(0);  // por si ha cambiado el dia
  if (n > stepsToday) addSteps(n - stepsToday);
}

uint8_t Pet::useMegaStone() {
  if (!megaFor(speciesId)) return MEGA_NO_FORM;
  if (isMega()) return MEGA_ALREADY;
  if (bag[ITEM_MEGA_STONE] == 0) return MEGA_NO_STONE;
  if (isEgg() || vacant || sleeping || ceremony != CER_NONE || !lastSeenEpoch) return MEGA_BUSY;
  bag[ITEM_MEGA_STONE]--;
  megaUntil = lastSeenEpoch + MEGA_SECONDS;
  megaEvent = 1;
  joy = clamp100(joy + 10);
  sfxPlay(SFX_EVOLVE);
  pendingSave = true;
  return MEGA_OK;
}

bool Pet::takeMegaChange(bool &on) {
  if (!megaEvent) return false;
  on = megaEvent == 1;
  megaEvent = 0;
  return true;
}

bool Pet::grantSportsStone() {
  uint32_t d = today();
  if (!d || d == stoneDay) return false;
  stoneDay = d;
  if (bag[ITEM_MEGA_STONE] < 999) bag[ITEM_MEGA_STONE]++;
  pendingSave = true;
  return true;
}

// semana (de lunes a domingo) y mes de un dia contado desde 1970 (el 1/1/1970 fue jueves)
static uint32_t weekOf(uint32_t day) { return (day + 3) / 7 + 1; }
static uint32_t monthOf(uint32_t day) {
  time_t t = (time_t)day * 86400;
  struct tm tm;
  gmtime_r(&t, &tm);
  return (uint32_t)(tm.tm_year + 1900) * 12 + tm.tm_mon + 1;
}

uint8_t Pet::grassReady() const {
  uint32_t d = today();
  if (!d) return GRASS_NONE;  // sin hora no se sabe que dia es
  if (grassMonth != monthOf(d)) return GRASS_MONTH;
  if (grassWeek != weekOf(d)) return GRASS_WEEK;
  if (grassDay != d) return GRASS_DAY;
  return GRASS_NONE;
}

uint8_t Pet::collectGrass(GrassPrize &g) {
  uint8_t tier = grassReady();
  if (!tier) return GRASS_NONE;
  uint32_t d = today();
  g.qty = 1;
  g.item2 = 0xFF;
  if (tier == GRASS_MONTH) {  // bueno: huevo legendario, piedra megaevolucion o pocion de evolucion
    static const uint8_t alto[3] = { ITEM_EGG_LEGEND, ITEM_MEGA_STONE, ITEM_POTION_EVO };
    g.item = alto[random(3)];
    grassMonth = monthOf(d);
  } else if (tier == GRASS_WEEK) {  // medio: huevo raro, amuleto shiny o dos pociones (la de
    uint8_t r = random(3);           // resistencia, que no se vende, y una de las otras)
    g.item = r == 0 ? ITEM_EGG_RARE : r == 1 ? ITEM_SHINY_CHARM : ITEM_POTION_END;
    if (r == 2) g.item2 = random(2) ? ITEM_POTION_STR : ITEM_POTION_SPD;
    grassWeek = weekOf(d);
  } else {  // poco: dos chuches, una medicina, una pocion o un huevo normal
    static const uint8_t bajo[6] = { ITEM_CANDY, ITEM_MEDICINE, ITEM_POTION_STR, ITEM_POTION_SPD, ITEM_POTION_END,
                                     ITEM_EGG_COMMON };
    g.item = bajo[random(6)];
    if (g.item == ITEM_CANDY) g.qty = 2;
    grassDay = d;
  }
  bag[g.item] = bag[g.item] + g.qty > 999 ? 999 : bag[g.item] + g.qty;
  if (g.item2 != 0xFF && bag[g.item2] < 999) bag[g.item2]++;
  pendingSave = true;
  return tier;
}

uint8_t Pet::leagueTitles() const {
  uint8_t n = 0;
  for (uint8_t r = 0; r < 9; r++) n += leagueChampion(r);
  return n;
}

uint16_t Pet::totalBadges() const {
  uint16_t n = 0;
  for (int r = 0; r < 9; r++) n += badgeCount(r);
  return n;
}

bool Pet::gymOpen(uint8_t region, uint8_t gym) const {
  if (region >= unlockedRegions() || gym >= 8) return false;
  return gym == 0 || hasBadge(region, gym - 1);
}

bool Pet::startGym(uint8_t region, uint8_t gym) {
  uint16_t d = (uint16_t)today();
  if (!d || !gymOpen(region, gym) || gymTry[region * 8 + gym] == d) return false;
  gymTry[region * 8 + gym] = d;
  pendingSave = true;
  return true;
}

bool Pet::winGym(uint8_t region, uint8_t gym, uint8_t &item) {
  item = 0xFF;
  if (region >= 9 || gym >= 8 || hasBadge(region, gym)) return false;
  badges[region] |= 1 << gym;
  uint8_t n = badgeCount(region);  // regalos por medallas de la region: 2, 4, 6 y 8
  if (n == 8) item = ITEM_EGG_LEGEND;
  else if (n % 2 == 0) item = ITEM_EGG_RARE;
  if (item != 0xFF && bag[item] < 999) bag[item]++;
  pendingSave = true;
  return true;
}

bool Pet::startLeague(uint8_t region) {
  uint16_t d = (uint16_t)today();
  if (!d || !leagueOpen(region) || leagueTry[region] == d) return false;
  leagueTry[region] = d;
  pendingSave = true;
  return true;
}

bool Pet::winLeague(uint8_t region) {
  if (region >= 9 || leagueChampion(region)) return false;
  leagueWon |= 1 << region;
  if (bag[ITEM_EGG_LEGEND] < 999) bag[ITEM_EGG_LEGEND]++;
  if (bag[ITEM_SHINY_CHARM] < 999) bag[ITEM_SHINY_CHARM]++;
  pendingSave = true;
  return true;
}

bool Pet::takeStepBonus() {
  if (!stepBonus) return false;
  stepBonus = false;
  return true;
}

void Pet::noteEnding(uint8_t how) {
  memset(&memo, 0, sizeof(memo));
  memo.ver = MEM_REC_VER;
  memo.endType = how;
  memo.shiny = shiny;
  memo.bond = bond;
  memo.speciesId = speciesId;
  memo.level = level();
  memo.medals = medals;
  memo.bornEpoch = bornEpoch;
  memo.endEpoch = lastSeenEpoch;
  strncpy(memo.nick, nick, sizeof(memo.nick) - 1);
  memoPending = true;
}

bool Pet::takeMemoir(MemRec &r) {
  if (!memoPending) return false;
  r = memo;
  memoPending = false;
  return true;
}

void Pet::startFarewell() {
  if (isEgg() || ceremony != CER_NONE) return;
  lastEnd = CER_FAREWELL;
  noteEnding(CER_FAREWELL);
  ceremony = CER_FAREWELL;
  ceremonyUntil = millis() + CEREMONY_MS;
  heartUntil = ceremonyUntil;  // corazones durante toda la despedida
  sfxPlay(SFX_BYE);
  save();
}

void Pet::startRunaway() {
  if (isEgg() || ceremony != CER_NONE) return;
  lastEnd = CER_RUNAWAY;
  noteEnding(CER_RUNAWAY);
  ceremony = CER_RUNAWAY;
  ceremonyUntil = millis() + CEREMONY_MS;
  sfxPlay(SFX_BYE);
  save();
}

// se va a la granja: final neutro (ni bendice ni maldice el huevo). El sketch
// ya escribio su PetRec en la SD antes de llamar aqui.
void Pet::startCollect() {
  if (isEgg() || ceremony != CER_NONE) return;
  lastEnd = CER_COLLECT;
  noteEnding(CER_COLLECT);
  ceremony = CER_COLLECT;
  ceremonyUntil = millis() + CEREMONY_MS;
  heartUntil = ceremonyUntil;
  sfxPlay(SFX_BYE);
  save();
}

void Pet::exportRec(PetRec &r) const {
  memset(&r, 0, sizeof(r));
  r.ver = PET_REC_VER;
  r.speciesId = speciesId;
  r.shiny = shiny;
  r.fullness = fullness; r.joy = joy; r.energy = energy; r.hygiene = hygiene;
  r.poops = poops; r.weight = weight;
  r.geneAtk = geneAtk; r.geneDef = geneDef; r.geneSpe = geneSpe;
  r.trAtk = trAtk; r.trDef = trDef; r.trSpe = trSpe;
  r.careMistakes = careMistakes; r.bond = bond;
  r.sick = sick; r.sickMinutes = sickMinutes;
  r.medals = medals; r.evoDeclinedLv = evoDeclinedLv;
  r.ageMinutes = ageMinutes; r.farDeclinedAge = farDeclinedAge;
  strncpy(r.nick, nick, sizeof(r.nick) - 1);
  r.bornEpoch = bornEpoch;
  r.nature = nature;
  r.farmEpoch = lastSeenEpoch;  // se exporta para guardarlo en la granja: desde ahora crece alli
  r.evoCount = evoCount;
  r.evoBoost = evoBoost;
}

// vuelve como lo dejaste, pero crecido (despacio) y sano: ver farmAge
void Pet::importRec(const PetRec &r) {
  speciesId = r.speciesId;
  megaUntil = 0;
  prevSpeciesId = -1;
  shiny = r.shiny;
  fullness = r.fullness; joy = r.joy; energy = r.energy; hygiene = r.hygiene;
  poops = r.poops; weight = r.weight;
  geneAtk = r.geneAtk; geneDef = r.geneDef; geneSpe = r.geneSpe;
  trAtk = r.trAtk; trDef = r.trDef; trSpe = r.trSpe;
  careMistakes = r.careMistakes; bond = r.bond;
  sick = r.sick; sickMinutes = r.sickMinutes;
  medals = r.medals & ~MED_RETIRED; evoDeclinedLv = r.evoDeclinedLv;
  ageMinutes = r.ageMinutes; farDeclinedAge = r.farDeclinedAge;
  memcpy(nick, r.nick, sizeof(nick));
  nick[sizeof(nick) - 1] = 0;
  bornEpoch = r.ver >= 2 ? r.bornEpoch : 0;
  nature = (r.ver >= 2 && r.nature < NAT_COUNT) ? r.nature : NAT_EASY;
  evoCount = r.ver >= 4 ? r.evoCount : 0;
  evoBoost = r.ver >= 4 && r.evoBoost;
  // lo que crecio en la granja (a 1/FARM_GROWTH_DIV) y, si estuvo al menos una
  // hora, vuelve sano: alli le cuidan
  uint32_t desde = r.ver >= 3 ? r.farmEpoch : 0;
  ageMinutes = farmAge(r.ageMinutes, desde, lastSeenEpoch);
  if (desde && lastSeenEpoch >= desde + 3600) {
    sick = false;
    sickMinutes = 0;
    poops = 0;
    hygiene = 100;
    if (fullness < 80) fullness = 80;
    if (joy < 80) joy = 80;
    if (energy < 80) energy = 80;
  }
  fromFarm = true;
  vacant = false;
  sleeping = false;
  ceremony = CER_NONE;
  neglectTicks = 0;
  mistakeCooldown = 0;
  goodTicks = 0;
  lvFloor = 0;  // el suelo era del compañero de antes
  newMedal = 0;
  eatUntil = heartUntil = evolveUntil = 0;
  lastTick = millis();
  save();
}

void Pet::release() {
  if (isEgg() || ceremony != CER_NONE) return;
  lastEnd = CER_RELEASE;
  noteEnding(CER_RELEASE);
  ceremony = CER_RELEASE;
  ceremonyUntil = millis() + CEREMONY_MS;
  heartUntil = ceremonyUntil;
  sfxPlay(SFX_BYE);
  save();
}

void Pet::hatch() {
  speciesId = eggTarget;
  megaUntil = 0;
  bornEpoch = lastSeenEpoch;
  raisedCount++;
  nature = (uint8_t)random(NAT_COUNT);
  evoCount = 0;
  evoBoost = false;
  shiny = eggShiny;
  // genes del individuo: 90-110% por stat (cada crianza es unica)
  geneAtk = 90 + random(21);
  geneDef = 90 + random(21);
  geneSpe = 90 + random(21);
  trAtk = trDef = trSpe = 0;
  bond = 0;          // vinculo, medallas y nombre son del individuo
  bondToday = 0;
  medals = 0;
  newMedal = 0;
  nick[0] = 0;
  registerSpecies(speciesId);  // criado = registrado en la pokedex
  checkMedals();     // por si nace ya en forma final (legendario)
  sfxPlay(SFX_HATCH);
  save();
}

// ¿se dan ya las condiciones para evolucionar? Cada descuido retrasa la
// evolucion 1 nivel, y ademas tiene que estar bien cuidado en ese momento
// (ninguna estadistica por debajo de 40). NO evoluciona sola: la dispara el
// usuario tocando al bicho (evolve()), para que vea la transformacion.
bool Pet::canEvolveNow() const { return evoBlock() == EVO_OK; }

// el mismo examen, diciendo que falta (en el orden en que se lo diria la ficha)
uint8_t Pet::evoBlock() const {
  if (isEgg() || ceremony != CER_NONE) return EVO_BUSY;
  if (DEX_TBL[speciesId].evolvesTo == 0) return EVO_FINAL;
  if (!evoBoost && ageMinutes < evoAgeNeeded()) return EVO_WAIT;
  if (lowestStat() < 40) return EVO_NEEDS;
  int16_t libres[MAX_EVO_TARGETS];
  if (freeTargets(*this, speciesId, libres) == 0) return EVO_FARM;  // ya estan en la granja
  if (sleeping) return EVO_ASLEEP;
  return EVO_OK;
}

uint32_t Pet::evoAgeNeeded() const {
  uint32_t base = evoCount == 0 ? EVO1_AGE_MIN : evoCount == 1 ? EVO2_AGE_MIN : EVO3_AGE_MIN;
  return base + (uint32_t)careMistakes * EVO_MISTAKE_MIN;
}

bool Pet::useEvoPotion() {
  if (bag[ITEM_POTION_EVO] == 0 || isEgg() || ceremony != CER_NONE || evoBoost) return false;
  if (DEX_TBL[speciesId].evolvesTo == 0 || ageMinutes >= evoAgeNeeded()) return false;
  int16_t libres[MAX_EVO_TARGETS];
  if (freeTargets(*this, speciesId, libres) == 0) return false;
  bag[ITEM_POTION_EVO]--;
  evoBoost = true;
  evoDeclinedLv = 0;  // que salga el boton ya
  save();
  return true;
}

void Pet::evolve() {
  if (!canEvolveNow()) return;
  prevSpeciesId = speciesId;
  // destinos que no estan en la granja (canEvolveNow garantiza al menos uno).
  // Con ramas (Eevee, Gloom...) prefiere la que falte en la pokedex.
  int16_t all[MAX_EVO_TARGETS];
  int nAll = freeTargets(*this, speciesId, all);
  int16_t opts[MAX_EVO_TARGETS];
  int n = 0;
  for (int i = 0; i < nAll; i++)
    if (!isRegistered(all[i])) opts[n++] = all[i];
  lvFloor = level();  // la forma nueva no baja de nivel (si evoluciono tarde a una etapa lenta)
  speciesId = n > 0 ? opts[random(n)] : all[random(nAll)];
  megaUntil = 0;  // la nueva forma no es mega
  evoCount++;
  evoBoost = false;
  registerSpecies(speciesId);
  sfxPlay(SFX_EVOLVE);
  evolveUntil = millis() + EVOLVE_ANIM_MS;
  save();
}

void Pet::feed() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  fullness = clamp100(fullness + (nature == NAT_GLUTTON ? 30 : 25));  // al gloton le llena mas
  lastFood = FOOD_BERRY;
  eatUntil = millis() + EAT_ANIM_MS;
  registerCare();
  save();
}

void Pet::feedCandy() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping || bag[ITEM_CANDY] == 0) return;
  bag[ITEM_CANDY]--;
  lastFood = FOOD_CANDY;
  fullness = clamp100(fullness + 10);
  joy = clamp100(joy + 12);
  weight = clamp100(weight + 12);  // las chuches pasan factura
  eatUntil = millis() + EAT_ANIM_MS;
  registerCare();
  save();
}

// medicina: un trago amargo (-5 animo) y se cura. Sana no hace nada.
void Pet::giveMedicine() {
  if (ceremony != CER_NONE || isEgg() || sleeping || !sick || bag[ITEM_MEDICINE] == 0) return;
  bag[ITEM_MEDICINE]--;
  sick = false;
  sickMinutes = 0;
  joy = clamp100(joy - 5);
  registerCare();
  checkMedals(MED_CURED);
  save();
}

void Pet::chessMedals(uint16_t wins, bool record) {
  // suerte del ocio: se llama tras cada sincronizacion, con los puntos ya sumados
  if (chessPtsTotal != chessSeen) {
    chessSeen = chessPtsTotal;
    chessDay = dayNow();
  }
  if (wins) chessWinDay = dayNow();
  ptRoll();
  ptAdd(ptNow.wins, wins);
  pendingSave = true;
  if (ceremony != CER_NONE) return;
  checkMedals((wins ? MED_CHESS : 0) | (record ? MED_RECORD : 0));
}

// ---------- ocio ----------

uint8_t Pet::chessLuck() const {
  uint16_t hoy = dayNow();
  if (!hoy) return LUCK_NONE;  // sin reloj: 0 es tambien "nunca jugo", y salia "gano hoy"
  return chessWinDay == hoy ? LUCK_WON : chessDay == hoy ? LUCK_PLAYED : LUCK_NONE;
}

bool Pet::spend(uint16_t pts) {
  if (points < pts) return false;
  points -= pts;
  ptLog(*this, PT_FUN, pts);
  pendingSave = true;
  return true;
}

void Pet::give(uint8_t item, uint16_t n) {
  if (item >= ITEM_COUNT) return;
  bag[item] = bag[item] + n > 999 ? 999 : bag[item] + n;
  while (bag[ITEM_MEGA_SHARD] >= SHARDS_PER_STONE && bag[ITEM_MEGA_STONE] < 999) {  // se juntan solos
    bag[ITEM_MEGA_SHARD] -= SHARDS_PER_STONE;
    bag[ITEM_MEGA_STONE]++;
  }
  pendingSave = true;
}

uint8_t Pet::eatFruit(uint8_t item) {
  if (!isFruit(item) || bag[item] == 0 || ceremony != CER_NONE || isEgg() || sleeping) return FRUIT_NO;
  uint16_t hoy = dayNow();
  if (fruitDay != hoy) { fruitDay = hoy; fruitCnt = 0; }
  uint8_t r = FRUIT_FULL;
  if (fruitCnt < FRUITS_PER_DAY) {
    uint8_t s = trAtk, d = trDef, v = trSpe;
    if (!fruitRaise(item, s, d, v)) return FRUIT_MAXED;  // ya al maximo: no se gasta
    trAtk = s; trDef = d; trSpe = v;
    fruitCnt++;
    r = FRUIT_UP;
  }
  bag[item]--;
  lastFood = FOOD_BERRY;
  fullness = clamp100(fullness + 10);
  if (item == ITEM_FRUIT_GOLD) joy = 100;
  eatUntil = millis() + EAT_ANIM_MS;
  registerCare();
  save();
  return r;
}

bool Pet::eatFish() {
  if (bag[ITEM_FISH] == 0 || ceremony != CER_NONE || isEgg() || sleeping) return false;
  bag[ITEM_FISH]--;
  lastFood = FOOD_BERRY;
  fullness = clamp100(fullness + 30);
  joy = clamp100(joy + 5);
  eatUntil = millis() + EAT_ANIM_MS;
  registerCare();
  save();
  return true;
}

bool Pet::usePearl() {
  if (bag[ITEM_PEARL] == 0 || ceremony != CER_NONE || isEgg()) return false;
  bag[ITEM_PEARL]--;
  joy = 100;
  heartUntil = millis() + HEART_MS;
  addBond(2);
  save();
  return true;
}

void Pet::funDone() {
  if (ceremony != CER_NONE || isEgg() || vacant) return;
  energy = clamp100(energy - 5);
  joy = clamp100(joy + 6);
  addBond(1);
  registerCare();
  pendingSave = true;
}

bool Pet::buy(uint8_t item) {
  if (item >= SHOP_ITEMS || points < ITEM_PRICE[item] || bag[item] >= 999) return false;
  points -= ITEM_PRICE[item];
  ptLog(*this, PT_SHOP, ITEM_PRICE[item]);
  bag[item]++;
  save();
  return true;
}

bool Pet::usePotion(uint8_t item) {
  if (item != ITEM_POTION_STR && item != ITEM_POTION_SPD && item != ITEM_POTION_END) return false;
  if (ceremony != CER_NONE || isEgg() || bag[item] == 0) return false;
  uint8_t &tr = (item == ITEM_POTION_STR) ? trAtk : (item == ITEM_POTION_SPD) ? trSpe : trDef;
  if (tr >= 100) return false;  // ya al maximo: no se gasta
  bag[item]--;
  tr = (tr + POTION_GAIN > 100) ? 100 : tr + POTION_GAIN;
  save();
  return true;
}

bool Pet::hatchEgg(uint8_t kind) {
  if (!vacant || kind >= EGG_KINDS || bag[eggItem(kind)] == 0) return false;
  bag[eggItem(kind)]--;
  newEgg(kind);
  return true;
}

// ---------- libro de puntos ----------

void Pet::ptRoll() {
  uint16_t w = ptWeekOf(today());
  // la misma semana, o la anterior: la hora volvio atras un poco al cambiar de semana (NTP); sin esto
  // se vaciaba el libro de la semana en curso. Sin reloj (w 0: arranque sin hora valida hasta el NTP),
  // tampoco: no se sabe en que semana estamos
  if (w == 0 || ptNow.week == w || w + 1 == ptNow.week) return;
  bool hubo = false;
  for (int k = 0; k < PT_KINDS; k++) hubo |= ptNow.cat[k] != 0;
  if (ptNow.week && ptNow.week + 1 == w && hubo) {  // la semana pasada: para el resumen
    ptLast = ptNow;
    ptReport = true;
  } else {
    ptLast = PtsWeek{};
    ptLast.week = w - 1;
  }
  ptNow = PtsWeek{};
  ptNow.week = w;
  pendingSave = true;
}

// el libro guardado: el de ahora o el de antes del premio de casa (6 apartados, sin PT_REWARD en
// medio), con cada numero en su sitio. Se mira cuanto ocupa: leido tal cual, se descolocaba
static void ptLoad(Preferences &prefs, const char *key, PtsWeek &w) {
  struct V1 { uint16_t week, dayIn[7], dayOut[7], cat[6], wins; };
  static const uint8_t V1_CAT[6] = { PT_CHESS, PT_STEPS, PT_HOCKEY, PT_OTHER, PT_SHOP, PT_FUN };
  uint8_t raw[sizeof(PtsWeek) > sizeof(V1) ? sizeof(PtsWeek) : sizeof(V1)];
  size_t n = prefs.getBytes(key, raw, sizeof(raw));
  if (n == sizeof(PtsWeek)) {
    memcpy(&w, raw, sizeof(w));
  } else if (n == sizeof(V1)) {
    V1 v;
    memcpy(&v, raw, sizeof(v));
    w = PtsWeek{};
    w.week = v.week;
    memcpy(w.dayIn, v.dayIn, sizeof(w.dayIn));
    memcpy(w.dayOut, v.dayOut, sizeof(w.dayOut));
    for (int i = 0; i < 6; i++) w.cat[V1_CAT[i]] = v.cat[i];
    w.wins = v.wins;
  }
}

static void ptAdd(uint16_t &v, int32_t n) { v = v + n > 65535 ? 65535 : v + n; }

// apunta n puntos de un origen (PT_CHESS..PT_OTHER) o destino (PT_SHOP, PT_FUN)
static void ptLog(Pet &p, uint8_t src, int32_t n) {
  if (n <= 0 || src >= PT_KINDS) return;
  p.ptRoll();
  uint8_t wd = ptWeekday(p.dayNow());
  ptAdd(src < PT_SHOP ? p.ptNow.dayIn[wd] : p.ptNow.dayOut[wd], n);
  ptAdd(p.ptNow.cat[src], n);
}

void Pet::earn(int32_t n, uint8_t src) {
  points += n;
  if (points < 0) points = 0;
  ptLog(*this, src, n);
  pendingSave = true;
}

uint8_t Pet::hockey(uint16_t min) {
  if (!min) return 0;
  uint16_t hoy = dayNow();
  if (hockeyDay != hoy) { hockeyDay = hoy; hockeyMin = 0; hockeyPts = 0; }
  hockeyMin = hockeyMin + min > 999 ? 999 : hockeyMin + min;
  uint16_t total = hockeyMin / HOCKEY_MIN_PER_PT;
  if (total > HOCKEY_PTS_MAX) total = HOCKEY_PTS_MAX;
  uint8_t nuevo = total > hockeyPts ? total - hockeyPts : 0;
  hockeyPts = (uint8_t)total;
  if (nuevo) earn(nuevo, PT_HOCKEY);
  if (!isEgg() && !vacant && ceremony == CER_NONE) joy = clamp100(joy + 5);  // le alegra que hagas deporte
  pendingSave = true;
  return nuevo;
}

uint8_t Pet::reward(uint8_t pts) {
  uint16_t hoy = dayNow();
  if (rewardDay != hoy) { rewardDay = hoy; rewardPts = 0; }
  uint8_t dar = pts;
  if (!dar) return 0;
  rewardPts = rewardPts + dar > 255 ? 255 : rewardPts + dar;
  earn(dar, PT_REWARD);
  if (!isEgg() && !vacant && ceremony == CER_NONE) joy = clamp100(joy + 5);  // le alegra que lo feliciten
  pendingSave = true;
  return dar;
}

void Pet::addPoints(int32_t n) {
  ptLog(*this, PT_OTHER, n);
  points += n;
  if (points < 0) points = 0;
  save();
}

// tras un final: sin compañero (ni bicho ni huevo) hasta abrir un huevo
void Pet::becomeVacant() {
  ceremony = CER_NONE;
  neglectTicks = 0;
  speciesId = -1;
  prevSpeciesId = -1;
  megaUntil = 0;
  vacant = true;
  fromFarm = false;
  sick = false;
  sickMinutes = 0;
  sleeping = false;
  poops = 0;
  eggTaps = 0;
  save();
}

void Pet::addFriendDex(const uint8_t *bits, size_t n) {
  if (n > sizeof(dexFriend)) n = sizeof(dexFriend);
  bool nuevo = false;
  for (size_t i = 0; i < n; i++) {
    uint8_t b = bits[i];
    if (i == PET_DEX_BYTES - 1 && PET_DEX_MAX % 8) b &= (uint8_t)((1 << (PET_DEX_MAX % 8)) - 1);  // nada mas alla de la 1025
    if (b & ~dexFriend[i]) nuevo = true;
    dexFriend[i] |= b;
  }
  if (nuevo) pendingSave = true;
}

void Pet::friendPlay() {
  if (ceremony != CER_NONE || isEgg() || vacant) return;
  joy = clamp100(joy + 15);
  heartUntil = millis() + HEART_MS;
  addBond(3);
  registerCare();
  pendingSave = true;
}

void Pet::sportsDone(bool won) {
  if (ceremony != CER_NONE || isEgg() || vacant) return;
  energy = clamp100(energy - 6);
  joy = clamp100(joy + (won ? 12 : 5));
  if (won) heartUntil = millis() + HEART_MS;
  addBond(won ? 2 : 1);
  registerCare();
  pendingSave = true;
}

void Pet::play() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  joy = clamp100(joy + (nature == NAT_PLAYFUL ? 30 : 25));  // al juguetón le alegra mas
  playSeq++;
  energy = clamp100(energy - 10);
  fullness = clamp100(fullness - 5);
  heartUntil = millis() + HEART_MS;
  addBond(2);
  registerCare();
  save();
}

void Pet::screenNap(bool screenOff) {
  if (screenOff) {
    if (sleeping || isEgg() || ceremony != CER_NONE) return;
    sleeping = true;
    napping = true;
    save();  // con la pantalla apagada la escritura no se nota
  } else if (napping) {
    napping = false;
    if (sleeping && lastSeenEpoch && isNightMinute((int)((lastSeenEpoch / 60) % 1440))) {
      autoSlept = true;  // de noche no se le despierta al encender: se levanta por la mañana
      pendingSave = true;
    } else if (sleeping) {
      sleeping = false;
      pendingSave = true;  // se vuelca al atenuar: guardar ya congelaria el tactil ~1 s
    }
  }
}

// la disciplina forja la resistencia: 3 h seguidas bien cuidado = +1
void Pet::forgeEndurance() {
  if (lowestStat() >= 40) {
    if (++goodTicks >= RES_GROW_MIN) {
      goodTicks = 0;
      if (trDef < 100) trDef++;
    }
  } else {
    goodTicks = 0;
  }
}

void Pet::toggleLight() {
  if (ceremony != CER_NONE) return;
  if (isEgg()) return;
  sleeping = !sleeping;
  autoSlept = false;  // lo decidio el jugador
  napping = false;
  save();
}

void Pet::nightCheck(int m, bool idle) {
  bool noche = isNightMinute(m);
  if (noche && !sleeping && idle && !isEgg() && ceremony == CER_NONE) {
    sleeping = true;  // nadie le apago la luz: se va a dormir solo
    autoSlept = true;
    save();
  } else if (!noche && autoSlept) {
    autoSlept = false;  // por la mañana se levanta solo
    if (sleeping) sleeping = false;
    save();
  }
}

bool Pet::buyAccessory(uint8_t a) {
  if (a >= ACC_COUNT || (accOwned & (1 << a)) || points < ACC_PRICE[a]) return false;
  points -= ACC_PRICE[a];
  ptLog(*this, PT_SHOP, ACC_PRICE[a]);
  accOwned |= (1 << a);
  accWorn = a + 1;  // recien comprado: puesto
  save();
  return true;
}

void Pet::wearAccessory(uint8_t a) {
  if (a >= ACC_COUNT || !(accOwned & (1 << a))) return;
  accWorn = (accWorn == a + 1) ? 0 : a + 1;
  save();
}

void Pet::clean() {
  if (ceremony != CER_NONE) return;
  poops = 0;
  hygiene = 100;
  if (nature == NAT_TIDY && !isEgg()) joy = clamp100(joy + 8);  // al limpio le encanta el baño
  addBond(1);
  registerCare();
  save();
}

void Pet::caress() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  joy = clamp100(joy + 5);
  heartUntil = millis() + HEART_MS;
  addBond(1);
  registerCare();
}

void Pet::stroke() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  joy = clamp100(joy + 8);
  heartUntil = millis() + HEART_MS;
  addBond(1);
  registerCare();
}

// cuidarse solo hasta la mitad, de una en una: primero comer, luego lavarse
// (tambien si hay 2 cacas: es lo que pone enfermo), descansar y jugar. No cuenta
// como cuidado del jugador (ni racha ni vinculo). En silencio: sin animacion ni
// aviso, que encendian la pantalla (con USB esta en el reloj de espera) y el altavoz.
void Pet::autoCare() {
  if (ceremony != CER_NONE || isEgg() || sleeping) return;
  if (fullness < AUTO_CARE_FROM) {
    fullness = AUTO_CARE_LEVEL;
  } else if (hygiene < AUTO_CARE_FROM || poops >= 2) {
    if (hygiene < AUTO_CARE_LEVEL) hygiene = AUTO_CARE_LEVEL;
    poops = 0;
  } else if (energy < AUTO_CARE_FROM) {
    energy = AUTO_CARE_LEVEL;
  } else if (joy < AUTO_CARE_FROM) {
    joy = AUTO_CARE_LEVEL;
  } else {
    return;
  }
  pendingSave = true;
}

void Pet::selfRest() {
  if (ceremony != CER_NONE || isEgg() || sleeping) return;
  energy = clamp100(energy + 4);
}

void Pet::selfPlay() {
  if (ceremony != CER_NONE || isEgg() || sleeping || sick) return;
  joy = clamp100(joy + 4);
  energy = clamp100(energy - 1);
}

bool Pet::takeSelfMed() {
  if (!selfMed) return false;
  selfMed = false;
  return true;
}

void Pet::pester() {
  if (ceremony != CER_NONE || isEgg() || sleeping) return;
  joy = clamp100(joy - 6);
}

bool Pet::farmHarvest(int farmCount, uint8_t &pts, uint8_t &item) {
  pts = 0;
  item = 0xFF;
  uint32_t d = today();
  if (!d || d == harvestDay || (int)((lastSeenEpoch / 60) % 1440) < HARVEST_FROM_MIN) return false;
  harvestDay = d;
  if (farmCount <= 0) {
    pendingSave = true;
    return false;
  }
  pts = (uint8_t)(farmCount / HARVEST_PER > HARVEST_MAX ? HARVEST_MAX : farmCount / HARVEST_PER);
  if (farmCount >= 3) {  // con algo de compañia, a veces encuentran algo
    int r = random(100);
    if (r < 15) item = ITEM_CANDY;
    else if (r < 20) item = ITEM_MEDICINE;
  }
  earn(pts, PT_OTHER);
  if (item != 0xFF && bag[item] < 999) bag[item]++;
  save();
  return pts || item != 0xFF;
}

void Pet::eggTap() {
  if (!isEgg() || vacant) return;
  if (++eggTaps >= 3) hatch();
  else save();
}

PetMood Pet::mood() const {
  if (sleeping) return MOOD_SLEEPING;
  if (eating()) return MOOD_EATING;
  if (sick || lowestStat() < 25) return MOOD_SAD;
  return MOOD_HAPPY;
}

void Pet::save() {
  ticksSinceSave = 0;
  pendingSave = false;
  // la partida ya existe: sin esto, una restauracion que fallo a medias (nvs_erase_all ya borro "init")
  // y luego guardo la de la RAM dejaba la NVS sin "init", y el siguiente arranque empezaba con un huevo
  prefs.putBool("init", true);
  prefs.putUChar("full", fullness);
  prefs.putUChar("joy", joy);
  prefs.putUChar("ene", energy);
  prefs.putUChar("hyg", hygiene);
  prefs.putUChar("poop", poops);
  prefs.putBool("sick", sick);
  prefs.putBool("memb", fromFarm);
  prefs.putBool("vac", vacant);
  prefs.putInt("pts", points);
  prefs.putBytes("bag", bag, sizeof(bag));
  prefs.putUShort("rgdone", regionDone);
  prefs.putBool("ezreg", easyRegions);
  prefs.putUChar("rgopen", regionsOpen);
  prefs.putUShort("rgegg", regionEggs);
  prefs.putUInt("born", bornEpoch);
  prefs.putUInt("stp", stepsToday);
  prefs.putUInt("stpd", stepsDay);
  prefs.putUChar("stpp", stepPts);
  prefs.putUInt("mega", megaUntil);
  prefs.putUInt("stoned", stoneDay);
  prefs.putUInt("grd", grassDay);
  prefs.putUInt("grw", grassWeek);
  prefs.putUInt("grm", grassMonth);
  prefs.putBytes("gymb", badges, sizeof(badges));
  prefs.putUShort("gyml", leagueWon);
  prefs.putBytes("gymt", gymTry, sizeof(gymTry));
  prefs.putBytes("leagt", leagueTry, sizeof(leagueTry));
  prefs.putBool("stpj", stepJoy);
  prefs.putUChar("nat", nature);
  prefs.putUChar("acco", accOwned);
  prefs.putUChar("accw", accWorn);
  prefs.putBool("asl", autoSlept);
  prefs.putBool("nap", napping);  // si no, tras un reinicio no despertaria al encender
  prefs.putUShort("gtk", goodTicks);  // la racha de la resistencia: un reinicio no la tira
  prefs.putUShort("lvf", lvFloor);
  prefs.putUInt("hday", harvestDay);
  prefs.putUChar("evoc", evoCount);
  prefs.putBool("evob", evoBoost);
  prefs.putUShort("raised", raisedCount);
  prefs.putUInt("stpt", stepsTotal);
  prefs.putUInt("chst", chessPtsTotal);
  prefs.putBytes("plot", plotAt, sizeof(plotAt));
  prefs.putUChar("plotw", plotWet);
  prefs.putUInt("qend", questEnd);
  prefs.putShort("qdex", questDex);
  prefs.putUShort("frday", fruitDay);
  prefs.putUChar("frcnt", fruitCnt);
  prefs.putUShort("chday", chessDay);
  prefs.putUShort("chwin", chessWinDay);
  prefs.putUInt("chseen", chessSeen);
  prefs.putUInt("funf", funFinds);
  prefs.putBytes("ptwk", &ptNow, sizeof(ptNow));  // (ptnow/ptlast: el de antes, sin hockey)
  prefs.putBytes("ptwl", &ptLast, sizeof(ptLast));
  prefs.putUShort("hkday", hockeyDay);
  prefs.putUShort("hkmin", hockeyMin);
  prefs.putUChar("hkpts", hockeyPts);
  prefs.putUShort("rwday", rewardDay);
  prefs.putUChar("rwpts", rewardPts);
  prefs.putBool("ptrep", ptReport);
  prefs.putUShort("sickm", sickMinutes);
  prefs.putUChar("wgt", weight);
  prefs.putUChar("gatk", geneAtk);
  prefs.putUChar("gdef", geneDef);
  prefs.putUChar("gspe", geneSpe);
  prefs.putUChar("tatk", trAtk);
  prefs.putUChar("tdef", trDef);
  prefs.putUChar("tspe", trSpe);
  prefs.putBool("shy", shiny);
  prefs.putBool("eshy", eggShiny);
  prefs.putBool("stpk", starterPick);
  prefs.putBytes("dexsh", dexShinyReg, sizeof(dexShinyReg));
  prefs.putBytes("dexfr", dexFriend, sizeof(dexFriend));
  prefs.putUInt("age", ageMinutes);
  prefs.putShort("dexn", speciesId);
  prefs.putShort("eggT2", eggTarget);
  prefs.putUChar("crack", eggTaps);
  prefs.putUChar("mist", careMistakes);
  prefs.putBool("sleep", sleeping);
  prefs.putUChar("lend", lastEnd);
  if (lastSeenEpoch) prefs.putUInt("seen", lastSeenEpoch);
  prefs.putBytes("dexreg", dexReg, sizeof(dexReg));
  prefs.putUShort("strk", streak);
  prefs.putUShort("bstrk", bestStreak);
  prefs.putUInt("cday", lastCareDay);
  prefs.putUChar("bond", bond);
  prefs.putUShort("medal", medals);
  prefs.putUShort("tmedal", totalMedals);
  prefs.putUShort("mstone", lastMilestone);
  prefs.putString("nick", nick);
}

void Pet::load() {
  fullness = prefs.getUChar("full", 80);
  joy = prefs.getUChar("joy", 80);
  energy = prefs.getUChar("ene", 80);
  hygiene = prefs.getUChar("hyg", 100);
  poops = prefs.getUChar("poop", 0);
  sick = prefs.getBool("sick", false);
  fromFarm = prefs.getBool("memb", false);
  vacant = prefs.getBool("vac", false);
  points = prefs.getInt("pts", START_POINTS);  // partida anterior a la tienda: START_POINTS
  prefs.getBytes("bag", bag, sizeof(bag));
  regionDone = prefs.getUShort("rgdone", 0);
  easyRegions = prefs.getBool("ezreg", false);
  regionsOpen = prefs.getUChar("rgopen", 1);
  if (regionsOpen < 1 || regionsOpen > REGION_COUNT) regionsOpen = 1;
  regionEggs = prefs.getUShort("rgegg", 0);
  bornEpoch = prefs.getUInt("born", 0);
  stepsToday = prefs.getUInt("stp", 0);
  stepsDay = prefs.getUInt("stpd", 0);
  stepPts = prefs.getUChar("stpp", 0);
  megaUntil = prefs.getUInt("mega", 0);
  stoneDay = prefs.getUInt("stoned", 0);
  grassDay = prefs.getUInt("grd", 0);
  grassWeek = prefs.getUInt("grw", 0);
  grassMonth = prefs.getUInt("grm", 0);
  prefs.getBytes("gymb", badges, sizeof(badges));
  leagueWon = prefs.getUShort("gyml", 0);
  prefs.getBytes("gymt", gymTry, sizeof(gymTry));
  prefs.getBytes("leagt", leagueTry, sizeof(leagueTry));
  stepJoy = prefs.getBool("stpj", false);
  nature = prefs.getUChar("nat", NAT_EASY);
  if (nature >= NAT_COUNT) nature = NAT_EASY;
  accOwned = prefs.getUChar("acco", 0);
  accWorn = prefs.getUChar("accw", 0);
  autoSlept = prefs.getBool("asl", false);
  napping = prefs.getBool("nap", false);
  goodTicks = prefs.getUShort("gtk", 0);
  lvFloor = prefs.getUShort("lvf", 0);
  harvestDay = prefs.getUInt("hday", 0);
  evoCount = prefs.getUChar("evoc", 0);
  evoBoost = prefs.getBool("evob", false);
  raisedCount = prefs.getUShort("raised", 0);
  stepsTotal = prefs.getUInt("stpt", stepsToday);
  chessPtsTotal = prefs.getUInt("chst", 0);
  prefs.getBytes("plot", plotAt, sizeof(plotAt));
  plotWet = prefs.getUChar("plotw", 0);
  questEnd = prefs.getUInt("qend", 0);
  questDex = prefs.getShort("qdex", 0);
  fruitDay = prefs.getUShort("frday", 0);
  fruitCnt = prefs.getUChar("frcnt", 0);
  chessDay = prefs.getUShort("chday", 0);
  chessWinDay = prefs.getUShort("chwin", 0);
  chessSeen = prefs.getUInt("chseen", chessPtsTotal);  // lo de antes no cuenta como de hoy
  funFinds = prefs.getUInt("funf", 0);
  ptLoad(prefs, "ptwk", ptNow);
  ptLoad(prefs, "ptwl", ptLast);
  hockeyDay = prefs.getUShort("hkday", 0);
  hockeyMin = prefs.getUShort("hkmin", 0);
  hockeyPts = prefs.getUChar("hkpts", 0);
  rewardDay = prefs.getUShort("rwday", 0);
  rewardPts = prefs.getUChar("rwpts", 0);
  ptReport = prefs.getBool("ptrep", false);
  sickMinutes = prefs.getUShort("sickm", 0);
  weight = prefs.getUChar("wgt", 0);
  geneAtk = prefs.getUChar("gatk", 0);
  geneDef = prefs.getUChar("gdef", 0);
  geneSpe = prefs.getUChar("gspe", 0);
  if (geneAtk == 0) {  // mascota anterior a los genes: tirada unica ahora
    geneAtk = 90 + random(21);
    geneDef = 90 + random(21);
    geneSpe = 90 + random(21);
  }
  trAtk = prefs.getUChar("tatk", 0);
  trDef = prefs.getUChar("tdef", 0);
  trSpe = prefs.getUChar("tspe", 0);
  shiny = prefs.getBool("shy", false);
  eggShiny = prefs.getBool("eshy", false);
  starterPick = prefs.getBool("stpk", false);
  prefs.getBytes("dexsh", dexShinyReg, sizeof(dexShinyReg));
  prefs.getBytes("dexfr", dexFriend, sizeof(dexFriend));
  ageMinutes = prefs.getUInt("age", 0);
  if (prefs.isKey("dexn")) {
    speciesId = prefs.getShort("dexn", -1);
    eggTarget = prefs.getShort("eggT2", 4);
  } else {
    // migracion desde la version con indices de flash (0-8)
    static const uint8_t OLD2DEX[9] = { 4, 5, 6, 1, 2, 3, 7, 8, 9 };
    int8_t old = prefs.getChar("spec", -1);
    speciesId = (old >= 0 && old < 9) ? OLD2DEX[old] : -1;
    int8_t oldT = prefs.getChar("eggT", 0);
    eggTarget = (oldT >= 0 && oldT < 9) ? OLD2DEX[oldT] : 4;
  }
  // partidas de antes de contarlo: al menos el que hay
  if (raisedCount == 0 && speciesId >= 1) raisedCount = 1;
  eggTaps = prefs.getUChar("crack", 0);
  careMistakes = prefs.getUChar("mist", 0);
  sleeping = prefs.getBool("sleep", false);
  lastEnd = prefs.getUChar("lend", CER_NONE);
  prefs.getBytes("dexreg", dexReg, sizeof(dexReg));
  streak = prefs.getUShort("strk", 0);
  bestStreak = prefs.getUShort("bstrk", 0);
  lastCareDay = prefs.getUInt("cday", 0);
  bond = prefs.getUChar("bond", 0);
  medals = prefs.getUShort("medal", 0) & ~MED_RETIRED;
  totalMedals = prefs.getUShort("tmedal", 0);
  lastMilestone = prefs.getUShort("mstone", 0);
  prefs.getString("nick", nick, sizeof(nick));
  // siembra: la mascota actual cuenta como criada (guardados antiguos)
  if (speciesId >= 1) registerSpecies(speciesId);
}
