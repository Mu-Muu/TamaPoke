#pragma once
#include <Arduino.h>

// Idiomas soportados: ingles y chino. El ingles usa la fuente 5x7 de Arduino_GFX
// (tabla CP437, un byte por caracter: la UI centra con strlen(t)*6, asi que nada
// de UTF-8 ahi). El chino va en UTF-8 con una fuente U8g2.
enum Lang : uint8_t { LANG_EN = 0, LANG_ZH, LANG_COUNT };
// idiomas que necesitan una fuente U8g2 y texto UTF-8 en vez de la tabla CP437
#define LANG_IS_CJK(l) ((l) == LANG_ZH)
// idioma por defecto (placa nueva o NVS borrada): ingles. El instalador web lo
// usa gente de fuera; una placa que ya eligio idioma lo tiene en la NVS ("lang2").
#define LANG_DEFAULT LANG_EN

extern Lang gLang;  // idioma activo (definido en i18n.cpp)

// IDs de cadena. El orden debe coincidir con la tabla STRINGS de i18n.cpp.
enum StrId : uint8_t {
  // estado del bicho (statusMsg)
  S_EVOLVING, S_EATING, S_LIKES, S_HUNGRY, S_NEEDS_BATH,
  S_EXHAUSTED, S_SAD, S_CHUBBY, S_IS_SHINY, S_HAPPY,
  // ceremonias de despedida
  S_FAREWELL, S_RUNAWAY, S_GOODBYE,
  // huevo
  S_EGG_HDR, S_EGG_LEGEND, S_EGG_RARE, S_EGG_TOUCH, S_EGG_MOVES, S_EGG_ALMOST,
  // formatos compartidos
  S_POKEDEX_FMT,   // "POKEDEX %u/1025"
  S_NAME_FMT,      // "%s%s Nv.%u"
  // dialogo soltar
  S_RELEASE_FMT, S_YES, S_NO,
  // reloj / ajustes
  S_SET_TIME, S_CLOCK_CANCEL, S_LANG_LABEL,  // el primero es el titulo de los ajustes
  // celebracion
  S_MEDAL_BANNER, S_GREAT, S_STREAK_DAYS_FMT,
  // ficha: perfil
  S_STREAK_FMT, S_VIN,
  S_INFO_FMT, S_RENAME_HINT,  // info: edad en dias
  // ficha: combate
  S_BATTLE, S_STAT_ATK, S_STAT_DEF, S_STAT_SPE, S_STAT_WGT,
  // ficha: medallas
  S_MEDALS_FMT, S_BACK,
  // teclado y galeria
  S_NAME, S_DETAIL_BACK,
  // barras
  S_BAR_FOOD, S_BAR_JOY, S_BAR_ENE, S_BAR_HYG,
  // ficha: pagina de progreso
  S_PROGRESS, S_LVL_FMT, S_NEXT_LVL_FMT, S_EVO_LABEL, S_FINAL_FORM,
  S_EVO_READY, S_EVO_BLOCKED, S_EVO_IN_FMT, S_MISTAKES_FMT,
  // interruptor de sonido (ajustes)
  S_SND_ON, S_SND_OFF,
  S_EVO_TAP,        // texto del boton de evolucion
  S_FAREWELL_BTN,   // texto del boton de despedida (lleva el nombre: "%s ...")
  S_RUNAWAY_BTN,    // texto del boton de escapada por abandono (final triste)
  // dialogos de decision (evolucionar/mantener, despedirse/quedaros)
  S_EVO_Q, S_EVO_KEEP, S_FAR_Q, S_FAR_GO, S_FAR_STAY,
  S_CHOOSE_STARTER,  // titulo de la eleccion del inicial (primera vez)
  S_NO_SPRITES, S_LOAD_SPRITES,  // aviso cuando falta el sprite en la SD
  S_SICK,          // enfermo (barra de estado)
  // granja
  S_COLLECT, S_RELEASE, S_FARM_FMT, S_FARM_EMPTY, S_SWAP, S_HATCH_FIRST, S_TO_FARM,
  S_NEED_SD,
  // regiones de la pokedex (gen 1-9, en el orden de REGIONS de regions.h)
  S_REGION_1, S_REGION_2, S_REGION_3, S_REGION_4, S_REGION_5, S_REGION_6, S_REGION_7,
  S_REGION_8, S_REGION_9,
  // tienda y mochila (orden de ITEM_* en pet.h para los nombres)
  S_SHOP_FMT, S_BAG,
  S_ITEM_CANDY, S_ITEM_MEDICINE, S_ITEM_POTION_STR, S_ITEM_POTION_SPD, S_ITEM_SHINY_CHARM,
  S_ITEM_EGG_COMMON, S_ITEM_EGG_RARE, S_ITEM_EGG_LEGEND,
  S_BOUGHT, S_NO_POINTS, S_NONE_LEFT, S_USED, S_MAXED, S_NOT_SICK, S_CHARM_AUTO,
  S_EGG_NEED_EMPTY, S_NO_PARTNER, S_PICK_EGG, S_GO_SHOP,
  // resumen del ajedrez (lo que gano ayer)
  S_REPORT_FMT, S_REPORT_PTS_FMT, S_REPORT_BEST,
  // wi-fi y sincronizacion
  S_WIFI_SETUP, S_WIFI_HINT1, S_WIFI_HINT2, S_WIFI_SAVED, S_SYNCING, S_SYNC_FAIL,
  // fiestas: saludo en la cabecera (orden de HOL_* en holiday.h, desde HOL_NEWYEAR)
  S_HOL_NEWYEAR, S_HOL_CNY, S_HOL_LANTERN, S_HOL_VALENTINE, S_HOL_EASTER, S_HOL_MOTHER,
  S_HOL_DRAGONBOAT, S_HOL_FATHER, S_HOL_CANADA, S_HOL_MIDAUTUMN, S_HOL_THANKSGIVING,
  S_HOL_HALLOWEEN, S_HOL_CHRISTMAS, S_HOL_BIRTHDAY,
  // pokedex de una region completa
  S_REGION_DONE_FMT, S_REGION_REWARD_FMT, S_ITEM_EGG_REGION,
  // el tiempo de fuera (cabecera)
  S_WX_LIKE_RAIN, S_WX_HATE_RAIN, S_WX_LIKE_SNOW, S_WX_COLD, S_WX_LIKE_SUN,
  // pasos de hoy (ficha)
  S_STEPS_FMT,
  // album de recuerdos (en la granja) y como acabo cada uno (orden de CER_FAREWELL..CER_COLLECT)
  S_ALBUM, S_ALBUM_FMT, S_ALBUM_EMPTY,
  S_END_FAREWELL, S_END_RUNAWAY, S_END_RELEASE, S_END_FARM,
  // caracter (orden de NAT_* en pet.h)
  S_NAT_EASY, S_NAT_GLUTTON, S_NAT_PLAYFUL, S_NAT_TIDY, S_NAT_LAZY,
  // bocadillos: lo que dice el compañero de vez en cuando
  S_SAY_MORNING, S_SAY_NIGHT, S_SAY_HUNGRY, S_SAY_DIRTY, S_SAY_BORED, S_SAY_TIRED,
  S_SAY_CHESS_WIN_FMT, S_SAY_CHESS_PLAYED, S_SAY_LOVE, S_SAY_HI,
  S_SAY_GLUTTON, S_SAY_PLAYFUL, S_SAY_TIDY, S_SAY_LAZY, S_SAY_EASY,
  // complementos (orden de ACC_* en pet.h) y su pagina de la tienda
  S_ACC_BOW, S_ACC_FLOWER, S_ACC_CAP, S_ACC_WITCH, S_ACC_TOPHAT, S_ACC_CROWN,
  S_ACCESSORIES, S_OWNED, S_WEARING, S_PUT_ON, S_TAKE_OFF,
  // visita de la granja
  S_VISIT_FMT,
  // copia de seguridad (ajustes)
  S_RESTORE_FMT, S_NO_BACKUP, S_RESTORE_Q, S_RESTORING,
  // caricia, cosecha de la granja
  S_SAY_STROKE, S_HARVEST, S_HARVEST_PTS_FMT,
  // gestos: cabeza, barriga, cosquillas, harto, vuelta
  S_SAY_HEAD, S_SAY_BELLY, S_SAY_TICKLE, S_SAY_ANNOYED, S_SAY_SPIN,
  // vida propia: comer, jugar, desperezarse, medicina por su cuenta
  S_SAY_YUM, S_SAY_FUN, S_SAY_STRETCH, S_SAY_SELFMED,
  // pocion de evolucion (tienda y mochila)
  S_ITEM_POTION_EVO, S_CANT_USE,
  // regiones que se abren en orden (pokedex)
  S_REGION_LOCKED, S_UNLOCK_HINT, S_REGION_UNLOCK,
  // capacidades: lo que hace cada una (pagina de la ficha)
  S_FX_STA_FMT, S_FX_VIT_FMT, S_FX_RES_FMT,
  // reloj de espera: fecha y dia de la semana (0 = domingo)
  S_DATE_FMT, S_WD_0, S_WD_1, S_WD_2, S_WD_3, S_WD_4, S_WD_5, S_WD_6,
  // estadisticas (boton en los ajustes)
  S_STATS, S_ST_RAISED, S_ST_DEX, S_ST_SHINY, S_ST_UNLOCKED, S_ST_DONE, S_ST_STREAK, S_ST_MEDALS,
  S_ST_STEPS, S_ST_CHESS, S_ST_FARM,
  // portada de cada region en la pokedex
  S_TAP_OPEN,
  // ajustes: sincronizar el ajedrez ya, y aviso si no habia nada nuevo
  S_SYNC_NOW, S_SYNC_NONE,
  // la cuenta de chess.com se pone en el portal Wi-Fi: pista en su pantalla y
  // aviso si se pide sincronizar sin ella
  S_WIFI_HINT3, S_NEED_CHESS,
  STR_COUNT
};

const char *T(StrId id);       // texto en el idioma activo
const char *medalName(int i);  // banner de medalla (MED_COUNT)
const char *medalLabel(int i); // etiqueta corta de medalla
const char *medalDesc(int i);  // descripcion larga de medalla

void loadLang();             // lee el idioma de NVS (llamar en setup)
void setLang(Lang l);        // cambia y persiste el idioma
