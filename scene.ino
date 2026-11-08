// Escena de fondo (bioma, cielo, tiempo, estaciones), reloj de espera,
// bocadillos, visitas de la granja, complementos y fiestas.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

// ---------- render ----------

uint16_t inkColor() { return gNight ? UI_INK_NIGHT : UI_INK; }

// ---------- escena de fondo: bioma del tipo + hora real del RTC ----------


uint16_t lerp565(uint16_t a, uint16_t b, int i, int n) {
  if (n <= 0) return a;
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)((((ar + (br - ar) * i / n) << 11)) |
                    (((ag + (bg - ag) * i / n) << 5)) | (ab + (bb - ab) * i / n));
}

// hora del dia 0-23 (de la hora real cacheada cada 30s; 13 si no hay reloj)
int sceneHour() {
  uint32_t e = pet.lastSeenEpoch;
  return e ? (int)((e / 3600) % 24) : 13;
}

// suelo de cada bioma de dia (de noche se mezcla hacia el azul nocturno)
static const uint16_t BIOME_SOIL[6] = {
  C565(0x7e, 0xc0, 0x7f),  // 0 pradera
  C565(0xdc, 0xca, 0x94),  // 1 playa (arena)
  C565(0x4f, 0x8a, 0x55),  // 2 bosque
  C565(0x8a, 0x55, 0x44),  // 3 volcan
  C565(0xa8, 0x90, 0x6a),  // 4 montana
  C565(0xe6, 0xee, 0xf5),  // 5 nieve
};

void drawClouds(uint32_t now, uint16_t col) {
  for (int k = 0; k < 2; k++) {
    int cx = (int)((now / 50 + k * 250) % 560) - 40;
    int cy = 70 + k * 34;
    gfx->fillCircle(cx, cy, 16, col);
    gfx->fillCircle(cx + 18, cy + 3, 13, col);
    gfx->fillCircle(cx - 15, cy + 4, 12, col);
  }
}

// noche sobre el paisaje de la SD: estrellas que titilan y luna, solo donde el
// fondo es cielo (azul dominante): sobre montes y arboles no se pintan
static bool bgIsSky(int x, int y) {
  if (!bg.px || x < 0 || y < 0 || x >= bg.w || y >= bg.h) return false;
  uint16_t v = bg.px[y * bg.w + x];
  int r = (v >> 11) & 31, g = ((v >> 5) & 63) / 2, b = v & 31;
  return b > g + 2 && b > r + 4;
}
static const int16_t SKY_STARS[][2] = { { 70, 52 },  { 112, 30 }, { 150, 74 }, { 196, 40 }, { 238, 22 },
                                        { 282, 58 }, { 324, 30 }, { 392, 96 }, { 52, 104 }, { 128, 118 },
                                        { 214, 92 }, { 300, 110 }, { 420, 140 }, { 30, 160 } };
void drawNightSky(uint32_t now) {
  int i = 0;
  for (auto &st : SKY_STARS) {
    if (bgIsSky(st[0], st[1])) {
      int k = (int)((now / 450 + i * 7) % 6);  // cada una titila a su ritmo
      int sz = k == 0 ? 2 : 3;
      gfx->fillRect(st[0], st[1], sz, sz, k == 0 ? C565(0x9a, 0xa8, 0xc8) : UI_WHITE);
    }
    i++;
  }
  if (gHoliday != HOL_MIDAUTUMN && bgIsSky(372, 70)) {  // luna creciente (en Medio Otoño, la llena)
    gfx->fillCircle(372, 70, 20, C565(0xf0, 0xf2, 0xd8));
    gfx->fillCircle(381, 64, 18, bg.px[64 * bg.w + 381]);
  }
}

// tiempo real sobre el paisaje (detras del bicho): nubes, niebla, lluvia,
// tormenta, nieve. Solo en la parte de arriba (el panel empieza en y=312).
#define WX_BOTTOM 312
void drawWeather(uint32_t now) {
  if (!gWxView.valid) return;
  uint8_t k = gWxView.kind;
  if (k == WX_CLOUDY || k == WX_RAIN || k == WX_STORM || k == WX_SNOW) {
    uint16_t c = k == WX_STORM ? C565(0x52, 0x55, 0x60) : gNight ? C565(0x5a, 0x60, 0x70) : C565(0xd6, 0xda, 0xe0);
    drawClouds(now, c);
    drawClouds(now / 2 + 9000, lerp565(c, gNight ? UI_BG_NIGHT : UI_WHITE, 1, 3));
  }
  if (k == WX_FOG) {  // jirones de niebla punteados que se mueven despacio
    uint16_t c = gNight ? C565(0x6a, 0x70, 0x80) : C565(0xe4, 0xe8, 0xec);
    for (int y = 150, b = 0; y < WX_BOTTOM - 6; y += 13, b++) {
      int off = (int)((now / 80 + b * 53) % 120);
      for (int x0 = -120 + off; x0 < 466; x0 += 120)
        for (int x = x0 + (b & 1) * 2; x < x0 + 70; x += 4)
          if (x >= 0 && x < 466) gfx->fillRect(x, y + ((x / 4) & 1), 2, 2, c);
    }
  }
  if (k == WX_RAIN || k == WX_STORM) {  // gotas inclinadas
    uint16_t c = gNight ? C565(0x70, 0x88, 0xb0) : C565(0x8c, 0xb4, 0xe0);
    for (int i = 0; i < 46; i++) {
      int y = (int)((i * 97 + now * 3 / 4) % WX_BOTTOM);
      int x = (int)((i * 61 + 466 - y / 4) % 466);
      gfx->drawLine(x, y, x - 3, y + 11, c);
    }
  }
  if (k == WX_STORM && (now / 150) % 46 == 0) {  // rayo de vez en cuando
    int x = 120 + (int)((now / 6900) % 5) * 50, y = 40;
    for (int j = 0; j < 5; j++) {
      int nx = x + ((j & 1) ? -16 : 18), ny = y + 34;
      gfx->drawLine(x, y, nx, ny, C565(0xff, 0xf0, 0x80));
      gfx->drawLine(x + 1, y, nx + 1, ny, C565(0xff, 0xf0, 0x80));
      x = nx;
      y = ny;
    }
  }
  if (k == WX_SNOW) {  // copos que bajan meciendose
    for (int i = 0; i < 40; i++) {
      int y = (int)((i * 89 + now * (20 + i % 3 * 8) / 1000) % WX_BOTTOM);
      int x = (int)((i * 67 + (int)(12 * sinf(now / 700.0f + i))) % 466);
      if (x < 0) x += 466;
      gfx->fillCircle(x, y, i % 3 == 0 ? 3 : 2, UI_WHITE);
    }
  }
}

// el tiempo que siente el compañero: lluvia/tormenta, nieve o sol de dia (PW_*)
uint8_t petWeather() {
  Weather w = weatherNow();
  if (gWxForce >= 0) {
    w.valid = true;
    w.kind = (uint8_t)gWxForce;
  }
  if (!w.valid) return PW_NONE;
  if (w.kind == WX_RAIN || w.kind == WX_STORM) return PW_RAIN;
  if (w.kind == WX_SNOW) return PW_SNOW;
  int mod = pet.lastSeenEpoch ? (int)((pet.lastSeenEpoch / 60) % 1440) : 13 * 60;
  if (w.kind == WX_CLEAR && dayPhase(mod, w.sunriseMin, w.sunsetMin) == PH_DAY) return PW_SUN;
  return PW_NONE;
}

// ---------- reloj de espera (enchufado y sin tocar) ----------
// Fondo negro (el AMOLED no gasta en negro): arriba la fecha, en medio la hora
// en cifras grandes de bloques, debajo el icono del tiempo con la temperatura y
// abajo el compañero (al mismo tamaño de siempre). Todo se mueve unos pixeles
// cada minuto para no marcar la pantalla.

// cifras 5x7 (cada fila: 5 bits, el de mas peso a la izquierda)
static const uint8_t BIG_DIG[10][7] = {
  { 0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e },  // 0
  { 0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e },  // 1
  { 0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f },  // 2
  { 0x1f, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0e },  // 3
  { 0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02 },  // 4
  { 0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e },  // 5
  { 0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e },  // 6
  { 0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 },  // 7
  { 0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e },  // 8
  { 0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c },  // 9
};

// texto de cifras con bloques de b px; x = borde izquierdo, y = borde de arriba.
// Cifra = 5 bloques y 1 de separacion; ':' = 1 + 1; '-' = 3 + 1; 'o' = el grado
// (cuadradito hueco de 3) + 1. Devuelve el ancho dibujado.
static int drawBigText(const char *t, int x, int y, int b, uint16_t col, bool draw = true) {
  int x0 = x;
  for (const char *c = t; *c; c++) {
    if (*c == ':') {
      if (draw) {
        gfx->fillRect(x, y + 2 * b, b, b, col);
        gfx->fillRect(x, y + 4 * b, b, b, col);
      }
      x += 2 * b;
    } else if (*c == '-') {
      if (draw) gfx->fillRect(x, y + 3 * b, 3 * b, b, col);
      x += 4 * b;
    } else if (*c == 'o') {
      if (draw) {
        gfx->fillRect(x, y, 3 * b, b, col);
        gfx->fillRect(x, y + 2 * b, 3 * b, b, col);
        gfx->fillRect(x, y + b, b, b, col);
        gfx->fillRect(x + 2 * b, y + b, b, b, col);
      }
      x += 4 * b;
    } else if (*c >= '0' && *c <= '9') {
      const uint8_t *d = BIG_DIG[*c - '0'];
      for (int r = 0; draw && r < 7; r++)
        for (int k = 0; k < 5; k++) {
          if (!(d[r] & (0x10 >> k))) continue;
          int n = 1;  // tramo seguido de la fila: un solo rectangulo
          while (k + n < 5 && (d[r] & (0x10 >> (k + n)))) n++;
          gfx->fillRect(x + k * b, y + r * b, n * b, b, col);
          k += n - 1;
        }
      x += 6 * b;
    }
  }
  return x - x0 - b;  // sin el aire de detras del ultimo
}
static int bigTextW(const char *t, int b) { return drawBigText(t, 0, 0, b, 0, false); }

// icono del tiempo (unos 66x55) con el centro en (cx, cy)
static void drawWxIcon(uint8_t kind, bool night, int cx, int cy) {
  if (kind == WX_UNKNOWN) return;
  auto S = [](int v) { return v * 3 / 2; };
  const uint16_t nube = C565(0xc8, 0xd0, 0xdc), sol = C565(0xff, 0xc8, 0x40);
  if (kind == WX_CLEAR) {
    if (night) {  // luna creciente
      gfx->fillCircle(cx, cy, S(15), C565(0xf0, 0xf2, 0xd8));
      gfx->fillCircle(cx + S(9), cy - S(6), S(13), RGB565_BLACK);
      return;
    }
    gfx->fillCircle(cx, cy, S(10), sol);
    for (int i = 0; i < 8; i++) {
      float a = i * 0.785398f;
      int x1 = cx + (int)(S(14) * cosf(a)), y1 = cy + (int)(S(14) * sinf(a));
      int x2 = cx + (int)(S(19) * cosf(a)), y2 = cy + (int)(S(19) * sinf(a));
      gfx->drawLine(x1, y1, x2, y2, sol);
      gfx->drawLine(x1 + 1, y1, x2 + 1, y2, sol);
      gfx->drawLine(x1, y1 + 1, x2, y2 + 1, sol);
    }
    return;
  }
  if (kind == WX_FOG) {
    for (int i = 0; i < 3; i++)
      gfx->fillRoundRect(cx - S(20) + (i & 1) * S(6), cy - S(12) + i * S(10), S(34), S(5), 3, nube);
    return;
  }
  // nube (y debajo, lo que cae); con lluvia, nieve o tormenta va un poco mas arriba
  int ny = (kind == WX_CLOUDY) ? cy : cy - S(7);
  gfx->fillCircle(cx - S(9), ny + S(2), S(9), nube);
  gfx->fillCircle(cx + S(3), ny - S(4), S(12), nube);
  gfx->fillCircle(cx + S(13), ny + S(3), S(8), nube);
  gfx->fillRect(cx - S(9), ny + S(3), S(22), S(9), nube);
  if (kind == WX_RAIN) {
    const uint16_t gota = C565(0x60, 0xa8, 0xff);
    for (int i = 0; i < 3; i++) {
      int x = cx - S(10) + i * S(10);
      for (int j = 0; j < 3; j++) gfx->drawLine(x + j, ny + S(16), x + j - S(3), ny + S(25), gota);
    }
  } else if (kind == WX_SNOW) {
    for (int i = 0; i < 3; i++) gfx->fillCircle(cx - S(10) + i * S(10), ny + S(20) + (i & 1) * S(4), 3, UI_WHITE);
  } else if (kind == WX_STORM) {
    gfx->fillTriangle(cx + S(2), ny + S(13), cx - S(6), ny + S(24), cx + S(1), ny + S(23), sol);
    gfx->fillTriangle(cx + S(1), ny + S(21), cx + S(6), ny + S(21), cx - S(3), ny + S(31), sol);
  }
}

void renderStandby() {
  uint32_t now = millis();
  uint32_t e = shownClock(pet.lastSeenEpoch ? pet.lastSeenEpoch : rtcEpoch());
  uint32_t min = e / 60;
  int dx = (int)((min * 7) % 25) - 12, dy = (int)((min * 13) % 21) - 10;
  gfx->fillScreen(RGB565_BLACK);
  const uint16_t gris = C565(0xa8, 0xb4, 0xc8);
  // fecha y dia de la semana
  time_t tt = e;
  struct tm tm;
  gmtime_r(&tt, &tm);
  char fecha[24], linea[48];
  snprintf(fecha, sizeof(fecha), T(S_DATE_FMT), (unsigned)(tm.tm_mon + 1), (unsigned)tm.tm_mday);
  snprintf(linea, sizeof(linea), "%s  %s", fecha, T((StrId)(S_WD_0 + tm.tm_wday)));
  gfx->setTextColor(gris);
  uint8_t ds = textW(linea, 5) <= 250 ? 5 : 4;  // en ingles a 5 no cabe en lo alto del circulo
  setSize(ds);
  setCur(centerX(linea, ds) + dx, 46 + dy);
  printT(linea);
  // la hora
  char t[8];
  snprintf(t, sizeof(t), "%02lu:%02lu", (unsigned long)((e / 3600) % 24), (unsigned long)((e / 60) % 60));
  drawBigText(t, CX - bigTextW(t, 13) / 2 + dx, 100 + dy, 13, C565(0xdc, 0xe4, 0xf0));
  // el tiempo: icono y temperatura (con las mismas cifras), centrados juntos
  Weather w = weatherNow();
  if (gWxForce >= 0) {  // consola WXSET
    w.valid = true;
    w.kind = (uint8_t)gWxForce;
  }
  if (w.valid) {
    char tc[8];
    snprintf(tc, sizeof(tc), "%do", w.tempC);
    const int b = 6;
    int tw = bigTextW(tc, b);
    int ancho = 66 + 20 + tw;
    int x = CX - ancho / 2 + dx, y = 246 + dy;  // y = centro de la fila
    uint8_t fase = gPhaseForce >= 0 ? (uint8_t)gPhaseForce  // consola PHASE
                                    : dayPhase((int)((e / 60) % 1440), w.sunriseMin, w.sunsetMin);
    bool noche = fase == PH_NIGHT;
    drawWxIcon(w.kind, noche, x + 33, y);
    drawBigText(tc, x + 66 + 20, y - 7 * b / 2, b, gris);
  }
  if (pet.isEgg()) {
    if (!pet.vacant) drawMap(SPR_EGG, SPRITE_H, CX - 32 + dx, 290 + dy, 2, false);
  } else if (pmd.loaded) {
    drawPmdAct(pet.sleeping && pmd.has(PMD_SLEEP) ? PMD_SLEEP : PMD_IDLE, CX + dx, 380 + dy, now, true, false, 3);
  }
  gfx->flush();
}

// ---------- bocadillos, visitas de la granja y complementos ----------


// la pantalla principal esta libre (sin otras pantallas ni dialogos encima)
bool mainScreenFree() {
  return !galleryOpen && !farmOpen && !shopOpen && !cardOpen && !kbOpen && !clockOpen && !wifiPortalActive() &&
         !feedMenuUntil && !actionUntil && !confirmUntil && !choiceKind && !reportPending && !pet.awaitingStarter();
}

// que decir: primero lo que necesita; si no, el ajedrez de anoche, la hora o su caracter
const char *pickBubble() {
  if (pet.fullness < 40) return T(S_SAY_HUNGRY);
  if (pet.hygiene < 40 || pet.poops) return T(S_SAY_DIRTY);
  if (pet.energy < 30) return T(S_SAY_TIRED);
  if (pet.joy < 40) return T(S_SAY_BORED);
  int r = random(100), h = sceneHour();
  ChessReport rep;
  uint32_t day;
  if (r < 30 && chessLoadReport(rep, day) && day == pet.lastSeenEpoch / 86400 && rep.games) {
    if (!rep.wins) return T(S_SAY_CHESS_PLAYED);
    snprintf(bubbleBuf, sizeof(bubbleBuf), T(S_SAY_CHESS_WIN_FMT), rep.wins);
    return bubbleBuf;
  }
  if (r < 50 && h >= 6 && h < 10) return T(S_SAY_MORNING);
  if (r < 50 && h >= 21) return T(S_SAY_NIGHT);
  if (r < 75) return T(pet.nature == NAT_EASY || pet.nature >= NAT_COUNT ? S_SAY_EASY
                                                                          : (StrId)(S_SAY_GLUTTON + pet.nature - 1));
  if (r >= 88) return T(S_SAY_HI);
  snprintf(bubbleBuf, sizeof(bubbleBuf), T(S_SAY_LOVE), OWNER_NAME);  // con el nombre del dueño (config.h)
  return bubbleBuf;
}

// cada 1-2 min, si esta despierto y la pantalla libre: un bocadillo de 4,5 s
void bubbleTick(uint32_t now) {
  if (bubbleUntil && !timeLeft(bubbleUntil)) bubbleUntil = 0;
  if (bubbleUntil || (int32_t)(now - nextBubble) < 0) return;
  nextBubble = now + 60000 + random(60000);
  if (!mainScreenFree() || pet.isEgg() || pet.vacant || pet.sleeping || pet.ceremony || dimStage || vis.on) return;
  bubbleText = pickBubble();
  bubbleUntil = now + 4500;
}

void drawBubble(int cx, int topY) {
  if (!bubbleUntil || !bubbleText || pet.sleeping) return;
  setSize(2);
  int w = textW(bubbleText, 2) + 24, h = 34;
  int x = cx - w / 2;
  if (x < 40) x = 40;
  if (x + w > 426) x = 426 - w;
  int y = topY - 64;
  if (y < 112) y = 112;  // bajo la cabecera
  gfx->fillRoundRect(x, y, w, h, 12, UI_WHITE);
  gfx->drawRoundRect(x, y, w, h, 12, UI_INK);
  int tx = cx < x + 16 ? x + 16 : cx > x + w - 16 ? x + w - 16 : cx;
  gfx->fillTriangle(tx - 7, y + h - 2, tx + 7, y + h - 2, tx, y + h + 9, UI_WHITE);
  gfx->drawLine(tx - 7, y + h - 1, tx, y + h + 9, UI_INK);
  gfx->drawLine(tx + 7, y + h - 1, tx, y + h + 9, UI_INK);
  gfx->setTextColor(UI_INK);
  setCur(x + 12, y + 9);
  printT(bubbleText);
}

// shiny: cada 4 s, un destello de estrellitas alrededor durante 0,9 s
void drawSparkles(int cx, int topY, uint32_t now) {
  uint32_t t = now % 4000;
  if (t > 900) return;
  uint32_t ciclo = now / 4000;
  int alto = PET_GROUND - topY;
  for (int i = 0; i < 4; i++) {
    uint32_t h = (ciclo * 2654435761u) ^ (i * 40503u);
    int x = cx - 60 + (int)(h % 121), y = topY + (int)((h >> 8) % (alto > 20 ? alto : 20));
    int r = (int)(t < 450 ? t / 60 : (900 - t) / 60);  // crece y se apaga
    uint16_t c = i & 1 ? UI_WHITE : C565(0xff, 0xe8, 0x70);
    gfx->drawFastHLine(x - r, y, 2 * r + 1, c);
    gfx->drawFastVLine(x, y - r, 2 * r + 1, c);
    if (r > 3) gfx->fillCircle(x, y, 1, c);
  }
}

// subir de nivel: un anillo dorado que se abre y "Lv.9!" que sube
void drawLevelUp(int cx, int topY, uint32_t now) {
  float t = 1.0f - timeLeft(levelUpUntil) / 3000.0f;
  int cy = (topY + PET_GROUND) / 2, r = 30 + (int)(t * 80);
  gfx->drawCircle(cx, cy, r, C_GOLD);
  gfx->drawCircle(cx, cy, r + 1, C_GOLD);
  gfx->drawCircle(cx, cy, r + 6, C565(0xff, 0xf2, 0xb0));
  char lv[12];
  snprintf(lv, sizeof(lv), "Lv.%u!", levelUpShow);
  setSize(3);
  int y = topY - 44 - (int)(t * 18);
  if (y < 112) y = 112;
  gfx->setTextColor(C_GOLD);
  setCur(cx - textW(lv, 3) / 2, y);
  printT(lv);
}

void endVisit() {
  vis.on = false;
  visitorPmd.unload();
}

// cada 10-25 min (la primera a los 5-12) alguien de la granja cruza el paisaje,
// se para al lado del compañero, saluda y se vuelve por donde vino
void visitTick(uint32_t now) {
  if (!vis.on) {
    if (!nextVisit) nextVisit = now + 300000 + random(420000);
    if ((int32_t)(now - nextVisit) < 0) return;
    nextVisit = now + 600000 + random(900000);
    if (!mainScreenFree() || pet.isEgg() || pet.vacant || pet.sleeping || pet.ceremony || dimStage || !farmReady() ||
        farmCount() == 0)
      return;
    const FarmEntry &e = farmAt(random(farmCount()));
    if (!visitorPmd.load(e.speciesId, e.shiny)) return;
    vis.on = true;
    vis.dir = random(2) ? 1 : -1;
    vis.x = vis.dir > 0 ? -50 : 516;
    vis.stopX = beh.x - vis.dir * 120;
    if (vis.stopX < 70) vis.stopX = 70;
    if (vis.stopX > 396) vis.stopX = 396;
    vis.phase = 0;
    vis.t0 = vis.last = now;
    snprintf(vis.name, sizeof(vis.name), "%s", e.nick[0] ? e.nick : dexName(e.speciesId));
    Serial.printf("visita: #%d llega\n", e.speciesId);
    return;
  }
  if (!mainScreenFree() || pet.sleeping || pet.ceremony || pet.isEgg()) {
    Serial.println("visita: cortada");
    endVisit();
    return;
  }
  float dt = (now - vis.last) / 1000.0f;
  if (dt > 0.3f) dt = 0.3f;  // un hueco largo (otra pantalla) no es un salto
  vis.last = now;
  if (vis.phase == 0) {
    float d = vis.stopX - vis.x, step = 35.0f * dt;
    if (fabsf(d) <= step) {
      vis.x = vis.stopX;
      vis.phase = 1;
      vis.until = now + 5000;  // se queda a saludar 5 s
      vis.t0 = now;
    } else {
      vis.x += d > 0 ? step : -step;
    }
  } else if (vis.phase == 1) {
    if ((int32_t)(now - vis.until) >= 0) {
      vis.phase = 2;
      vis.dir = -vis.dir;  // se vuelve por donde vino
      vis.t0 = now;
    }
  } else {
    vis.x += vis.dir * 35.0f * dt;
    if (vis.x < -60 || vis.x > 526) {
      Serial.println("visita: se fue");
      endVisit();
    }
  }
}

void drawVisitor(uint32_t now) {
  if (!vis.on || !visitorPmd.loaded) return;
  uint8_t act = vis.phase == 1 ? PMD_IDLE : (vis.dir > 0 ? PMD_WALKR : PMD_WALKL);
  if (!visitorPmd.has(act)) act = PMD_IDLE;
  drawPmdActM(visitorPmd, act, (int)vis.x, PET_GROUND - 6, now - vis.t0, true, false, 3);
  if (vis.phase == 1) drawMap(SPR_HEART, 32, gPmdTopX - 16, gPmdTopY - 36, 1, false);  // saluda
}

// complemento sobre la cabeza: (x, y) = centro de la cima del sprite
void drawAccessory(uint8_t a, int x, int y) {
  switch (a) {
    case ACC_BOW: {
      uint16_t c = C565(0xf0, 0x5a, 0x8c);
      gfx->fillTriangle(x - 2, y - 6, x - 20, y - 16, x - 20, y + 4, c);
      gfx->fillTriangle(x + 2, y - 6, x + 20, y - 16, x + 20, y + 4, c);
      gfx->fillCircle(x, y - 6, 5, C565(0xc8, 0x30, 0x60));
      break;
    }
    case ACC_FLOWER:
      for (int k = 0; k < 5; k++) {
        float an = k * (2 * PI / 5) - PI / 2;
        gfx->fillCircle(x + (int)(8 * cosf(an)), y - 10 + (int)(8 * sinf(an)), 6, C565(0xff, 0xf0, 0xf4));
      }
      gfx->fillCircle(x, y - 10, 5, C565(0xf8, 0xc8, 0x30));
      break;
    case ACC_CAP:
      gfx->fillRoundRect(x - 20, y - 18, 40, 22, 11, C565(0x34, 0x78, 0xd8));
      gfx->fillRect(x - 20, y - 4, 40, 6, C565(0x34, 0x78, 0xd8));
      gfx->fillRoundRect(x + 10, y - 2, 24, 7, 3, C565(0x24, 0x58, 0xa8));
      gfx->fillCircle(x, y - 18, 3, UI_WHITE);
      break;
    case ACC_WITCH: {
      uint16_t c = C565(0x6a, 0x3c, 0xa8);
      gfx->fillTriangle(x - 16, y - 4, x + 16, y - 4, x + 10, y - 44, c);
      gfx->fillTriangle(x + 4, y - 40, x + 16, y - 44, x + 22, y - 34, c);  // punta doblada
      gfx->fillRect(x - 15, y - 12, 30, 5, C565(0xf0, 0xc8, 0x30));
      gfx->fillRoundRect(x - 26, y - 6, 52, 8, 4, c);
      break;
    }
    case ACC_TOPHAT: {
      uint16_t k = C565(0x22, 0x22, 0x2a);
      gfx->fillRect(x - 14, y - 36, 28, 32, k);
      gfx->fillRect(x - 14, y - 14, 28, 5, C565(0xd0, 0x30, 0x40));
      gfx->fillRoundRect(x - 24, y - 6, 48, 7, 3, k);
      break;
    }
    case ACC_CROWN:
      gfx->fillRect(x - 18, y - 10, 37, 10, C_GOLD);
      gfx->fillTriangle(x - 18, y - 9, x - 18, y - 28, x - 6, y - 9, C_GOLD);
      gfx->fillTriangle(x - 8, y - 9, x, y - 32, x + 8, y - 9, C_GOLD);
      gfx->fillTriangle(x + 6, y - 9, x + 18, y - 28, x + 18, y - 9, C_GOLD);
      gfx->fillCircle(x, y - 5, 3, C_RED);
      gfx->fillCircle(x - 11, y - 5, 2, C565(0x40, 0xa0, 0xf0));
      gfx->fillCircle(x + 11, y - 5, 2, C565(0x40, 0xc0, 0x70));
      break;
  }
}

// ---------- fiestas: decorado del fondo (detras del bicho) y gorros ----------

// estacion de hoy (por la fecha local)
uint8_t todaySeason() {
  if (gSeasonForce >= 0) return (uint8_t)gSeasonForce;
  uint32_t e = pet.lastSeenEpoch;
  if (!e) return SEASON_SUMMER;
  time_t t = e;
  struct tm tm;
  gmtime_r(&t, &tm);
  return seasonOf(tm.tm_mon + 1);
}

static void petal(int x, int y, uint32_t now, int i) {  // petalo de cerezo
  int w = 3 + (int)(2 * fabsf(sinf(now / 300.0f + i)));  // gira: se estrecha y ensancha
  gfx->fillEllipse(x, y, w, 3, C565(0xf8, 0xc0, 0xd8));
  gfx->drawPixel(x, y, C565(0xe8, 0x80, 0xa8));
}

static void butterfly(int x, int y, uint32_t now, int i) {
  uint16_t c = i & 1 ? C565(0xf8, 0xe0, 0x50) : C565(0xa8, 0xd8, 0xf8);
  int a = ((now / 120) + i) % 2 ? 7 : 3;  // aletea
  gfx->fillEllipse(x - a, y - 2, a, 5, c);
  gfx->fillEllipse(x + a, y - 2, a, 5, c);
  gfx->drawFastVLine(x, y - 5, 9, UI_INK);
}

// estacion sobre el paisaje (detras del bicho). Con lluvia o nieve manda el
// tiempo real; la nieve del suelo en invierno se queda siempre.
void drawSeason(uint32_t now) {
  uint8_t s = todaySeason();
  bool precip = gWxView.valid && (gWxView.kind == WX_RAIN || gWxView.kind == WX_STORM || gWxView.kind == WX_SNOW);
  if (s == SEASON_WINTER) {  // nieve en el suelo: un manto con bultos bajo los pies
    for (int x = -10; x < 476; x += 26) {
      int bump = 4 + (x * 7) % 5;
      gfx->fillEllipse(x, WX_BOTTOM - 6, 22, bump + 4, UI_WHITE);
    }
    gfx->fillRect(0, WX_BOTTOM - 6, 466, 6, UI_WHITE);
    if (!precip)  // algun copo suelto
      for (int i = 0; i < 8; i++) {
        int y = (int)((i * 131 + now * (14 + i % 3 * 5) / 1000) % WX_BOTTOM);
        int x = (int)((i * 97 + (int)(10 * sinf(now / 900.0f + i))) % 466);
        gfx->fillCircle(x < 0 ? x + 466 : x, y, 2, UI_WHITE);
      }
    return;
  }
  if (precip || gHoliday == HOL_THANKSGIVING) return;
  if (s == SEASON_SPRING) {  // petalos de cerezo que caen meciendose
    for (int i = 0; i < 14; i++) {
      int y = (int)((i * 113 + now * (22 + i % 4 * 6) / 1000) % WX_BOTTOM);
      int x = (int)((i * 83 + (int)(18 * sinf(now / 800.0f + i * 1.7f))) % 466);
      petal(x < 0 ? x + 466 : x, y, now, i);
    }
  } else if (s == SEASON_SUMMER) {
    if (gNight) {  // luciernagas que se encienden y apagan
      for (int i = 0; i < 10; i++) {
        int k = (int)((now / 150 + i * 7) % 20);
        if (k > 12) continue;
        int x = 60 + (i * 53) % 350 + (int)(12 * sinf(now / 1300.0f + i));
        int y = 140 + (i * 37) % 150 + (int)(8 * cosf(now / 1100.0f + i));
        gfx->fillCircle(x, y, 4, C565(0x50, 0x70, 0x20));
        gfx->fillCircle(x, y, 2, C565(0xe0, 0xff, 0x70));
      }
    } else {  // mariposas
      for (int i = 0; i < 2; i++) {
        int x = (int)((now / 30 + i * 230) % 520) - 30;
        int y = 150 + i * 50 + (int)(20 * sinf(now / 500.0f + i));
        butterfly(x, y, now, i);
      }
    }
  } else {  // otoño: hojas cayendo
    static const uint16_t c[3] = { C565(0xe0, 0x70, 0x20), C565(0xc8, 0x40, 0x20), C565(0xe8, 0xb0, 0x30) };
    falling(now, 9, c, 3, true);
  }
}

uint8_t todayHoliday() {
  if (gHolForce >= 0) return (uint8_t)gHolForce;
  static uint32_t day = 0xFFFFFFFF;
  static uint8_t cached = HOL_NONE;
  uint32_t e = pet.lastSeenEpoch;  // hora local "de pared"
  if (!e) return HOL_NONE;
  if (e / 86400 != day) {
    day = e / 86400;
    time_t t = e;
    struct tm tm;
    gmtime_r(&t, &tm);
    cached = holidayOn(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, gBdayM, gBdayD);
  }
  return cached;
}


void drawCrown(int x, int y) {  // corona dorada (pokedex de region completa)
  gfx->fillRect(x - 10, y + 8, 21, 6, C_GOLD);
  gfx->fillTriangle(x - 10, y + 9, x - 10, y - 2, x - 3, y + 9, C_GOLD);
  gfx->fillTriangle(x - 5, y + 9, x, y - 4, x + 5, y + 9, C_GOLD);
  gfx->fillTriangle(x + 3, y + 9, x + 10, y - 2, x + 10, y + 9, C_GOLD);
  gfx->fillCircle(x, y + 10, 2, C_RED);
}

void drawSantaHat(int x, int y) {
  uint16_t white = C565(0xf4, 0xf4, 0xf4);
  gfx->fillTriangle(x - 26, y + 2, x + 24, y + 2, x + 34, y - 42, C565(0xd8, 0x20, 0x28));
  gfx->fillCircle(x + 34, y - 42, 7, white);
  gfx->fillRoundRect(x - 31, y - 4, 62, 13, 6, white);
}

void drawPartyHat(int x, int y) {
  gfx->fillTriangle(x - 18, y + 4, x + 18, y + 4, x + 2, y - 40, C565(0x3c, 0x8c, 0xe0));
  gfx->fillCircle(x - 6, y - 6, 3, C_GOLD);
  gfx->fillCircle(x + 6, y - 14, 3, C565(0xf0, 0x60, 0xa0));
  gfx->fillCircle(x + 1, y - 26, 3, C_GOLD);
  gfx->fillCircle(x + 2, y - 42, 6, C565(0xf0, 0x60, 0xa0));
}

static void heartShape(int x, int y, int r, uint16_t c) {
  gfx->fillCircle(x - r / 2, y, r * 6 / 10, c);
  gfx->fillCircle(x + r / 2, y, r * 6 / 10, c);
  gfx->fillTriangle(x - r - 1, y + r / 6, x + r + 1, y + r / 6, x, y + r + r / 3, c);
}

static void lantern(int x, int y, int sz, uint32_t now, int i, bool string) {
  int sway = (int)(4 * sinf(now / 600.0f + i));
  int cx = x + sway;
  if (string) gfx->drawLine(x, 0, cx, y - sz, C565(0x60, 0x40, 0x20));
  gfx->fillRect(cx - sz / 3, y - sz - 3, sz * 2 / 3, 5, C_GOLD);
  gfx->fillEllipse(cx, y, sz, sz * 8 / 10, C_RED);
  gfx->drawEllipse(cx, y, sz / 2, sz * 8 / 10, C565(0xa8, 0x10, 0x10));
  gfx->drawFastVLine(cx, y - sz * 8 / 10, sz * 16 / 10, C565(0xa8, 0x10, 0x10));
  gfx->fillRect(cx - sz / 3, y + sz * 8 / 10 - 1, sz * 2 / 3, 5, C_GOLD);
  for (int k = -sz / 4; k <= sz / 4; k += sz / 4 > 0 ? sz / 4 : 1)
    gfx->drawFastVLine(cx + k, y + sz * 8 / 10 + 4, sz / 2, C_GOLD);
}

static void pumpkin(int cx, int ground, int r, bool glow) {
  int cy = ground - r;
  uint16_t o = C565(0xf0, 0x80, 0x10), od = C565(0xc8, 0x5c, 0x08), face = glow ? C565(0xff, 0xe0, 0x50) : C565(0x30, 0x18, 0x08);
  gfx->fillRect(cx - 3, cy - r - 7, 6, 10, C565(0x40, 0x70, 0x20));
  gfx->fillEllipse(cx - r / 2, cy, r * 6 / 10, r, od);
  gfx->fillEllipse(cx + r / 2, cy, r * 6 / 10, r, od);
  gfx->fillEllipse(cx, cy, r * 6 / 10, r, o);
  gfx->fillTriangle(cx - r / 2, cy - r / 6, cx - r / 5, cy - r / 6, cx - r * 7 / 20, cy - r / 2, face);
  gfx->fillTriangle(cx + r / 5, cy - r / 6, cx + r / 2, cy - r / 6, cx + r * 7 / 20, cy - r / 2, face);
  gfx->fillTriangle(cx - r / 2, cy + r / 6, cx + r / 2, cy + r / 6, cx, cy + r / 2, face);
}

static void bat(int x, int y, uint32_t now, int i) {
  uint16_t c = C565(0x22, 0x18, 0x2a);
  int wy = ((now / 160) + i) % 2 ? -11 : 6;
  gfx->fillCircle(x, y, 5, c);
  gfx->fillTriangle(x - 3, y, x - 22, y + wy, x - 11, y + 5, c);
  gfx->fillTriangle(x + 3, y, x + 22, y + wy, x + 11, y + 5, c);
}

static void leaf(int x, int y, int s, uint16_t c) {  // hoja (de arce, simplificada)
  gfx->fillTriangle(x, y - 2 * s, x - s, y + s, x + s, y + s, c);
  gfx->fillTriangle(x - 2 * s, y - s / 2, x, y + s / 2, x - s / 2, y + s, c);
  gfx->fillTriangle(x + 2 * s, y - s / 2, x, y + s / 2, x + s / 2, y + s, c);
  gfx->drawFastVLine(x, y + s, s + 1, c);
}

// hojas o confeti cayendo en bucle por la parte de arriba
static void falling(uint32_t now, int n, const uint16_t *cols, int ncol, bool leaves) {
  for (int i = 0; i < n; i++) {
    int y = (int)((i * 113 + now * (30 + i % 4 * 9) / 1000) % WX_BOTTOM);
    int x = (int)((i * 79 + (int)(16 * sinf(now / 900.0f + i))) % 466);
    if (x < 0) x += 466;
    if (leaves) leaf(x, y, 4, cols[i % ncol]);
    else gfx->fillRect(x, y, 4, 6, cols[i % ncol]);
  }
}

static void fireworks(uint32_t now) {
  static const uint16_t COL[4] = { C565(0xff, 0x50, 0x50), C565(0x60, 0xd0, 0xff), C565(0xff, 0xe0, 0x40),
                                   C565(0xc0, 0x80, 0xff) };
  for (int b = 0; b < 3; b++) {
    uint32_t per = 1900 + b * 300, t = (now + b * 700) % per;
    if (t > 1300) continue;  // pausa entre estallidos
    uint32_t cyc = (now + b * 700) / per;
    int cx = 90 + (int)((cyc * 131 + b * 97) % 290), cy = 60 + (int)((cyc * 53 + b * 41) % 90);
    int r = (int)(t * 55 / 1300);
    for (int k = 0; k < 12; k++) {
      float a = k * (2 * PI / 12);
      gfx->fillCircle(cx + (int)(r * cosf(a)), cy + (int)(r * sinf(a)), t > 900 ? 1 : 2, COL[(b + cyc) % 4]);
    }
  }
}

static void balloon(int x, int y, uint16_t c, uint32_t now, int i) {
  int dy = (int)(5 * sinf(now / 700.0f + i));
  gfx->drawLine(x, y + dy + 22, x + 4, y + 70, UI_TRACK);
  gfx->fillEllipse(x, y + dy, 15, 20, c);
  gfx->fillTriangle(x - 3, y + dy + 22, x + 3, y + dy + 22, x, y + dy + 18, c);
  gfx->fillCircle(x - 5, y + dy - 7, 3, UI_WHITE);
}

static void cake(int cx, int ground, uint32_t now) {
  gfx->fillRoundRect(cx - 30, ground - 26, 60, 26, 4, C565(0xf6, 0xd6, 0xe0));
  gfx->fillRoundRect(cx - 22, ground - 44, 44, 20, 4, C565(0xfa, 0xee, 0xf2));
  gfx->fillRect(cx - 30, ground - 18, 60, 4, C565(0xe0, 0x60, 0x90));
  for (int k = -1; k <= 1; k++) {
    int x = cx + k * 13;
    gfx->fillRect(x - 2, ground - 58, 4, 14, C565(0x60, 0xa0, 0xf0));
    int f = (int)((now / 120 + k) % 3);
    gfx->fillEllipse(x, ground - 63, 3, 4 + f, C565(0xff, 0xb0, 0x30));
  }
}

static void easterEgg(int x, int ground, uint16_t c, uint16_t band) {
  gfx->fillEllipse(x, ground - 15, 11, 15, c);
  gfx->fillRect(x - 10, ground - 18, 21, 4, band);
}

static void mooncake(int x, int ground) {
  gfx->fillEllipse(x, ground - 9, 20, 10, C565(0xb8, 0x74, 0x30));
  gfx->fillEllipse(x, ground - 12, 18, 8, C565(0xd8, 0x98, 0x48));
  gfx->drawEllipse(x, ground - 12, 10, 4, C565(0x9a, 0x5a, 0x20));
}

static void zongzi(int x, int ground) {
  uint16_t g = C565(0x4c, 0x9a, 0x3c);
  gfx->fillTriangle(x - 20, ground, x + 20, ground, x, ground - 34, g);
  gfx->drawLine(x - 14, ground - 10, x + 14, ground - 10, C565(0xe8, 0xd8, 0xa8));
  gfx->drawLine(x - 9, ground - 19, x + 9, ground - 19, C565(0xe8, 0xd8, 0xa8));
}

// guirnalda de luces colgando de lado a lado bajo la cabecera (comba en el centro)
static int garlandY(int x) { return 122 + (int)(26.0f * (1.0f - (float)((x - 233) * (x - 233)) / (205.0f * 205.0f))); }
static void stringLights(uint32_t now) {
  static const uint16_t COL[4] = { C565(0xff, 0x40, 0x40), C565(0x40, 0xe0, 0x60), C565(0xff, 0xd0, 0x40),
                                   C565(0x50, 0xa0, 0xff) };
  for (int x = 30; x < 436; x += 6) gfx->drawLine(x, garlandY(x), x + 6, garlandY(x + 6), C565(0x30, 0x50, 0x30));
  for (int i = 0, x = 44; x <= 422; x += 27, i++) {
    bool on = ((now / 500) + i) % 3 != 0;
    gfx->fillEllipse(x, garlandY(x) + 7, 4, 6, on ? COL[i % 4] : lerp565(COL[i % 4], UI_INK, 1, 2));
  }
}

#define HOL_GROUND 306  // suelo de los adornos (el panel empieza en 312)

void drawHoliday(uint32_t now) {
  switch (gHoliday) {
    case HOL_NEWYEAR:
      fireworks(now);
      break;
    case HOL_CNY:
      lantern(78, 120, 24, now, 0, true);
      lantern(388, 120, 24, now, 1, true);
      {
        static const uint16_t c[2] = { C_RED, C_GOLD };
        falling(now, 18, c, 2, false);
      }
      break;
    case HOL_LANTERN:
      for (int i = 0; i < 5; i++) {  // farolillos que suben
        int y = WX_BOTTOM - (int)((now / 40 + i * 70) % (WX_BOTTOM + 40));
        lantern(70 + i * 82, y, 13, now, i, false);
      }
      break;
    case HOL_VALENTINE:
    case HOL_MOTHER:
    case HOL_FATHER: {
      uint16_t c = gHoliday == HOL_FATHER ? C565(0x40, 0x88, 0xe0)
                 : gHoliday == HOL_MOTHER ? C565(0xf0, 0x70, 0xa8) : C565(0xe8, 0x30, 0x50);
      for (int i = 0; i < 7; i++) {  // corazones que suben
        int y = WX_BOTTOM - (int)((now / 30 + i * 53) % (WX_BOTTOM + 30));
        int x = 50 + i * 60 + (int)(10 * sinf(now / 800.0f + i));
        heartShape(x, y, 9 + i % 3 * 3, c);
      }
      break;
    }
    case HOL_EASTER:
      easterEgg(62, HOL_GROUND, C565(0xf4, 0xa0, 0xc8), C565(0x90, 0xd0, 0xf0));
      easterEgg(92, HOL_GROUND, C565(0xa8, 0xe0, 0x90), C565(0xf8, 0xe0, 0x70));
      easterEgg(380, HOL_GROUND, C565(0xf8, 0xe0, 0x70), C565(0xd0, 0x90, 0xf0));
      easterEgg(410, HOL_GROUND, C565(0x90, 0xc8, 0xf8), C565(0xf4, 0xa0, 0xc8));
      break;
    case HOL_DRAGONBOAT:
      zongzi(74, HOL_GROUND);
      zongzi(392, HOL_GROUND);
      zongzi(420, HOL_GROUND);
      break;
    case HOL_CANADA: {
      static const uint16_t c[2] = { C565(0xe0, 0x20, 0x20), C565(0xf6, 0xf6, 0xf6) };
      falling(now, 16, c, 2, true);
      if (gNight) fireworks(now);
      break;
    }
    case HOL_MIDAUTUMN:
      gfx->fillCircle(110, 92, 34, C565(0xf8, 0xe6, 0x9a));
      gfx->fillCircle(98, 84, 6, C565(0xe8, 0xd2, 0x80));
      gfx->fillCircle(122, 104, 4, C565(0xe8, 0xd2, 0x80));
      mooncake(76, HOL_GROUND);
      mooncake(392, HOL_GROUND);
      break;
    case HOL_THANKSGIVING: {
      static const uint16_t c[3] = { C565(0xe0, 0x70, 0x20), C565(0xc0, 0x40, 0x20), C565(0xe8, 0xb0, 0x30) };
      falling(now, 16, c, 3, true);
      pumpkin(392, HOL_GROUND, 22, false);
      break;
    }
    case HOL_HALLOWEEN:
      for (int i = 0; i < 3; i++) {
        int x = (int)((now / 22 + i * 190) % 560) - 50;
        bat(x, 70 + i * 34 + (int)(10 * sinf(now / 300.0f + i)), now, i);
      }
      pumpkin(70, HOL_GROUND, 24, gNight);
      pumpkin(396, HOL_GROUND, 20, gNight);
      break;
    case HOL_CHRISTMAS:
      stringLights(now);
      if (!(gWxView.valid && gWxView.kind == WX_SNOW)) {  // nieve de adorno (si no nieva ya de verdad)
        for (int i = 0; i < 24; i++) {
          int y = (int)((i * 89 + now * (18 + i % 3 * 6) / 1000) % WX_BOTTOM);
          int x = (int)((i * 67 + (int)(12 * sinf(now / 700.0f + i))) % 466);
          if (x < 0) x += 466;
          gfx->fillCircle(x, y, 2, UI_WHITE);
        }
      }
      break;
    case HOL_BIRTHDAY:
      balloon(70, 150, C565(0xf0, 0x50, 0x70), now, 0);
      balloon(104, 128, C565(0x50, 0xa0, 0xf0), now, 1);
      balloon(392, 140, C565(0xf0, 0xc8, 0x30), now, 2);
      cake(396, HOL_GROUND, now);
      break;
  }
}

// region (0-8) de una especie, -1 si no esta en ninguna
int regionOf(int16_t dex) {
  for (int r = 0; r < REGION_COUNT; r++)
    if (dex >= REGIONS[r].first && dex <= REGIONS[r].last) return r;
  return -1;
}

// fondo de la pantalla principal: el paisaje de la region del compañero (SD);
// el huevo, o si falta la SD o el fichero, el escenario dibujado por bioma
void drawBackground() {
  int r = pet.isEgg() ? -1 : regionOf(pet.speciesId);
  if (r >= 0 && bg.load(r, gNight ? BG_NIGHT : gPhase == PH_TWILIGHT ? BG_TWILIGHT : BG_DAY)) {
    gfx->draw16bitRGBBitmap(0, 0, bg.px, bg.w, bg.h);
    gfx->fillRect(0, bg.h, 466, 466 - bg.h, gNight ? UI_BG_NIGHT : UI_BG_DAY);
    if (gNight) drawNightSky(millis());
    return;
  }
  drawScene(pet.isEgg() ? 0 : DEX_TBL[pet.speciesId].biome, millis(), gNight);
}

void drawScene(uint8_t biome, uint32_t now, bool night) {
  int h = sceneHour();
  uint16_t top, bot;
  bool dawn = gPhase == PH_TWILIGHT && h < 12, dusk = gPhase == PH_TWILIGHT && h >= 12;
  if (night)            { top = C565(0x0c, 0x12, 0x24); bot = C565(0x1e, 0x26, 0x46); }
  else if (dawn)        { top = C565(0xd1, 0x6a, 0x86); bot = C565(0xf3, 0xb8, 0x7c); }  // amanecer
  else if (!dusk)       { top = C565(0x8f, 0xc8, 0xea); bot = C565(0xdc, 0xee, 0xe6); }  // dia
  else                  { top = C565(0xc7, 0x5a, 0x4a); bot = C565(0xf0, 0xae, 0x64); }  // atardecer

  // cielo en bandas
  for (int y = 0; y < HORIZON; y += 8)
    gfx->fillRect(0, y, 466, 8, lerp565(top, bot, y, HORIZON));

  // sol o luna
  if (night) {
    gfx->fillCircle(360, 78, 24, C565(0xe8, 0xee, 0xf5));
    gfx->fillCircle(370, 72, 22, lerp565(top, bot, 78, HORIZON));  // creciente
    for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);
  } else if (!dusk) {
    gfx->fillCircle(360, 84, 26, dawn ? C565(0xff, 0xd9, 0x8a) : C565(0xff, 0xe7, 0x9f));
    drawClouds(now, C565(0xff, 0xff, 0xff));
  } else {
    gfx->fillCircle(233, HORIZON - 6, 34, C565(0xff, 0xf1, 0xc8));  // sol poniente
  }

  // mar de la playa: una franja de agua sobre la arena
  uint16_t soil = BIOME_SOIL[biome < 6 ? biome : 0];
  if (night) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
  if (biome == 1) {
    uint16_t sea = night ? C565(0x1c, 0x34, 0x52) : C565(0x4f, 0x96, 0xc4);
    gfx->fillRect(0, HORIZON - 26, 466, 26, sea);
    for (int i = 0; i < 3; i++) {
      int wy = HORIZON - 22 + i * 7;
      uint16_t fc = night ? C565(0x3a, 0x58, 0x78) : C565(0xbf, 0xe6, 0xf5);
      gfx->fillRect(60 + ((now / 60 + i * 30) % 60), wy, 26, 2, fc);
      gfx->fillRect(300 - ((now / 60 + i * 20) % 60), wy, 26, 2, fc);
    }
  }

  // suelo
  gfx->fillRect(0, HORIZON, 466, 466 - HORIZON, soil);
  uint16_t hill = lerp565(soil, night ? C565(0x0c, 0x12, 0x24) : C565(0xff, 0xff, 0xff), 3, 16);
  gfx->fillRoundRect(-60, HORIZON - 14, 586, 60, 30, hill);

  // detalles del bioma
  uint16_t dk = lerp565(soil, C565(0x10, 0x18, 0x20), night ? 11 : 7, 16);
  if (biome == 2) {  // bosque: coniferas en silueta
    for (int tx : { 60, 150, 360, 416 }) {
      gfx->fillTriangle(tx, HORIZON - 46, tx - 16, HORIZON, tx + 16, HORIZON, dk);
      gfx->fillTriangle(tx, HORIZON - 60, tx - 12, HORIZON - 28, tx + 12, HORIZON - 28, dk);
    }
  } else if (biome == 3) {  // volcan: rocas y brasas
    gfx->fillTriangle(70, HORIZON, 40, HORIZON + 30, 100, HORIZON + 30, dk);
    gfx->fillTriangle(400, HORIZON + 4, 372, HORIZON + 30, 430, HORIZON + 30, dk);
    if (!night)
      for (int e = 0; e < 4; e++)
        gfx->fillRect(120 + e * 70, HORIZON + 8 + (e % 2) * 6, 4, 4, C565(0xff, 0x9b, 0x3a));
  } else if (biome == 4) {  // montana: cumbres al fondo
    gfx->fillTriangle(140, HORIZON - 50, 60, HORIZON, 220, HORIZON, dk);
    gfx->fillTriangle(330, HORIZON - 38, 250, HORIZON, 410, HORIZON, dk);
  } else if (biome == 5 && !night) {  // nieve: copos cayendo
    for (int f = 0; f < 10; f++) {
      int fx = (f * 53 + now / 40) % 466;
      int fy = (f * 90 + now / 18) % HORIZON;
      gfx->fillRect(fx, fy, 3, 3, UI_WHITE);
    }
  } else if (biome == 0) {  // pradera: matas de hierba
    for (int gx : { 80, 175, 300, 395 })
      for (int b = -1; b <= 1; b++)
        gfx->fillRect(gx + b * 5, HORIZON + 6, 2, 8 + (b == 0 ? 4 : 0), dk);
  }
}

// primera partida: elige inicial entre Bulbasaur / Charmander / Squirtle
void renderStarterSelect() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  const char *t = T(S_CHOOSE_STARTER);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(t, 2), 68);
  printT(t);
  for (int i = 0; i < 3; i++) {
    int16_t d = STARTER_DEX[i];
    const DexEntry &de = DEX_TBL[d];
    int ry = STARTER_ROW_Y + i * (STARTER_ROW_H + STARTER_ROW_GAP);
    gfx->fillRoundRect(70, ry, 326, STARTER_ROW_H, 14, lerp565(de.accent, UI_WHITE, 6, 8));
    gfx->drawRoundRect(70, ry, 326, STARTER_ROW_H, 14, de.accent);
    const uint8_t *th = thumbs.get(d);     // miniatura del inicial (si la SD esta lista)
    if (th) drawThumb(th, 76, ry - 5, 3, false);
    gfx->setTextColor(UI_INK);
    setSize(3);
    setCur(178, ry + 24);
    printT(dexName(d));
  }
  gfx->flush();
}
