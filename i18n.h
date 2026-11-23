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
enum StrId : uint16_t {  // mas de 256 textos
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
  S_NO_BACKUP, S_RESTORE_Q, S_RESTORING,
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
  // diez mil pasos en el dia: premio
  S_SAY_STEP_BONUS,
  // pagina de actividades (a la izquierda de la principal): granja y pruebas deportivas
  S_HUB_TITLE, S_HUB_SPORTS, S_HUB_SPORTS_HINT, S_ASLEEP, S_SWIPE_R_BACK,
  // megaevolucion: la piedra, el nombre, por que no se puede, y los avisos de empezar y acabar
  S_ITEM_MEGA_STONE, S_MEGA_NAME_FMT, S_MEGA_NO_FORM, S_MEGA_ALREADY, S_SAY_MEGA, S_SAY_MEGA_END,
  // sports day: nombres, que cuenta en cada prueba, rivales, marcas y resultado
  S_EV_SPRINT, S_EV_HURDLES, S_EV_SWIM, S_EV_LONGJUMP, S_EV_HIGHJUMP,
  S_HOW_SPRINT, S_HOW_HURDLES, S_HOW_SWIM, S_HOW_LONGJUMP, S_HOW_HIGHJUMP,
  S_TAP_START, S_GO, S_GOLD, S_SILVER, S_BRONZE,
  S_GOT_STONE, S_STONE_TODAY, S_SECS_FMT, S_METERS_FMT, S_VS_FMT, S_TOO_TIRED, S_YOU,
  // tipos con ventaja en cada prueba, y lo que le toca al compañero
  S_BONUS_SPRINT, S_BONUS_HURDLES, S_BONUS_SWIM, S_BONUS_LONGJUMP, S_BONUS_HIGHJUMP, S_BONUS_UP_FMT, S_BONUS_DOWN_FMT,
  // premio de la hierba: delante va lo que habia
  S_GRASS_FOUND,
  // estadisticas: codigo para el Atajo del movil que manda los pasos
  S_ST_PHONE,
  // pruebas nuevas (gimnasios y torneo) y gimnasios: lista, equipo, rondas, tipos
  S_EV_LIFT, S_EV_CLIMB, S_EV_DIVE, S_EV_SKATE, S_EV_TORCH, S_EV_TRAIL,
  S_HOW_LIFT, S_HOW_CLIMB, S_HOW_DIVE, S_HOW_SKATE, S_HOW_TORCH, S_HOW_TRAIL,
  S_BONUS_LIFT, S_BONUS_CLIMB, S_BONUS_DIVE, S_BONUS_SKATE, S_BONUS_TORCH, S_BONUS_TRAIL,
  S_KG_FMT, S_PTS_FMT, S_MINSEC_FMT, S_GYMS, S_GYMS_TITLE_FMT, S_BADGES_FMT,
  S_LEAGUE, S_CHAMPION, S_GYMS_HINT, S_LEAGUE_LOCKED, S_GYM_TRIED, S_GYM_NEED_BADGE,
  S_NEED_FARM2, S_ELITE, S_CHAMP_ROLE, S_CHALLENGE, S_REMATCH, S_PICK_TWO,
  S_NEXT, S_LINEUP, S_LINEUP_HINT, S_START, S_ROUND_FMT, S_ROUND_WON,
  S_ROUND_LOST, S_TAP_NEXT, S_GOT_BADGE_FMT, S_ALSO_GOT_FMT, S_GYM_LOST, S_LEAGUE_WON,
  S_LEAGUE_LOST, S_TY_NORMAL, S_TY_FIRE, S_TY_WATER, S_TY_GRASS, S_TY_ELECTRIC,
  S_TY_ICE, S_TY_FIGHTING, S_TY_POISON, S_TY_GROUND, S_TY_FLYING, S_TY_PSYCHIC,
  S_TY_BUG, S_TY_ROCK, S_TY_GHOST, S_TY_DRAGON, S_TY_DARK, S_TY_STEEL,
  S_TY_FAIRY,
  S_BADGES_TOTAL_FMT,
  // ocio (fun.ino): actividades, huerto, pesca, mina, expedicion, objetos y frutas
  S_HUB_FUN, S_HUB_FUN_HINT, S_FUN_TITLE_FMT, S_FUN_GARDEN, S_FUN_FISH,
  S_FUN_MINE, S_FUN_QUEST, S_NEED_GARDEN, S_NEED_FISH, S_NEED_MINE,
  S_NEED_QUEST, S_FUN_WHO_FMT, S_PTS_COST_FMT, S_LUCK_WON, S_LUCK_PLAYED,
  S_LUCK_NONE, S_WHO_GOES, S_AWAY, S_MIN_LEFT_FMT, S_HOURS_LEFT_FMT,
  S_RIPE_FMT, S_QUEST_BACK, S_PLANT, S_WATER, S_REAP,
  S_WATERED, S_RIPE, S_TIME_UNSET, S_CAST, S_WAIT_BITE,
  S_BITE, S_GREAT_CATCH, S_SMALL_CATCH, S_DIG, S_TAP_ROCK,
  S_GO_OUT, S_BACK_IN_HOUR, S_CLAIM, S_QUEST_BUSY, S_LOOT,
  S_ITEM_FRUIT_STA, S_ITEM_FRUIT_RES, S_ITEM_FRUIT_VIT, S_ITEM_FRUIT_GOLD, S_ITEM_FISH,
  S_ITEM_PEARL, S_ITEM_SHARD, S_ITEM_EGG_WATER, S_ITEM_EGG_FOSSIL, S_FRUIT_FULL,
  S_SHARD_AUTO, S_FRUIT_STA_UP, S_FRUIT_RES_UP, S_FRUIT_VIT_UP, S_FRUIT_GOLD_UP,
  S_PEARL_HAPPY, S_FISH_YUM,
  // revision de pantallas: activ., ajustes, tienda, pokedex, granja, gimnasio, estadisticas
  S_HUB_FARM, S_FARM_SUB_FMT, S_RESTORE, S_OWN_FMT, S_GOOD_AT_FMT,
  S_BASE_FMT, S_TRAIN_FMT, S_GYM_TIP1, S_GYM_TIP2, S_ST_BADGES,
  S_ST_CHAMP, S_ST_FUN,
  S_HOURS_FMT, S_MINS_FMT,  // huerto: lo que le falta a cada parcela (corto, para que quepa)
  S_BUY_FOR_FMT, S_SPEND_Q_FMT,  // confirmar antes de gastar puntos (tienda, ocio)
  // libro de puntos (ultima pagina de la tienda) y resumen de la semana
  S_BOOK_FMT, S_THIS_WEEK, S_BOOK_CHESS_FMT, S_BOOK_IN_FMT, S_BOOK_OUT_FMT,
  S_BOOK_LAST_FMT, S_BOOK_CHESS_TOP, S_BOOK_MORE_CHESS, S_WEEK_BANNER, S_WEEK_SUM_FMT,
  S_HOCKEY_BANNER, S_HOCKEY_FMT, S_HOCKEY_MAX_FMT,  // practica de hockey desde el movil
  // "见面": encontrar otra placa y hacerse amigos (link_ui.ino)
  S_HUB_LINK, S_HUB_LINK_HINT, S_LINK_WAITING, S_LINK_NOBODY, S_LINK_TAP,
  S_LINK_FRIEND_OK, S_LINK_FRIEND, S_LINK_REPAIR, S_LINK_FAR, S_LINK_UPDATE,
  S_LINK_ASK_FMT, S_LINK_ASK2, S_LINK_ASKED2, S_LINK_WAIT_YES,
  S_LINK_CODE_HINT, S_LINK_DONE_FMT, S_LINK_FAIL, S_LINK_SAID_NO, S_LINK_REPAIR_Q,
  S_LINK_FULL,
  // juegos con un amigo (friend_play.ino): menu, jugar, regalos, invitaciones, fotos
  S_FR_PLAY, S_FR_PLAY_SUB, S_FR_GIFT, S_FR_GIFT_SUB, S_FR_RACE,
  S_FR_RACE_SUB, S_FR_GYM, S_FR_GYM_SUB, S_FR_PHOTO, S_FR_PHOTO_SUB_FMT,
  S_FR_OLD, S_FR_HIFIVE_HINT, S_FR_HIFIVE, S_FR_GIFT_PICK, S_FR_GIFT_EMPTY,
  S_FR_GIFT_Q, S_FR_GIFT_WAIT, S_FR_GIFT_IN_FMT, S_FR_GIFT_IN2_FMT, S_FR_GIFT_SENT,
  S_FR_GIFT_BACK, S_FR_GIFT_GOT_FMT, S_FR_RACE_IN_FMT, S_FR_GYM_IN_FMT, S_FR_GO_Q_FMT,
  S_FR_WAIT_FRIEND, S_FR_BUSY, S_FR_CANT, S_FR_GONE, S_PICK_ONE,
  S_FR_WON, S_FR_LOST, S_FR_NO_PHOTOS, S_FR_RUN, S_FR_CATCH,
  S_FR_DANCE,
  // amigos lejos (mail_ui.ino): pagina "好友", mandar y el mensajero
  S_LINK_TITLE, S_HUB_FRIENDS_SUB, S_ML_READY, S_ML_NEED_MEET, S_ML_NO_FRIENDS,
  S_ML_SYNC, S_ML_SYNCING, S_ML_VISIT, S_ML_VISIT_SUB, S_ML_CARD,
  S_ML_CARD_SUB, S_ML_GIFT, S_ML_ONWAY_FMT, S_ML_PICK_MSGR, S_ML_PICK_PHRASE,
  S_ML_PICK_STICKER, S_ML_CARRIES_FMT, S_ML_SEND_Q, S_ML_VISIT_Q_FMT, S_ML_SENT,
  S_ML_FULL, S_ML_LIMIT, S_ML_FROM_CARD_FMT, S_ML_FROM_GIFT_FMT, S_ML_VISITED_FMT,
  S_ML_BACK, S_ML_TAP,
  S_PH_HELLO, S_PH_MISS, S_PH_PLAY, S_PH_TOMORROW, S_PH_MORNING, S_PH_DREAMS,
  S_PH_THANKS, S_PH_CHEER, S_PH_BIRTHDAY, S_PH_HAPPY, S_PH_CHESS, S_PH_EVOLVED,
  S_BACKUP, S_BK_SAVE, S_BK_SAVE_Q, S_BK_SAVED, S_BK_FAIL,
  S_REWARD_BANNER, S_REWARD_FMT, S_BOOK_EXTRA_FMT,
  // ficha "成长": por que el boton de evolucionar esta apagado
  S_EVO_ASLEEP, S_EVO_FARM,
  // la ciudad del portal Wi-Fi, al buscarla
  S_PLACE_OK, S_PLACE_BAD,
  S_ITEM_POTION_END,
  S_VOL_HIGH, S_VOL_MED, S_VOL_LOW,
  S_NEXT_LVL_H_FMT,
  S_ML_NO_WIFI,  // amigos lejos: sin Wi-Fi configurado el correo no sale nunca
  S_UNLOCK_EASY_HINT,  // region cerrada con las regiones faciles del portal (issue #13)
  STR_COUNT
};

const char *T(StrId id);       // texto en el idioma activo
const char *medalName(int i);  // banner de medalla (MED_COUNT)
const char *medalLabel(int i); // etiqueta corta de medalla
const char *medalDesc(int i);  // descripcion larga de medalla

void loadLang();             // lee el idioma de NVS (llamar en setup)
void setLang(Lang l);        // cambia y persiste el idioma
