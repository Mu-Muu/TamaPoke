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

// Confirmar antes de gastar puntos (tienda, ocio): que (l1) y cuanto (l2), SI / NO. Como el
// de restaurar la copia. askTap: 1 = SI, 0 = NO o fuera (se cancela)
void drawAskBox(const char *l1, const char *l2) {
  gfx->fillRoundRect(64, 160, 338, 160, 16, UI_WHITE);
  gfx->drawRoundRect(64, 160, 338, 160, 16, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(l1, 2), 184);
  printT(l1);
  setCur(centerX(l2, 2), 214);
  printT(l2);
  gfx->fillRoundRect(118, 252, 100, 52, 12, UI_BAR_OK);
  gfx->fillRoundRect(248, 252, 100, 52, 12, UI_BAR_BAD);
  gfx->setTextColor(UI_WHITE);
  setCur(118 + (100 - textW(T(S_YES), 2)) / 2, 270);
  printT(T(S_YES));
  setCur(248 + (100 - textW(T(S_NO), 2)) / 2, 270);
  printT(T(S_NO));
}
int askTap(int16_t x, int16_t y) { return (x >= 118 && x <= 218 && y >= 252 && y <= 304) ? 1 : 0; }

// ---------- favoritos (issue #17) ----------

// corazon de medio ancho r en (cx, cy): dos lobulos y un triangulo; in > 0, el mismo in px
// mas adentro (para dejar solo el contorno)
static void heartFill(int cx, int cy, int r, int in, uint16_t c) {
  int a = (r * 5 + 4) / 9, d = r * 4 / 9, ly = cy - r * 7 / 18;  // lobulos: radio, separacion y altura
  int w = d + a - in, by = ly + a / 3 - in;                        // base del triangulo
  gfx->fillCircle(cx - d, ly, a - in, c);
  gfx->fillCircle(cx + d, ly, a - in, c);
  gfx->fillTriangle(cx - w, by, cx + w, by, cx, cy + r * 17 / 18 - in * 3 / 2, c);
}

// el corazon de los favoritos: grande en la ficha de la granja (lleno o solo el contorno, sobre
// el fondo de dia) y chico (r 6) arriba a la izquierda de las casillas de elegir (a la derecha
// ya van el shiny y la estrella del gimnasio). El rosa de los corazones de los gestos
void drawFavHeart(int cx, int cy, int r, bool on) {
  heartFill(cx, cy, r, 0, C565(0xe8, 0x40, 0x68));
  if (!on) heartFill(cx, cy, r, 3, UI_BG_DAY);
}

// las listas de elegir de la granja (gimnasios, ocio): en order (sitio para PET_DEX_MAX) sus
// indices en la granja, los favoritos primero; devuelve cuantos (0: sin memoria)
int farmFavOrder(int *order) {
  static int16_t *dex = nullptr;  // en PSRAM, como la granja
  if (!dex) dex = (int16_t *)ps_malloc(PET_DEX_MAX * sizeof(int16_t));
  if (!dex) return 0;
  int n = farmCount();
  for (int i = 0; i < n; i++) dex[i] = farmAt(i).speciesId;
  return favFirst(pet, dex, n, order);
}

// despedida (al final de su ciclo): se va a vivir a la granja, no desaparece. Antes se iba del
// todo y un toque sin querer a "despedirse" lo perdia. Si no se puede guardar, no se despide
void farewellToFarm() {
  if (pet.isEgg() || pet.ceremony) return;
  if (!farmReady()) { toast(T(S_NEED_SD)); return; }
  PetRec r;
  pet.exportRec(r);
  r.farDeclinedAge = r.ageMinutes + 1440;  // si vuelve a salir, no lo pide enseguida
  if (!farmPut(pet, r)) { toast(T(S_NEED_SD)); return; }
  pet.startFarewell();
}

// RECOGER: guarda al compañero tal cual en la granja y se despide hacia alla
void collectCurrent() {
  if (pet.isEgg() || pet.ceremony) return;
  if (!farmReady()) { toast(T(S_NEED_SD)); return; }
  PetRec r;
  pet.exportRec(r);
  if (farmPut(pet, r)) pet.startCollect();
}

// ---------- actividades: la pagina a la izquierda de la principal ----------
// cinco filas: granja, torneo, gimnasios, ocio y "见面" (la quinta acaba en y=384, aun dentro
// del circulo: alli se puede dibujar de x=54 a 412)
#define HUB_ROW_X 63
#define HUB_ROW_W 340
#define HUB_ROW_H 56
#define HUB_ROW_Y0 80
#define HUB_ROW_STEP 62
enum { HUB_FARM, HUB_SPORTS, HUB_GYMS, HUB_FUN, HUB_LINK, HUB_ROWS };

void openHub() {
  if (pet.ceremony) return;
  hubOpen = true;
}

// por que no se puede competir ahora (nullptr = si se puede)
const char *sportsBlocked() {
  if (pet.vacant || pet.isEgg()) return T(S_NO_PARTNER);
  if (pet.sleeping) return T(S_ASLEEP);
  if (pet.sick) return T(S_SICK);
  if (pet.evolving()) return T(S_EVOLVING);
  if (pet.questAway()) return T(S_AWAY);
  if (pet.energy < 15) return T(S_TOO_TIRED);
  return nullptr;
}

static void hubRow(int row, uint16_t fill, const char *const *icon, bool on, const char *title, const char *sub) {
  int y = HUB_ROW_Y0 + row * HUB_ROW_STEP;
  gfx->fillRoundRect(HUB_ROW_X, y, HUB_ROW_W, HUB_ROW_H, 18, on ? fill : UI_TRACK);
  gfx->drawRoundRect(HUB_ROW_X, y, HUB_ROW_W, HUB_ROW_H, 18, UI_INK);
  drawMap(icon, 16, HUB_ROW_X + 14, y + 4, 3, !on);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(HUB_ROW_X + 76, y + 5);
  printT(title);
  setSize(2);
  setCur(HUB_ROW_X + 76, y + 34);
  printT(sub);
}

void renderHub() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_HUB_TITLE), 3), 50);
  printT(T(S_HUB_TITLE));
  char n[24];
  snprintf(n, sizeof(n), T(S_FARM_SUB_FMT), (unsigned)farmCount());
  hubRow(HUB_FARM, 0xB6F2, SPR_ICON_FARM, true, T(S_HUB_FARM), farmReady() ? n : T(S_NEED_SD));
  const char *why = sportsBlocked();
  hubRow(HUB_SPORTS, 0xFF2E, SPR_ICON_TROPHY, !why, T(S_HUB_SPORTS), why ? why : T(S_HUB_SPORTS_HINT));
  char b[32];
  snprintf(b, sizeof(b), T(S_BADGES_TOTAL_FMT), (unsigned)pet.totalBadges());
  hubRow(HUB_GYMS, 0xAEFF, SPR_ICON_GYM, true, T(S_GYMS), b);  // la lista sirve tambien de vitrina
  hubRow(HUB_FUN, C565(0xf8, 0xd0, 0xc8), SPR_ICON_FUN, true, T(S_HUB_FUN), T(S_HUB_FUN_HINT));
  hubRow(HUB_LINK, C565(0xd8, 0xd8, 0xf8), SPR_ICON_LINK, true, T(S_HUB_LINK), T(S_HUB_FRIENDS_SUB));
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_SWIPE_R_BACK), 2), 392);
  printT(T(S_SWIPE_R_BACK));
  drawToast();
  gfx->flush();
}

void hubTap(int16_t x, int16_t y) {
  if (x < HUB_ROW_X || x >= HUB_ROW_X + HUB_ROW_W || y < HUB_ROW_Y0) return;
  int row = (y - HUB_ROW_Y0) / HUB_ROW_STEP;
  if (row >= HUB_ROWS || y - HUB_ROW_Y0 - row * HUB_ROW_STEP > HUB_ROW_H) return;  // entre filas
  if (row == HUB_FARM) {
    sfxPlay(SFX_TAP);
    openFarm();
  } else if (row == HUB_SPORTS) {
    const char *why = sportsBlocked();
    if (why) { toast(why); return; }
    sfxPlay(SFX_TAP);
    openSports();
  } else if (row == HUB_GYMS) {
    sfxPlay(SFX_TAP);
    openGyms();
  } else if (row == HUB_FUN) {
    sfxPlay(SFX_TAP);
    openFun();
  } else {
    sfxPlay(SFX_TAP);
    openFriends();  // "好友": "见面" con los de al lado y el correo con los de lejos (mail_ui.ino)
  }
}

void hubSwipe(int dir) {
  if (dir > 0) hubOpen = false;  // a la derecha: vuelta a la principal
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
#define FARM_CELL 106
#define FARM_X0 (CX - FARM_COLS * FARM_CELL / 2)
#define FARM_Y0 70
static int farmPages() { return farmCount() > 0 ? (farmCount() + FARM_PER_PAGE - 1) / FARM_PER_PAGE : 1; }

// ya ha entrado: a la principal, a verlo (issue #14: la granja se abre desde las actividades, y al
// cerrarla sola se volvia a ellas; parecia que no habia funcionado y se repetia)
static void swappedIn() {
  closeFarm();
  hubOpen = false;
}

// sacar al elegido: el compañero actual entra en la granja en su lugar. Sin compañero
// (hueco) sale sin mas: antes el hueco contaba como huevo y no dejaba sacar a nadie
void farmSwapIn(int idx) {
  if (!pet.canTakeFromFarm()) { toast(T(S_HATCH_FIRST)); return; }
  if (pet.ceremony || idx < 0 || idx >= farmCount()) return;
  int16_t dex = farmAt(idx).speciesId;
  PetRec cur, sale;
  // se lee sin sacarlo y se borra de la granja lo ultimo, ya guardado el cambio (importRec guarda en
  // la NVS): si algo falla o se va la luz en medio, como mucho queda repetido, nunca se pierde
  if (!farmPeek(dex, sale)) return;
  if (pet.vacant) {
    pet.importRec(sale);
    farmTake(pet, dex, sale);
    swappedIn();
    return;
  }
  pet.exportRec(cur);
  if (!farmPut(pet, cur)) {  // no se pudo guardar al actual: todo sigue como estaba
    toast(T(S_NEED_SD));
    return;
  }
  pet.importRec(sale);
  if (cur.speciesId != dex) farmTake(pet, dex, sale);  // misma especie: su fichero ya es el del que entro
  swappedIn();
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
#define ALBUM_BTN_Y 398  // mas abajo lo cortaba el circulo
#define ALBUM_BTN_W 120
#define ALBUM_BTN_H 32

// frutas para los de la granja: la misma regla que el compañero (FRUITS_PER_DAY al dia,
// en su ficha); alli no pasan hambre, asi que pasado el tope no se gasta
#define FF_Y 128
#define FAV_X (CX + 150)  // el corazon de favorito, a la derecha (entre las frutas y el boton)
#define FAV_Y 200
static PetRec farmRec;        // la ficha (SD) del que se esta viendo: tipo y capacidades
static bool farmRecOk = false;
static PetRec farmUndo;       // la ficha de antes de la ultima fruta (consola FARMUNDO, pruebas)
static bool farmUndoOk = false;
// sigue megaevolucionado en la granja (issue #18: sus 24 h no se paran al entrar)
static bool farmRecMega() { return farmRecOk && recIsMega(farmRec, pet.lastSeenEpoch); }
static int fruitRowX(int k) { return CX - 96 + k * 64; }
static bool hasStatFruit() {
  for (int k = 0; k < 4; k++)
    if (pet.bag[ITEM_FRUIT_STA + k]) return true;
  return false;
}
static void feedFarmFruit(int idx, uint8_t item) {
  if (pet.bag[item] == 0) { toast(T(S_NONE_LEFT)); return; }
  PetRec r;
  if (!farmPeek(farmAt(idx).speciesId, r)) return;
  uint8_t hoy = (uint8_t)(pet.dayNow() & 0xFF);
  if (r.fruitDay8 != hoy) { r.fruitDay8 = hoy; r.fruitCnt = 0; }
  if (r.fruitCnt >= FRUITS_PER_DAY) { toast(T(S_FRUIT_FULL)); return; }
  if (!fruitRaise(item, r.trAtk, r.trDef, r.trSpe)) { toast(T(S_MAXED)); return; }
  farmUndo = farmRec;  // como estaba (farmRec: leida al abrir la ficha)
  farmUndoOk = farmRecOk;
  r.fruitCnt++;
  if (!farmPut(pet, r)) return;
  farmRec = r;
  pet.bag[item]--;
  pet.markDirty();
  toast(T(fruitUpMsg(item)));
  sfxPlay(SFX_EAT);
}

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
    if (hasStatFruit() && abs(y - FF_Y) <= 26) {  // fila de frutas: darle una
      for (int k = 0; k < 4; k++)
        if (abs(x - fruitRowX(k)) <= 28) {
          feedFarmFruit(farmDetail, ITEM_FRUIT_STA + k);
          return;
        }
    }
    int dx = x - FAV_X, dy = y - FAV_Y;
    if (dx * dx + dy * dy <= 28 * 28 && farmDetail < farmCount()) {  // el corazon: favorito o no
      int16_t d = farmAt(farmDetail).speciesId;
      pet.setFav(d, !pet.isFav(d));
      sfxPlay(SFX_TAP);
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
  const FarmEntry &e = farmAt(idx);
  farmRecOk = farmPeek(e.speciesId, farmRec);  // antes que el sprite: dice si sigue Mega
  bool mega = farmRecMega();
  galleryPmd.load(e.speciesId, e.shiny, mega);
  if (mega && !galleryPmd.loaded) galleryPmd.load(e.speciesId, e.shiny);  // sin sprite Mega en la SD: el normal
}

// consola FARMUNDO: deja como estaba al ultimo de la granja al que se le dio una fruta
bool farmUndoFruit() {
  if (!farmUndoOk) return false;
  farmUndoOk = false;
  if (!farmPut(pet, farmUndo)) return false;
  farmRec = farmUndo;
  return true;
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
    char head[48], nb[40];
    const char *nm = e.nick[0] ? e.nick : dexName(e.speciesId);
    const MegaForm *m = farmRecMega() ? megaFor(e.speciesId) : nullptr;
    if (m) {  // megaevolucionado: "MEGA ... X", como petName (render.ino)
      char tag[4] = "";
      if (m->tag[0]) snprintf(tag, sizeof(tag), " %s", m->tag);
      snprintf(nb, sizeof(nb), T(S_MEGA_NAME_FMT), nm, tag);
      nm = nb;
    }
    snprintf(head, sizeof(head), "%s%s", e.shiny ? "*" : "", nm);
    gfx->setTextColor(d.accent);
    setSize(3);
    int ts = (textW(head, 3) <= 260) ? 3 : 2;
    setSize(ts);
    setCur(centerX(head, ts), 52);
    printT(head);
    // nivel y tipo en una linea; abajo, sus capacidades (lo que cuenta en gimnasios y torneo)
    char lv[16];
    snprintf(lv, sizeof(lv), T(S_LVL_FMT), farmLevel(e, pet.lastSeenEpoch));
    uint8_t ty = typeOf(e.speciesId);
    const char *tn = T((StrId)(S_TY_NORMAL + ty));
    setSize(2);
    int wl = textW(lv, 2), x0 = CX - (wl + 16 + textW(tn, 2)) / 2;
    gfx->setTextColor(UI_INK);
    setCur(x0, 92);
    printT(lv);
    gfx->setTextColor(TYPE_COL[ty]);
    setCur(x0 + wl + 16, 92);
    printT(tn);
    if (farmRecOk) {
      char tr[48];
      snprintf(tr, sizeof(tr), T(S_TRAIN_FMT), farmRec.trAtk, farmRec.trDef, farmRec.trSpe);
      gfx->setTextColor(UI_INK);
      setCur(centerX(tr, 2), 326);
      printT(tr);
    }
    if (galleryPmd.loaded) drawPmdActM(galleryPmd, PMD_IDLE, CX, 320, millis(), true, false, 5);
    drawFavHeart(FAV_X, FAV_Y, 16, pet.isFav(e.speciesId));  // encima del bicho: siempre se ve
    if (hasStatFruit())  // las frutas de la mochila: tocar una se la da
      for (int k = 0; k < 4; k++) {
        uint8_t it = ITEM_FRUIT_STA + k;
        drawFruitIcon(it, fruitRowX(k) - 8, FF_Y, 13, pet.bag[it] == 0);
        char n[8];
        snprintf(n, sizeof(n), "x%u", pet.bag[it]);
        gfx->setTextColor(UI_INK);
        setSize(2);
        setCur(fruitRowX(k) + 8, FF_Y - 7);
        printT(n);
      }
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
    if (pet.isFav(e.speciesId)) drawFavHeart(x + 12, y + 12, 6, true);
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
#define ALBUM_X 70
#define ALBUM_Y 78
#define ALBUM_W 326
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
