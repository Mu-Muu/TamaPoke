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

static void setButton(int x, int y, const char *l, bool on, bool apagado) {
  gfx->fillRoundRect(x, y, SET_BTN_W, SET_BTN_H, 14, apagado ? UI_TRACK : on ? UI_BAR_OK : UI_WHITE);
  gfx->drawRoundRect(x, y, SET_BTN_W, SET_BTN_H, 14, UI_INK);
  gfx->setTextColor(on ? UI_BG_DAY : UI_INK);
  int ts = textW(l, 3) <= SET_BTN_W - 16 ? 3 : 2;  // tan grande como quepa
  setSize(ts);
  setCur(x + (SET_BTN_W - textW(l, ts)) / 2, y + (SET_BTN_H - 7 * ts) / 2);
  printT(l);
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

  // sonido, wi-fi (verde si ya hay red), idioma, restaurar la copia
  setButton(SET_BTN_X0, SET_BTN_Y0, audioEnabled() ? T(S_SND_ON) : T(S_SND_OFF), audioEnabled(), false);
  setButton(SET_BTN_X1, SET_BTN_Y0, T(S_WIFI_SETUP), wifiHasCreds(), false);
  char lp[10];
  snprintf(lp, sizeof(lp), "%s >", LANG_CODES[gLang]);
  setButton(SET_BTN_X0, SET_BTN_Y1, lp, false, false);
  uint32_t bk = 0;
  bool hayCopia = backupInfo(bk);
  char rl[40];
  if (hayCopia) {
    time_t bt = bk;
    struct tm btm;
    gmtime_r(&bt, &btm);
    snprintf(rl, sizeof(rl), T(S_RESTORE_FMT), (unsigned)(btm.tm_mon + 1), (unsigned)btm.tm_mday);
  } else {
    snprintf(rl, sizeof(rl), "%s", T(S_NO_BACKUP));
  }
  setButton(SET_BTN_X1, SET_BTN_Y1, rl, false, !hayCopia);

  // debajo: estadisticas, brillo y sincronizar el ajedrez ya (gris sin wi-fi o sin cuenta de chess.com)
  const int16_t bx[2] = { SET_BTN_X0, STATS_BTN_X1 };
  const StrId bl[2] = { S_STATS, S_SYNC_NOW };
  bool puedeSync = wifiHasCreds() && chessHasAccount();
  for (int i = 0; i < 2; i++) {
    gfx->fillRoundRect(bx[i], STATS_BTN_Y, STATS_BTN_W, STATS_BTN_H, 14, UI_WHITE);
    gfx->drawRoundRect(bx[i], STATS_BTN_Y, STATS_BTN_W, STATS_BTN_H, 14, UI_INK);
    gfx->setTextColor(i == 1 && !puedeSync ? UI_TRACK : UI_INK);
    const char *l = T(bl[i]);
    int ts = textW(l, 3) <= STATS_BTN_W - 16 ? 3 : 2;  // tan grande como quepa (como setButton)
    setSize(ts);
    setCur(bx[i] + (STATS_BTN_W - textW(l, ts)) / 2, STATS_BTN_Y + (STATS_BTN_H - 7 * ts) / 2);
    printT(l);
  }
  drawBrightnessBtn();

  gfx->setTextColor(UI_TRACK);
  setSize(2);
  setCur(centerX(T(S_CLOCK_CANCEL), 2), 396);
  printT(T(S_CLOCK_CANCEL));

  // version del firmware (discreta, abajo del todo)
  char ver[20];
  snprintf(ver, sizeof(ver), "TamaPoke v%s", FW_VERSION);
  setSize(1);
  setCur(centerX(ver, 1), 422);
  printT(ver);

  if (restoreAsk) {  // ¿seguro?
    gfx->fillRoundRect(64, 168, 338, 152, 16, UI_WHITE);
    gfx->drawRoundRect(64, 168, 338, 152, 16, UI_INK);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(T(S_RESTORE_Q), 2), 200);
    printT(T(S_RESTORE_Q));
    gfx->fillRoundRect(118, 252, 100, 52, 12, UI_BAR_OK);
    gfx->setTextColor(UI_WHITE);
    setCur(118 + (100 - textW(T(S_YES), 2)) / 2, 270);
    printT(T(S_YES));
    gfx->fillRoundRect(248, 252, 100, 52, 12, UI_BAR_BAD);
    setCur(248 + (100 - textW(T(S_NO), 2)) / 2, 270);
    printT(T(S_NO));
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
  int y = 84;
  snprintf(v, sizeof(v), "%u", pet.raisedCount);
  statRow(y, S_ST_RAISED, v);
  snprintf(v, sizeof(v), "%u/%d", pet.registeredCount(), PET_DEX_MAX);
  statRow(y += 28, S_ST_DEX, v);
  snprintf(v, sizeof(v), "%u", shiny);
  statRow(y += 28, S_ST_SHINY, v);
  snprintf(v, sizeof(v), "%u/%d", pet.unlockedRegions(), REGION_COUNT);
  statRow(y += 28, S_ST_UNLOCKED, v);
  snprintf(v, sizeof(v), "%d/%d", __builtin_popcount(pet.regionDone), REGION_COUNT);
  statRow(y += 28, S_ST_DONE, v);
  snprintf(v, sizeof(v), "%u", pet.bestStreak);
  statRow(y += 28, S_ST_STREAK, v);
  snprintf(v, sizeof(v), "%u", pet.totalMedals);
  statRow(y += 28, S_ST_MEDALS, v);
  snprintf(v, sizeof(v), "%lu", (unsigned long)pet.stepsTotal);
  statRow(y += 28, S_ST_STEPS, v);
  snprintf(v, sizeof(v), "%lu", (unsigned long)pet.chessPtsTotal);
  statRow(y += 28, S_ST_CHESS, v);
  if (farmReady()) snprintf(v, sizeof(v), "%d", farmCount());
  else snprintf(v, sizeof(v), "-");
  statRow(y += 28, S_ST_FARM, v);
  gfx->setTextColor(UI_TRACK);
  setSize(2);
  setCur(centerX(T(S_BACK), 2), 396);
  printT(T(S_BACK));
  gfx->flush();
}

void clockTap(int16_t x, int16_t y) {
  if (restoreAsk) {
    restoreAsk = false;
    if (x >= 118 && x <= 218 && y >= 252 && y <= 304) {  // SI: se restaura y se reinicia
      gfx->fillScreen(RGB565_BLACK);
      gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
      gfx->setTextColor(UI_INK);
      setSize(2);
      setCur(centerX(T(S_RESTORING), 2), CY - 8);
      printT(T(S_RESTORING));
      gfx->flush();
      if (backupRestore()) {
        delay(500);
        ESP.restart();  // sin guardar nada mas: la mascota en RAM es la de antes
      }
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
    audioSetEnabled(!audioEnabled());
    if (audioEnabled()) sfxPlay(SFX_TAP);  // confirma al encender
  } else if (row0 && col1) {  // configurar wi-fi
    clockOpen = false;
    wifiPortalStart();
  } else if (row1 && col0) {  // cicla idioma
    setLang((Lang)((gLang + 1) % LANG_COUNT));
    applyLangFont();  // la fuente cambia con el idioma
    sfxPlay(SFX_TAP);
  } else if (row1 && col1) {  // restaurar la copia (con confirmacion)
    uint32_t bk;
    if (backupInfo(bk)) restoreAsk = true;
  }
}

// llama + numero de racha: en la 2a fila de arriba, a la izquierda. En la
// esquina (26,16) quedaba fuera del circulo de la pantalla y no se veia.
void drawStreakBadge() {
  if (pet.streak < 1) return;
  int x = 144, y = 32;
  gfx->fillTriangle(x + 8, y, x + 1, y + 17, x + 15, y + 17, UI_BAR_BAD);
  gfx->fillTriangle(x + 8, y + 7, x + 4, y + 17, x + 12, y + 17, UI_BAR_WARN);
  char s[6];
  snprintf(s, sizeof(s), "%u", pet.streak);
  gfx->setTextColor(inkColor());
  setSize(2);
  setCur(x + 22, y + 2);
  printT(s);
}

// banner temporal: medalla nueva o hito de racha
void drawCelebration() {
  const char *l1 = nullptr, *l2 = nullptr;
  // 32 y no 20: "%u にちれんぞく！" ya pasa de 20 con una sola cifra, y snprintf
  // cortaba a mitad de un caracter de 3 bytes.
  char buf[32];
  char reg[40];
  if (timeLeft(unlockUntil) && unlockShow < REGION_COUNT) {  // region nueva
    l1 = T(S_REGION_UNLOCK);
    l2 = T((StrId)(S_REGION_1 + unlockShow));
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
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  char head[32];
  snprintf(head, sizeof(head), T(S_NAME_FMT), pet.shiny ? "*" : "", nm, pet.level());
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
    gfx->setTextColor(UI_TRACK);
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

  gfx->setTextColor(UI_TRACK);
  setCur(centerX(T(S_RENAME_HINT), 2), 332);
  printT(T(S_RENAME_HINT));
}

// pagina 1: combate (4 barras; se entrenan con las pociones de la tienda)
// una capacidad: barra 0-100 y, debajo, lo que hace ahora mismo
static void drawAbility(int y, const char *label, uint8_t val, uint16_t color, StrId fx, uint8_t pct) {
  drawCardStat(y, label, val, 100, color);
  char l[48];
  snprintf(l, sizeof(l), T(fx), pct);
  gfx->setTextColor(pct ? UI_INK : UI_TRACK);
  setSize(1);
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
    int x = 28 + (k % 2) * 206, y = 104 + (k / 2) * 54;
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
  const DexEntry &d = DEX_TBL[pet.speciesId];
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

  // barra de progreso al siguiente nivel (1 nivel = 60 min de juego)
  uint8_t into = pet.ageMinutes % MINUTES_PER_LEVEL;
  int bx = 93, bw = 280, by = 158, bh = 22;
  gfx->fillRoundRect(bx, by, bw, bh, 6, UI_TRACK);
  int fw = (bw - 4) * into / MINUTES_PER_LEVEL;
  if (fw > 0) gfx->fillRoundRect(bx + 2, by + 2, fw, bh - 4, 5, UI_BAR_OK);
  char nx[32];
  snprintf(nx, sizeof(nx), T(S_NEXT_LVL_FMT), MINUTES_PER_LEVEL - into, pet.level() + 1);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(nx, 2), by + 32);
  printT(nx);

  // estado de evolucion
  gfx->setTextColor(UI_TRACK);
  setCur(centerX(T(S_EVO_LABEL), 2), 230);
  printT(T(S_EVO_LABEL));
  char evoBuf[32];
  const char *evo;
  uint16_t evoCol = UI_INK;
  if (d.evolvesTo == 0) {
    evo = T(S_FINAL_FORM);
  } else {
    uint32_t needed = pet.evoAgeNeeded();  // 3 dias la 1a, 5 la 2a (+ descuidos)
    if (pet.evoBoost || pet.ageMinutes >= needed) {
      if (pet.lowestStat() >= 40) { evo = T(S_EVO_READY); evoCol = UI_BAR_OK; }
      else { evo = T(S_EVO_BLOCKED); evoCol = UI_BAR_BAD; }
    } else {
      uint32_t falta = needed - pet.ageMinutes;
      unsigned dias = falta / 1440, horas = (falta % 1440 + 59) / 60;
      if (horas == 24) { dias++; horas = 0; }
      snprintf(evoBuf, sizeof(evoBuf), T(S_EVO_IN_FMT), dias, horas);
      evo = evoBuf;
    }
  }
  gfx->setTextColor(evoCol);
  setCur(centerX(evo, 2), 256);
  printT(evo);

  // descuidos (retrasan la evolucion)
  char ms[24];
  snprintf(ms, sizeof(ms), T(S_MISTAKES_FMT), pet.careMistakes);
  gfx->setTextColor(pet.careMistakes > 0 ? UI_BAR_BAD : UI_INK);
  setCur(centerX(ms, 2), 312);
  printT(ms);
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
  gfx->setTextColor(UI_TRACK);
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
