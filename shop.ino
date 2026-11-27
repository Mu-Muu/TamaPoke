// Tienda, mochila y elegir huevo cuando no hay compañero.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

// ---------- tienda y mochila ----------


// moneda + puntos arriba, a la derecha de la bateria (tocarla abre la tienda)
void drawPointsBadge(int x, int y) {
  gfx->fillCircle(x + 7, y + 7, 7, UI_BAR_WARN);
  gfx->drawCircle(x + 7, y + 7, 7, UI_INK);
  char n[12];
  snprintf(n, sizeof(n), "%ld", (long)pet.points);
  gfx->setTextColor(inkColor());
  setSize(2);
  setCur(x + 18, y);
  printT(n);
}

// una fruta del ocio (centro y radio); apagada: gris
static const uint16_t FRUIT_COL[4] = { 0xEA87, 0x4C98, 0xF427, 0xE5CC };  // aguante, resistencia, vitalidad, dorada
void drawFruitIcon(uint8_t item, int cx, int cy, int r, bool off) {
  uint16_t c = off ? UI_TRACK : FRUIT_COL[(item - ITEM_FRUIT_STA) & 3];
  gfx->fillCircle(cx, cy, r, c);
  gfx->drawCircle(cx, cy, r, UI_INK);
  gfx->fillCircle(cx - r / 3, cy - r / 3, r / 4 + 1, UI_WHITE);  // brillo
  gfx->fillCircle(cx + r / 2, cy - r - 2, r / 3 + 1, off ? UI_TRACK : 0x5DCD);  // hoja
  if (item == ITEM_FRUIT_GOLD && !off)
    gfx->fillTriangle(cx + r, cy - r, cx + r - 3, cy - r + 6, cx + r + 3, cy - r + 6, UI_WHITE);
}

// lo que dice comerse una fruta que si subio
StrId fruitUpMsg(uint8_t item) {
  return item == ITEM_FRUIT_STA ? S_FRUIT_STA_UP : item == ITEM_FRUIT_RES ? S_FRUIT_RES_UP
         : item == ITEM_FRUIT_VIT ? S_FRUIT_VIT_UP : S_FRUIT_GOLD_UP;
}

// tipo de huevo de un objeto (-1: no es un huevo)
static int eggKindOfItem(int item) {
  for (int k = 0; k < EGG_KINDS; k++)
    if (eggItem(k) == item) return k;
  return -1;
}

// icono de un objeto en una caja de 48x48
void drawItemIcon(int item, int x, int y) {
  if (item >= ITEM_FRUIT_STA && item <= ITEM_FRUIT_GOLD) {
    drawFruitIcon(item, x + 24, y + 28, 15, false);
    return;
  }
  switch (item) {
    case ITEM_FISH:
      gfx->fillCircle(x + 20, y + 26, 13, 0x4C98);
      gfx->fillTriangle(x + 30, y + 26, x + 44, y + 14, x + 44, y + 38, 0x4C98);
      gfx->fillCircle(x + 13, y + 22, 3, UI_WHITE);
      gfx->fillCircle(x + 13, y + 22, 1, UI_INK);
      break;
    case ITEM_PEARL:  // perla en su concha (abanico rosa debajo)
      gfx->fillRoundRect(x + 6, y + 28, 36, 16, 8, C565(0xf8, 0xb8, 0xc8));
      gfx->drawRoundRect(x + 6, y + 28, 36, 16, 8, C565(0xc0, 0x78, 0x90));
      for (int k = 1; k < 4; k++) gfx->drawFastVLine(x + 6 + k * 9, y + 32, 10, C565(0xc0, 0x78, 0x90));
      gfx->fillCircle(x + 24, y + 26, 11, C565(0xf4, 0xf0, 0xf8));
      gfx->drawCircle(x + 24, y + 26, 11, C565(0xa0, 0xa0, 0xb0));
      gfx->fillCircle(x + 20, y + 22, 3, UI_WHITE);
      break;
    case ITEM_MEGA_SHARD:  // un trocito de piedra mega
      gfx->fillTriangle(x + 24, y + 6, x + 10, y + 34, x + 36, y + 42, C565(0x58, 0xc8, 0xf0));
      gfx->fillTriangle(x + 24, y + 6, x + 36, y + 42, x + 40, y + 20, C565(0xf8, 0x98, 0xc8));
      gfx->drawLine(x + 24, y + 6, x + 10, y + 34, UI_INK);
      gfx->drawLine(x + 10, y + 34, x + 36, y + 42, UI_INK);
      gfx->drawLine(x + 36, y + 42, x + 40, y + 20, UI_INK);
      gfx->drawLine(x + 40, y + 20, x + 24, y + 6, UI_INK);
      break;
    case ITEM_CANDY:       drawMap(SPR_ICON_CANDY, 16, x, y, 3, false); break;
    case ITEM_MEDICINE:    drawMap(SPR_ICON_PILL, 16, x, y, 3, false); break;
    case ITEM_POTION_STR:  drawMap(SPR_ICON_POTION_STR, 16, x, y, 3, false); break;
    case ITEM_POTION_SPD:  drawMap(SPR_ICON_POTION_SPD, 16, x, y, 3, false); break;
    case ITEM_POTION_END:  drawMap(SPR_ICON_POTION_END, 16, x, y, 3, false); break;
    case ITEM_SHINY_CHARM: drawMap(SPR_ICON_STAR, 16, x, y, 3, false); break;
    case ITEM_POTION_EVO: {  // frasco con un remolino de colores y un destello
      gfx->fillRoundRect(x + 18, y + 2, 12, 10, 3, C565(0x9a, 0x70, 0x50));
      gfx->fillCircle(x + 24, y + 30, 16, C565(0xc0, 0x80, 0xf0));
      gfx->fillCircle(x + 19, y + 34, 7, C565(0x70, 0xc8, 0xf8));
      gfx->fillCircle(x + 30, y + 26, 6, C565(0xf8, 0x90, 0xc0));
      gfx->drawCircle(x + 24, y + 30, 16, UI_INK);
      gfx->fillRect(x + 20, y + 10, 8, 6, C565(0xc0, 0x80, 0xf0));
      gfx->fillTriangle(x + 38, y + 4, x + 36, y + 10, x + 40, y + 10, UI_BAR_WARN);
      gfx->fillTriangle(x + 38, y + 16, x + 36, y + 10, x + 40, y + 10, UI_BAR_WARN);
      gfx->fillTriangle(x + 32, y + 10, x + 38, y + 8, x + 38, y + 12, UI_BAR_WARN);
      gfx->fillTriangle(x + 44, y + 10, x + 38, y + 8, x + 38, y + 12, UI_BAR_WARN);
      break;
    }
    case ITEM_MEGA_STONE: {  // esfera de colores con el simbolo (dos hebras) en blanco
      gfx->fillCircle(x + 24, y + 24, 18, C565(0x58, 0xc8, 0xf0));
      gfx->fillCircle(x + 19, y + 19, 10, C565(0xf8, 0x98, 0xc8));
      gfx->fillCircle(x + 30, y + 30, 9, C565(0xf8, 0xd0, 0x58));
      for (int i = 0; i < 5; i++) {  // la doble helice del simbolo mega
        gfx->fillCircle(x + 14 + i * 5, y + 30 - i * 3, 2, UI_WHITE);
        gfx->fillCircle(x + 16 + i * 5, y + 20 + i * 2, 2, UI_WHITE);
      }
      gfx->drawCircle(x + 24, y + 24, 18, UI_INK);
      gfx->fillCircle(x + 17, y + 14, 3, UI_WHITE);  // brillo
      break;
    }
    default: {  // huevos: el de siempre, con una gema del color de su rareza
      drawMap(SPR_EGG, SPRITE_H, x + 8, y + 8, 1, false);
      if (item == ITEM_EGG_RARE) gfx->fillCircle(x + 24, y + 26, 5, 0x4C98);
      if (item == ITEM_EGG_LEGEND) gfx->fillCircle(x + 24, y + 26, 5, UI_BAR_WARN);
      if (item == ITEM_EGG_WATER) gfx->fillCircle(x + 24, y + 26, 5, C565(0x60, 0xd8, 0xf8));
      if (item == ITEM_EGG_FOSSIL) gfx->fillCircle(x + 24, y + 26, 5, C565(0x9a, 0x68, 0x40));
    }
  }
}

#define SHOP_X0 56     // 2 columnas x 5 filas de casillas
#define SHOP_Y0 86
#define SHOP_W 177
#define SHOP_H 58

static int8_t shopAsk = -1;      // confirmando la compra de esto (objeto o complemento); -1 = nada
static bool shopAskAcc = false;

void openShop(int page) {
  if (pet.ceremony) return;
  shopAsk = -1;
  shopOpen = true;
  shopPage = page;
  shopDirty = true;
}

void closeShop() { shopOpen = false; }

void shopSwipe(int dir) {
  if (shopAsk >= 0) { shopAsk = -1; shopDirty = true; return; }  // deslizar cancela la compra
  int np = shopPage - dir;  // a la izquierda: tienda -> mochila -> cosecha (ocio) -> complementos -> libro
  if (np < 0) { closeShop(); return; }
  if (np > 4) np = 4;
  if (np != shopPage) { shopPage = np; shopDirty = true; }
}

// usar algo de la mochila
void useItem(int item) {
  if (pet.bag[item] == 0) { toast(T(S_NONE_LEFT)); return; }
  if (pet.questAway() && eggKindOfItem(item) < 0) { toast(T(S_AWAY)); return; }  // de expedicion
  if (item >= ITEM_FRUIT_STA && item <= ITEM_FRUIT_GOLD) {
    uint8_t r = pet.eatFruit(item);
    if (r == FRUIT_UP) sfxPlay(SFX_EAT);
    toast(r == FRUIT_UP ? T(fruitUpMsg(item)) : r == FRUIT_FULL ? T(S_FRUIT_FULL)
          : r == FRUIT_MAXED ? T(S_MAXED) : pet.sleeping ? T(S_ASLEEP) : T(S_CANT_USE));
    shopDirty = true;
    return;
  }
  switch (item) {
    case ITEM_FISH:
      if (pet.eatFish()) { sfxPlay(SFX_EAT); toast(T(S_FISH_YUM)); }
      else toast(pet.sleeping ? T(S_ASLEEP) : T(S_CANT_USE));
      break;
    case ITEM_PEARL:
      toast(pet.usePearl() ? T(S_PEARL_HAPPY) : T(S_CANT_USE));
      break;
    case ITEM_MEGA_SHARD:
      toast(T(S_SHARD_AUTO));
      break;
    case ITEM_CANDY:
      if (pet.isEgg()) return;
      pet.feedCandy();
      toast(T(S_USED));
      break;
    case ITEM_MEDICINE:
      if (!pet.sick) { toast(T(S_NOT_SICK)); return; }
      pet.giveMedicine();
      toast(T(S_USED));
      break;
    case ITEM_POTION_STR:
    case ITEM_POTION_SPD:
    case ITEM_POTION_END:
      if (pet.isEgg()) return;
      toast(pet.usePotion(item) ? T(S_USED) : T(S_MAXED));
      break;
    case ITEM_SHINY_CHARM:
      toast(T(S_CHARM_AUTO));
      break;
    case ITEM_MEGA_STONE: {  // megaevoluciona un dia; si no se puede, dice por que (y no la gasta)
      uint8_t r = pet.useMegaStone();
      if (r == MEGA_OK) { closeShop(); return; }  // a la principal, a verlo
      toast(T(r == MEGA_NO_FORM ? S_MEGA_NO_FORM : r == MEGA_ALREADY ? S_MEGA_ALREADY
              : pet.sleeping ? S_ASLEEP : S_CANT_USE));
      break;
    }
    case ITEM_POTION_EVO:  // ya puede evolucionar (si le hacia falta)
      toast(pet.useEvoPotion() ? T(S_EVO_READY) : T(S_CANT_USE));
      break;
    default:  // huevos: solo con el hueco libre
      if (!pet.vacant) { toast(T(S_EGG_NEED_EMPTY)); return; }
      if (pet.hatchEgg(eggKindOfItem(item))) closeShop();
  }
  shopDirty = true;
}

void shopTap(int16_t x, int16_t y) {
  if (shopAsk >= 0) {  // ¿comprar? (issue #3: se compraba sin querer al pasar de pagina)
    int8_t que = shopAsk;
    shopAsk = -1;
    shopDirty = true;
    if (!askTap(x, y)) return;
    if (shopAskAcc) {
      if (pet.buyAccessory(que)) { toast(T(S_BOUGHT)); sfxPlay(SFX_MEDAL); }
      else toast(T(S_NO_POINTS));
    } else {
      if (pet.buy(que)) { toast(T(S_BOUGHT)); sfxPlay(SFX_EAT); }
      else toast(T(S_NO_POINTS));
    }
    return;
  }
  if (y < 72) {  // cabecera: los puntos (a la derecha, en la tienda) abren el libro; lo demas, salir
    if (shopPage == 0 && x > CX) { shopPage = 4; shopDirty = true; }
    else closeShop();
    return;
  }
  if (shopPage == 4) return;  // el libro solo se mira
  if (x < SHOP_X0 || y < SHOP_Y0) return;
  int c = (x - SHOP_X0) / SHOP_W, r = (y - SHOP_Y0) / SHOP_H;
  if (c > 1 || r > 4) return;
  int item = r * 2 + c;
  if (shopPage == 2) {  // cosecha: lo del ocio
    if (item < FUN_ITEMS) useItem(FUN_ITEM0 + item);
    shopDirty = true;
    return;
  }
  if (shopPage == 3) {  // complementos: comprar, o ponerse/quitarse el que ya tiene
    if (item >= ACC_COUNT) return;
    if (pet.accOwned & (1 << item)) {
      pet.wearAccessory(item);
      toast(pet.accWorn == item + 1 ? T(S_PUT_ON) : T(S_TAKE_OFF));
    } else if (pet.points >= ACC_PRICE[item]) {
      shopAsk = item;  // primero, confirmar
      shopAskAcc = true;
    } else {
      toast(T(S_NO_POINTS));
    }
    shopDirty = true;
    return;
  }
  if (shopPage == 0) {
    if (item >= SHOP_ITEMS) return;
    if (pet.points >= ITEM_PRICE[item] && pet.bag[item] < 999) {
      shopAsk = item;  // primero, confirmar
      shopAskAcc = false;
    } else {
      toast(T(S_NO_POINTS));
    }
  } else {
    if (item == ITEM_MEGA_STONE && !pet.canMega()) return;  // apagada: tocarla no hace nada
    useItem(item);
  }
  shopDirty = true;
}

// libro de puntos: esta semana dia a dia (ganados en verde, gastados en naranja), de donde
// vienen y en que se van, la semana pasada y un empujon al ajedrez
static void renderBook() {
  pet.ptRoll();  // por si cambio de semana sin apuntar nada
  const PtsWeek &w = pet.ptNow;
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_THIS_WEEK), 2), 84);
  printT(T(S_THIS_WEEK));
  int mx = 10;
  for (int d = 0; d < 7; d++) {
    if (w.dayIn[d] > mx) mx = w.dayIn[d];
    if (w.dayOut[d] > mx) mx = w.dayOut[d];
  }
  const int x0 = CX - 147, base = 196, hmax = 86;
  uint8_t hoy = ptWeekday(pet.dayNow());
  for (int d = 0; d < 7; d++) {
    int gx = x0 + d * 42, hi = w.dayIn[d] * hmax / mx, ho = w.dayOut[d] * hmax / mx;
    if (hi) gfx->fillRect(gx + 8, base - hi, 12, hi, UI_BAR_OK);
    if (ho) gfx->fillRect(gx + 22, base - ho, 12, ho, UI_BAR_WARN);
    const char *l = T((StrId)(S_WD_0 + (d + 1) % 7));  // d = 0 es el lunes; S_WD_0 es el domingo
    gfx->setTextColor(d == hoy ? UI_INK : UI_HINT);
    setCur(gx + 21 - textW(l, 2) / 2, base + 6);
    printT(l);
  }
  gfx->drawFastHLine(x0 + 4, base, 42 * 7 - 8, UI_INK);
  char l[48];
  gfx->setTextColor(C565(0x3c, 0x8a, 0x4c));
  snprintf(l, sizeof(l), T(S_BOOK_CHESS_FMT), w.cat[PT_CHESS], w.wins);
  setCur(centerX(l, 2), 228);
  printT(l);
  snprintf(l, sizeof(l), T(S_BOOK_IN_FMT), w.cat[PT_STEPS], w.cat[PT_OTHER]);
  setCur(centerX(l, 2), 250);
  printT(l);
  snprintf(l, sizeof(l), T(S_BOOK_EXTRA_FMT), w.cat[PT_HOCKEY], w.cat[PT_REWARD]);  // los dos del movil
  setCur(centerX(l, 2), 272);
  printT(l);
  gfx->setTextColor(C565(0xb0, 0x60, 0x10));
  snprintf(l, sizeof(l), T(S_BOOK_OUT_FMT), w.cat[PT_SHOP], w.cat[PT_FUN]);
  setCur(centerX(l, 2), 294);
  printT(l);
  const PtsWeek &p = pet.ptLast;
  unsigned in = 0, gan = 0;
  for (int k = 0; k < PT_SHOP; k++) {  // lo ganado: todo lo que va antes de la tienda
    in += p.cat[k];
    gan += w.cat[k];
  }
  unsigned out = p.cat[PT_SHOP] + p.cat[PT_FUN];
  gfx->setTextColor(UI_HINT);
  snprintf(l, sizeof(l), T(S_BOOK_LAST_FMT), in, out);
  setCur(centerX(l, 2), 316);
  printT(l);
  const char *m = T(w.cat[PT_CHESS] && w.cat[PT_CHESS] * 2 >= gan ? S_BOOK_CHESS_TOP : S_BOOK_MORE_CHESS);
  gfx->setTextColor(UI_INK);
  setCur(centerX(m, 2), 346);
  printT(m);
}

void renderShop() {
  bool avisando = toastMsg && timeLeft(toastUntil);
  if (!shopDirty && !avisando) return;
  shopDirty = avisando;  // repinta una vez mas al apagarse el aviso
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  char head[32];
  if (shopPage == 0) snprintf(head, sizeof(head), T(S_SHOP_FMT), (long)pet.points);
  else if (shopPage == 4) snprintf(head, sizeof(head), T(S_BOOK_FMT), (long)pet.points);
  else snprintf(head, sizeof(head), "%s", T(shopPage == 1 ? S_BAG : shopPage == 2 ? S_REAP : S_ACCESSORIES));
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(head, 3), 52);
  printT(head);
  for (int i = 0; shopPage == 3 && i < ACC_COUNT; i++) {  // complementos
    int x = SHOP_X0 + (i % 2) * SHOP_W, y = SHOP_Y0 + (i / 2) * SHOP_H;
    bool tiene = pet.accOwned & (1 << i), puesto = pet.accWorn == i + 1;
    bool apagado = !tiene && pet.points < ACC_PRICE[i];
    gfx->fillRoundRect(x + 2, y + 2, SHOP_W - 4, SHOP_H - 4, 10, apagado ? UI_TRACK : UI_WHITE);
    gfx->drawRoundRect(x + 2, y + 2, SHOP_W - 4, SHOP_H - 4, 10, puesto ? UI_BAR_OK : UI_INK);
    drawAccessory(i, x + 30, y + 50);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(x + 58, y + 12);
    printT(T((StrId)(S_ACC_BOW + i)));
    char l2[20];
    if (tiene) snprintf(l2, sizeof(l2), "%s", T(puesto ? S_WEARING : S_OWNED));
    else snprintf(l2, sizeof(l2), "%u", ACC_PRICE[i]);
    gfx->setTextColor(tiene ? (puesto ? UI_BAR_OK : UI_INK) : UI_BAR_WARN);
    setCur(x + 58, y + 38);
    printT(l2);
  }
  int nItems = shopPage < 2 ? SHOP_ITEMS : (shopPage == 2 ? FUN_ITEMS : 0);
  for (int k = 0; k < nItems; k++) {
    int i = shopPage == 2 ? FUN_ITEM0 + k : k;  // la cosecha: los del ocio
    int x = SHOP_X0 + (k % 2) * SHOP_W, y = SHOP_Y0 + (k / 2) * SHOP_H;
    bool apagado = (shopPage == 0) ? pet.points < ITEM_PRICE[i]
                   : pet.bag[i] == 0 || (i == ITEM_MEGA_STONE && !pet.canMega());  // sin mega: no se puede
    gfx->fillRoundRect(x + 2, y + 2, SHOP_W - 4, SHOP_H - 4, 10, apagado ? UI_TRACK : UI_WHITE);
    gfx->drawRoundRect(x + 2, y + 2, SHOP_W - 4, SHOP_H - 4, 10, UI_INK);
    drawItemIcon(i, x + 3, y + 9);
    gfx->setTextColor(UI_INK);
    const char *nm = T(ITEM_NAME[i]);
    int ns = (textW(nm, 2) <= SHOP_W - 56) ? 2 : 1;  // los nombres ya caben todos a 2 (10 letras)
    setSize(ns);
    setCur(x + 53, y + 12);
    printT(nm);
    // debajo: precio (con su moneda) y, a la derecha, cuantos tienes
    char own[16];
    snprintf(own, sizeof(own), T(S_OWN_FMT), pet.bag[i]);
    setSize(2);
    if (shopPage == 0) {
      char pr[8];
      snprintf(pr, sizeof(pr), "%u", ITEM_PRICE[i]);
      gfx->fillCircle(x + 59, y + 44, 6, UI_BAR_WARN);
      gfx->drawCircle(x + 59, y + 44, 6, UI_INK);
      gfx->setTextColor(C565(0xb0, 0x60, 0x10));
      setCur(x + 69, y + 37);
      printT(pr);
    }
    gfx->setTextColor(UI_INK);
    setCur(x + SHOP_W - 8 - textW(own, 2), y + 37);
    printT(own);
  }
  if (shopPage == 4) renderBook();
  // paginas: tienda / mochila / cosecha / complementos / libro
  for (int i = 0; i < 5; i++) {
    if (i == shopPage) gfx->fillCircle(CX - 40 + i * 20, 394, 5, UI_INK);
    else gfx->drawCircle(CX - 40 + i * 20, 394, 4, UI_INK);
  }
  if (shopAsk >= 0) {  // ¿comprar?
    char l2[40];
    snprintf(l2, sizeof(l2), T(S_BUY_FOR_FMT), (unsigned)(shopAskAcc ? ACC_PRICE[shopAsk] : ITEM_PRICE[shopAsk]));
    drawAskBox(shopAskAcc ? T((StrId)(S_ACC_BOW + shopAsk)) : T(ITEM_NAME[shopAsk]), l2);
  }
  drawToast();
  gfx->flush();
}

// ---------- sin compañero: elegir huevo de la mochila ----------

#define VAC_X 103
#define VAC_W 260

// filas de la pantalla de elegir huevo: los tres de la tienda siempre, los del ocio si hay,
// el de region si hay y el boton de la tienda. Con muchas, mas juntas y empezando mas arriba
enum : int8_t { VAC_REGION = 20, VAC_SHOP = 21 };
static int vacRows(int8_t out[10]) {
  int n = 0;
  for (int k = 0; k < EGG_KINDS; k++)
    if (k < EGG_BASIC || pet.bag[eggItem(k)] > 0) out[n++] = k;
  if (pet.regionEggs) out[n++] = VAC_REGION;
  out[n++] = VAC_SHOP;
  return n;
}
static int vacStep(int n) { return n > 5 ? 46 : 54; }
static int vacY0(int n) { return n > 5 ? 116 : 150; }

void drawVacant() {
  int8_t rows[10];
  int n = vacRows(rows), step = vacStep(n), h = step - 8;
  for (int i = 0; i < n; i++) {
    int y = vacY0(n) + i * step;
    if (rows[i] == VAC_SHOP) {  // sin huevos: a la tienda
      gfx->fillRoundRect(VAC_X + 40, y + 6, VAC_W - 80, h, 12, UI_BAR_OK);
      gfx->setTextColor(UI_WHITE);
      setSize(2);
      setCur(VAC_X + 40 + (VAC_W - 80 - textW(T(S_GO_SHOP), 2)) / 2, y + 6 + h / 2 - 8);
      printT(T(S_GO_SHOP));
      continue;
    }
    char l[32];
    if (rows[i] == VAC_REGION) {  // huevos legendarios shiny de region (premio de pokedex completa)
      gfx->fillRoundRect(VAC_X, y, VAC_W, h, 12, UI_WHITE);
      gfx->drawRoundRect(VAC_X, y, VAC_W, h, 12, UI_BAR_WARN);
      drawMap(SPR_EGG, SPRITE_H, VAC_X + 10, y + h / 2 - 16, 1, false);
      drawMap(SPR_ICON_STAR, 16, VAC_X + 18, y + h / 2 - 8, 1, false);
      snprintf(l, sizeof(l), "%s  x%u", T(S_ITEM_EGG_REGION), (unsigned)__builtin_popcount(pet.regionEggs));
    } else {
      int item = eggItem(rows[i]);
      bool hay = pet.bag[item] > 0;
      gfx->fillRoundRect(VAC_X, y, VAC_W, h, 12, hay ? UI_WHITE : UI_TRACK);
      gfx->drawRoundRect(VAC_X, y, VAC_W, h, 12, inkColor());
      drawItemIcon(item, VAC_X + 2, y + h / 2 - 26);
      snprintf(l, sizeof(l), "%s  x%u", T(ITEM_NAME[item]), pet.bag[item]);
    }
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(VAC_X + 54, y + h / 2 - 8);
    printT(l);
  }
}

void vacantTap(int16_t x, int16_t y) {
  if (x < VAC_X || x > VAC_X + VAC_W) return;
  int8_t rows[10];
  int n = vacRows(rows), step = vacStep(n);
  if (y < vacY0(n)) return;
  int i = (y - vacY0(n)) / step;
  if (i >= n) return;
  if (rows[i] == VAC_SHOP) openShop(0);
  else if (rows[i] == VAC_REGION) pet.hatchRegionEgg();
  else if (!pet.hatchEgg(rows[i])) toast(T(S_NONE_LEFT));
}
