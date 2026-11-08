#include "pet.h"
#include "dex.h"
#include "regions.h"  // rangos de cada region (los paisajes no se usan aqui: el compilador los quita)
#include "audio.h"
#include <string.h>

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
  if (ceremony != CER_NONE) return;  // el tiempo se detiene en la despedida
  if (vacant) return;                // sin compañero no pasa nada
  if (starterPick) return;  // la partida no empieza hasta elegir inicial: si el
                            // tiempo corriera aqui, el huevo eclosionaria solo a
                            // los 3 min con la especie sorteada y se perderia la
                            // eleccion del jugador
  ageMinutes++;

  if (isEgg()) {
    if (ageMinutes >= 3) hatch();  // si no lo tocas, eclosiona solo a los 3 min
    return;
  }

  // el sueño es descanso: la energia se recupera y las necesidades bajan MUCHO
  // mas lento que despierto y con suelo (amanece pidiendo algo de mimo, no a
  // cero, sin descuidos ni escapadas). despierto: comida -2/min, hig/joy -1/min.
  // El peso aun se quema; la racha de buen cuidado (goodTicks) queda en pausa.
  if (sleeping) {
    energy = clamp100(energy + (nature == NAT_LAZY ? 8 : 6));  // al perezoso le sienta mejor
    if (weight > 0 && ageMinutes % 3 == 0) weight--;
    if (ageMinutes % 2 == 0) {                 // ~4x mas lento que despierto
      fullness = dropTo(fullness, 1, 30);
      joy = dropTo(joy, 1, 35);
    }
    if (ageMinutes % 3 == 0) hygiene = dropTo(hygiene, 1, 45);
    checkMedals();  // aun puede cruzar un nivel por edad mientras duerme
    if (++ticksSinceSave >= 5) pendingSave = true;
    return;
  }

  if (ageMinutes % MINUTES_PER_LEVEL == 0) sfxPlay(SFX_LEVEL);  // subio de nivel (despierto)

  fullness = clamp100(fullness - 2);
  // aguante: algunos minutos la energia no baja (staminaPct de cada 100)
  if ((int)(ageMinutes % 100) >= staminaPct()) energy = clamp100(energy - 1);
  if (fullness > 40 && poops < 3 && random(100) < 15) poops++;

  hygiene = clamp100(hygiene - 1 - 4 * poops);
  // el sobrepeso da pereza: la energia cae el doble
  if (weight > 50) energy = clamp100(energy - 1);
  if (weight > 0 && ageMinutes % 3 == 0) weight--;

  // la disciplina forja la resistencia: 3 h seguidas bien cuidado = +1
  if (lowestStat() >= 40) {
    if (++goodTicks >= RES_GROW_MIN) {
      goodTicks = 0;
      if (trDef < 100) trDef++;
    }
  } else {
    goodTicks = 0;
  }

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
      careMistakes++;
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
    careMistakes++;
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
  int16_t maxDex = REGIONS[unlockedRegions() - 1].last;

  // candidatos del tier con linea incompleta; si no hay, baja de tier;
  // si la pokedex del tier esta completa, vale cualquiera del tier
  for (int pass = 0; pass < 2; pass++) {
    for (int t = tier; t >= R_COMUN; t--) {
      static int16_t cand[DEX_COUNT];  // static: 2 KB fuera de la pila
      int n = 0;
      for (int16_t d = 1; d <= maxDex; d++) {
        if (DEX_TBL[d].rarity != t) continue;
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
  for (int d = REGIONS[r].first; d <= REGIONS[r].last; d++) {
    uint8_t ra = DEX_TBL[d].rarity;
    if ((ra == R_COMUN || ra == R_RARO) && !isRegistered(d)) return false;
  }
  return true;
}

uint8_t Pet::unlockedRegions() const {
  uint8_t n = 1;
  while (n < REGION_COUNT && regionHatchDone(n - 1)) n++;
  return n;
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
    points += REGION_REWARD_PTS;
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
  if (d == 0 || d == lastCareDay) return;  // sin reloj, o ya conto hoy
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
  if (level() >= 25) medals |= MED_LV25;
  if (level() >= 50) medals |= MED_LV50;
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
  uint32_t ganados = stepsToday / STEP_PTS_EVERY;
  if (ganados > STEP_PTS_MAX) ganados = STEP_PTS_MAX;
  if (ganados > stepPts) {
    points += ganados - stepPts;
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
bool Pet::canEvolveNow() const {
  if (isEgg() || sleeping || ceremony != CER_NONE) return false;
  const DexEntry &d = DEX_TBL[speciesId];
  if (d.evolvesTo == 0) return false;
  int16_t libres[MAX_EVO_TARGETS];
  if (freeTargets(*this, speciesId, libres) == 0) return false;  // ya estan en la granja
  return (evoBoost || ageMinutes >= evoAgeNeeded()) && lowestStat() >= 40;
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
  speciesId = n > 0 ? opts[random(n)] : all[random(nAll)];
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
  if (ceremony != CER_NONE) return;
  checkMedals((wins ? MED_CHESS : 0) | (record ? MED_RECORD : 0));
}

bool Pet::buy(uint8_t item) {
  if (item >= ITEM_COUNT || points < ITEM_PRICE[item] || bag[item] >= 999) return false;
  points -= ITEM_PRICE[item];
  bag[item]++;
  save();
  return true;
}

bool Pet::usePotion(uint8_t item) {
  if (item != ITEM_POTION_STR && item != ITEM_POTION_SPD) return false;
  if (ceremony != CER_NONE || isEgg() || bag[item] == 0) return false;
  uint8_t &tr = (item == ITEM_POTION_STR) ? trAtk : trSpe;
  if (tr >= 100) return false;  // ya al maximo: no se gasta
  bag[item]--;
  tr = (tr + POTION_GAIN > 100) ? 100 : tr + POTION_GAIN;
  save();
  return true;
}

bool Pet::hatchEgg(uint8_t kind) {
  if (!vacant || kind >= EGG_KINDS || bag[ITEM_EGG_COMMON + kind] == 0) return false;
  bag[ITEM_EGG_COMMON + kind]--;
  newEgg(kind);
  return true;
}

void Pet::addPoints(int32_t n) {
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
  vacant = true;
  fromFarm = false;
  sick = false;
  sickMinutes = 0;
  sleeping = false;
  poops = 0;
  eggTaps = 0;
  save();
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
// como cuidado del jugador (ni racha ni vinculo).
void Pet::autoCare() {
  if (ceremony != CER_NONE || isEgg() || sleeping) return;
  if (fullness < AUTO_CARE_FROM) {
    fullness = AUTO_CARE_LEVEL;
    lastFood = FOOD_BERRY;
    eatUntil = millis() + EAT_ANIM_MS;
  } else if (hygiene < AUTO_CARE_FROM || poops >= 2) {
    if (hygiene < AUTO_CARE_LEVEL) hygiene = AUTO_CARE_LEVEL;
    poops = 0;
    selfBathSeq++;
  } else if (energy < AUTO_CARE_FROM) {
    energy = AUTO_CARE_LEVEL;
    selfDozeSeq++;
  } else if (joy < AUTO_CARE_FROM) {
    joy = AUTO_CARE_LEVEL;
    playSeq++;
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
  points += pts;
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
  prefs.putUShort("rgegg", regionEggs);
  prefs.putUInt("born", bornEpoch);
  prefs.putUInt("stp", stepsToday);
  prefs.putUInt("stpd", stepsDay);
  prefs.putUChar("stpp", stepPts);
  prefs.putBool("stpj", stepJoy);
  prefs.putUChar("nat", nature);
  prefs.putUChar("acco", accOwned);
  prefs.putUChar("accw", accWorn);
  prefs.putBool("asl", autoSlept);
  prefs.putBool("nap", napping);  // si no, tras un reinicio no despertaria al encender
  prefs.putUInt("hday", harvestDay);
  prefs.putUChar("evoc", evoCount);
  prefs.putBool("evob", evoBoost);
  prefs.putUShort("raised", raisedCount);
  prefs.putUInt("stpt", stepsTotal);
  prefs.putUInt("chst", chessPtsTotal);
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
  regionEggs = prefs.getUShort("rgegg", 0);
  bornEpoch = prefs.getUInt("born", 0);
  stepsToday = prefs.getUInt("stp", 0);
  stepsDay = prefs.getUInt("stpd", 0);
  stepPts = prefs.getUChar("stpp", 0);
  stepJoy = prefs.getBool("stpj", false);
  nature = prefs.getUChar("nat", NAT_EASY);
  if (nature >= NAT_COUNT) nature = NAT_EASY;
  accOwned = prefs.getUChar("acco", 0);
  accWorn = prefs.getUChar("accw", 0);
  autoSlept = prefs.getBool("asl", false);
  napping = prefs.getBool("nap", false);
  harvestDay = prefs.getUInt("hday", 0);
  evoCount = prefs.getUChar("evoc", 0);
  evoBoost = prefs.getBool("evob", false);
  raisedCount = prefs.getUShort("raised", 0);
  stepsTotal = prefs.getUInt("stpt", stepsToday);
  chessPtsTotal = prefs.getUInt("chst", 0);
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
