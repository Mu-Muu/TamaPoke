// TamaPoke - tamagotchi pixel art inspirado en la gen 1
// para Waveshare ESP32-S3-Touch-AMOLED-1.75
//
// Librerias (Library Manager o repo de Waveshare):
//   - "GFX Library for Arduino" (moononournation), con soporte CO5300 QSPI
//   - "SensorLib" (Lewis He), driver tactil CST9217
//
// Placa: ESP32S3 Dev Module | Flash 16MB | PSRAM: OPI PSRAM | USB CDC On Boot: Enabled
//
// Los sprites y la tabla de especies se generan con tools/sprites.py (emit).

#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include "TouchDrvCSTXXX.hpp"
#include <ImuDrv.hpp>  // acelerometro QMI8658 (sacudidas)
#include <U8g2lib.h>  // formato de fuente U8g2 (la china es la recortada de zhfont.h)
#include "zhfont.h"   // fuente china recortada (tools/make_zh_font.py)
#include <esp_sleep.h>
#include <driver/gpio.h>
#include "pin_config.h"
#include "species.h"
#include "dex.h"
#include "pet.h"
#include "sdmon.h"
#include "rtcbat.h"
#include "i18n.h"
#include "audio.h"
#include "blit.h"
#include "power.h"
#include "farm.h"
#include "regions.h"
#include "chess.h"
#include "wifisync.h"
#include "shake.h"
#include "stackchan.h"
#include "holiday.h"
#include "memoir.h"
#include "backup.h"
#include "walltime.h"
#include "config.h"
#include <Preferences.h>

// Version del firmware. Subir este numero en cada release (y manifest.json para
// el instalador web). Se muestra en la pantalla de ajustes y por serie al arrancar.
#define FW_VERSION "1.18"

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_CO5300 *panel = new Arduino_CO5300(
  bus, LCD_RESET, 0 /*rotation*/, LCD_WIDTH, LCD_HEIGHT, 6, 0, 0, 0);
// Framebuffer completo en PSRAM: dibujamos todo y hacemos flush() (sin parpadeo)
Arduino_Canvas *gfx = new Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, panel);

TouchDrvCST92xx touch;
// fuente CJK activa: de momento siempre false, la carga llegara con el
// soporte de japones/coreano/chino (ver PR #13)
bool gCjkFont = false;
#define TOUCH_ADDR 0x5A  // CST9217
Pet pet;

// sprite animado de la SD para la especie actual (si existe el archivo)
SdMon mon;          // sprite B/N (respaldo y minijuego si no hay PMD)
PmdMon pmd;         // sprite PMD multi-accion (pantalla principal)
PmdMon evoPmd;      // forma anterior, solo durante el parpadeo de evolucion
int16_t monFor = -2;
bool monShinyFor = false;

// comportamiento del bicho en pantalla
struct {
  uint8_t mode = 0;     // 0 idle, 1 paseo, 2 gesto one-shot
  uint8_t act = PMD_IDLE;
  uint32_t t0 = 0;      // inicio de la animacion en curso
  uint32_t until = 0;   // fin del estado actual
  float x = 233, targetX = 233;
  uint8_t activity = 0; // ACT_*: que esta haciendo por su cuenta
} beh;
// vida propia del bicho (cuando esta contento y nadie le pide nada)
enum : uint8_t { ACT_IDLE = 0, ACT_LOOK, ACT_WALK, ACT_GESTURE, ACT_HOP, ACT_BALL, ACT_DOZE, ACT_SIT, ACT_SEEK, ACT_COUNT };
// expresiones sobre la cabeza (en un bocadillo redondo)
enum : uint8_t { EMO_NONE = 0, EMO_HEART, EMO_NOTE, EMO_EXCLAIM, EMO_QUESTION, EMO_SWEAT, EMO_DOTS, EMO_ZZZ,
                 EMO_ANGRY, EMO_SPARK, EMO_FOOD };
int8_t forceAct = -1;  // consola ACT n: la proxima actividad (pruebas)
uint8_t emote = EMO_NONE;
uint32_t emoteUntil = 0;
struct { bool on = false; float x = 0, vx = 0; uint32_t last = 0; } ball;  // pelota para jugar solo
// comer y jugar: cada vez una forma distinta (al azar)
uint8_t eatStyle = 0, playStyle = 0;
uint32_t eatStart = 0;
uint32_t walkLast = 0;  // instante del ultimo paso del paseo (0 = ninguno aun)
#define PET_GROUND 304  // linea de suelo de la mascota
PmdMon galleryPmd;  // sprite grande de la vista detalle de la galeria (PMD/TPK2, legal)

// galeria pokedex
bool galleryOpen = false;
bool galleryDirty = false;
// la pokedex abre en el menu de regiones; dentro de una region, paginas de 16
int galleryRegion = 0;      // region que se ve (0-8): una pagina por region
bool galleryGrid = false;   // dentro de la region (rejilla); false = su portada
int galleryPage = 0;        // pagina dentro de la region
int16_t galleryDetail = 0;  // dex en vista detalle, 0 = rejilla
// DEXALL (consola serie, pruebas): la galeria muestra toda la pokedex como
// vista, animada y a color. Solo en RAM: no registra nada ni se guarda.
bool galleryShowAll = false;

bool screenOff = false;       // pulsacion corta del boton PWR
uint8_t userDim = 0;          // nivel de brillo elegido en ajustes (power.h): 0 alto, 1 medio, 2 bajo
bool tempF = false;           // temperatura en grados F (ajustes; solo lo que se enseña)
bool cardOpen = false;        // ficha del bicho (deslizar vertical)
bool kbOpen = false;          // teclado para renombrar al bicho
char nameBuf[12] = "";
uint8_t nameLen = 0;
uint8_t cardPage = 0;         // 0 perfil, 1 combate, 2-3 medallas, 4 progreso
#define CARD_PAGES 5
bool clockOpen = false;       // pantalla de ajuste de hora (deslizar abajo)

// escena de bano: espuma sobre el bicho y limpieza al reventar
uint32_t bathUntil = 0;
bool bathPending = false;
struct { int16_t x, y; uint8_t r, ph; } bubbles[14];
uint32_t feedMenuUntil = 0;   // selector de comida abierto hasta este millis

// las 9 especies con sprite propio en flash (respaldo sin SD): dex -> indice
int flashIdxForDex(int16_t dex) {
  static const int8_t IDX[10] = { -1, 3, 4, 5, 0, 1, 2, 6, 7, 8 };
  return (dex >= 1 && dex <= 9) ? IDX[dex] : -1;
}

#define CX 233  // centro de la pantalla redonda
#define CY 233
#define PET_CY 202  // centro vertical del sprite

static const uint16_t INK_K = 0x18C4;  // spriteColor('k')

// botones de icono siguiendo el arco inferior de la pantalla redonda
// (los exteriores van mas altos para no salirse del circulo)
struct Btn {
  int16_t cx, cy;
  const char *const *icon;
};
Btn buttons[4] = {
  { 140, 390, SPR_ICON_FOOD },   // comer
  { 202, 404, SPR_ICON_PLAY },   // jugar
  { 264, 404, SPR_ICON_LIGHT },  // luz
  { 326, 390, SPR_ICON_CLEAN },  // bano
};
#define BTN_HALF 26  // boton de 52x52
#define BTN_HIT 36   // radio tactil (un poco mas generoso)
#define FEED_X 134    // selector de comida: 3 casillas de 66 px (baya, chuche, medicina)

// grietas del huevo (pixeles 'k' sobre el sprite)
static const uint8_t CRACK1[][2] = { {15,8},{16,9},{15,10} };
static const uint8_t CRACK2[][2] = { {11,13},{12,14},{11,15},{20,12},{19,13},{20,14} };
// estrellas del modo noche
static const uint16_t STARS[][2] = { {120,140},{330,120},{370,210},{95,230},{280,90},{160,95} };

bool wasPressed = false;
// eleccion de inicial (primera partida): Bulbasaur / Charmander / Squirtle, 3 filas
static const int16_t STARTER_DEX[3] = { 1, 4, 7 };
#define STARTER_ROW_Y 110
#define STARTER_ROW_H 70
#define STARTER_ROW_GAP 8
// boton-CTA de evolucion (centrado, mitad de pantalla)
#define EVO_BTN_W 256
#define EVO_BTN_H 64
#define EVO_BTN_X (CX - EVO_BTN_W / 2)
#define EVO_BTN_Y 172
// boton-CTA de despedida (mas ancho: lleva el nombre + frase)
#define FAR_BTN_W 408
#define FAR_BTN_H 58
#define FAR_BTN_X (CX - FAR_BTN_W / 2)
#define FAR_BTN_Y 176
// el CST9217 avisa por el pin INT cuando hay datos tactiles; lo usamos para no
// leer el bus I2C mientras el chip esta dormido (esa lectura se colgaba ~1s)
volatile bool gTouchIrq = false;
void IRAM_ATTR touchIsr() { gTouchIrq = true; }
bool gTouchLog = false;  // TOUCHLOG (consola): traza de cada gesto, para diagnostico
// acelerometro (mismo bus I2C que el tactil): agitar el huevo / marear al compañero
SensorQMI8658 imu;
bool imuOk = false;
bool gImuLog = false;  // IMULOG (consola): modulo de la aceleracion, para afinar umbrales
ShakeDetector shaker;
uint32_t dizzyUntil = 0;  // mareado (solo visual: no cambia ninguna estadistica)
#define DIZZY_MS 4000
uint8_t gPhase = PH_DAY;  // PH_DAY / PH_TWILIGHT / PH_NIGHT (por la salida y la puesta del sol)
Weather gWxView;          // el tiempo de este frame
// consola CLOCKAT: hora (minuto del dia) que enseñan los relojes, solo en pantalla (-1 = la real)
int16_t gClockForce = -1;
uint32_t shownClock(uint32_t e) { return gClockForce < 0 ? e : e - e % 86400 + (uint32_t)gClockForce * 60; }
int8_t gWxForce = -1, gPhaseForce = -1;  // consola WXSET / PHASE: ver un tiempo o una luz sin esperar
// fiestas: la de hoy (la calcula render) y el cumpleaños del dueño (NVS "fest", consola BDAY)
uint8_t gHoliday = HOL_NONE;
int8_t gHolForce = -1;  // consola HOLI n: ver una fiesta sin esperar a su dia
int8_t gSeasonForce = -1;  // consola SEASON n: ver una estacion
uint8_t gBdayM = 0, gBdayD = 0;
// bocadillo: lo que dice el compañero de vez en cuando
const char *bubbleText = nullptr;
char bubbleBuf[48];
uint32_t bubbleUntil = 0, nextBubble = 0;
// visita de un compañero de la granja (cruza el paisaje, saluda y se va)
PmdMon visitorPmd;
struct {
  bool on = false;
  int8_t dir = 1;      // +1 hacia la derecha
  uint8_t phase = 0;   // 0 llega, 1 saluda, 2 se va
  float x = 0, stopX = 0;
  uint32_t t0 = 0, until = 0, last = 0;
  char name[24] = "";
} vis;
uint32_t nextVisit = 0;
bool restoreAsk = false;  // ajustes: confirmar restaurar la copia
bool statsOpen = false;   // ajustes: pagina de estadisticas
// cima del ultimo sprite PMD dibujado (gorros, complementos, mareo, bocadillo)
int gPmdTopY = 304 - 150, gPmdTopX = 233;
uint32_t lastRender = 0;
uint32_t sdEpoch = 0;  // +1 cada vez que se recarga la SD (las miniaturas cambian)
// proteccion del AMOLED: atenuado por inactividad
uint32_t lastInteract = 0;
uint32_t lastUserAt = 0;     // ultimo toque de verdad (comer solo y demas no cuentan): cuidarse solo
uint8_t dimStage = 0;        // 0 despierto, 1 atenuado (90s), 2 casi apagado (5min)
bool swallowGesture = false; // el toque que despierta no acciona nada
uint32_t holdStart = 0;     // pulsacion larga sobre el bicho
uint32_t confirmUntil = 0;  // dialogo "soltar?" activo hasta este millis
uint32_t actionUntil = 0;   // dialogo de pulsacion larga: recoger (granja) / soltar
// aviso breve sobre cualquier pantalla ("primero el huevo", "falta la SD")
const char *toastMsg = nullptr;
uint32_t toastUntil = 0;
void toast(const char *m) { toastMsg = m; toastUntil = millis() + 2500; }

// granja (compañeros guardados en la SD)
bool farmOpen = false;
bool farmDirty = false;
int farmPage = 0;
int farmDetail = -1;  // indice en la granja en vista detalle, -1 = rejilla
bool albumOpen = false;  // album de recuerdos (dentro de la granja)
int albumPage = 0;

// tienda (pagina 0: comprar) y mochila (pagina 1: usar)
bool shopOpen = false;
bool shopDirty = false;
int shopPage = 0;

// resumen de la ultima sincronizacion del ajedrez: se enseña una vez
ChessReport lastReport;
bool reportPending = false;
bool syncRequested = false;  // consola SYNC o recien guardado el wi-fi: sincronizar ya
uint8_t choiceKind = 0;     // dialogo de decision: 0 ninguno, 1 evolucion, 2 despedida
uint32_t choiceUntil = 0;   // se cierra solo a este millis
int16_t tX0, tY0, tXl, tYl; // gesto en curso (inicio y ultima posicion)
uint32_t tStart = 0;
bool holdFired = false;
bool gestureMoved = false;  // el dedo se ha movido en este gesto (ya no es pulsacion larga)
// caricia: pasar el dedo de un lado a otro sobre el bicho
bool strokeFired = false;   // este gesto ya fue caricia (al levantar no es toque ni deslizar)
int16_t strokeLastX = 0, strokeExt = 0;
int8_t strokeDir = 0;
uint8_t strokeTurns = 0;
uint32_t lastStroke = 0;
// reaccion del bicho a un gesto: accion PMD (o REACT_SPIN) durante un rato
#define REACT_SPIN 0xFE    // vuelta: mira a un lado y a otro muy rapido
#define REACT_BOUNCE 0xFD  // saltitos en el sitio (con su tamaño de siempre: la accion
                           // Hop de PMD tiene un lienzo alto y se dibujaria mas pequeño)
uint8_t reactAct = 0;
uint32_t reactUntil = 0;
float walkSpeed = 30.0f;  // px/s del paseo (mas rapido si lo arrastras o lo inclinas)
// arrastrar: pulsar quieto sobre el bicho un momento y luego mover el dedo
bool dragOn = false, dragFired = false, earlyMove = false;
// circulo alrededor del bicho: angulo acumulado
float circAng = 0, circLast = 0;
bool circHave = false, circFired = false;
// toques sobre el bicho: doble toque, cosquillas
uint32_t lastPetTap = 0, tapTimes[16];
uint8_t tapCount = 0;
uint32_t lastTickle = 0, lastAnnoy = 0;
// orientacion: boca abajo apaga y duerme; inclinado, el bicho va hacia abajo
int8_t faceUpSign = -1;   // signo de z con la pantalla hacia arriba (se aprende al tocarla)
bool faceDownOff = false; // la pantalla la apago ponerla boca abajo
#define TILT_SIGN 1       // si al inclinar va hacia el lado alto, cambiar a -1
// celebraciones: subir de nivel y cosecha de la granja
uint32_t levelUpUntil = 0, harvestUntil = 0;
uint16_t levelUpShow = 0;
uint32_t unlockUntil = 0;  // region recien abierta (cartel)
uint8_t unlockShow = 0;
char harvestMsg[48];
// nombre de cada objeto de la tienda (orden de ITEM_*)
static const StrId ITEM_NAME[ITEM_COUNT] = {
  S_ITEM_CANDY, S_ITEM_MEDICINE, S_ITEM_POTION_STR, S_ITEM_POTION_SPD, S_ITEM_SHINY_CHARM,
  S_ITEM_EGG_COMMON, S_ITEM_EGG_RARE, S_ITEM_EGG_LEGEND, S_ITEM_POTION_EVO,
};

// ---------- compartido entre pestañas (las .ino se unen por orden alfabetico) ----------
#define C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define HORIZON 232  // linea donde el cielo se encuentra con el suelo
#define C_GOLD C565(0xf0, 0xc0, 0x30)
#define C_RED C565(0xe0, 0x22, 0x22)
bool gNight = false;  // noche real (por hora) o durmiendo: lo fija render()
uint8_t gTextSize = 1;
int gFontAscent = 0;  // px del borde superior a la linea base, 0 = fuente clasica
#define CJK_FONT_ZH u8g2_font_tp_zh16
#define CJK_FONT_ZH_S u8g2_font_tp_zh12
int gAsc16 = 0, gAsc12 = 0;  // ascenso medido de cada fuente china
uint8_t gReqSize = 1;        // ultimo tamano pedido (textW lo restaura)
bool screenKeyValid = false;  // (el renderer del host lo invalida entre escenas)
uint32_t lastScreenKey = 0;

void setup() {
  Serial.setRxBufferSize(8192);  // la transferencia a SD llega en bloques de 2 KB
  Serial.begin(115200);
  // CRITICO: sin esto, Serial.print BLOQUEA el juego cuando no hay un
  // monitor serie abierto en el host (el bufer TX del USB CDC se llena
  // y nadie lo vacia) -> con timeout 0 los mensajes se descartan
  Serial.setTxTimeoutMs(0);
  Serial.printf("TamaPoke fw v%s\n", FW_VERSION);
  loadLang();  // idioma guardado (ES por defecto)
  Wire.begin(IIC_SDA, IIC_SCL);
  // CST9217 (tactil), AXP2101 (PMU) y PCF85063 (RTC) comparten este bus I2C.
  // Red de seguridad para PMU/RTC (SensorLib NO respeta este timeout en el
  // tactil; el cuelgue del tactil dormido se resuelve gateando por INT, ver
  // handleTouch).
  Wire.setTimeOut(50);

  // CRITICO: encender la alimentacion del panel (BLDO1=OLED VDD 3.3V) ANTES de
  // inicializar el display. Si el PMU se reseteo (drenaje total), este rail
  // queda OFF y la pantalla se ve negra aunque el resto de la placa funcione.
  pmuEnablePanel();

  // QSPI a 80MHz (por defecto 40): el flush del framebuffer es el cuello de
  // botella del fps (~56ms a 40MHz). Si el panel mostrara basura, bajar a 40M.
  if (!gfx->begin(80000000)) Serial.println("gfx->begin() fallo");
  {  // nivel de brillo (ya desde el primer fotograma, sin fogonazo) y unidad de temperatura
    Preferences p;
    p.begin("tamapoke", true);
    userDim = p.getUChar("bri", 0) % BRIGHT_LEVELS;
    tempF = p.getBool("tempf", false);
    p.end();
  }
  panel->setBrightness((uint8_t)(180 * userBrightPct(userDim) / 100));
  applyLangFont();  // fuente del idioma guardado (clasica salvo CJK)

  touch.setPins(TP_RESET, TP_INT);
  bool touchOk = false;
  for (int i = 0; i < 3 && !touchOk; i++) {  // a veces falla al primer intento
    touchOk = touch.begin(Wire, 0x5A, IIC_SDA, IIC_SCL);
    if (!touchOk) delay(150);
  }
  if (!touchOk) Serial.println("CST9217 no detectado");
  // begin() deja el chip en modo comando (lee la identidad y no sale);
  // hace falta un reset por hardware para que vuelva a reportar toques
  touch.reset();
  touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
  touch.setMirrorXY(true, true);  // el panel esta montado girado 180 grados
  // INT activo-bajo: salta cuando hay datos. Gatea las lecturas I2C (ver loop)
  pinMode(TP_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TP_INT), touchIsr, FALLING);

  imuOk = imu.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL) ||
          imu.begin(Wire, QMI8658_H_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
  if (imuOk) {
    imu.configAccel(AccelFullScaleRange::FS_8G, 125.0f, SensorQMI8658::LpfMode::MODE_0);
    imu.enableAccel();
    // podometro del propio chip (cuenta solo, sin gastar CPU): se lee cada 5 s
    if (!imu.configPedometerDefault(125.0f) || !imu.enablePedometer()) Serial.println("podometro no disponible");
  } else {
    Serial.println("QMI8658 no detectado");
  }

  pet.begin();
  sdBegin();
  thumbs.load();
  farmLoad(pet);  // especies de la granja: no salen de huevo ni de evolucion

  // reloj real: aplica el tiempo que estuvo apagado
  rtcBegin();
  batBegin();
  pwrSetup();
  uint32_t e = rtcEpoch();
  if (e == 0) {
    // Sin pila de respaldo, el PCF85063 pierde la hora al cortar la
    // alimentacion. Sembrar siempre la fecha fija haria RETROCEDER el tiempo de
    // juego respecto a lo ya guardado, y eso rompe la racha, la edad y la hora
    // de la escena. Se siembra con la ultima hora vista, si es posterior, para
    // que el tiempo no vaya nunca hacia atras. Los minutos apagado se pierden
    // igual (sin RTC no hay forma de saberlos), pero nada se descuadra.
    uint32_t seed = 1767225600UL;             // RTC virgen de verdad
    uint32_t seen = pet.savedEpoch();         // usa la NVS que pet ya tiene abierta
    if (seen > seed) seed = seen;
    rtcSetEpoch(seed);
    e = rtcEpoch();
    Serial.printf("RTC sin hora: sembrado en %u%s\n", seed,
                  seen > 1767225600UL ? " (desde la ultima hora guardada)" : "");
  }
  pet.syncClock(e);
  // arranca con la pantalla encendida: si dormia por tenerla apagada, despierta
  // (de noche sigue dormido hasta la mañana)
  pet.screenNap(false);

  audioBegin();  // ES8311 + I2S + amplificador (suena un jingle de arranque)

  {
    Preferences f;
    f.begin("fest", true);
    gBdayM = f.getUChar("bm", 0);
    gBdayD = f.getUChar("bd", 0);
    f.end();
  }

  netSetUsb(usbPresent());
  netBegin();  // tiempo cada hora y, con USB, Stack-chan (en el otro nucleo)

  lastInteract = lastUserAt = millis();
}

// carga/descarga el sprite de SD cuando cambia la especie
void ensureMon() {
  if (pet.speciesId == monFor && monShinyFor == pet.shiny && !sdDirty) return;
  // los ficheros recien recibidos pueden incluir un thumbs.bin nuevo, que se
  // cargaba solo en setup(): sin esto no se veia en la galeria hasta reiniciar
  if (sdDirty) { thumbs.unload(); thumbs.load(); sdEpoch++; }
  sdDirty = false;
  monFor = pet.speciesId;
  monShinyFor = pet.shiny;
  mon.unload();
  pmd.unload();
  beh.x = beh.targetX = 233;
  beh.mode = 0;
  beh.until = 0;
  if (pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT) {
    pmd.load(pet.speciesId, pet.shiny);          // principal: PMD
    if (!pmd.loaded) mon.load(pet.speciesId, pet.shiny);  // respaldo: B/N
  }
}

void loop() {
  uint32_t now = millis();
  pet.update(now);

  // el amplificador sigue al estado de sueno (la llamada sale sola si no cambia).
  // Aqui cubre todas las vias: el boton de luz, el sueno nocturno y el que llega
  // aplicado desde la progresion offline.
  audioSetSleeping(pet.sleeping);

  // avisa con un sonido cuando el bicho pasa a estar listo para evolucionar
  // (incluye el caso de cumplir al despertar). canEvolveNow es false durmiendo.
  static bool wasEvoReady = false;
  bool evoReady = pet.wantEvolveButton();
  if (evoReady && !wasEvoReady) sfxPlay(SFX_MEDAL);
  wasEvoReady = evoReady;
  // aviso sombrio cuando el bicho esta a punto de escaparse por abandono
  static bool wasRunReady = false;
  bool runReady = pet.canRunawayNow();
  if (runReady && !wasRunReady) sfxPlay(SFX_DENY);
  wasRunReady = runReady;

  handleTouch();
  handleShake(now);
  handleSerial();
  ensureMon();

  // pulsacion corta del PWR: pantalla on/off
  static uint32_t lastPwr = 0;
  if (now - lastPwr > 250) {
    lastPwr = now;
    if (pwrShortPressed()) {
      screenOff = !screenOff;
      pet.screenNap(screenOff);  // pantalla apagada = a dormir; encendida = despierta
      if (!screenOff) wakeUI();
    }
  }

  updateBrightness(now);

  // bateria al limite seguro (sin USB): se guarda la partida y se apaga el PMU antes de que la
  // celda baje mas; el RTC conserva la hora
  if (batShouldShutdown() && !usbPresent()) {
    pet.flushSave();
    panel->setBrightness(0);
    batPowerOff();
  }

  // vuelca el autoguardado periodico SOLO con la pantalla atenuada/apagada o
  // durmiendo: la escritura a NVS congela ~1s ambos cores (caché de flash off),
  // y aqui no hay animacion que se corte ni dedo esperando respuesta. Con 90s
  // de inactividad la pantalla ya atenua, asi que se vuelca enseguida; el uso
  // activo persiste igual por los guardados de cada accion (comer/jugar/...).
  if (pet.savePending() && (screenOff || dimStage >= 1 || pet.sleeping)) {
    pet.flushSave();
  }

  // red: con USB se queda en linea para Stack-chan; sus ordenes se hacen aqui
  static uint32_t lastUsb = 0;
  if (now - lastUsb > 2000) {
    lastUsb = now;
    bool usb = usbPresent();
    netSetUsb(usb);
    // enchufado y sin que nadie lo atienda 10 min: se cuida solo (hasta la mitad)
    pet.setGuardian(usb && idleSince(now, lastUserAt) > 10UL * 60 * 1000);
  }
  // lo que hace al cuidarse solo, que se vea: lavarse (sin el "limpio del todo"
  // del baño de verdad) o echarse una cabezada
  static uint16_t banosSolo = 0xFFFF, siestasSolo = 0xFFFF;
  if (banosSolo == 0xFFFF) { banosSolo = pet.selfBathSeq; siestasSolo = pet.selfDozeSeq; }
  if (pet.selfBathSeq != banosSolo) {
    banosSolo = pet.selfBathSeq;
    startBath(false);
  }
  if (pet.selfDozeSeq != siestasSolo) {
    siestasSolo = pet.selfDozeSeq;
    forceAct = ACT_DOZE;
    beh.until = 0;
  }
  stackchanService();

  // hora del NTP (al arrancar y cada hora): si el reloj se ha ido mas de 10 s
  // (o se paro sin corriente), se corrige. Sin aplicar progresion: si estuvo
  // parado, ese rato no cuenta para el bicho.
  uint32_t utcNtp;
  if (netTimeTake(utcNtp) && utcNtp > 1700000000) {
    uint32_t wall = wallFromUtc(utcNtp), rtc = rtcEpoch();
    int32_t diff = (int32_t)(wall - rtc);
    if (diff >= 10 || diff <= -10) {
      rtcSetEpoch(wall);
      pet.setClock(wall);
      Serial.printf("hora: corregida %+ld s\n", (long)diff);
    }
  }

  // portal de configuracion del wi-fi: al guardar, cerrar y sincronizar
  if (wifiPortalActive() && wifiPortalLoop()) {
    delay(1500);  // que el movil reciba la pagina de "guardado"
    wifiPortalStop();
    toast(T(S_WIFI_SAVED));
    syncRequested = true;
  }

  // sincronizacion diaria del ajedrez: la primera vez que se ve un dia nuevo
  // (a medianoche si esta encendida; si estaba apagada, al encenderla). Si
  // falla, se reintenta a los 30 min. Sin cuenta de chess.com no se intenta.
  static uint32_t nextSyncCheck = 0;
  if (!wifiPortalActive() && (syncRequested || (int32_t)(now - nextSyncCheck) >= 0)) {
    nextSyncCheck = now + 60000;
    if (syncRequested || (wifiHasCreds() && rtcEpoch() / 86400 != chessSyncDay())) {
      bool manual = syncRequested;
      syncRequested = false;
      if (chessHasAccount() && !runChessSync(manual)) nextSyncCheck = now + 30UL * 60 * 1000;
    }
  }

  // anota la hora real cada 30 s (se persiste en cada save del juego)
  static uint32_t lastClock = 0;
  if (now - lastClock > 30000) {
    lastClock = now;
    uint32_t e = rtcEpoch();
    if (e) pet.lastSeenEpoch = e;
    pet.setWeather(petWeather());
    // de noche, si nadie le apaga la luz, se duerme solo (y por la mañana se levanta)
    if (pet.lastSeenEpoch)
      pet.nightCheck((int)((pet.lastSeenEpoch / 60) % 1440), idleSince(now, lastInteract) > 10UL * 60 * 1000);
  }

  // copia de seguridad diaria en la SD (una sola: la nueva sustituye a la de ayer),
  // con la pantalla a oscuras o el bicho dormido (vuelca antes lo pendiente)
  static uint32_t backupDay = 0, nextBackupTry = 0;
  if (sdReady && pet.lastSeenEpoch && (screenOff || dimStage >= 1 || pet.sleeping) &&
      (int32_t)(now - nextBackupTry) >= 0) {
    nextBackupTry = now + 60000;
    uint32_t d = pet.lastSeenEpoch / 86400, e;
    if (!backupDay) backupDay = backupInfo(e) ? e / 86400 : 1;  // 1: aun no hay ninguna
    if (d != backupDay) {
      pet.flushSave();
      if (backupSave(pet.lastSeenEpoch)) backupDay = d;
      else nextBackupTry = now + 3600000UL;
    }
  }

  visitTick(now);
  bubbleTick(now);

  // comer: cada vez una forma (y a veces lo dice)
  static bool comia = false;
  if (pet.eating() && !comia) {
    eatStart = now;
    eatStyle = random(4);
    if (eatStyle == 1) showEmote(EMO_HEART, EAT_ANIM_MS);
    if (eatStyle == 2) showEmote(EMO_NOTE, EAT_ANIM_MS);
    if (eatStyle == 3 || !random(3)) {
      bubbleText = T(S_SAY_YUM);
      bubbleUntil = now + EAT_ANIM_MS;
    }
  }
  comia = pet.eating();
  // jugar: saltitos, vuelta, pelota, pose o salto con "que divertido"
  static uint16_t jugadas = 0xFFFF;
  if (jugadas == 0xFFFF) jugadas = pet.playSeq;
  if (pet.playSeq != jugadas) {
    jugadas = pet.playSeq;
    playStyle = random(5);
    if (playStyle == 0) { react(REACT_BOUNCE, 1800, nullptr); showEmote(EMO_NOTE, 1800); }
    else if (playStyle == 1) { react(REACT_SPIN, 1500, nullptr); showEmote(EMO_SPARK, 1500); }
    else if (playStyle == 2 && pmd.has(PMD_WALKL)) {  // a la pelota un rato
      behEnd(now);
      beh.activity = ACT_BALL;
      beh.mode = 0;
      ball.on = true;
      ball.x = beh.x + 40;
      ball.vx = 160;
      ball.last = now;
      beh.until = now + 6000;
      showEmote(EMO_EXCLAIM, 1000);
    } else if (playStyle == 3) { react(pmd.has(PMD_POSE) ? PMD_POSE : REACT_BOUNCE, 1600, nullptr); showEmote(EMO_HEART, 1600); }
    else react(REACT_BOUNCE, 1500, T(S_SAY_FUN));
  }
  // se tomo una medicina el solo
  if (pet.takeSelfMed()) {
    bubbleText = T(S_SAY_SELFMED);
    bubbleUntil = now + 4000;
    sfxPlay(SFX_EAT);
  }

  // region nueva abierta (se registraron todas las de huevo de la anterior)
  static uint32_t lastUnlockCheck = 0;
  static uint8_t knownUnlocked = 0;
  if (now - lastUnlockCheck > 5000) {
    lastUnlockCheck = now;
    uint8_t u = pet.unlockedRegions();
    if (knownUnlocked && u > knownUnlocked) {
      unlockShow = u - 1;
      unlockUntil = now + 6000;
      sfxPlay(SFX_MEDAL);
    }
    knownUnlocked = u;
  }

  // subio de nivel (mismo individuo): a celebrarlo. Cambiar de especie (nacer,
  // evolucionar, sacar de la granja) solo reinicia la cuenta.
  static int16_t lvSpecies = -2;
  static uint16_t lvLast = 0;
  if (pet.speciesId != lvSpecies || pet.isEgg()) {
    lvSpecies = pet.speciesId;
    lvLast = pet.level();
  } else if (pet.level() > lvLast) {
    lvLast = pet.level();
    levelUpShow = lvLast;
    levelUpUntil = now + 3000;
  }

  // cosecha de la granja: por la mañana, la primera vez que se mira la pantalla
  static uint32_t lastHarvestTry = 0;
  if (now - lastHarvestTry > 20000 && dimStage == 0 && mainScreenFree() && !pet.isEgg()) {
    lastHarvestTry = now;
    uint8_t hp, hi;
    if (pet.farmHarvest(farmReady() ? farmCount() : 0, hp, hi)) {
      char p1[16];
      snprintf(p1, sizeof(p1), T(S_HARVEST_PTS_FMT), hp);
      snprintf(harvestMsg, sizeof(harvestMsg), "%s%s%s", hp ? p1 : "", hp && hi != 0xFF ? "  " : "",
               hi != 0xFF ? T(ITEM_NAME[hi]) : "");
      harvestUntil = now + 6000;
      sfxPlay(SFX_MEDAL);
    }
  }

  // pasos (podometro del QMI8658) cada 5 s; tambien cambia de dia la cuenta
  static uint32_t lastStepRead = 0, stepCount = 0;
  if (imuOk && now - lastStepRead > 5000) {
    lastStepRead = now;
    uint32_t c = imu.getStepCount();
    uint32_t nuevos = c >= stepCount ? c - stepCount : c;  // el chip se reinicio: cuenta desde 0
    stepCount = c;
    pet.addSteps(nuevos);
  }

  // album: cada final se apunta en la SD
  MemRec mem;
  if (pet.takeMemoir(mem)) memoirAdd(mem);

  // latido de salud cada 5 min (para el soak test; se descarta si no hay monitor)
  static uint32_t lastHealth = 0;
  if (now - lastHealth > 300000) {
    lastHealth = now;
    Serial.printf("HEALTH up=%lus heap=%u min=%u bat=%d%% mv=%d chg=%d usb=%d dim=%u off=%d\n",
                  (unsigned long)(now / 1000), ESP.getFreeHeap(), ESP.getMinFreeHeap(),
                  batPercent(), batMillivolts(), batCharging() ? 1 : 0,
                  usbPresent() ? 1 : 0, dimStage, screenOff ? 1 : 0);
  }

  // Ritmo de dibujo (power.h): 10 fps con el bicho en uso, menos cuanto mas rato sin tocar,
  // 85 ms en juego/saco (margen seguro para que el redibujado no pise el envio DMA del frame
  // anterior: a 40-65 ms solapaba y causaba flashes negros).
  // Con la pantalla apagada no se dibuja: a brillo 0 no se ve nada, pero el redibujado y el
  // flush DMA del framebuffer del lienzo seguian corriendo a 10 fps, y medido son 70 ms de
  // trabajo por cada 100 (46 de ellos solo el volcado). Cada render repinta la escena
  // entera, asi que no queda nada a medias; y al quedarse lastRender congelado, el primer
  // frame tras despertar sale en el acto.
  PowerCtx pc = powerCtx(millis());
  audioSetIdle(!amplifierWanted(pc));  // el amplificador consume en reposo
  batSetPollMs(powerCacheMs(pc));
  uint32_t interval = frameIntervalMs(pc);
  bool drewFrame = false;
  if (interval && now - lastRender >= interval) {
    lastRender = now;
    drewFrame = render();
  }
  // cede la CPU en los pases que no dibujan: sin esto el loop gira a pleno rendimiento
  // entre frames; 1 ms no se nota en el tactil (se lee cada 20 ms) y el frame no se retrasa
  if (!drewFrame) idleWait(pc);
}

// estado de consumo actual (ver power.h)
PowerCtx powerCtx(uint32_t now) {
  PowerCtx c;
  c.idleMs = idleSince(now, lastInteract);
  c.screenOff = screenOff;
  c.sleeping = pet.sleeping;
  c.usb = usbPresent();
  c.charging = batCharging();
  c.batPct = batPercent();
  c.userDim = userDim;
  return c;
}

// el usuario ha interactuado: vuelve el ritmo normal de dibujo (sin esperar al proximo
// frame de un ritmo lento) y el amplificador se enciende antes de que suene nada
void wakeUI() {
  uint32_t now = millis();
  lastUserAt = now;
  wakeScreen(now);
}

// solo la pantalla (lo usan tambien los eventos visibles, que no son un toque)
void wakeScreen(uint32_t now) {
  uint32_t iv = frameIntervalMs(powerCtx(now));
  bool slow = !iv || iv > 100;  // pantalla apagada o ritmo lento: se dibuja ya, sin esperar al frame
  lastInteract = now;
  // el frame forzado respeta los 85 ms entre redibujados (si no, solapa el DMA del anterior)
  if (slow) lastRender = (now - lastRender >= 85) ? 0 : lastRender - 15;
  audioSetIdle(false);
}

// brillo segun sueno + inactividad (proteccion del AMOLED) y bateria
void updateBrightness(uint32_t now) {
  // los eventos visibles despiertan la pantalla solos
  if (!screenOff && (pet.evolving() || pet.ceremony || pet.eating() || pet.showHeart())) {
    if (dimStageFor(idleSince(now, lastInteract)) != DIM_AWAKE) wakeScreen(now);  // comer solo no es un toque
    else lastInteract = now;
  }
  PowerCtx c = powerCtx(now);
  dimStage = dimStageFor(c.idleMs);
  uint8_t target = brightnessFor(c);
  static uint8_t current = 255;
  if (target != current) {
    current = target;
    panel->setBrightness(target);
  }
}

// Pausa de un pase del bucle que no ha dibujado. Con la pantalla apagada, o encendida pero casi
// apagada (5 min sin tocar), y sin USB se duerme la CPU (light sleep) hasta el proximo frame o
// 200 ms: despierta por el temporizador o por un toque (el INT del tactil baja). Con USB no, para
// no perder el puerto serie de la tarjeta SD: se mira sin cache, por si se acaba de enchufar.
void idleWait(PowerCtx c) {
  // no se duerme justo tras dibujar (el volcado DMA del frame sigue en marcha) ni con un aviso sonando
  if (lightSleepAllowed(c) && millis() - lastRender >= 100 && !audioBusy()) {
    c.usb = usbPresentNow();
    if (!lightSleepAllowed(c)) { delay(idleDelayMs(c)); return; }
    if (digitalRead(TP_INT) == LOW) { gTouchIrq = true; delay(1); return; }  // toque pendiente: no se duerme
    uint32_t ms = POWER_SLEEP_MS;
    uint32_t iv = frameIntervalMs(c);
    if (iv) {  // pantalla encendida: hasta el proximo frame
      uint32_t due = lastRender + iv - millis();
      if ((int32_t)due < (int32_t)ms) ms = (int32_t)due > 1 ? due : 1;
    }
    esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000);
    gpio_wakeup_enable((gpio_num_t)TP_INT, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
    // la interrupcion del tactil vuelve a ser por flanco (como la dejo attachInterrupt); un toque
    // que ya haya bajado la linea se registra a mano
    gpio_wakeup_disable((gpio_num_t)TP_INT);
    gpio_set_intr_type((gpio_num_t)TP_INT, GPIO_INTR_NEGEDGE);
    if (digitalRead(TP_INT) == LOW) gTouchIrq = true;
    return;
  }
  delay(idleDelayMs(c));
}
