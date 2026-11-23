// Ficha del bicho, ajustes, estadisticas, medallas y teclado.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

// ---------- ficha del bicho (deslizar vertical) ----------

// x donde arrancan las barras de la ficha. Estaba fijo en 150, que daba de sobra
// para etiquetas latinas de 3 caracteres pero no para las japonesas, mas anchas:
// こうげき se metia dentro de la barra. Se calcula a partir de la etiqueta mas
// larga para que las cuatro barras sigan alineadas en cualquier idioma, con el
// valor original como suelo (asi en los idiomas latinos no cambia nada).
// Requiere tener ya puesto el tamano de texto 2, porque textW() lo necesita.
static int statBarX() {
  const StrId ids[] = { S_STAT_ATK, S_STAT_DEF, S_STAT_SPE, S_STAT_WGT, S_VIN };
  int ancho = 0;
  for (StrId id : ids) {
    int w = textW(T(id), 2);
    if (w > ancho) ancho = w;
  }
  int x = 96 + ancho + 12;   // 12 px de aire entre etiqueta y barra
  return x < 150 ? 150 : x;
}

void drawCardStat(int y, const char *label, uint16_t val, uint16_t maxBar, uint16_t color) {
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(96, y);
  printT(label);
  char num[8];
  snprintf(num, sizeof(num), "%u", val);
  setCur(330, y);
  printT(num);
  int bx = statBarX();
  int bw = 310 - bx;   // la barra siempre acaba en 310, dejando aire hasta el numero
  int fw = (int)val * bw / maxBar;
  if (fw > bw) fw = bw;
  gfx->fillRoundRect(bx, y + 2, bw, 11, 3, UI_TRACK);
  if (fw > 2) gfx->fillRoundRect(bx, y + 2, fw, 11, 3, color);
}

// ---------- ajustes (deslizar abajo) ----------
// La hora ya no se pone a mano: se corrige sola por NTP (al arrancar, cada hora
// y en la sincronizacion de medianoche). Aqui solo se enseña.

void openClock() { clockOpen = true; }

// ajustes: la hora en grande y cuatro botones grandes en 2 x 2
#define SET_BTN_W 156
#define SET_BTN_H 60
#define SET_BTN_X0 72    // columnas: 72 y 238
#define SET_BTN_X1 238
#define SET_BTN_Y0 190   // filas: 190 y 262
#define SET_BTN_Y1 262
#define STATS_BTN_Y 334
#define STATS_BTN_H 48
// la fila de abajo son dos botones de texto (72 y 276) con el brillo en medio,
// mas estrecho porque solo lleva su icono
#define STATS_BTN_W 118
#define STATS_BTN_X1 276
#define BRI_BTN_X 198
#define BRI_BTN_W 70
// unidad de la temperatura: una pildora pequeña entre el reloj y los botones
#define TEMP_PILL_X 193
#define TEMP_PILL_W 80
#define TEMP_PILL_Y 155
#define TEMP_PILL_H 28
static const char *const LANG_CODES[LANG_COUNT] = { "EN", "ZH" };

// Boton de brillo: un sol y tres barras, encendidas las del nivel de ahora.
// Sin texto a proposito: cabe en poco sitio y se entiende en los dos idiomas.
static void drawBrightnessBtn() {
  gfx->fillRoundRect(BRI_BTN_X, STATS_BTN_Y, BRI_BTN_W, STATS_BTN_H, 14, UI_WHITE);
  gfx->drawRoundRect(BRI_BTN_X, STATS_BTN_Y, BRI_BTN_W, STATS_BTN_H, 14, UI_INK);
  int sx = BRI_BTN_X + 20, sy = STATS_BTN_Y + STATS_BTN_H / 2;
  gfx->fillCircle(sx, sy, 5, UI_INK);
  static const int8_t RAY[8][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
                                    { 1, 1 }, { -1, -1 }, { 1, -1 }, { -1, 1 } };
  for (int i = 0; i < 8; i++) {
    int k = i < 4 ? 8 : 6, e = i < 4 ? 11 : 9;  // las diagonales algo mas cortas
    gfx->drawLine(sx + RAY[i][0] * k, sy + RAY[i][1] * k, sx + RAY[i][0] * e, sy + RAY[i][1] * e, UI_INK);
  }
  for (int i = 0; i < BRIGHT_LEVELS; i++) {  // alto: tres barras; medio: dos; bajo: una
    int bh = 9 + i * 5, x = BRI_BTN_X + 34 + i * 11, y = sy + 11 - bh;
    if (i < BRIGHT_LEVELS - userDim) gfx->fillRect(x, y, 8, bh, UI_BAR_OK);
    gfx->drawRect(x, y, 8, bh, UI_INK);
  }
}

// un boton de los ajustes; ts: el tamaño de toda la pagina (el mismo en todos); sub: 2a linea
static void setButton(int x, int y, const char *l, bool on, bool apagado, int ts, const char *sub = nullptr) {
  gfx->fillRoundRect(x, y, SET_BTN_W, SET_BTN_H, 14, apagado ? UI_TRACK : on ? UI_BAR_OK : UI_WHITE);
  gfx->drawRoundRect(x, y, SET_BTN_W, SET_BTN_H, 14, UI_INK);
  gfx->setTextColor(on ? UI_BG_DAY : apagado ? UI_HINT : UI_INK);
  setSize(ts);
  int alto = 7 * ts + (sub ? 18 : 0), y0 = y + (SET_BTN_H - alto) / 2;
  setCur(x + (SET_BTN_W - textW(l, ts)) / 2, y0);
  printT(l);
  if (sub) {
    gfx->setTextColor(on ? UI_BG_DAY : apagado ? UI_HINT : UI_INK);
    setSize(2);
    setCur(x + (SET_BTN_W - textW(sub, 2)) / 2, y0 + 7 * ts + 4);
    printT(sub);
  }
}

void renderClock() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_SET_TIME), 3), 40);
  printT(T(S_SET_TIME));

  // la hora de ahora, en grande (se corrige sola por Wi-Fi)
  uint32_t ahora = shownClock(pet.lastSeenEpoch ? pet.lastSeenEpoch : rtcEpoch());
  char t[8];
  snprintf(t, sizeof(t), "%02lu:%02lu", (unsigned long)((ahora / 3600) % 24), (unsigned long)((ahora / 60) % 60));
  setSize(10);
  setCur(centerX(t, 10), 78);  // algo subido: debajo va la pildora de la temperatura
  printT(t);

  // unidad de la temperatura: "°C >" o "°F >", cicla al tocar
  gfx->fillRoundRect(TEMP_PILL_X, TEMP_PILL_Y, TEMP_PILL_W, TEMP_PILL_H, 10, UI_WHITE);
  gfx->drawRoundRect(TEMP_PILL_X, TEMP_PILL_Y, TEMP_PILL_W, TEMP_PILL_H, 10, UI_INK);
  {
    const char *u = tempF ? "F >" : "C >";
    setSize(2);
    int x0 = TEMP_PILL_X + (TEMP_PILL_W - textW(u, 2) - 8) / 2, y0 = TEMP_PILL_Y + (TEMP_PILL_H - 14) / 2;
    gfx->drawCircle(x0 + 2, y0 + 2, 2, UI_INK);  // el grado, como en la cabecera
    gfx->setTextColor(UI_INK);
    setCur(x0 + 8, y0);
    printT(u);
  }

  // sonido, wi-fi (verde si ya hay red), idioma, la copia de seguridad (su fecha debajo)
  char lp[10];
  snprintf(lp, sizeof(lp), "%s >", LANG_CODES[gLang]);
  uint32_t bk = 0;
  bool hayCopia = backupInfo(bk);
  char fecha[12] = "";
  if (hayCopia) {
    time_t bt = bk;
    struct tm btm;
    gmtime_r(&bt, &btm);
    snprintf(fecha, sizeof(fecha), "%u/%u", (unsigned)(btm.tm_mon + 1), (unsigned)btm.tm_mday);
  }
  const char *snd = audioEnabled() ? T(S_SND_ON) : T(S_SND_OFF);
  const StrId bl[2] = { S_STATS, S_SYNC_NOW };
  // un solo tamaño para todos los botones: el 3 si caben todos, si no el 2
  const char *grandes[4] = { snd, T(S_WIFI_SETUP), lp, T(S_BACKUP) };
  int ts = 3;
  for (const char *l : grandes)
    if (textW(l, 3) > SET_BTN_W - 16) ts = 2;
  for (StrId id : bl)
    if (textW(T(id), 3) > STATS_BTN_W - 14) ts = 2;
  static const StrId VOL_TXT[VOL_LEVELS] = { S_VOL_HIGH, S_VOL_MED, S_VOL_LOW };  // debajo, el volumen
  setButton(SET_BTN_X0, SET_BTN_Y0, snd, audioEnabled(), false, ts,
            audioEnabled() ? T(VOL_TXT[audioVolume() % VOL_LEVELS]) : nullptr);
  setButton(SET_BTN_X1, SET_BTN_Y0, T(S_WIFI_SETUP), wifiHasCreds(), false, ts);
  setButton(SET_BTN_X0, SET_BTN_Y1, lp, false, false, ts);
  setButton(SET_BTN_X1, SET_BTN_Y1, T(S_BACKUP), false, !sdReady, ts, hayCopia ? fecha : nullptr);

  // debajo: estadisticas, brillo y sincronizar el ajedrez ya (gris sin wi-fi o sin cuenta de chess.com)
  const int16_t bx[2] = { SET_BTN_X0, STATS_BTN_X1 };
  bool puedeSync = wifiHasCreds() && chessHasAccount();
  for (int i = 0; i < 2; i++) {
    gfx->fillRoundRect(bx[i], STATS_BTN_Y, STATS_BTN_W, STATS_BTN_H, 14, UI_WHITE);
    gfx->drawRoundRect(bx[i], STATS_BTN_Y, STATS_BTN_W, STATS_BTN_H, 14, UI_INK);
    gfx->setTextColor(i == 1 && !puedeSync ? UI_HINT : UI_INK);
    const char *l = T(bl[i]);
    setSize(ts);
    setCur(bx[i] + (STATS_BTN_W - textW(l, ts)) / 2, STATS_BTN_Y + (STATS_BTN_H - 7 * ts) / 2);
    printT(l);
  }
  drawBrightnessBtn();

  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_CLOCK_CANCEL), 2), 396);
  printT(T(S_CLOCK_CANCEL));

  // version del firmware (discreta, abajo del todo)
  char ver[32];  // "TamaPoke v2026.11.15": con 20 se cortaba la ultima cifra
  snprintf(ver, sizeof(ver), "TamaPoke v%s", FW_VERSION);
  setSize(1);
  setCur(centerX(ver, 1), 422);
  printT(ver);

  drawToast();  // "copia guardada" (o que fallo)
  if (backupAsk) {  // la copia: que hacer con ella y, despues, ¿seguro?
    gfx->fillRoundRect(64, 168, 338, 152, 16, UI_WHITE);
    gfx->drawRoundRect(64, 168, 338, 152, 16, UI_INK);
    char tit[48];
    if (backupAsk != BK_MENU) snprintf(tit, sizeof(tit), "%s", T(backupAsk == BK_ASK_SAVE ? S_BK_SAVE_Q : S_RESTORE_Q));
    else if (hayCopia) snprintf(tit, sizeof(tit), "%s %s", T(S_BACKUP), fecha);
    else snprintf(tit, sizeof(tit), "%s", T(S_NO_BACKUP));
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(tit, 2), 200);
    printT(tit);
    int ty = BK_BTN_Y + (BK_BTN_H - 14) / 2;
    if (backupAsk == BK_MENU) {  // guardar ya / restaurar (gris si aun no hay copia)
      const char *l[2] = { T(S_BK_SAVE), T(S_RESTORE) };
      const int16_t bx[2] = { BK_MENU_X0, BK_MENU_X1 };
      for (int i = 0; i < 2; i++) {
        bool gris = i == 1 && !hayCopia;
        gfx->fillRoundRect(bx[i], BK_BTN_Y, BK_MENU_W, BK_BTN_H, 12, gris ? UI_TRACK : UI_WHITE);
        gfx->drawRoundRect(bx[i], BK_BTN_Y, BK_MENU_W, BK_BTN_H, 12, UI_INK);
        gfx->setTextColor(gris ? UI_HINT : UI_INK);
        setCur(bx[i] + (BK_MENU_W - textW(l[i], 2)) / 2, ty);
        printT(l[i]);
      }
    } else {
      gfx->fillRoundRect(BK_YES_X, BK_BTN_Y, BK_YN_W, BK_BTN_H, 12, UI_BAR_OK);
      gfx->setTextColor(UI_WHITE);
      setCur(BK_YES_X + (BK_YN_W - textW(T(S_YES), 2)) / 2, ty - 1);
      printT(T(S_YES));
      gfx->fillRoundRect(BK_NO_X, BK_BTN_Y, BK_YN_W, BK_BTN_H, 12, UI_BAR_BAD);
      setCur(BK_NO_X + (BK_YN_W - textW(T(S_NO), 2)) / 2, ty - 1);
      printT(T(S_NO));
    }
  }
  gfx->flush();
}

// estadisticas de siempre (desde los ajustes)
static void statRow(int y, StrId label, const char *val) {
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(78, y);
  printT(T(label));
  gfx->setTextColor(0x4C98);
  setCur(388 - textW(val, 2), y);
  printT(val);
}

void renderStats() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_STATS), 3), 40);
  printT(T(S_STATS));
  unsigned shiny = 0;
  for (int d = 1; d <= PET_DEX_MAX; d++)
    if (pet.isShinyRegistered(d)) shiny++;
  char v[24];
  int y = 76, dy = 24;  // 14 filas: mas juntas, para que quepan en el circulo
  snprintf(v, sizeof(v), "%u", pet.raisedCount);
  statRow(y, S_ST_RAISED, v);
  snprintf(v, sizeof(v), "%u/%d", pet.registeredCount(), PET_DEX_MAX);
  statRow(y += dy, S_ST_DEX, v);
  snprintf(v, sizeof(v), "%u", shiny);
  statRow(y += dy, S_ST_SHINY, v);
  snprintf(v, sizeof(v), "%u/%d", pet.unlockedRegions(), REGION_COUNT);
  statRow(y += dy, S_ST_UNLOCKED, v);
  snprintf(v, sizeof(v), "%d/%d", __builtin_popcount(pet.regionDone), REGION_COUNT);
  statRow(y += dy, S_ST_DONE, v);
  snprintf(v, sizeof(v), "%u", pet.bestStreak);
  statRow(y += dy, S_ST_STREAK, v);
  snprintf(v, sizeof(v), "%u", pet.totalMedals);
  statRow(y += dy, S_ST_MEDALS, v);
  snprintf(v, sizeof(v), "%u", pet.totalBadges());
  statRow(y += dy, S_ST_BADGES, v);
  snprintf(v, sizeof(v), "%u", pet.leagueTitles());
  statRow(y += dy, S_ST_CHAMP, v);
  snprintf(v, sizeof(v), "%lu", (unsigned long)pet.funFinds);
  statRow(y += dy, S_ST_FUN, v);
  snprintf(v, sizeof(v), "%lu", (unsigned long)pet.stepsTotal);
  statRow(y += dy, S_ST_STEPS, v);
  snprintf(v, sizeof(v), "%lu", (unsigned long)pet.chessPtsTotal);
  statRow(y += dy, S_ST_CHESS, v);
  if (farmReady()) snprintf(v, sizeof(v), "%d", farmCount());
  else snprintf(v, sizeof(v), "-");
  statRow(y += dy, S_ST_FARM, v);
  snprintf(v, sizeof(v), "%u", phoneStepsCode());  // para el Atajo del movil que manda los pasos
  statRow(y += dy, S_ST_PHONE, v);
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_BACK), 2), 424);
  printT(T(S_BACK));
  gfx->flush();
}

void clockTap(int16_t x, int16_t y) {
  if (backupAsk) {  // cualquier toque fuera de los botones lo cierra
    uint8_t ask = backupAsk;
    backupAsk = BK_NONE;
    if (y < BK_BTN_Y || y > BK_BTN_Y + BK_BTN_H) return;
    if (ask == BK_MENU) {  // guardar ya o restaurar: las dos preguntan antes
      uint32_t bk;
      if (x >= BK_MENU_X0 && x <= BK_MENU_X0 + BK_MENU_W) backupAsk = BK_ASK_SAVE;
      else if (x >= BK_MENU_X1 && x <= BK_MENU_X1 + BK_MENU_W && backupInfo(bk)) backupAsk = BK_ASK_RESTORE;
    } else if (x < BK_YES_X || x > BK_YES_X + BK_YN_W) {
      return;  // NO
    } else if (ask == BK_ASK_SAVE) {  // SI: la copia nueva sustituye a la que hubiera
      pet.flushSave();
      toast(T(backupSave(pet.lastSeenEpoch) ? S_BK_SAVED : S_BK_FAIL));
    } else {  // SI: se restaura y se reinicia
      gfx->fillScreen(RGB565_BLACK);
      gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
      gfx->setTextColor(UI_INK);
      setSize(2);
      setCur(centerX(T(S_RESTORING), 2), CY - 8);
      printT(T(S_RESTORING));
      gfx->flush();
      // la tarea de red parada (como con "见面"): que no escriba la NVS del correo mientras se restaura
      // ni en el medio segundo antes de reiniciar
      bool radio = !linkActive();
      if (radio) netRadioTake();
      if (backupRestore()) {
        delay(500);
        ESP.restart();  // sin guardar nada mas: la mascota en RAM es la de antes
      }
      if (radio) netRadioGive();
      // no se pudo (copia rota, o la NVS fallo a medio escribir): la partida de la RAM vuelve a la NVS
      pet.markDirty();
      pet.flushSave();
      toast(T(S_NO_BACKUP));
    }
    return;
  }
  if (statsOpen) {  // en las estadisticas, cualquier toque vuelve
    statsOpen = false;
    return;
  }
  if (x >= TEMP_PILL_X && x < TEMP_PILL_X + TEMP_PILL_W && y >= TEMP_PILL_Y - 4 &&
      y < TEMP_PILL_Y + TEMP_PILL_H + 4) {  // grados C <-> F (por dentro todo sigue en C)
    tempF = !tempF;
    Preferences p;
    p.begin("tamapoke", false);
    p.putBool("tempf", tempF);
    p.end();
    sfxPlay(SFX_TAP);
    return;
  }
  bool col0 = x >= SET_BTN_X0 && x < SET_BTN_X0 + SET_BTN_W, col1 = x >= SET_BTN_X1 && x < SET_BTN_X1 + SET_BTN_W;
  if (y >= STATS_BTN_Y && y < STATS_BTN_Y + STATS_BTN_H) {  // esta fila tiene sus propias columnas
    if (x >= SET_BTN_X0 && x < SET_BTN_X0 + STATS_BTN_W) {
      statsOpen = true;
    } else if (x >= BRI_BTN_X && x < BRI_BTN_X + BRI_BTN_W) {  // cicla el brillo: alto, medio, bajo
      userDim = (userDim + 1) % BRIGHT_LEVELS;
      Preferences p;
      p.begin("tamapoke", false);
      p.putUChar("bri", userDim);
      p.end();
      sfxPlay(SFX_TAP);
    } else if (x >= STATS_BTN_X1 && x < STATS_BTN_X1 + STATS_BTN_W && wifiHasCreds()) {
      clockOpen = false;  // sincronizar ya: el resumen sale en la pantalla principal
      if (chessHasAccount()) syncRequested = true;
      else toast(T(S_NEED_CHESS));  // sin cuenta: decir donde se pone
    }
    return;
  }
  bool row0 = y >= SET_BTN_Y0 && y < SET_BTN_Y0 + SET_BTN_H, row1 = y >= SET_BTN_Y1 && y < SET_BTN_Y1 + SET_BTN_H;
  if (row0 && col0) {  // sonido
    uint8_t n = volCycle(audioEnabled() ? audioVolume() : VOL_LEVELS);  // alto, medio, bajo, apagado
    if (n < VOL_LEVELS) audioSetVolume(n);
    if (audioEnabled() != (n < VOL_LEVELS)) audioSetEnabled(n < VOL_LEVELS);
    if (audioEnabled()) sfxPlay(SFX_TAP);  // se oye el nivel nuevo
  } else if (row0 && col1) {  // configurar wi-fi
    clockOpen = false;
    if (linkActive()) linkStop();  // la radio no puede estar en las dos cosas
    wifiPortalStart(pet.easyRegions);
  } else if (row1 && col0) {  // cicla idioma
    setLang((Lang)((gLang + 1) % LANG_COUNT));
    applyLangFont();  // la fuente cambia con el idioma
    sfxPlay(SFX_TAP);
  } else if (row1 && col1) {  // la copia de seguridad: guardar una ya o restaurarla
    if (sdReady) backupAsk = BK_MENU;
  }
}

// banner temporal: medalla nueva o hito de racha
void drawCelebration() {
  const char *l1 = nullptr, *l2 = nullptr;
  // 32 y no 20: "%u にちれんぞく！" ya pasa de 20 con una sola cifra, y snprintf
  // cortaba a mitad de un caracter de 3 bytes.
  char buf[32];
  char reg[40];
  char wk[64];
  static uint32_t weekUntil = 0;
  if (timeLeft(unlockUntil) && unlockShow < REGION_COUNT) {  // region nueva
    l1 = T(S_REGION_UNLOCK);
    l2 = T((StrId)(S_REGION_1 + unlockShow));
  } else if (timeLeft(hockeyUntil)) {  // practica de hockey o premio de casa (Atajo del movil)
    l1 = T(phoneBanner);
    l2 = hockeyMsg;
  } else if (timeLeft(harvestUntil)) {  // cosecha de la granja
    l1 = T(S_HARVEST);
    l2 = harvestMsg;
  } else if (pet.showRegionDone() && pet.regionJustDone >= 1 && pet.regionJustDone <= REGION_COUNT) {
    snprintf(reg, sizeof(reg), T(S_REGION_DONE_FMT), T((StrId)(S_REGION_1 + pet.regionJustDone - 1)));
    snprintf(buf, sizeof(buf), T(S_REGION_REWARD_FMT), REGION_REWARD_PTS);
    l1 = reg;
    l2 = buf;
  } else if (pet.showMedal()) {
    for (int i = 0; i < MED_COUNT; i++)
      if (pet.newMedal & medalBit(i)) { l2 = medalName(i); break; }
    l1 = T(S_MEDAL_BANNER);
  } else if (pet.showMilestone()) {
    snprintf(buf, sizeof(buf), T(S_STREAK_DAYS_FMT), pet.streak);
    l1 = T(S_GREAT);
    l2 = buf;
  } else if (timeLeft(weekUntil) || pet.ptReport) {  // resumen de la semana pasada (libro de puntos)
    if (pet.ptReport) {
      weekUntil = millis() + 9000;
      pet.ptReport = false;
      pet.markDirty();
    }
    const PtsWeek &w = pet.ptLast;
    snprintf(wk, sizeof(wk), T(S_WEEK_SUM_FMT), w.cat[PT_CHESS], w.cat[PT_SHOP] + w.cat[PT_FUN]);
    l1 = T(S_WEEK_BANNER);
    l2 = wk;
  }
  if (!l1) return;
  gfx->fillRoundRect(73, 150, 320, 96, 16, UI_BAR_WARN);
  gfx->drawRoundRect(73, 150, 320, 96, 16, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(l1, 3), 176);
  printT(l1);
  setSize(2);
  setCur(centerX(l2, 2), 212);
  printT(l2);
}

// medallas en la ficha: badge con etiqueta, color si conseguida
void drawMedalBadge(int x, int y, int i) {
  bool got = pet.hasMedal(medalBit(i));
  gfx->fillRoundRect(x, y, 100, 24, 6, got ? UI_BAR_OK : UI_TRACK);
  if (!got) gfx->drawRoundRect(x, y, 100, 24, 6, UI_TRACK);
  gfx->setTextColor(got ? UI_BG_DAY : 0x9492);
  setSize(2);
  setCur(x + (100 - textW(medalLabel(i), 2)) / 2, y + 5);
  printT(medalLabel(i));
}

// pagina 0: perfil (retrato grande, identidad, racha, vinculo, edad)
void renderCardProfile() {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  char head[48], nb[40];
  snprintf(head, sizeof(head), T(S_NAME_FMT), pet.shiny ? "*" : "", petName(nb, sizeof(nb)), pet.level());
  gfx->setTextColor(d.accent);
  // auto-encoge: a tamano 3 los nombres largos no caben en la franja estrecha de
  // arriba de la pantalla redonda, asi que se cortaban por el borde. El ancho se
  // MIDE con textW(): strlen() cuenta bytes, y en una fila UTF-8 no hay un byte
  // por caracter ni todos miden lo mismo. Con la fuente clasica textW() devuelve
  // strlen()*6*size, o sea exactamente la cuenta anterior: no mueve un pixel en
  // los seis idiomas latinos (11 caracteres a tamano 3 son esos 198 px).
  setSize(3);
  int hts = (textW(head, 3) <= 198) ? 3 : 2;
  setSize(hts);
  setCur(centerX(head, hts), hts == 3 ? 34 : 40);
  printT(head);
  if (pet.nick[0]) {  // especie real bajo el apodo
    const char *sp = dexName(pet.speciesId);
    char par[32];
    snprintf(par, sizeof(par), "(%s)", sp);
    gfx->setTextColor(UI_HINT);
    setSize(2);
    setCur(centerX(par, 2), 64);   // medido, no contado en bytes
    printT(par);
  }

  // retrato grande animado
  if (pmd.loaded) drawPmdAct(PMD_IDLE, CX, 206, millis(), true, false, 4);

  // racha con llama
  int sx = 138, sy = 224;
  gfx->fillTriangle(sx + 8, sy, sx + 1, sy + 18, sx + 15, sy + 18, UI_BAR_BAD);
  gfx->fillTriangle(sx + 8, sy + 7, sx + 4, sy + 18, sx + 12, sy + 18, UI_BAR_WARN);
  char rl[40];  // en japones pierde cifras con 30 a partir de 100 dias
  snprintf(rl, sizeof(rl), T(S_STREAK_FMT), pet.streak, pet.bestStreak);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(sx + 24, sy + 2);
  printT(rl);

  drawCardStat(258, T(S_VIN), pet.bond, 100, C565(0xd4, 0x52, 0x7e));

  char info[48];
  snprintf(info, sizeof(info), T(S_INFO_FMT), (unsigned long)(pet.ageMinutes / 1440));
  char lin[96];  // caracter y edad
  snprintf(lin, sizeof(lin), "%s   %s", T((StrId)(S_NAT_EASY + (pet.nature < NAT_COUNT ? pet.nature : 0))), info);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(lin, 2), 296);
  printT(lin);

  gfx->setTextColor(UI_HINT);
  setCur(centerX(T(S_RENAME_HINT), 2), 332);
  printT(T(S_RENAME_HINT));
}

// pagina 1: combate (4 barras; se entrenan con las pociones de la tienda)
// una capacidad: barra 0-100 y, debajo, lo que hace ahora mismo
static void drawAbility(int y, const char *label, uint8_t val, uint16_t color, StrId fx, uint8_t pct) {
  drawCardStat(y, label, val, 100, color);
  char l[48];
  snprintf(l, sizeof(l), T(fx), pct);
  gfx->setTextColor(pct ? UI_INK : UI_HINT);
  setSize(2);
  setCur(statBarX(), y + 20);
  printT(l);
}

void renderCardStats() {
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_BATTLE), 3), 48);
  printT(T(S_BATTLE));

  // aguante (pocion de fuerza), vitalidad (pocion de velocidad), resistencia
  // (cuidandolo bien): cada una con su efecto
  drawAbility(100, T(S_STAT_ATK), pet.trAtk, UI_BAR_BAD, S_FX_STA_FMT, pet.staminaPct());
  drawAbility(148, T(S_STAT_SPE), pet.trSpe, UI_BAR_WARN, S_FX_VIT_FMT, pet.vitalityPct());
  drawAbility(196, T(S_STAT_DEF), pet.trDef, 0x4C98, S_FX_RES_FMT, pet.resistPct());
  drawCardStat(248, T(S_STAT_WGT), pet.weight, 100, 0xB3C8);

  char pasos[40];
  snprintf(pasos, sizeof(pasos), T(S_STEPS_FMT), (unsigned long)pet.stepsToday);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(pasos, 2), 300);
  printT(pasos);
}

// paginas 2 y 3: medallas con etiqueta descriptiva, MED_PER_PAGE en cada una
#define MED_PER_PAGE 8
void renderCardMedals(int first) {
  int got = 0;
  for (int i = 0; i < MED_COUNT; i++)
    if (pet.hasMedal(medalBit(i))) got++;
  char head[24];
  snprintf(head, sizeof(head), T(S_MEDALS_FMT), got, MED_COUNT);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(head, 3), 48);
  printT(head);

  for (int i = first; i < MED_COUNT && i < first + MED_PER_PAGE; i++) {
    int k = i - first;
    int x = CX - 201 + (k % 2) * 206, y = 120 + (k / 2) * 54;  // a y=120 el circulo ya deja 402 px
    bool g = pet.hasMedal(medalBit(i));
    gfx->fillRoundRect(x, y, 196, 44, 10, g ? UI_BAR_OK : UI_TRACK);
    if (g) {  // marca de conseguida
      gfx->fillCircle(x + 22, y + 22, 11, UI_BG_DAY);
      gfx->setTextColor(UI_BAR_OK);
      setSize(2);
      setCur(x + 16, y + 13);
      printT("v");
    }
    gfx->setTextColor(g ? UI_BG_DAY : 0x8410);
    setSize(2);
    setCur(x + 44, y + 14);
    printT(medalDesc(i));
  }
}

// pagina 3: progreso (nivel, evolucion, descuidos) — saca a la luz mecanicas
// que antes eran invisibles (cuanto falta para subir/evolucionar y por que)
void renderCardProgress() {
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_PROGRESS), 3), 44);
  printT(T(S_PROGRESS));

  // nivel grande
  char lv[10];
  snprintf(lv, sizeof(lv), T(S_LVL_FMT), pet.level());
  setSize(5);
  setCur(centerX(lv, 5), 86);
  printT(lv);

  // barra de progreso al siguiente nivel (cada nivel dura distinto: ver levelOf)
  uint32_t into = 0, span = 0;
  pet.levelSpan(into, span);
  int bx = 93, bw = 280, by = 158, bh = 22;
  gfx->fillRoundRect(bx, by, bw, bh, 6, UI_TRACK);
  int fw = span ? (int)((uint64_t)(bw - 4) * into / span) : bw - 4;  // sin siguiente (nivel 100): llena
  if (fw > 0) gfx->fillRoundRect(bx + 2, by + 2, fw, bh - 4, 5, UI_BAR_OK);
  char nx[40] = "";
  uint32_t left = span - into;
  if (span && left >= 60)
    snprintf(nx, sizeof(nx), T(S_NEXT_LVL_H_FMT), (unsigned)(left / 60), (unsigned)(left % 60), pet.level() + 1);
  else if (span)
    snprintf(nx, sizeof(nx), T(S_NEXT_LVL_FMT), (unsigned)left, pet.level() + 1);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(nx, 2), by + 32);
  printT(nx);

  // estado de evolucion
  gfx->setTextColor(UI_HINT);
  setCur(centerX(T(S_EVO_LABEL), 2), 230);
  printT(T(S_EVO_LABEL));
  char evoBuf[32];
  const char *evo = "";
  uint16_t evoCol = UI_INK;
  uint8_t eb = pet.evoBlock();
  if (eb == EVO_FINAL) {
    evo = T(S_FINAL_FORM);
  } else if (eb == EVO_WAIT) {
    uint32_t falta = pet.evoAgeNeeded() - pet.ageMinutes;  // 3 dias la 1a, 5 la 2a (+ descuidos)
    unsigned dias = falta / 1440, horas = (falta % 1440 + 59) / 60;
    if (horas == 24) { dias++; horas = 0; }
    snprintf(evoBuf, sizeof(evoBuf), T(S_EVO_IN_FMT), dias, horas);
    evo = evoBuf;
  } else if (eb == EVO_NEEDS) {
    evo = T(S_EVO_BLOCKED); evoCol = UI_BAR_BAD;
  } else if (eb == EVO_FARM) {
    evo = T(S_EVO_FARM); evoCol = UI_BAR_BAD;
  } else if (eb == EVO_ASLEEP) {
    evo = T(S_EVO_ASLEEP); evoCol = UI_HINT;
  } else if (eb == EVO_OK) {
    evo = T(S_EVO_READY); evoCol = UI_BAR_OK;
  }
  gfx->setTextColor(evoCol);
  setCur(centerX(evo, 2), 256);
  printT(evo);

  // descuidos (retrasan la evolucion)
  char ms[24];
  snprintf(ms, sizeof(ms), T(S_MISTAKES_FMT), pet.careMistakes);
  gfx->setTextColor(pet.careMistakes > 0 ? UI_BAR_BAD : UI_INK);
  setCur(centerX(ms, 2), 278);
  printT(ms);

  // boton de evolucionar: rojo si puede (abre la pregunta de la principal), gris si no
  bool puede = eb == EVO_OK;
  gfx->fillRoundRect(CEVO_BTN_X, CEVO_BTN_Y, CEVO_BTN_W, CEVO_BTN_H, 14, puede ? UI_BAR_BAD : UI_TRACK);
  gfx->setTextColor(puede ? UI_WHITE : UI_HINT);
  setCur(centerX(T(S_EVO_TAP), 2), CEVO_BTN_Y + 14);
  printT(T(S_EVO_TAP));
}

void renderCard() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  if (cardPage == 0) renderCardProfile();
  else if (cardPage == 1) renderCardStats();
  else if (cardPage == 2) renderCardMedals(0);
  else if (cardPage == 3) renderCardMedals(MED_PER_PAGE);
  else renderCardProgress();

  // indicador de paginas + ayuda
  int dot0 = CX - (CARD_PAGES - 1) * 13;
  for (int i = 0; i < CARD_PAGES; i++) {
    if (i == cardPage) gfx->fillCircle(dot0 + i * 26, 374, 5, UI_INK);
    else gfx->drawCircle(dot0 + i * 26, 374, 4, UI_INK);
  }
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_BACK), 2), 398);
  printT(T(S_BACK));
  gfx->flush();
}

// ---------- teclado para renombrar ----------

static const char KB_KEYS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-";  // 28 + DEL + OK = 30
#define KB_COLS 6
#define KB_X 40
#define KB_Y 150
#define KB_W 64
#define KB_H 52

void openKeyboard() {
  kbOpen = true;
  strncpy(nameBuf, pet.nick, sizeof(nameBuf) - 1);
  nameBuf[sizeof(nameBuf) - 1] = 0;
  nameLen = strlen(nameBuf);
}

void renderKeyboard() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(T(S_NAME), 2), 56);
  printT(T(S_NAME));
  // buffer actual
  gfx->fillRoundRect(83, 84, 300, 40, 8, UI_WHITE);
  gfx->drawRoundRect(83, 84, 300, 40, 8, UI_INK);
  setSize(3);
  setCur(95, 94);
  printT(nameLen ? nameBuf : "_");

  for (int i = 0; i < 30; i++) {
    int x = KB_X + (i % KB_COLS) * KB_W, y = KB_Y + (i / KB_COLS) * KB_H;
    bool special = (i >= 28);
    gfx->fillRoundRect(x, y, KB_W - 6, KB_H - 6, 6, special ? UI_BAR_WARN : UI_WHITE);
    gfx->drawRoundRect(x, y, KB_W - 6, KB_H - 6, 6, UI_INK);
    gfx->setTextColor(UI_INK);
    setSize(2);
    if (i < 28) {
      setCur(x + KB_W / 2 - 9, y + KB_H / 2 - 10);
      printT(KB_KEYS[i]);
    } else {
      const char *lab = (i == 28) ? "<-" : "OK";
      setCur(x + KB_W / 2 - 15, y + KB_H / 2 - 10);
      printT(lab);
    }
  }
  gfx->flush();
}

void keyboardTap(int16_t x, int16_t y) {
  // filtra ANTES de dividir: con x/y menores que KB_X/KB_Y la resta da negativo
  // y la division entera trunca hacia cero (no hacia -inf), asi que col/row
  // saldria 0 en vez de negativo y el filtro de abajo no lo detectaria
  if (x < KB_X || y < KB_Y) return;
  int col = (x - KB_X) / KB_W, row = (y - KB_Y) / KB_H;
  if (col >= KB_COLS || row >= 5) return;
  int i = row * KB_COLS + col;
  if (i >= 30) return;
  if (i == 28) {  // borrar
    if (nameLen) nameBuf[--nameLen] = 0;
  } else if (i == 29) {  // OK
    pet.rename(nameBuf);
    kbOpen = false;
  } else if (nameLen < sizeof(nameBuf) - 1) {
    nameBuf[nameLen++] = KB_KEYS[i];
    nameBuf[nameLen] = 0;
  }
}
