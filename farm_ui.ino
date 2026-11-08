// Pantallas de la granja y del album de recuerdos.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

// ---------- granja: compañeros guardados en la SD ----------

// aviso breve arriba (lo usan la pantalla principal y la granja)
void drawToast() {
  if (!toastMsg || !timeLeft(toastUntil)) return;
  setSize(2);
  int w = textW(toastMsg, 2) + 32;
  gfx->fillRoundRect(CX - w / 2, 92, w, 40, 12, UI_INK);
  gfx->setTextColor(UI_WHITE);
  setCur(CX - w / 2 + 16, 104);
  printT(toastMsg);
}

// RECOGER: guarda al compañero tal cual en la granja y se despide hacia alla
void collectCurrent() {
  if (pet.isEgg() || pet.ceremony) return;
  if (!farmReady()) { toast(T(S_NEED_SD)); return; }
  PetRec r;
  pet.exportRec(r);
  if (farmPut(pet, r)) pet.startCollect();
}

void openFarm() {
  if (pet.ceremony) return;
  farmOpen = true;
  farmPage = 0;
  farmDetail = -1;
  farmDirty = true;
}

void closeFarm() {
  farmOpen = false;
  albumOpen = false;
  farmDetail = -1;
  galleryPmd.unload();
}

// rejilla de la granja: 3 x 3 casillas grandes (miniaturas a x3)
#define FARM_COLS 3
#define FARM_PER_PAGE 9
#define FARM_CELL 112
#define FARM_X0 (CX - FARM_COLS * FARM_CELL / 2)
#define FARM_Y0 70
static int farmPages() { return farmCount() > 0 ? (farmCount() + FARM_PER_PAGE - 1) / FARM_PER_PAGE : 1; }

// sacar al elegido: el compañero actual entra en la granja en su lugar
void farmSwapIn(int idx) {
  if (pet.isEgg()) { toast(T(S_HATCH_FIRST)); return; }
  if (pet.ceremony || idx < 0 || idx >= farmCount()) return;
  int16_t dex = farmAt(idx).speciesId;
  PetRec cur, sale;
  pet.exportRec(cur);
  if (!farmTake(pet, dex, sale)) return;
  if (!farmPut(pet, cur)) {  // no se pudo guardar al actual: se deshace
    farmPut(pet, sale);
    toast(T(S_NEED_SD));
    return;
  }
  pet.importRec(sale);
  closeFarm();
}

void farmSwipe(int dir) {
  if (albumOpen) {
    albumSwipe(dir);
    return;
  }
  if (farmDetail >= 0) {  // en detalle: volver a la rejilla
    farmDetail = -1;
    galleryPmd.unload();
    farmDirty = true;
    return;
  }
  int np = farmPage - dir;  // deslizar a la izquierda avanza pagina
  if (np < 0) { closeFarm(); return; }  // retroceder desde la primera = salir
  if (np > farmPages() - 1) np = farmPages() - 1;
  if (np != farmPage) {
    farmPage = np;
    farmDirty = true;
  }
}

#define FARM_BTN_X 143
#define FARM_BTN_Y 352
#define FARM_BTN_W 180
#define FARM_BTN_H 48

#define ALBUM_BTN_X 173
#define ALBUM_BTN_Y 424
#define ALBUM_BTN_W 120
#define ALBUM_BTN_H 32

void farmTap(int16_t x, int16_t y) {
  if (albumOpen) {
    albumTap(x, y);
    return;
  }
  if (farmDetail < 0 && x >= ALBUM_BTN_X && x <= ALBUM_BTN_X + ALBUM_BTN_W && y >= ALBUM_BTN_Y - 6) {
    albumOpen = true;  // boton del album, abajo
    albumPage = 0;
    farmDirty = true;
    return;
  }
  if (farmDetail >= 0) {
    if (x >= FARM_BTN_X && x <= FARM_BTN_X + FARM_BTN_W && y >= FARM_BTN_Y && y <= FARM_BTN_Y + FARM_BTN_H) {
      farmSwapIn(farmDetail);
      return;
    }
    farmDetail = -1;  // fuera del boton: volver a la rejilla
    galleryPmd.unload();
    farmDirty = true;
    return;
  }
  if (y < FARM_Y0) { closeFarm(); return; }  // tocar la cabecera = salir
  if (x < FARM_X0) return;
  int c = (x - FARM_X0) / FARM_CELL, r = (y - FARM_Y0) / FARM_CELL;
  if (c >= FARM_COLS || r >= FARM_COLS) return;
  int idx = farmPage * FARM_PER_PAGE + r * FARM_COLS + c;
  if (idx >= farmCount()) return;
  farmDetail = idx;
  galleryPmd.load(farmAt(idx).speciesId, farmAt(idx).shiny);
}

void renderFarm() {
  if (albumOpen) {
    renderAlbum();
    return;
  }
  if (farmDetail >= 0 && farmDetail < farmCount()) {  // detalle: animado, se repinta siempre
    const FarmEntry &e = farmAt(farmDetail);
    const DexEntry &d = DEX_TBL[e.speciesId];
    gfx->fillScreen(RGB565_BLACK);
    gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
    char head[40];
    snprintf(head, sizeof(head), "%s%s", e.shiny ? "*" : "", e.nick[0] ? e.nick : dexName(e.speciesId));
    gfx->setTextColor(d.accent);
    setSize(3);
    int ts = (textW(head, 3) <= 260) ? 3 : 2;
    setSize(ts);
    setCur(centerX(head, ts), 52);
    printT(head);
    char lv[16];
    snprintf(lv, sizeof(lv), T(S_LVL_FMT), farmLevel(e, pet.lastSeenEpoch));
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(lv, 2), 92);
    printT(lv);
    if (galleryPmd.loaded) drawPmdActM(galleryPmd, PMD_IDLE, CX, 320, millis(), true, false, 5);
    gfx->fillRoundRect(FARM_BTN_X, FARM_BTN_Y, FARM_BTN_W, FARM_BTN_H, 12, UI_BAR_OK);
    gfx->setTextColor(UI_WHITE);
    setSize(2);
    setCur(FARM_BTN_X + (FARM_BTN_W - textW(T(S_SWAP), 2)) / 2, FARM_BTN_Y + 16);
    printT(T(S_SWAP));
    drawToast();
    gfx->flush();
    return;
  }

  // la rejilla es estatica, salvo mientras dura un aviso
  bool avisando = toastMsg && timeLeft(toastUntil);
  if (!farmDirty && !avisando) return;
  farmDirty = avisando;  // repinta una vez mas al apagarse el aviso
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  char head[24];
  snprintf(head, sizeof(head), T(S_FARM_FMT), (unsigned)farmCount());
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(head, 3), 36);
  printT(head);
  if (!farmReady() || farmCount() == 0) {
    const char *m = farmReady() ? T(S_FARM_EMPTY) : T(S_NEED_SD);
    setSize(2);
    setCur(centerX(m, 2), CY - 8);
    printT(m);
  }
  for (int i = 0; i < FARM_PER_PAGE; i++) {
    int idx = farmPage * FARM_PER_PAGE + i;
    if (idx >= farmCount()) break;
    const FarmEntry &e = farmAt(idx);
    int x = FARM_X0 + (i % FARM_COLS) * FARM_CELL, y = FARM_Y0 + (i / FARM_COLS) * FARM_CELL;
    const uint8_t *t = thumbs.get(e.speciesId);
    if (t) drawThumbIn(t, x, y, 3, false, FARM_CELL);
    if (e.shiny) {
      gfx->setTextColor(UI_BAR_WARN);
      setSize(2);
      setCur(x + FARM_CELL - 22, y + 6);
      printT("*");
    }
  }
  if (farmPages() > 1) {  // pagina, a la izquierda del boton del album
    char pg[8];
    snprintf(pg, sizeof(pg), "%d/%d", farmPage + 1, farmPages());
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(ALBUM_BTN_X - 12 - textW(pg, 2), ALBUM_BTN_Y + 8);
    printT(pg);
  }
  // boton del album de recuerdos
  gfx->fillRoundRect(ALBUM_BTN_X, ALBUM_BTN_Y, ALBUM_BTN_W, ALBUM_BTN_H, 10, 0x4C98);
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(ALBUM_BTN_X + (ALBUM_BTN_W - textW(T(S_ALBUM), 2)) / 2, ALBUM_BTN_Y + 8);
  printT(T(S_ALBUM));
  drawToast();
  gfx->flush();
}

// ---------- album de recuerdos: una fila por compañero que acabo ----------

#define ALBUM_ROWS 4
#define ALBUM_X 62
#define ALBUM_Y 78
#define ALBUM_W 342
#define ALBUM_RH 76

static int albumPages() {
  int n = memoirCount();
  return n > 0 ? (n + ALBUM_ROWS - 1) / ALBUM_ROWS : 1;
}

static void albumDate(uint32_t e, char *out, size_t cap) {
  if (!e) { snprintf(out, cap, "?"); return; }
  time_t t = e;
  struct tm tm;
  gmtime_r(&t, &tm);
  snprintf(out, cap, "%d/%02d/%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

void renderAlbum() {
  bool avisando = toastMsg && timeLeft(toastUntil);
  if (!farmDirty && !avisando) return;
  farmDirty = avisando;
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  int n = memoirCount();
  char head[24];
  snprintf(head, sizeof(head), T(S_ALBUM_FMT), n);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(head, 3), 36);
  printT(head);
  if (n == 0) {
    const char *m = farmReady() ? T(S_ALBUM_EMPTY) : T(S_NEED_SD);
    setSize(2);
    setCur(centerX(m, 2), CY - 8);
    printT(m);
  }
  static const uint16_t END_COL[4] = { UI_BAR_OK, 0x8410, UI_BAR_BAD, 0x4C98 };
  for (int k = 0; k < ALBUM_ROWS; k++) {
    MemRec r;
    if (!memoirGet(albumPage * ALBUM_ROWS + k, r)) break;
    int y = ALBUM_Y + k * ALBUM_RH;
    gfx->fillRoundRect(ALBUM_X, y, ALBUM_W, ALBUM_RH - 6, 10, UI_WHITE);
    const uint8_t *t = thumbs.get(r.speciesId);
    if (t) drawThumb(t, ALBUM_X + 4, y + 3, 2, false);
    char l1[48];
    snprintf(l1, sizeof(l1), "%s%s Lv.%u", r.shiny ? "*" : "", r.nick[0] ? r.nick : dexName(r.speciesId), r.level);
    gfx->setTextColor(DEX_TBL[r.speciesId].accent);
    setSize(2);
    setCur(ALBUM_X + 76, y + 6);
    printT(l1);
    uint8_t e = (r.endType >= CER_FAREWELL && r.endType <= CER_COLLECT) ? r.endType : CER_FAREWELL;
    gfx->setTextColor(END_COL[e - CER_FAREWELL]);
    setCur(ALBUM_X + 76, y + 28);
    printT(T((StrId)(S_END_FAREWELL + e - CER_FAREWELL)));
    int medals = __builtin_popcount(r.medals & ~MED_RETIRED);
    if (medals) {  // estrella + numero de medallas
      drawMap(SPR_ICON_STAR, 16, ALBUM_X + ALBUM_W - 64, y + 26, 1, false);
      char m[8];
      snprintf(m, sizeof(m), "%d", medals);
      gfx->setTextColor(UI_INK);
      setCur(ALBUM_X + ALBUM_W - 44, y + 28);
      printT(m);
    }
    char d0[16], d1[16], l3[40];
    albumDate(r.bornEpoch, d0, sizeof(d0));
    albumDate(r.endEpoch, d1, sizeof(d1));
    snprintf(l3, sizeof(l3), "%s - %s", d0, d1);
    gfx->setTextColor(0x8410);
    setSize(1);
    setCur(ALBUM_X + 76, y + 52);
    printT(l3);
  }
  if (albumPages() > 1) {
    char pg[8];
    snprintf(pg, sizeof(pg), "%d/%d", albumPage + 1, albumPages());
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(pg, 2), 396);
    printT(pg);
  }
  drawToast();
  gfx->flush();
}

void albumSwipe(int dir) {
  int np = albumPage - dir;  // izquierda avanza
  if (np < 0) {              // retroceder desde la primera = volver a la granja
    albumOpen = false;
    farmDirty = true;
    return;
  }
  if (np > albumPages() - 1) np = albumPages() - 1;
  if (np != albumPage) {
    albumPage = np;
    farmDirty = true;
  }
}

void albumTap(int16_t x, int16_t y) {
  if (y < 72) {  // la cabecera vuelve a la granja
    albumOpen = false;
    farmDirty = true;
  }
}
