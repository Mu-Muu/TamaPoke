// Tienda, mochila y elegir huevo cuando no hay compañero.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

// ---------- tienda y mochila ----------


// moneda + puntos arriba, a la derecha de la bateria (tocarla abre la tienda)
void drawPointsBadge() {
  int x = 258, y = 14;
  gfx->fillCircle(x + 7, y + 7, 7, UI_BAR_WARN);
  gfx->drawCircle(x + 7, y + 7, 7, UI_INK);
  char n[12];
  snprintf(n, sizeof(n), "%ld", (long)pet.points);
  gfx->setTextColor(inkColor());
  setSize(2);
  setCur(x + 18, y);
  printT(n);
}

// icono de un objeto en una caja de 48x48
void drawItemIcon(int item, int x, int y) {
  switch (item) {
    case ITEM_CANDY:       drawMap(SPR_ICON_CANDY, 16, x, y, 3, false); break;
    case ITEM_MEDICINE:    drawMap(SPR_ICON_PILL, 16, x, y, 3, false); break;
    case ITEM_POTION_STR:  drawMap(SPR_ICON_POTION_STR, 16, x, y, 3, false); break;
    case ITEM_POTION_SPD:  drawMap(SPR_ICON_POTION_SPD, 16, x, y, 3, false); break;
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
    default: {  // huevos: el de siempre, con una gema del color de su rareza
      drawMap(SPR_EGG, SPRITE_H, x + 8, y + 8, 1, false);
      if (item == ITEM_EGG_RARE) gfx->fillCircle(x + 24, y + 26, 5, 0x4C98);
      if (item == ITEM_EGG_LEGEND) gfx->fillCircle(x + 24, y + 26, 5, UI_BAR_WARN);
    }
  }
}

#define SHOP_X0 56     // 2 columnas x 5 filas de casillas
#define SHOP_Y0 86
#define SHOP_W 177
#define SHOP_H 58

void openShop(int page) {
  if (pet.ceremony) return;
  shopOpen = true;
  shopPage = page;
  shopDirty = true;
}

void closeShop() { shopOpen = false; }

void shopSwipe(int dir) {
  int np = shopPage - dir;  // a la izquierda: tienda -> mochila -> complementos
  if (np < 0) { closeShop(); return; }
  if (np > 2) np = 2;
  if (np != shopPage) { shopPage = np; shopDirty = true; }
}

// usar algo de la mochila
void useItem(int item) {
  if (pet.bag[item] == 0) { toast(T(S_NONE_LEFT)); return; }
  switch (item) {
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
      if (pet.isEgg()) return;
      toast(pet.usePotion(item) ? T(S_USED) : T(S_MAXED));
      break;
    case ITEM_SHINY_CHARM:
      toast(T(S_CHARM_AUTO));
      break;
    case ITEM_POTION_EVO:  // ya puede evolucionar (si le hacia falta)
      toast(pet.useEvoPotion() ? T(S_EVO_READY) : T(S_CANT_USE));
      break;
    default:  // huevos: solo con el hueco libre
      if (!pet.vacant) { toast(T(S_EGG_NEED_EMPTY)); return; }
      if (pet.hatchEgg(item - ITEM_EGG_COMMON)) closeShop();
  }
  shopDirty = true;
}

void shopTap(int16_t x, int16_t y) {
  if (y < 72) { closeShop(); return; }  // cabecera = salir
  if (x < SHOP_X0 || y < SHOP_Y0) return;
  int c = (x - SHOP_X0) / SHOP_W, r = (y - SHOP_Y0) / SHOP_H;
  if (c > 1 || r > 4) return;
  int item = r * 2 + c;
  if (shopPage == 2) {  // complementos: comprar, o ponerse/quitarse el que ya tiene
    if (item >= ACC_COUNT) return;
    if (pet.accOwned & (1 << item)) {
      pet.wearAccessory(item);
      toast(pet.accWorn == item + 1 ? T(S_PUT_ON) : T(S_TAKE_OFF));
    } else if (pet.buyAccessory(item)) {
      toast(T(S_BOUGHT));
      sfxPlay(SFX_MEDAL);
    } else {
      toast(T(S_NO_POINTS));
    }
    shopDirty = true;
    return;
  }
  if (shopPage == 0) {
    if (pet.buy(item)) { toast(T(S_BOUGHT)); sfxPlay(SFX_EAT); }
    else toast(T(S_NO_POINTS));
  } else {
    useItem(item);
  }
  shopDirty = true;
}

void renderShop() {
  bool avisando = toastMsg && timeLeft(toastUntil);
  if (!shopDirty && !avisando) return;
  shopDirty = avisando;  // repinta una vez mas al apagarse el aviso
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  char head[32];
  if (shopPage == 0) snprintf(head, sizeof(head), T(S_SHOP_FMT), (long)pet.points);
  else snprintf(head, sizeof(head), "%s", T(shopPage == 1 ? S_BAG : S_ACCESSORIES));
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(head, 3), 52);
  printT(head);
  for (int i = 0; shopPage == 2 && i < ACC_COUNT; i++) {  // complementos
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
  for (int i = 0; shopPage < 2 && i < ITEM_COUNT; i++) {
    int x = SHOP_X0 + (i % 2) * SHOP_W, y = SHOP_Y0 + (i / 2) * SHOP_H;
    bool apagado = (shopPage == 0) ? pet.points < ITEM_PRICE[i] : pet.bag[i] == 0;
    gfx->fillRoundRect(x + 2, y + 2, SHOP_W - 4, SHOP_H - 4, 10, apagado ? UI_TRACK : UI_WHITE);
    gfx->drawRoundRect(x + 2, y + 2, SHOP_W - 4, SHOP_H - 4, 10, UI_INK);
    drawItemIcon(i, x + 6, y + 9);
    gfx->setTextColor(UI_INK);
    setSize(2);
    const char *nm = T(ITEM_NAME[i]);
    int ns = (textW(nm, 2) <= SHOP_W - 64) ? 2 : 1;
    setSize(ns);
    setCur(x + 58, y + 12);
    printT(nm);
    char l2[20];
    if (shopPage == 0) snprintf(l2, sizeof(l2), "%u  x%u", ITEM_PRICE[i], pet.bag[i]);
    else snprintf(l2, sizeof(l2), "x%u", pet.bag[i]);
    setSize(2);
    gfx->setTextColor(shopPage == 0 ? UI_BAR_WARN : UI_INK);
    setCur(x + 58, y + 38);
    printT(l2);
  }
  // paginas: tienda / mochila / complementos
  for (int i = 0; i < 3; i++) {
    if (i == shopPage) gfx->fillCircle(CX - 20 + i * 20, 394, 5, UI_INK);
    else gfx->drawCircle(CX - 20 + i * 20, 394, 4, UI_INK);
  }
  drawToast();
  gfx->flush();
}

// ---------- sin compañero: elegir huevo de la mochila ----------

#define VAC_X 103
#define VAC_Y 150
#define VAC_W 260
#define VAC_H 46

void drawVacant() {
  for (int k = 0; k < EGG_KINDS; k++) {
    int y = VAC_Y + k * (VAC_H + 8);
    int item = ITEM_EGG_COMMON + k;
    bool hay = pet.bag[item] > 0;
    gfx->fillRoundRect(VAC_X, y, VAC_W, VAC_H, 12, hay ? UI_WHITE : UI_TRACK);
    gfx->drawRoundRect(VAC_X, y, VAC_W, VAC_H, 12, inkColor());
    drawMap(SPR_EGG, SPRITE_H, VAC_X + 10, y + 7, 1, false);
    if (item == ITEM_EGG_RARE) gfx->fillCircle(VAC_X + 26, y + 25, 4, 0x4C98);
    if (item == ITEM_EGG_LEGEND) gfx->fillCircle(VAC_X + 26, y + 25, 4, UI_BAR_WARN);
    char l[28];
    snprintf(l, sizeof(l), "%s  x%u", T(ITEM_NAME[item]), pet.bag[item]);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(VAC_X + 54, y + 15);
    printT(l);
  }
  // huevos legendarios shiny de region (premio de pokedex completa)
  int rows = EGG_KINDS;
  if (pet.regionEggs) {
    int y = VAC_Y + rows * (VAC_H + 8);
    gfx->fillRoundRect(VAC_X, y, VAC_W, VAC_H, 12, UI_WHITE);
    gfx->drawRoundRect(VAC_X, y, VAC_W, VAC_H, 12, UI_BAR_WARN);
    drawMap(SPR_EGG, SPRITE_H, VAC_X + 10, y + 7, 1, false);
    drawMap(SPR_ICON_STAR, 16, VAC_X + 18, y + 15, 1, false);
    char l[32];
    snprintf(l, sizeof(l), "%s  x%u", T(S_ITEM_EGG_REGION), (unsigned)__builtin_popcount(pet.regionEggs));
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(VAC_X + 54, y + 15);
    printT(l);
    rows++;
  }
  // sin huevos: a la tienda
  int y = VAC_Y + rows * (VAC_H + 8) + 6;
  gfx->fillRoundRect(VAC_X + 40, y, VAC_W - 80, VAC_H, 12, UI_BAR_OK);
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(VAC_X + 40 + (VAC_W - 80 - textW(T(S_GO_SHOP), 2)) / 2, y + 15);
  printT(T(S_GO_SHOP));
}

void vacantTap(int16_t x, int16_t y) {
  if (x < VAC_X || x > VAC_X + VAC_W || y < VAC_Y) return;
  int k = (y - VAC_Y) / (VAC_H + 8);
  if (k < EGG_KINDS) {
    if (!pet.hatchEgg(k)) toast(T(S_NONE_LEFT));
    return;
  }
  if (pet.regionEggs) {
    if (k == EGG_KINDS) {
      pet.hatchRegionEgg();
      return;
    }
    k--;  // la fila del huevo de region empuja la tienda una abajo
  }
  if (k == EGG_KINDS) openShop(0);
}
