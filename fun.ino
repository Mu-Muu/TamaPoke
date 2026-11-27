// Ocio: cuatro cosas que hacer segun el tipo (fun.h: reglas y lo que se saca). Cuestan
// puntos (los del ajedrez) y no dan puntos. Las hace el compañero si es de un tipo que
// vale, y si no (o no puede ahora), uno de la granja que lo sea.
//  - huerto: tres parcelas en tiempo real (plantar, regar una vez, cosechar)
//  - pesca: se lanza y se toca la pantalla cuando pica
//  - mina: se rompe la piedra a toques
//  - expedicion: sale una hora (si va el compañero, no esta en la principal)
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

enum : uint8_t { FN_LIST = 0, FN_PICK, FN_PLAY, FN_LOOT };
static uint8_t fnStage = FN_LIST, fnAct = FUN_GARDEN;
static int16_t fnDex = 0;             // quien lo hace
static bool fnMine = true;            // el compañero (si no, uno de la granja)
static bool fnPractice = false;       // consola FUN: sin puntos ni premios, el tiempo 600x
static PmdMon *fnPmd = nullptr;
static int16_t fnCand[64];            // los de la granja que valen (indices)
static uint8_t fnCandN = 0, fnPage = 0;
static uint8_t fnWho[FUN_COUNT];      // cuantos pueden ir a cada una (al entrar)
// pesca
enum : uint8_t { FS_IDLE = 0, FS_WAIT, FS_BITE };
static uint8_t fsStage = FS_IDLE;
static uint32_t fsT0 = 0, fsBite = 0;
// mina
static uint8_t mnHits = 0;
static bool mnRock = false;
static uint32_t mnHitAt = 0;
#define MN_HITS 5
// confirmar antes de gastar puntos: plantar (en que parcela), lanzar, picar, salir
enum : uint8_t { ASK_NONE = 0, ASK_PLANT, ASK_CAST, ASK_DIG, ASK_QUEST };
static uint8_t fnAsk = ASK_NONE, fnAskPlot = 0;
// lo que salio
static FunLoot fnLoot;
static uint8_t fnAcc = 0xFF;
static StrId fnLootTitle = S_LOOT;
static uint32_t fnLootAt = 0;
// practica: huerto y expedicion de mentira (con su reloj)
static uint32_t prPlot[GARDEN_PLOTS];
static uint8_t prWet = 0;
static uint32_t prQuest = 0;

static const StrId FUN_NAME[FUN_COUNT] = { S_FUN_GARDEN, S_FUN_FISH, S_FUN_MINE, S_FUN_QUEST };
static const StrId FUN_NEED[FUN_COUNT] = { S_NEED_GARDEN, S_NEED_FISH, S_NEED_MINE, S_NEED_QUEST };
static const uint16_t FUN_COL[FUN_COUNT] = { 0xB6F2, 0xAEFF, C565(0xf4, 0xc8, 0x98), 0xFF2E };

#define FUN_DIV (fnPractice ? 600UL : 1UL)
static uint32_t fnNow() {
  if (fnPractice) return millis() / 1000 + 1;  // nunca 0 (0 = parcela vacia)
  uint32_t e = rtcEpoch();
  return e ? e : pet.lastSeenEpoch;
}
static uint32_t &plotRef(int p) { return fnPractice ? prPlot[p] : pet.plotAt[p]; }
static uint8_t &wetRef() { return fnPractice ? prWet : pet.plotWet; }
static uint32_t &questRef() { return fnPractice ? prQuest : pet.questEnd; }
static float fuRand() { return random(10000) / 10000.0f; }

// ---------- quien puede ir ----------

static bool petFits(uint8_t act) { return !pet.vacant && !pet.isEgg() && funOf(typeOf(pet.speciesId)) == act; }
static bool outOnQuest(int16_t dex) { return pet.questEnd && pet.questDex == dex && fnNow() < pet.questEnd; }
static void listCands(uint8_t act) {
  static int *ord = nullptr;  // la granja con los favoritos primero (PSRAM): asi caben en los 64
  if (!ord) ord = (int *)ps_malloc(PET_DEX_MAX * sizeof(int));
  int n = ord ? farmFavOrder(ord) : 0;
  fnCandN = 0;
  for (int k = 0; k < n && fnCandN < 64; k++) {
    int16_t d = farmAt(ord[k]).speciesId;
    if (funOf(typeOf(d)) == act && !outOnQuest(d)) fnCand[fnCandN++] = ord[k];
  }
}
static void countWho() {
  for (int a = 0; a < FUN_COUNT; a++) {
    listCands(a);
    fnWho[a] = fnCandN + (petFits(a) ? 1 : 0);
  }
}

void openFun() {
  fnAsk = ASK_NONE;
  if (pet.ceremony) return;
  funOpen = true;
  fnPractice = false;
  fnStage = FN_LIST;
  countWho();
}

static void closeFun() {
  funOpen = false;
  rivalPmd[0].unload();
}

static void startAct(bool mine, int farmIdx) {
  fnAsk = ASK_NONE;
  fnMine = mine;
  rivalPmd[0].unload();
  if (mine) {
    fnDex = pet.speciesId;
    fnPmd = &pmd;
  } else {
    fnDex = farmAt(farmIdx).speciesId;
    rivalPmd[0].load(fnDex, farmAt(farmIdx).shiny);
    fnPmd = &rivalPmd[0];
  }
  fsStage = FS_IDLE;
  mnRock = false;
  mnHits = 0;
  fnStage = FN_PLAY;
}

// consola: directo a una actividad, de practica (sin puntos ni premios y el tiempo 600x)
void funPractice(int act) {
  if (act < 0 || act >= FUN_COUNT) {
    if (funOpen) closeFun();
    return;
  }
  hubOpen = funOpen = true;
  fnPractice = true;
  memset(prPlot, 0, sizeof(prPlot));  // cada practica empieza de cero (huerto vacio, nadie fuera)
  prWet = 0;
  prQuest = 0;
  fnAct = act;
  countWho();
  listCands(act);
  startAct(petFits(act) || !fnCandN, fnCandN ? fnCand[0] : 0);
}

// ¿llega? (si no, se dice y no se pregunta nada)
static bool canPay(uint16_t cost) {
  if (fnPractice || pet.points >= (int32_t)cost) return true;
  toast(T(S_NO_POINTS));
  sfxPlay(SFX_DENY);
  return false;
}

// paga lo que cuesta (en practica, nada); si lo hace el compañero, le cansa un poco
static bool funPay(uint16_t cost) {
  if (fnPractice) return true;
  if (!pet.spend(cost)) {
    toast(T(S_NO_POINTS));
    sfxPlay(SFX_DENY);
    return false;
  }
  if (fnMine) pet.funDone();
  return true;
}

static void showLoot(StrId title) {
  if (!fnPractice) {
    for (int i = 0; i < fnLoot.n; i++) pet.give(fnLoot.item[i], fnLoot.qty[i]);
    if (fnAcc != 0xFF) pet.accOwned |= 1 << fnAcc;
    pet.funFinds += fnLoot.total() + (fnAcc != 0xFF ? 1 : 0);
    pet.markDirty();
  }
  fnLootTitle = title;
  fnLootAt = millis();
  fnStage = FN_LOOT;
  sfxPlay(SFX_MEDAL);
  Serial.printf("ocio %u dex=%d practica=%d suerte=%u:", fnAct, fnDex, fnPractice, pet.chessLuck());
  for (int i = 0; i < fnLoot.n; i++) Serial.printf(" %u x%u", fnLoot.item[i], fnLoot.qty[i]);
  Serial.printf(" acc=%d\n", fnAcc == 0xFF ? -1 : fnAcc);
}

// ---------- dibujos ----------

// icono de cada actividad en una caja de 48x48
static void drawFunIcon(uint8_t a, int x, int y) {
  switch (a) {
    case FUN_GARDEN:  // brote en la tierra
      gfx->fillRoundRect(x + 4, y + 32, 40, 12, 6, C565(0x9a, 0x68, 0x40));
      gfx->fillRect(x + 22, y + 14, 4, 20, 0x3C49);
      gfx->fillCircle(x + 15, y + 15, 8, 0x5DCD);
      gfx->fillCircle(x + 33, y + 12, 8, 0x5DCD);
      break;
    case FUN_FISH:  // un pez
      gfx->fillCircle(x + 22, y + 24, 14, 0x4C98);
      gfx->fillTriangle(x + 34, y + 24, x + 46, y + 12, x + 46, y + 36, 0x4C98);
      gfx->fillCircle(x + 14, y + 20, 3, UI_WHITE);
      gfx->fillCircle(x + 14, y + 20, 1, UI_INK);
      break;
    case FUN_MINE:  // pico
      gfx->fillTriangle(x + 8, y + 42, x + 12, y + 46, x + 34, y + 18, C565(0x8a, 0x60, 0x38));
      gfx->fillTriangle(x + 8, y + 42, x + 30, y + 14, x + 34, y + 18, C565(0x8a, 0x60, 0x38));
      gfx->fillTriangle(x + 14, y + 8, x + 42, y + 30, x + 32, y + 14, C565(0x90, 0x98, 0xa8));
      gfx->fillTriangle(x + 14, y + 8, x + 32, y + 14, x + 26, y + 6, C565(0x90, 0x98, 0xa8));
      break;
    default:  // mapa con la X
      gfx->fillRoundRect(x + 4, y + 6, 40, 36, 4, C565(0xf0, 0xdc, 0xa8));
      gfx->drawRoundRect(x + 4, y + 6, 40, 36, 4, C565(0x8a, 0x60, 0x38));
      for (int i = 0; i < 4; i++) gfx->fillCircle(x + 10 + i * 6, y + 34 - i * 4, 1, C565(0x8a, 0x60, 0x38));
      gfx->drawLine(x + 31, y + 13, x + 39, y + 21, UI_BAR_BAD);
      gfx->drawLine(x + 39, y + 13, x + 31, y + 21, UI_BAR_BAD);
      gfx->drawLine(x + 32, y + 13, x + 40, y + 21, UI_BAR_BAD);
      gfx->drawLine(x + 40, y + 13, x + 32, y + 21, UI_BAR_BAD);
  }
}

// cartel de la principal mientras el compañero esta de expedicion
void drawAwaySign() {
  uint16_t madera = C565(0xc0, 0x90, 0x58);
  gfx->fillRect(CX - 5, 236, 10, 70, C565(0x7a, 0x50, 0x30));
  gfx->fillRoundRect(CX - 120, 166, 240, 76, 12, madera);
  gfx->drawRoundRect(CX - 120, 166, 240, 76, 12, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(T(S_AWAY), 2), 182);
  printT(T(S_AWAY));
  uint32_t now = rtcEpoch() ? rtcEpoch() : pet.lastSeenEpoch;
  char t[32];
  snprintf(t, sizeof(t), T(S_MIN_LEFT_FMT), (unsigned)((pet.questEnd > now ? pet.questEnd - now : 0) + 59) / 60);
  setCur(centerX(t, 2), 212);
  printT(t);
}

static void drawWorker(int x, int ground) {
  if (fnPmd && fnPmd->loaded) drawPmdActM(*fnPmd, PMD_IDLE, x, ground, millis(), true, false, 3);
  else gfx->fillCircle(x, ground - 20, 18, UI_BAR_WARN);
}

// cabecera de cada actividad: su nombre y los puntos; debajo, la suerte del ajedrez
static void funHeader(const char *title, uint16_t ink) {
  gfx->setTextColor(ink);
  setSize(3);
  setCur(centerX(title, 3), 30);
  printT(title);
  uint8_t l = pet.chessLuck();
  const char *s = T(l == LUCK_WON ? S_LUCK_WON : l == LUCK_PLAYED ? S_LUCK_PLAYED : S_LUCK_NONE);
  gfx->setTextColor(l == LUCK_WON ? 0xE5CC : l == LUCK_PLAYED ? UI_BAR_OK : ink);
  setSize(2);
  setCur(centerX(s, 2), 66);
  printT(s);
}

static void funButton(int y, const char *label, uint16_t fill) {
  gfx->fillRoundRect(CX - 100, y, 200, 46, 14, fill);
  gfx->drawRoundRect(CX - 100, y, 200, 46, 14, UI_INK);
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(CX - textW(label, 2) / 2, y + 15);
  printT(label);
}
static bool inButton(int16_t x, int16_t y, int by) { return y >= by && y <= by + 46 && abs(x - CX) <= 100; }

// "plantar 8 pts": la accion con su precio
static void costLabel(char *b, size_t n, StrId what, uint8_t act) {
  char c[16];
  snprintf(c, sizeof(c), T(S_PTS_COST_FMT), (unsigned)FUN_COST[act]);
  snprintf(b, n, "%s  %s", T(what), c);
}

// lo que falta ("还要 3 小时"); corto: sin el "falta", para debajo de las parcelas
static void timeLeftText(char *b, size_t n, uint32_t secs, bool corto = false) {
  uint32_t real = secs * FUN_DIV;
  if (real >= 3600) snprintf(b, n, T(corto ? S_HOURS_FMT : S_HOURS_LEFT_FMT), (unsigned)((real + 3599) / 3600));
  else snprintf(b, n, T(corto ? S_MINS_FMT : S_MIN_LEFT_FMT), (unsigned)((real + 59) / 60));
}

// ---------- lista ----------

#define FL_X0 (CX - 172)
#define FL_Y0 100
#define FL_W 168
#define FL_H 124
static void renderFunList() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  char t[40];
  snprintf(t, sizeof(t), T(S_FUN_TITLE_FMT), (long)pet.points);
  funHeader(t, UI_INK);
  uint32_t now = fnNow();
  for (int a = 0; a < FUN_COUNT; a++) {
    int x = FL_X0 + (a % 2) * (FL_W + 8), y = FL_Y0 + (a / 2) * (FL_H + 8);
    gfx->fillRoundRect(x, y, FL_W, FL_H, 16, fnWho[a] ? FUN_COL[a] : UI_TRACK);
    gfx->drawRoundRect(x, y, FL_W, FL_H, 16, UI_INK);
    drawFunIcon(a, x + 8, y + 8);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(x + 62, y + 12);
    printT(T(FUN_NAME[a]));
    snprintf(t, sizeof(t), T(S_PTS_COST_FMT), (unsigned)FUN_COST[a]);
    gfx->setTextColor(C565(0xb0, 0x60, 0x10));
    setCur(x + 62, y + 38);
    printT(t);
    int k = 0;  // los tipos que valen, en sus colores
    for (int ty = 0; ty < TY_COUNT; ty++)
      if (funOf(ty) == a) {
        gfx->fillCircle(x + 16 + k * 15, y + 70, 6, TYPE_COL[ty]);
        gfx->drawCircle(x + 16 + k * 15, y + 70, 6, UI_INK);
        k++;
      }
    // como va: el huerto y la expedicion siguen solos; si no, cuantos pueden ir
    t[0] = 0;
    if (a == FUN_GARDEN) {
      int ripe = 0;
      uint32_t soon = 0;
      for (int p = 0; p < GARDEN_PLOTS; p++) {
        bool wet = (wetRef() >> p) & 1;
        uint8_t st = plotState(plotRef(p), wet, now, FUN_DIV);
        if (st == PLOT_RIPE) ripe++;
        else if (st == PLOT_GROWING) {
          uint32_t l = plotLeft(plotRef(p), wet, now, FUN_DIV);
          if (!soon || l < soon) soon = l;
        }
      }
      if (ripe) snprintf(t, sizeof(t), T(S_RIPE_FMT), (unsigned)ripe);
      else if (soon) timeLeftText(t, sizeof(t), soon);
    } else if (a == FUN_QUEST && questRef()) {
      if (now >= questRef()) snprintf(t, sizeof(t), "%s", T(S_QUEST_BACK));
      else timeLeftText(t, sizeof(t), questRef() - now);
    }
    if (!t[0]) snprintf(t, sizeof(t), T(S_FUN_WHO_FMT), (unsigned)fnWho[a]);
    gfx->setTextColor(UI_INK);
    int ts = textW(t, 2) <= FL_W - 16 ? 2 : 1;
    setSize(ts);
    setCur(x + 10, y + 92);
    printT(t);
  }
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_SWIPE_R_BACK), 2), 372);
  printT(T(S_SWIPE_R_BACK));
}

static void funListTap(int16_t x, int16_t y) {
  if (x < FL_X0 || y < FL_Y0 || x > FL_X0 + 2 * FL_W + 8 || y > FL_Y0 + 2 * FL_H + 8) return;
  uint8_t a = (y >= FL_Y0 + FL_H + 4 ? 2 : 0) + (x >= FL_X0 + FL_W + 4 ? 1 : 0);
  fnAct = a;
  listCands(a);
  if (petFits(a)) {
    const char *no = pet.questAway() ? T(S_AWAY) : sportsBlocked();
    if (!no) {
      sfxPlay(SFX_TAP);
      startAct(true, -1);
      return;
    }
    if (!fnCandN) { toast(no); return; }
  }
  if (!fnCandN) { toast(T(FUN_NEED[a])); sfxPlay(SFX_DENY); return; }
  sfxPlay(SFX_TAP);
  fnPage = 0;
  fnStage = FN_PICK;
}

// ---------- quien va (de la granja) ----------

#define FP_X0 (CX - 3 * 46)
#define FP_Y0 120
#define FP_CELL 92
static void renderFunPick() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_WHO_GOES), 3), 40);
  printT(T(S_WHO_GOES));
  setSize(2);
  setCur(centerX(T(FUN_NAME[fnAct]), 2), 84);
  printT(T(FUN_NAME[fnAct]));
  for (int i = 0; i < 6; i++) {
    int k = fnPage * 6 + i;
    if (k >= fnCandN) break;
    int x = FP_X0 + (i % 3) * FP_CELL, y = FP_Y0 + (i / 3) * FP_CELL;
    gfx->drawRoundRect(x, y, FP_CELL - 6, FP_CELL - 6, 12, UI_TRACK);
    const uint8_t *th = thumbs.get(farmAt(fnCand[k]).speciesId);
    if (th) drawThumbIn(th, x, y, 2, false, FP_CELL - 6);
    if (pet.isFav(farmAt(fnCand[k]).speciesId)) drawFavHeart(x + 12, y + 12, 6, true);
  }
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_SWIPE_R_BACK), 2), 330);
  printT(T(S_SWIPE_R_BACK));
}

static void funPickTap(int16_t x, int16_t y) {
  if (x < FP_X0 || y < FP_Y0) return;
  int c = (x - FP_X0) / FP_CELL, r = (y - FP_Y0) / FP_CELL;
  if (c > 2 || r > 1) return;
  int k = fnPage * 6 + r * 3 + c;
  if (k >= fnCandN) return;
  sfxPlay(SFX_TAP);
  startAct(false, fnCand[k]);
}

// ---------- huerto ----------

#define GD_Y 236
#define GD_W 96  // "HARVEST" y "WATERED" (7 letras) caben a tamaño 2
#define GD_H 66
#define GD_BTN (GD_Y + GD_H + 30)
static int plotX(int p) { return CX - 148 + p * 100; }

static void drawPlot(int p, uint32_t now) {
  int x = plotX(p), y = GD_Y, cx = x + GD_W / 2;
  bool wet = (wetRef() >> p) & 1;
  uint32_t at = plotRef(p);
  uint8_t st = plotState(at, wet, now, FUN_DIV);
  gfx->fillRoundRect(x, y, GD_W, GD_H, 10, wet && st == PLOT_GROWING ? C565(0x6a, 0x44, 0x28) : C565(0x9a, 0x68, 0x40));
  for (int k = 0; k < 3; k++) gfx->drawFastHLine(x + 10, y + 16 + k * 16, GD_W - 20, C565(0x7a, 0x50, 0x30));
  char t[24];
  const char *b = nullptr;
  uint16_t bc = UI_TRACK;
  if (st == PLOT_EMPTY) {
    snprintf(t, sizeof(t), T(S_PTS_COST_FMT), (unsigned)FUN_COST[FUN_GARDEN]);
    b = T(S_PLANT);
    bc = UI_BAR_OK;
  } else if (st == PLOT_GROWING) {
    uint32_t left = plotLeft(at, wet, now, FUN_DIV);
    float q = 1.0f - left / (float)(plotGrowSecs(wet) / FUN_DIV);
    int h = 8 + (int)(q * 36), base = y + GD_H - 10, r = 5 + (int)(q * 5);
    gfx->fillRect(cx - 2, base - h, 4, h, 0x3C49);
    gfx->fillCircle(cx - 7, base - h + 3, r, 0x5DCD);
    gfx->fillCircle(cx + 7, base - h + 1, r, 0x5DCD);
    if (wet)
      for (int k = 0; k < 3; k++) gfx->fillCircle(x + 14 + k * 28, y + GD_H - 8, 3, C565(0x70, 0xc0, 0xf8));
    timeLeftText(t, sizeof(t), left, true);
    b = wet ? T(S_WATERED) : T(S_WATER);
    bc = wet ? UI_TRACK : 0x4C98;
  } else {  // madura: arbusto con frutas
    gfx->fillCircle(cx, y + 26, 26, 0x3C49);
    gfx->fillCircle(cx - 12, y + 20, 7, 0xEA87);
    gfx->fillCircle(cx + 10, y + 16, 7, 0x4C98);
    gfx->fillCircle(cx + 2, y + 36, 7, 0xF427);
    snprintf(t, sizeof(t), "%s", T(S_RIPE));
    b = T(S_REAP);
    bc = 0xF427;
  }
  gfx->setTextColor(UI_INK);
  int ts = textW(t, 2) <= GD_W + 4 ? 2 : 1;
  setSize(ts);
  setCur(cx - textW(t, ts) / 2, y + GD_H + 8);
  printT(t);
  gfx->fillRoundRect(x, GD_BTN, GD_W, 40, 12, bc);
  gfx->drawRoundRect(x, GD_BTN, GD_W, 40, 12, UI_INK);
  gfx->setTextColor(bc == UI_TRACK ? UI_HINT : UI_WHITE);
  ts = textW(b, 2) <= GD_W - 8 ? 2 : 1;
  setSize(ts);
  setCur(cx - textW(b, ts) / 2, GD_BTN + (ts == 2 ? 12 : 16));
  printT(b);
}

static void renderGarden() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillRect(0, 0, 466, 200, C565(0xa8, 0xdc, 0xf8));
  gfx->fillRect(0, 200, 466, 266, C565(0x78, 0xc0, 0x60));
  funHeader(T(S_FUN_GARDEN), UI_INK);
  drawWorker(CX, 226);
  uint32_t now = fnNow();
  for (int p = 0; p < GARDEN_PLOTS; p++) drawPlot(p, now);
}

static void gardenTap(int16_t x, int16_t y) {
  if (y < GD_Y || y > GD_BTN + 40) return;
  int p = -1;
  for (int k = 0; k < GARDEN_PLOTS; k++)
    if (x >= plotX(k) && x <= plotX(k) + GD_W) p = k;
  if (p < 0) return;
  uint32_t now = fnNow();
  if (!now) { toast(T(S_TIME_UNSET)); return; }
  uint8_t bit = 1 << p;
  bool wet = wetRef() & bit;
  uint8_t st = plotState(plotRef(p), wet, now, FUN_DIV);
  if (st == PLOT_EMPTY) {
    if (!canPay(FUN_COST[FUN_GARDEN])) return;
    fnAsk = ASK_PLANT;  // primero, confirmar
    fnAskPlot = p;
    return;
  } else if (st == PLOT_GROWING) {
    if (wet) { toast(T(S_WATERED)); return; }
    wetRef() |= bit;
    sfxPlay(SFX_PLAY);
  } else {
    float u[5];
    for (int i = 0; i < 5; i++) u[i] = fuRand();
    fnLoot = gardenLoot(u, pet.chessLuck());
    fnAcc = 0xFF;
    plotRef(p) = 0;
    wetRef() &= ~bit;
    showLoot(S_LOOT);
  }
  if (!fnPractice) pet.markDirty();
}

static void doPlant(int p) {
  uint32_t now = fnNow();
  if (!now || plotRef(p) || !funPay(FUN_COST[FUN_GARDEN])) return;
  plotRef(p) = now;
  wetRef() &= ~(1 << p);
  sfxPlay(SFX_TAP);
  if (!fnPractice) pet.markDirty();
}

// ---------- pesca ----------

#define FS_BTN 360
static void renderFish() {
  uint32_t ms = millis();
  if (fsStage == FS_WAIT && ms >= fsBite) {
    fsStage = FS_BITE;
    fsT0 = ms;
    Serial.println("ocio pica");
  }
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillRect(0, 0, 466, 190, C565(0xa8, 0xdc, 0xf8));
  gfx->fillRect(0, 190, 466, 276, C565(0x38, 0x88, 0xd0));
  for (int k = 0; k < 6; k++) gfx->drawFastHLine(40 + (k * 70) % 380, 214 + k * 22, 40, C565(0x70, 0xb8, 0xf0));
  gfx->fillTriangle(0, 250, 0, 466, 190, 466, C565(0xe8, 0xd0, 0x98));  // orilla
  gfx->fillRect(0, 290, 110, 176, C565(0xe8, 0xd0, 0x98));
  funHeader(T(S_FUN_FISH), UI_INK);
  drawWorker(80, 300);
  int bx = 300, by = 236;
  float s = (ms % 100000) / 1000.0f;
  if (fsStage == FS_WAIT) by += (int)(sinf(s * 3) * 3);
  if (fsStage == FS_BITE) {
    by += 10 + (int)(sinf(s * 25) * 4);
    for (int r = 14; r <= 34; r += 10) gfx->drawCircle(bx, 246, r, C565(0xc8, 0xe8, 0xff));
  }
  gfx->drawLine(120, 250, 200, 150, C565(0x8a, 0x60, 0x38));  // caña y sedal
  gfx->drawLine(121, 250, 201, 150, C565(0x8a, 0x60, 0x38));
  if (fsStage != FS_IDLE) {
    gfx->drawLine(200, 150, bx, by - 6, UI_WHITE);
    gfx->fillCircle(bx, by, 8, UI_WHITE);
    gfx->fillCircle(bx, by - 3, 8, UI_BAR_BAD);
    gfx->fillRect(bx - 9, by, 19, 9, UI_WHITE);
    gfx->fillCircle(bx, by + 2, 7, UI_WHITE);
  }
  if (fsStage == FS_IDLE) {
    char b[32];
    costLabel(b, sizeof(b), S_CAST, FUN_FISH);
    funButton(FS_BTN, b, UI_BAR_OK);
  } else {
    const char *m = T(fsStage == FS_BITE ? S_BITE : S_WAIT_BITE);
    int sz = fsStage == FS_BITE ? 3 : 2;
    gfx->setTextColor(fsStage == FS_BITE ? UI_BAR_BAD : UI_WHITE);
    setSize(sz);
    setCur(centerX(m, sz), 110);
    printT(m);
  }
  if (fsStage == FS_BITE && ms - fsT0 > 1200) {  // no se recogio a tiempo: solo un pez pequeño
    fnLoot = fishLoot(false, fuRand(), pet.chessLuck());
    fnAcc = 0xFF;
    fsStage = FS_IDLE;
    showLoot(S_SMALL_CATCH);
  }
}

static void fishTap(int16_t x, int16_t y) {
  if (fsStage == FS_IDLE) {
    if (!inButton(x, y, FS_BTN) || !canPay(FUN_COST[FUN_FISH])) return;
    fnAsk = ASK_CAST;  // primero, confirmar
    return;
  }
  bool good = fsStage == FS_BITE;  // antes de que pique: se asusta (pez pequeño)
  fnLoot = fishLoot(good, fuRand(), pet.chessLuck());
  fnAcc = 0xFF;
  fsStage = FS_IDLE;
  showLoot(good ? S_GREAT_CATCH : S_SMALL_CATCH);
}

static void doCast() {
  if (!funPay(FUN_COST[FUN_FISH])) return;
  sfxPlay(SFX_TAP);
  fsStage = FS_WAIT;
  fsBite = millis() + 2000 + random(4000);
}

// ---------- mina ----------

#define MN_X (CX + 50)
#define MN_Y 250
#define MN_R 64
static void renderMine() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillRect(0, 0, 466, 330, C565(0x4a, 0x40, 0x48));
  gfx->fillRect(0, 330, 466, 136, C565(0x6a, 0x58, 0x48));
  static const int16_t CR[6][2] = { { 70, 160 }, { 390, 150 }, { 150, 230 }, { 410, 290 }, { 40, 260 }, { 330, 350 } };
  for (auto &c : CR) {  // cristales en la pared
    gfx->fillTriangle(c[0], c[1] - 10, c[0] - 5, c[1] + 4, c[0] + 5, c[1] + 4, C565(0x88, 0xd8, 0xf8));
  }
  funHeader(T(S_FUN_MINE), UI_WHITE);
  drawWorker(96, 340);
  int dx = millis() - mnHitAt < 120 ? 5 : 0;
  int rx = MN_X + dx;
  gfx->fillCircle(rx, MN_Y, MN_R, C565(0x90, 0x88, 0x88));
  gfx->fillCircle(rx - 18, MN_Y - 20, MN_R / 2, C565(0xa8, 0xa0, 0xa0));
  gfx->drawCircle(rx, MN_Y, MN_R, UI_INK);
  for (int k = 0; k < mnHits; k++) {  // grietas
    float a = k * 1.3f + 0.4f;
    gfx->drawLine(rx, MN_Y, rx + (int)(cosf(a) * MN_R * 0.9f), MN_Y + (int)(sinf(a) * MN_R * 0.9f), UI_INK);
    gfx->drawLine(rx + 1, MN_Y, rx + 1 + (int)(cosf(a) * MN_R * 0.9f), MN_Y + (int)(sinf(a) * MN_R * 0.9f), UI_INK);
  }
  if (!mnRock) {
    char b[32];
    costLabel(b, sizeof(b), S_DIG, FUN_MINE);
    funButton(360, b, UI_BAR_OK);
  } else {
    gfx->setTextColor(UI_WHITE);
    setSize(2);
    setCur(centerX(T(S_TAP_ROCK), 2), 110);
    printT(T(S_TAP_ROCK));
  }
}

static void mineTap(int16_t x, int16_t y) {
  if (!mnRock) {
    if (!inButton(x, y, 360) || !canPay(FUN_COST[FUN_MINE])) return;
    fnAsk = ASK_DIG;  // primero, confirmar
    return;
  }
  int dx = x - MN_X, dy = y - MN_Y;
  if (dx * dx + dy * dy > (MN_R + 20) * (MN_R + 20)) return;
  mnHits++;
  mnHitAt = millis();
  sfxPlay(SFX_PLAY);
  if (mnHits < MN_HITS) return;
  fnLoot = mineLoot(fuRand(), pet.chessLuck());
  fnAcc = 0xFF;
  mnRock = false;
  mnHits = 0;
  showLoot(S_LOOT);
}

static void doDig() {
  if (!funPay(FUN_COST[FUN_MINE])) return;
  sfxPlay(SFX_TAP);
  mnRock = true;
  mnHits = 0;
}

// ---------- expedicion ----------

static void questPathPos(float q, int &x, int &y) {  // del campamento a la X
  x = 90 + (int)(q * 270);
  y = 300 - (int)(q * 130) + (int)(sinf(q * 6.28f) * 24);
}

static void renderQuest() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillRect(0, 0, 466, 466, C565(0xf0, 0xdc, 0xa8));
  for (int i = 0; i <= 20; i++) {  // camino de puntos
    int x, y;
    questPathPos(i / 20.0f, x, y);
    gfx->fillCircle(x, y, 2, C565(0x8a, 0x60, 0x38));
  }
  gfx->fillCircle(150, 160, 30, C565(0x9c, 0xd0, 0x80));  // islas del mapa
  gfx->fillCircle(330, 330, 36, C565(0x9c, 0xd0, 0x80));
  for (int k = -1; k <= 1; k++) {
    gfx->drawLine(345 + k, 155, 375 + k, 185, UI_BAR_BAD);
    gfx->drawLine(375 + k, 155, 345 + k, 185, UI_BAR_BAD);
  }
  funHeader(T(S_FUN_QUEST), UI_INK);
  uint32_t now = fnNow(), end = questRef();
  if (!end) {
    drawWorker(90, 320);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(T(S_BACK_IN_HOUR), 2), 108);
    printT(T(S_BACK_IN_HOUR));
    char b[32];
    costLabel(b, sizeof(b), S_GO_OUT, FUN_QUEST);
    funButton(360, b, UI_BAR_OK);
    return;
  }
  // de camino (o ya de vuelta): su miniatura sobre el mapa
  uint32_t total = QUEST_SECS / FUN_DIV;
  float q = now >= end ? 1.0f : 1.0f - (end - now) / (float)total;
  int x, y;
  questPathPos(q, x, y);
  int16_t quien = fnPractice ? fnDex : pet.questDex;
  const uint8_t *th = thumbs.get(quien);
  if (th) drawThumb(th, x - 32, y - 56, 2, false);
  else gfx->fillCircle(x, y - 16, 14, UI_BAR_WARN);
  char t[32];
  if (now >= end) {
    snprintf(t, sizeof(t), "%s", T(S_QUEST_BACK));
    funButton(360, T(S_CLAIM), 0xF427);
  } else {
    timeLeftText(t, sizeof(t), end - now);
  }
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(t, 2), 108);
  printT(t);
}

static int8_t missingAccessory() {
  int8_t falta[ACC_COUNT];
  int n = 0;
  for (int a = 0; a < ACC_COUNT; a++)
    if (!(pet.accOwned & (1 << a))) falta[n++] = a;
  return n ? falta[random(n)] : -1;
}

static void questTap(int16_t x, int16_t y) {
  if (!inButton(x, y, 360)) return;
  uint32_t now = fnNow(), end = questRef();
  if (!now) { toast(T(S_TIME_UNSET)); return; }
  if (!end) {
    if (!canPay(FUN_COST[FUN_QUEST])) return;
    fnAsk = ASK_QUEST;  // primero, confirmar
    return;
  }
  if (now < end) return;
  int8_t acc = missingAccessory();
  uint8_t a = acc < 0 ? 0xFF : (uint8_t)acc;
  fnLoot = questLoot(fuRand(), fuRand(), pet.chessLuck(), a);
  fnAcc = a;
  questRef() = 0;
  if (!fnPractice) pet.markDirty();
  showLoot(S_LOOT);
}

static void doQuest() {
  uint32_t now = fnNow();
  if (!now || questRef() || !funPay(FUN_COST[FUN_QUEST])) return;
  questRef() = now + QUEST_SECS / FUN_DIV;
  if (!fnPractice) {
    pet.questDex = fnDex;
    pet.markDirty();
  }
  sfxPlay(SFX_TAP);
  if (fnMine && !fnPractice) closeFun();  // se va: a la principal, a ver el cartel
  hubOpen = hubOpen && !(fnMine && !fnPractice);
}

// ---------- lo que salio ----------

static void renderFunLoot() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(fnLootTitle), 3), 70);
  printT(T(fnLootTitle));
  int rows = fnLoot.n + (fnAcc != 0xFF ? 1 : 0);
  int y = 230 - rows * 30;
  for (int i = 0; i < rows; i++, y += 60) {
    char t[40];
    if (i < fnLoot.n) {
      drawItemIcon(fnLoot.item[i], CX - 120, y);
      snprintf(t, sizeof(t), "%s  x%u", T(ITEM_NAME[fnLoot.item[i]]), (unsigned)fnLoot.qty[i]);
    } else {
      drawAccessory(fnAcc, CX - 96, y + 46);
      snprintf(t, sizeof(t), "%s", T((StrId)(S_ACC_BOW + fnAcc)));
    }
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(CX - 60, y + 16);
    printT(t);
  }
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_TAP_NEXT), 2), 380);
  printT(T(S_TAP_NEXT));
}

// ---------- pantalla, toques y deslizar ----------

void renderFun() {
  switch (fnStage) {
    case FN_LIST: renderFunList(); break;
    case FN_PICK: renderFunPick(); break;
    case FN_LOOT: renderFunLoot(); break;
    default:
      if (fnAct == FUN_GARDEN) renderGarden();
      else if (fnAct == FUN_FISH) renderFish();
      else if (fnAct == FUN_MINE) renderMine();
      else renderQuest();
  }
  if (fnAsk && fnStage == FN_PLAY) {  // ¿gastar los puntos?
    static const StrId QUE[5] = { S_PLANT, S_PLANT, S_CAST, S_DIG, S_GO_OUT };
    char l2[32];
    snprintf(l2, sizeof(l2), T(S_SPEND_Q_FMT), (unsigned)FUN_COST[fnAct]);
    drawAskBox(T(QUE[fnAsk]), l2);
  }
  drawToast();
  gfx->flush();
}

void funTap(int16_t x, int16_t y) {
  if (fnAsk) {  // SI: se hace (y se paga); NO o fuera: nada
    uint8_t a = fnAsk;
    fnAsk = ASK_NONE;
    if (!askTap(x, y) || fnStage != FN_PLAY) return;
    if (a == ASK_PLANT) doPlant(fnAskPlot);
    else if (a == ASK_CAST) doCast();
    else if (a == ASK_DIG) doDig();
    else doQuest();
    return;
  }
  switch (fnStage) {
    case FN_LIST: funListTap(x, y); break;
    case FN_PICK: funPickTap(x, y); break;
    case FN_LOOT:
      if (millis() - fnLootAt < 1000) break;  // no con un toque que aun llega
      fnStage = FN_PLAY;
      countWho();
      break;
    default:
      if (fnAct == FUN_GARDEN) gardenTap(x, y);
      else if (fnAct == FUN_FISH) fishTap(x, y);
      else if (fnAct == FUN_MINE) mineTap(x, y);
      else questTap(x, y);
  }
}

// a la derecha: atras (la lista; desde la lista, a actividades). Pescando o con la piedra a
// medias no se sale sin querer; en la granja, a la izquierda, la pagina siguiente
void funSwipe(int dir) {
  if (fnAsk) { fnAsk = ASK_NONE; return; }  // deslizar cancela la pregunta
  if (dir <= 0) {
    if (fnStage == FN_PICK && fnCandN > 6) fnPage = (fnPage + 1) % ((fnCandN + 5) / 6);
    return;
  }
  if (fnStage == FN_PLAY && (fsStage != FS_IDLE || mnRock)) return;
  if (fnStage == FN_LIST || fnPractice) { closeFun(); return; }
  rivalPmd[0].unload();
  fnStage = FN_LIST;
  countWho();
}
