#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <stddef.h>  // offsetof

// 1 tick = 1 minuto de juego. Baja este valor para probar mas rapido
// (p. ej. 5000UL = las estadisticas caen 12x mas rapido).
#define PET_TICK_MS 60000UL
// Minutos de juego por nivel. Con 60, CHARMANDER evoluciona a las ~16 h
// de juego con cuidado perfecto. Baja a 1 para ver evoluciones al momento.
#define MINUTES_PER_LEVEL 60
#define EAT_ANIM_MS 2500UL
#define HEART_MS 1500UL
#define EVOLVE_ANIM_MS 5200UL              // animacion de evolucion (mas larga = mas epica)
#define CEREMONY_MS 10000UL                // duracion de la despedida en pantalla
#define FAREWELL_AGE_MIN (7UL * 24 * 60)   // se despide a los 7 dias de juego (en forma final)
// evolucion por edad, igual para todos: la 1a a los 3 dias, la 2a a los 5 (una
// 3a, que no existe en ninguna linea, a los 7). Cada descuido la retrasa
// EVO_MISTAKE_MIN. Ademas hay que estar bien cuidado (todo >= 40).
#define EVO1_AGE_MIN (3UL * 24 * 60)
#define EVO2_AGE_MIN (5UL * 24 * 60)
#define EVO3_AGE_MIN (7UL * 24 * 60)
#define EVO_MISTAKE_MIN 120
#define RUNAWAY_TICKS 60                   // se escapa tras 1 h con TODO a cero
#define SICK_MISTAKE_MIN 120               // enfermo sin medicina: un descuido cada 2 h
#define SELF_MED_MIN 10                    // enfermo y despierto 10 min: se toma una de la mochila solo
// con corriente y sin que nadie lo atienda, se cuida solo, pero solo hasta la
// mitad: cuando algo baja de AUTO_CARE_FROM lo sube a AUTO_CARE_LEVEL. Una cosa
// por minuto (comer, lavarse, descansar, jugar)
#define AUTO_CARE_LEVEL 50
#define AUTO_CARE_FROM 40
// Tamano de la Pokedex. pet.h no incluye dex.h (son tablas estaticas grandes
// y se duplicarian en cada .cpp), asi que va aqui y pet.cpp comprueba que
// coincide con DEX_COUNT.
#define PET_DEX_MAX 1025
#define PET_DEX_BYTES ((PET_DEX_MAX + 7) / 8)

// milisegundos que faltan hasta `deadline` (0 si ya paso, o si deadline==0:
// "temporizador inactivo"). Todos los temporizadores del juego (dialogos,
// animaciones) se guardan como un instante absoluto "deadline = millis() +
// duracion" y se comparaban con millis() < deadline / millis() > deadline;
// eso se rompe cuando millis() da la vuelta a los ~49,7 dias de uptime. La
// resta sin signo, reinterpretada como con signo, es segura frente a esa
// vuelta mientras el intervalo real sea muy inferior a ~24,8 dias (aqui el
// temporizador mas largo son ~12 s), sin cambiar como se guarda el deadline.
static inline uint32_t timeLeft(uint32_t deadline) {
  if (!deadline) return 0;
  int32_t left = (int32_t)(deadline - millis());
  return left > 0 ? (uint32_t)left : 0;
}

// ceremonias de fin de ciclo (COLLECT: se va a la granja, final neutro)
enum : uint8_t { CER_NONE = 0, CER_FAREWELL, CER_RUNAWAY, CER_RELEASE, CER_COLLECT };

// Tienda y mochila (del jugador: duran entre compañeros). Los puntos se ganan
// jugando al ajedrez (ver chess.h); la comida basica, el baño y dormir son gratis.
enum : uint8_t {
  ITEM_CANDY = 0, ITEM_MEDICINE, ITEM_POTION_STR, ITEM_POTION_SPD, ITEM_SHINY_CHARM,
  ITEM_EGG_COMMON, ITEM_EGG_RARE, ITEM_EGG_LEGEND,
  ITEM_POTION_EVO,  // al final: la mochila guardada antes sigue cuadrando
  ITEM_COUNT
};
static const uint16_t ITEM_PRICE[ITEM_COUNT] = { 2, 3, 5, 5, 50, 10, 30, 100, 100 };
// tipos de huevo (ITEM_EGG_COMMON + tipo) y su probabilidad base (%) de salir
// raro y legendario; el resto es comun. El cuidado y la despedida la mejoran.
enum : uint8_t { EGG_COMMON = 0, EGG_RARE, EGG_LEGEND, EGG_KINDS };
static const uint8_t EGG_RARE_PCT[EGG_KINDS] = { 10, 20, 30 };
static const uint8_t EGG_LEG_PCT[EGG_KINDS] = { 3, 6, 10 };
#define POTION_GAIN 5      // cada pocion de fuerza/velocidad
// Capacidades (no hay combates): cada una de 0 a 100 y cada una cuida algo.
//   aguante (trAtk, pocion de fuerza): la energia baja hasta un 50% mas despacio
//   vitalidad (trSpe, pocion de velocidad): el animo baja hasta un 50% mas despacio
//   resistencia (trDef, solo cuidandolo bien): hasta un 60% menos de enfermar
// Las rebajas son de verdad en minutos: con 50, uno de cada cuatro no baja.
#define RES_GROW_MIN 180   // resistencia: +1 por cada 3 h seguidas bien cuidado (despierto)
#define START_POINTS 10    // puntos al empezar
// completar la pokedex de una region (todas sus especies criadas alguna vez):
// puntos, corona en la pokedex y un huevo legendario shiny de esa region
#define REGION_REWARD_PTS 200
// pasos (podometro del QMI8658, solo con la placa encendida): un punto por cada
// STEP_PTS_EVERY, como mucho STEP_PTS_MAX al dia; al llegar a STEP_JOY_AT, el
// compañero se alegra (paseo)
#define STEP_PTS_EVERY 1000
#define STEP_PTS_MAX 5
#define STEP_JOY_AT 3000
// el tiempo de fuera que siente el compañero (lo pone el sketch con el real)
enum : uint8_t { PW_NONE = 0, PW_RAIN, PW_SNOW, PW_SUN };
// caracter del individuo (se sortea al nacer): una necesidad le baja un poco
// mas rapido y su cuidado favorito rinde mas. NAT_EASY no tiene manias.
enum : uint8_t { NAT_EASY = 0, NAT_GLUTTON, NAT_PLAYFUL, NAT_TIDY, NAT_LAZY, NAT_COUNT };
// complementos (del jugador: se compran una vez y se los pone el compañero que haya)
enum : uint8_t { ACC_BOW = 0, ACC_FLOWER, ACC_CAP, ACC_WITCH, ACC_TOPHAT, ACC_CROWN, ACC_COUNT };
static const uint16_t ACC_PRICE[ACC_COUNT] = { 15, 15, 25, 30, 40, 80 };
// cosecha de la granja: cada mañana (desde HARVEST_FROM_MIN) un punto por cada
// HARVEST_PER compañeros guardados, como mucho HARVEST_MAX; y a veces una chuche
// o una medicina. Poco a proposito: los puntos se ganan sobre todo con el ajedrez.
#define HARVEST_PER 5
#define HARVEST_MAX 3
#define HARVEST_FROM_MIN (7 * 60)
// de noche se duerme solo si nadie le apaga la luz (y se despierta por la mañana)
#define NIGHT_SLEEP_MIN (23 * 60)
#define NIGHT_WAKE_MIN (7 * 60)

// Un individuo congelado en la granja: todo lo que es SUYO (especie, genes,
// estado, edad...). Lo del jugador (racha, pokedex, records) no va aqui.
// Se escribe tal cual en la SD (/farm/fNNNN.bin); PET_REC_VER cambia si cambia
// la estructura.
#define PET_REC_VER 4  // 2: + bornEpoch; 3: + farmEpoch; 4: + evoluciones (la granja lee todas)
struct PetRec {
  uint8_t ver;
  uint8_t shiny, fullness, joy, energy, hygiene, poops, weight;
  int16_t speciesId;
  uint8_t geneAtk, geneDef, geneSpe, trAtk, trDef, trSpe;
  uint8_t nature, careMistakes, bond, sick;  // nature: v2 (en v1 era la baya favorita)
  uint16_t sickMinutes, medals, evoDeclinedLv;
  uint32_t ageMinutes, farDeclinedAge;
  char nick[12];
  uint32_t bornEpoch;  // v2
  uint32_t farmEpoch;  // v3: hora local a la que entro en la granja (0 = no se sabe)
  uint8_t evoCount, evoBoost, pad4[2];  // v4: veces que ha evolucionado; pocion tomada
};
#define PET_REC_V1_SIZE offsetof(PetRec, bornEpoch)
#define PET_REC_V2_SIZE offsetof(PetRec, farmEpoch)
#define PET_REC_V3_SIZE offsetof(PetRec, evoCount)

// En la granja tambien crece, sano, pero FARM_GROWTH_DIV veces mas despacio que
// criado por el jugador (y sin evolucionar: eso lo ve el jugador al sacarlo).
#define FARM_GROWTH_DIV 10
// edad (minutos) de uno de la granja a la hora local `now`
static inline uint32_t farmAge(uint32_t ageMinutes, uint32_t farmEpoch, uint32_t now) {
  return ageMinutes + ((farmEpoch && now > farmEpoch) ? (now - farmEpoch) / 60 / FARM_GROWTH_DIV : 0);
}
static inline uint16_t levelOfAge(uint32_t ageMinutes) {
  uint32_t lv = 1 + ageMinutes / MINUTES_PER_LEVEL;
  return lv > 999 ? 999 : (uint16_t)lv;
}

// una pagina del album de recuerdos: como acabo cada compañero (SD /memoir.bin)
#define MEM_REC_VER 1
struct MemRec {
  uint8_t ver, endType, shiny, bond;  // endType: CER_FAREWELL / RUNAWAY / RELEASE / COLLECT
  int16_t speciesId;
  uint16_t level, medals;
  uint32_t bornEpoch, endEpoch;        // hora local; 0 = no se sabe
  char nick[12];
};

enum PetMood : uint8_t { MOOD_HAPPY, MOOD_SAD, MOOD_EATING, MOOD_SLEEPING };
enum : uint8_t { FOOD_BERRY = 0, FOOD_CANDY, FOOD_PILL };

// medallas del individuo (bitmask). El bit 3 era la de la baya favorita, que
// ya no existe: no se reutiliza, para que una ficha guardada antes (NVS o
// granja) no aparezca con una medalla nueva que no gano; se borra al cargar.
enum : uint16_t {
  MED_LV10 = 1 << 0, MED_LV25 = 1 << 1, MED_LV50 = 1 << 2,
  MED_RETIRED = 1 << 3, MED_STREAK7 = 1 << 4, MED_BOND = 1 << 5,
  MED_FINAL = 1 << 6, MED_FIT = 1 << 7,
  MED_CURED = 1 << 8,      // curado con medicina
  MED_SHINY = 1 << 9,      // compañero shiny
  MED_LEGEND = 1 << 10,    // compañero legendario
  MED_STRONG = 1 << 11,    // entrenamiento de fuerza (pociones) >= MED_TRAIN_MIN
  MED_SWIFT = 1 << 12,     // entrenamiento de velocidad (pociones) >= MED_TRAIN_MIN
  MED_STREAK30 = 1 << 13,  // racha de 30 dias
  MED_CHESS = 1 << 14,     // victoria de ajedrez sincronizada con el de compañero
  MED_RECORD = 1 << 15,    // record de ajedrez (rating, tacticas o rush) con el de compañero
};
#define MED_COUNT 15       // en la ficha (sin la retirada)
#define MED_TRAIN_MIN 50
// medalla i de la ficha (0..MED_COUNT-1) -> su bit (se salta el retirado)
static inline uint16_t medalBit(int i) { return (uint16_t)(1u << (i < 3 ? i : i + 1)); }

class Pet {
public:
  // Estadisticas 0..100
  uint8_t fullness = 80;  // comida
  uint8_t joy = 80;       // felicidad
  uint8_t energy = 80;    // energia
  uint8_t hygiene = 100;  // limpieza
  uint8_t poops = 0;      // cacas en pantalla (max 3)
  uint8_t weight = 0;     // 0-100: las chuches engordan, con el tiempo se quema
  // genes (90-110%, se tiran al eclosionar) y entrenamiento (0-100)
  uint8_t geneAtk = 100, geneDef = 100, geneSpe = 100;
  uint8_t trAtk = 0, trDef = 0, trSpe = 0;
  bool shiny = false;       // variante de color rara (se sortea en el huevo)
  uint32_t ageMinutes = 0;
  int16_t speciesId = -1;      // numero de Pokedex (1-PET_DEX_MAX), -1 = huevo
  int16_t prevSpeciesId = -1;  // para la animacion de evolucion
  uint8_t careMistakes = 0;   // descuidos: cada uno retrasa la evolucion 1 nivel
  bool sleeping = false;
  // enfermedad: sale con mal cuidado (cacas, sobrepeso, alguna necesidad
  // muy baja) y se cura con una medicina. Sin curar, cada SICK_MISTAKE_MIN
  // despierto cuenta como un descuido.
  bool sick = false;
  uint16_t sickMinutes = 0;
  // granja: especies guardadas en ella (las rellena el sketch al leer la SD).
  // Ni salen de un huevo ni son destino de evolucion mientras esten ahi.
  uint8_t farmReg[PET_DEX_BYTES] = { 0 };
  bool fromFarm = false;  // el compañero actual es uno de la granja (sacado)
  // sin compañero: tras un final no sale huevo solo; hay que abrir uno de la
  // mochila (o sacar uno de la granja). speciesId sigue en -1, asi que isEgg()
  // es true y todas las acciones quedan bloqueadas como con el huevo.
  bool vacant = false;
  int32_t points = START_POINTS;
  uint16_t bag[ITEM_COUNT] = { 0 };
  uint16_t regionDone = 0;    // regiones con la pokedex completa (bit r, orden de regions.h)
  uint16_t regionEggs = 0;    // huevos legendarios shiny de region sin abrir (bit r)
  uint8_t regionJustDone = 0; // region recien completada + 1 (para celebrarlo)
  uint32_t bornEpoch = 0;     // hora local a la que salio del huevo (0 = no se sabe)
  // pasos de hoy (del jugador) y lo que ya se dio por ellos
  uint32_t stepsToday = 0, stepsDay = 0;
  uint8_t stepPts = 0;
  bool stepJoy = false;
  uint8_t nature = NAT_EASY;  // del individuo
  uint8_t evoCount = 0;       // veces que ha evolucionado este individuo
  bool evoBoost = false;      // tomo la pocion de evolucion: puede evolucionar ya
  uint8_t accOwned = 0;       // complementos comprados (bit ACC_*)
  uint8_t accWorn = 0;        // el que lleva puesto + 1 (0 = ninguno)
  bool autoSlept = false;     // se durmio solo de noche (se despierta solo por la mañana)
  uint32_t harvestDay = 0;    // ultimo dia de cosecha de la granja
  // de por vida (del jugador), para la pagina de estadisticas
  uint16_t raisedCount = 0;   // compañeros nacidos
  uint32_t stepsTotal = 0;    // pasos de siempre
  uint32_t chessPtsTotal = 0; // puntos ganados con el ajedrez
  // para la animacion: que comio la ultima vez (FOOD_*) y cuantas veces ha jugado
  uint8_t lastFood = 0;
  uint16_t playSeq = 0;
  uint16_t selfBathSeq = 0, selfDozeSeq = 0;  // se lavo / se echo una cabezada solo (animacion)
  uint32_t lastSeenEpoch = 0;   // ultima hora RTC vista (para progresion offline)
  uint8_t ceremony = CER_NONE;  // despedida/escapada/liberacion en curso
  uint8_t lastEnd = CER_NONE;   // como acabo la anterior (afecta al huevo)
  // Pokedex en bits. Crece sin migracion: getBytes() de un blob guardado mas
  // corto (los 19 bytes de cuando habia 151) lo copia y deja el resto a 0.
  uint8_t dexReg[PET_DEX_BYTES] = { 0 };       // pokedex de criados
  uint8_t dexShinyReg[PET_DEX_BYTES] = { 0 };  // criados en version shiny
  // racha de cuidado diario (del jugador: persiste entre crianzas)
  uint16_t streak = 0, bestStreak = 0;
  uint32_t lastCareDay = 0;
  // vinculo (del bicho: sube lento con cuidado, se resetea al nacer otro)
  uint8_t bond = 0;
  char nick[12] = "";    // apodo (vacio = nombre de especie)
  // medallas: del individuo + contador acumulado entre todas las crianzas
  uint16_t medals = 0, totalMedals = 0;
  uint16_t newMedal = 0;   // recien conseguida(s), para celebrar
  uint16_t lastMilestone = 0;  // hito de racha ya celebrado

  void begin();                 // carga estado de NVS (o crea el primer huevo)
  void update(uint32_t nowMs);  // llamar en cada loop()

  // Acciones (botones tactiles)
  void feed();              // baya (la unica comida gratis)
  void feedCandy();
  void giveMedicine();  // cura la enfermedad con una de la mochila (sana: no gasta)
  bool buy(uint8_t item);      // paga y lo mete en la mochila
  bool usePotion(uint8_t item);  // ITEM_POTION_STR / _SPD: +POTION_GAIN
  bool hatchEgg(uint8_t kind);   // sin compañero: abre un huevo de la mochila
  bool hatchRegionEgg();         // sin compañero: abre un huevo de region (el de la region mas baja)
  // pocion de evolucion: si aun le falta tiempo para evolucionar, ya puede. No
  // se gasta si no sirve (no evoluciona, ya puede, o ya la tomo)
  bool useEvoPotion();
  uint32_t evoAgeNeeded() const;  // edad (min) para la siguiente evolucion, con los descuidos
  bool showRegionDone() const { return timeLeft(regionUntil) > 0; }
  void dbgRegionBanner(int r) { regionJustDone = r + 1; regionUntil = millis() + 6000; }  // consola REGTEST
  void checkRegionOf(int16_t dex);  // ¿completa su region? (al registrar; la consola REGTEST)
  // Las regiones se abren en orden: la siguiente cuando estan registradas todas
  // las que pueden salir de un huevo (comunes y raras) de la anterior. Los
  // huevos solo traen especies de las regiones abiertas.
  bool regionHatchDone(int r) const;
  uint8_t unlockedRegions() const;  // 1..REGION_COUNT (la 0 siempre esta abierta)
  void setWeather(uint8_t pw) { weather = pw; }
  int8_t weatherFeel() const;  // +1 le gusta el tiempo, -1 no (o tiene frio), 0 le da igual
  bool feelsCold() const { return weather == PW_SNOW && weatherFeel() < 0; }
  void addSteps(uint32_t n);   // pasos nuevos (0: solo cambia de dia si toca)
  bool takeMemoir(MemRec &r);  // el ultimo final, una vez, para guardarlo en el album
  bool buyAccessory(uint8_t a);   // paga y lo guarda (ya comprado: false)
  void wearAccessory(uint8_t a);  // ponerselo; si ya lo llevaba, quitarselo
  // cada pocos segundos: minuto del dia local y si lleva rato sin que lo toquen
  void nightCheck(int minuteOfDay, bool idle);
  // cosecha de hoy (una vez al dia, desde las 7): pts y item (0xFF = ninguno);
  // true si trajeron algo
  bool farmHarvest(int farmCount, uint8_t &pts, uint8_t &item);
  static bool isNightMinute(int m) { return m >= NIGHT_SLEEP_MIN || m < NIGHT_WAKE_MIN; }
  void addPoints(int32_t n);
  // sincronizacion de ajedrez: medallas para el compañero que estaba presente
  void chessMedals(uint16_t wins, bool record);

  // stats de combate: base real de gen 1 x genes + nivel + entrenamiento
  uint16_t atkStat() const;
  uint16_t defStat() const;
  uint16_t speStat() const;
  // efecto de cada capacidad, en %
  uint8_t staminaPct() const { return trAtk / 2; }
  uint8_t vitalityPct() const { return trSpe / 2; }
  uint8_t resistPct() const { return (uint8_t)(trDef * 6 / 10); }
  void play();
  void toggleLight();  // dormir / despertar
  // pantalla apagada con el boton PWR: se duerme; al encenderla despierta, pero
  // solo si se durmio por eso (si ya dormia, sigue dormido)
  void screenNap(bool screenOff);
  void clean();
  void caress();  // tocar al bicho
  void stroke();  // acariciarlo (pasar el dedo de un lado a otro): le gusta mas
  void pester();  // demasiadas cosquillas: se agobia un poco
  // lo que hace solo (vida propia): una siesta corta le da algo de energia y
  // jugar solo algo de animo
  void selfRest();
  void selfPlay();
  // se tomo una medicina de la mochila el solo: true una vez, para enseñarlo
  bool takeSelfMed();
  // el sketch: con USB y sin atenderlo un rato, se cuida solo (hasta la mitad)
  void setGuardian(bool on) { guardian = on; }
  void eggTap();  // tocar el huevo: 3 toques y eclosiona
  void newEgg(uint8_t kind = EGG_COMMON);  // huevo nuevo (el primero: eleccion de inicial)
  void release();  // soltar (pulsacion larga + confirmar)
  void syncClock(uint32_t nowEpoch);  // aplica el tiempo transcurrido apagado
  void setClock(uint32_t nowEpoch);   // fija la hora sin aplicar progresion
  void startFarewell();  // tambien usable desde la consola serie (BYE)
  void startRunaway();   // tambien usable desde la consola serie (RUN)
  void startCollect();   // se va a la granja (el sketch ya guardo su PetRec)
  void exportRec(PetRec &r) const;   // el individuo actual, para guardarlo
  void importRec(const PetRec &r);   // sacar uno de la granja como compañero
  bool inFarm(int16_t dex) const {
    return dex >= 1 && dex <= PET_DEX_MAX && (farmReg[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }

  bool isEgg() const { return speciesId < 0; }
  uint8_t eggCracks() const { return eggTaps; }
  bool eating() const { return timeLeft(eatUntil) > 0; }
  bool showHeart() const { return timeLeft(heartUntil) > 0; }
  bool evolving() const { return timeLeft(evolveUntil) > 0; }
  float evolveT() const {     // progreso de la animacion de evolucion 0..1
    return 1.0f - (float)timeLeft(evolveUntil) / (float)EVOLVE_ANIM_MS;
  }
  bool canEvolveNow() const;  // condiciones de evolucion cumplidas (lista)
  void evolve();              // dispara la transformacion (la llama un toque del usuario)
  bool canFarewellNow() const;  // forma final + 7 dias: lista para despedirse (boton)
  bool canRunawayNow() const;   // abandono total 1h: lista para escaparse (boton triste)
  // el usuario decide en un dialogo; "mantener/quedaros" pospone y re-ofrece luego
  bool wantEvolveButton() const { return canEvolveNow() && level() > evoDeclinedLv; }
  bool wantFarewellButton() const { return canFarewellNow() && ageMinutes >= farDeclinedAge; }
  void declineEvolve() { evoDeclinedLv = level(); }              // re-ofrece al subir de nivel
  void declineFarewell() { farDeclinedAge = ageMinutes + 1440; } // re-ofrece dentro de 1 dia
  // primera partida: el jugador elige inicial (Bulbasaur/Charmander/Squirtle)
  bool awaitingStarter() const { return starterPick; }
  void chooseStarter(int16_t dex) { eggTarget = dex; starterPick = false; save(); }
  void factoryReset() { prefs.clear(); }  // borra la NVS (test: comando serie WIPE)
  void dbgRunawayReady() { fullness = joy = energy = hygiene = 0; neglectTicks = RUNAWAY_TICKS; }  // test
  void dbgHeal() {  // consola HEAL: todo al maximo
    fullness = joy = energy = hygiene = 100;
    poops = 0;
    sick = false;
    sickMinutes = 0;
    neglectTicks = 0;
    save();
  }
  void dbgSetMistakes(uint8_t n) { careMistakes = n; save(); }  // consola MISTAKES n
  void dbgNeeds(int f, int j, int e, int h) {  // consola NEEDS
    fullness = clamp100(f); joy = clamp100(j); energy = clamp100(e); hygiene = clamp100(h);
    save();
  }
  // uint16_t, no uint8_t: con MINUTES_PER_LEVEL=60 el nivel son las horas, asi
  // que un uint8_t da la vuelta a 0 en el nivel 256, a los ~10,6 dias. Y eso se
  // alcanza jugando normal, porque "quedaros juntos" permite posponer la
  // despedida indefinidamente. Se topa en 999 en vez de desbordar.
  uint16_t level() const {
    uint32_t lv = 1 + ageMinutes / MINUTES_PER_LEVEL;
    return lv > 999 ? 999 : (uint16_t)lv;
  }
  bool isRegistered(int16_t dex) const {
    return dex >= 1 && dex <= PET_DEX_MAX && (dexReg[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  bool isShinyRegistered(int16_t dex) const {
    return dex >= 1 && dex <= PET_DEX_MAX && (dexShinyReg[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  uint16_t registeredCount() const;
  bool lineHasUnregistered(int16_t base) const;
  uint8_t eggRarity() const;       // rareza del huevo actual (sin revelar especie)
  int16_t pickEggSpecies(uint8_t kind = EGG_COMMON);  // publica para simular tiradas (EGGS)
  uint8_t lowestStat() const { return min(min(fullness, joy), min(energy, hygiene)); }
  PetMood mood() const;
  // progreso de la ceremonia de despedida/escapada, 0..1 (para animarla)
  float ceremonyT() const {
    if (ceremony == CER_NONE) return 0.0f;
    return 1.0f - (float)timeLeft(ceremonyUntil) / (float)CEREMONY_MS;
  }

  // racha / vinculo / medallas / nombre
  void rename(const char *name);
  bool hasMedal(uint16_t m) const { return medals & m; }
  bool showMedal() const { return timeLeft(medalUntil) > 0; }
  bool showMilestone() const { return timeLeft(milestoneUntil) > 0; }
  int careBonus() const;  // mejora del huevo por racha + vinculo

  // guardado periodico diferido: tick() marca pendiente y el loop lo vuelca
  // cuando la pantalla esta atenuada/apagada (la escritura a flash congela
  // ~1s ambos cores: asi no se ve ni corta el tactil)
  bool savePending() const { return pendingSave; }
  // ultima hora real persistida; sirve para resembrar un RTC que perdio la hora
  uint32_t savedEpoch() { return prefs.getUInt("seen", 0); }
  void flushSave();

private:
  Preferences prefs;
  uint32_t lastTick = 0;
  uint32_t eatUntil = 0;
  uint32_t heartUntil = 0;
  uint32_t evolveUntil = 0;
  int16_t eggTarget = 1;       // dex oculto que saldra del huevo
  bool eggShiny = false;       // sorpresa sorteada al crear el huevo
  uint8_t eggTaps = 0;
  uint8_t mistakeCooldown = 0;
  uint8_t ticksSinceSave = 0;
  bool pendingSave = false;     // guardado periodico pendiente de volcar
  bool napping = false;         // dormido por apagar la pantalla (screenNap)
  uint16_t evoDeclinedLv = 0;   // "mantener forma": no ofrecer evolucion hasta subir de nivel
  uint32_t farDeclinedAge = 0;  // "quedaros juntos": no ofrecer despedida hasta esta edad
  bool starterPick = false;     // primera partida: esperando que el jugador elija inicial
  uint8_t neglectTicks = 0;
  uint16_t goodTicks = 0;  // racha bien cuidado: forja la DEF
  uint32_t ceremonyUntil = 0;
  uint8_t bondToday = 0;       // tope diario de subida de vinculo
  uint32_t medalUntil = 0;     // celebracion de medalla en pantalla
  uint32_t milestoneUntil = 0; // celebracion de hito de racha
  uint32_t regionUntil = 0;    // celebracion de region completada
  bool selfMed = false;        // se curo solo con una de la mochila
  bool guardian = false;       // se cuida solo (ver setGuardian); asi no enferma
  void autoCare();
  uint8_t weather = PW_NONE;
  bool memoPending = false;
  MemRec memo;
  void noteEnding(uint8_t how);  // apunta el final para el album

  uint32_t today() const { return lastSeenEpoch ? lastSeenEpoch / 86400 : 0; }
  void registerCare();   // primer cuidado del dia: racha + vinculo
  void becomeVacant();   // tras un final: sin compañero hasta abrir un huevo
  void addBond(uint8_t amt);
  void checkMedals(uint16_t earned = 0);  // earned: medallas de un suceso (curar, ajedrez)
  void tick();
  void hatch();
  void registerSpecies(int16_t dex);
  void save();
  void load();
  static uint8_t clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
};
