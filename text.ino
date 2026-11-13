// Texto: tamano, cursor, fuente china y medida.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

// ---------- texto: tamano y cursor (preparado para fuentes CJK) ----------
// La fuente clasica ancla el cursor en la ESQUINA SUPERIOR del texto, y las 74
// coordenadas Y del codigo estan escritas asi. Las fuentes U8g2 lo anclan en la
// LINEA BASE (drawChar hace curY = y - baseline*size), asi que al cambiar de
// fuente todo el texto subiria. setCur() compensa esa diferencia en un solo
// sitio, para que la Y siga significando "arriba" con cualquier fuente.
//
// setSize() existe solo porque la libreria tiene textsize_x protegido y setCur()
// necesita saber la escala activa para multiplicar el ascenso.
//
// Con la fuente clasica gFontAscent es 0, asi que setCur(x,y) es exactamente
// setCur(x,y): esto no mueve un pixel en los seis idiomas actuales.

// --- fuente china ---
// El latino es la 5x7 clasica: a tamano n mide 7n px de alto. El chino es una
// fuente U8g2 que solo escala por enteros, asi que con una sola de 16 px el
// tamano 3 (21 px en latino) se quedaba en 16 y los titulos salian pequenos.
// Con dos alturas, 12 y 16 px, cada tamano coge la combinacion fuente x escala
// mas cercana a 7n: 2 -> 16, 3 -> 12x2 = 24, 4 -> 16x2 = 32...
//
// Se probo una fuente realmente negrita (b10_b) y NO sirve a este tamano: los
// trazos se juntan y pierden detalle. Se usan las WenQuanYi finas y el trazo se
// compensa en printT() repintando 1 px desplazado (pseudo-negrita).
// Son las mismas WenQuanYi de las wqy*_t_gb2312 de U8g2, pero recortadas a los
// caracteres que de verdad se pintan (textos de i18n.cpp y los 1025 nombres):
// unos 80 KB en vez de 500, y con los que GB2312 no tenia (椪 de Ogerpon).
// Se regeneran con tools/make_zh_font.py al cambiar textos o nombres. El
// ascenso de cada una se mide en applyLangFont().

void setSize(uint8_t n) {
  gReqSize = n;
  if (!gCjkFont) {
    gTextSize = n;
    gfx->setTextSize(n);
    return;
  }
  // la fuente y la escala cuya altura mas se acerca a la del latino (7n px)
  int objetivo = 7 * n, mejorErr = 1000;
  bool chica = false;
  uint8_t escala = 1;
  for (uint8_t k = 1; k <= 4; k++) {
    for (int alto : { 16, 12 }) {  // a igual distancia, la de 16 (se lee mejor)
      int err = abs(alto * k - objetivo);
      if (err < mejorErr) { mejorErr = err; chica = (alto == 12); escala = k; }
    }
  }
  gfx->setFont(chica ? CJK_FONT_ZH_S : CJK_FONT_ZH);
  gFontAscent = chica ? gAsc12 : gAsc16;
  // gTextSize guarda la escala REALMENTE aplicada: setCur() multiplica el
  // ascenso por ella y tiene que cuadrar con lo que se pinta
  gTextSize = escala;
  gfx->setTextSize(escala);
}

void setCur(int x, int y) {
  gfx->setCursor(x, y + gFontAscent * gTextSize);
}

// Fija la fuente del idioma activo. Es el UNICO sitio que la toca: el resto del
// codigo dibuja igual para todos los idiomas gracias a textW()/setCur().
//
// El ascenso se mide en vez de codificarlo: getTextBounds() devuelve y1 como
// desplazamiento del borde superior respecto al cursor, negativo con las fuentes
// U8g2 (que anclan en la linea base). Asi setCur() puede seguir tratando la Y
// como "arriba" sea cual sea la fuente, sin constantes magicas por fuente.
void applyLangFont() {
  gCjkFont = LANG_IS_CJK(gLang);
  if (!gCjkFont) {
    gfx->setFont();            // 5x7 clasica, CP437: la Y ya es el borde superior
    gfx->setUTF8Print(false);
    gFontAscent = 0;
    return;
  }
  gfx->setUTF8Print(true);     // las cadenas CJK son UTF-8 multibyte
  int16_t x1, y1;
  uint16_t w, h;
  gfx->setTextSize(1);
  gfx->setFont(CJK_FONT_ZH_S);
  gfx->getTextBounds("A", 0, 0, &x1, &y1, &w, &h);
  gAsc12 = -y1;                // y1 negativo: subir desde la linea base
  gfx->setFont(CJK_FONT_ZH);
  gfx->getTextBounds("A", 0, 0, &x1, &y1, &w, &h);
  gAsc16 = -y1;
  setSize(gReqSize);           // vuelve a la fuente y escala del tamano activo
}

// ---------- medida de texto (preparado para fuentes CJK) ----------
// La fuente clasica de Arduino_GFX avanza 6 px por caracter a tamano 1, y la UI
// centraba con strlen(s)*6*n a pelo. Eso solo vale para un byte por caracter y
// ancho fijo: con una fuente U8g2 para japones/coreano/chino, ni strlen() cuenta
// caracteres (UTF-8 es multibyte) ni todos miden lo mismo. Centralizarlo aqui es
// lo que permitira anadir esos idiomas sin tocar cada punto de dibujado.
//
// Para la fuente clasica devuelve EXACTAMENTE la misma cuenta que habia antes,
// asi que este cambio no mueve un pixel en los seis idiomas actuales.
uint16_t textW(const char *s, uint8_t size) {
  if (!gCjkFont) return (uint16_t)strlen(s) * 6 * size;
  // en chino la fuente depende del tamano: se mide con la de `size` y se
  // deja puesta la que estaba
  uint8_t antes = gReqSize;
  setSize(size);
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  setSize(antes);
  return w;
}

// x del cursor para dejar el texto centrado en CX
int centerX(const char *s, uint8_t size) { return CX - textW(s, size) / 2; }

// Imprime respetando la fuente activa. Con la CJK repinta el texto desplazado
// 1 px: la unifont a escala reducida deja trazos de 1 px y se ve tenue, y una
// fuente de verdad negrita a este tamano junta los trazos y estropea los kana.
// Repintar engorda el trazo sin deformar el glifo. Probado en placa por
// usakomint, que comparo las dos opciones en japones.
// El teclado de apodos es solo alfabeto (se deja asi a proposito), de modo que
// aqui nunca hay CJK y no hace falta engordar nada.
void printT(char c) { gfx->print(c); }

void printT(const char *s) {
  if (gCjkFont) {
    int16_t x = gfx->getCursorX(), y = gfx->getCursorY();
    gfx->print(s);
    gfx->setCursor(x + 1, y);
  }
  gfx->print(s);
}

// Pantallas que solo cambian con un toque (o con un dato que cambia muy de vez en cuando):
// mientras su "clave" no cambie no se repintan ni se vuelcan por QSPI.
static bool staticScreen() {
  if (pet.awaitingStarter()) return true;
  if (galleryOpen || farmOpen || shopOpen) return false;  // gestionan su propio repintado
  if (kbOpen || clockOpen) return true;
  return cardOpen && cardPage != 0;  // la pagina 0 tiene el retrato animado
}
static uint32_t mixKey(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }
static uint32_t screenKey() {
  uint32_t h = 2166136261u;
  h = mixKey(h, pet.awaitingStarter() ? 1 : kbOpen ? 2 : clockOpen ? 3 + statsOpen * 100 : 4 + cardPage);
  h = mixKey(h, sdEpoch);
  if (kbOpen) {
    h = mixKey(h, nameLen);
    for (int i = 0; i < nameLen; i++) h = mixKey(h, (uint8_t)nameBuf[i]);
  } else if (clockOpen) {
    h = mixKey(h, pet.lastSeenEpoch / 60);  // la hora que se enseña
    h = mixKey(h, audioEnabled());
    h = mixKey(h, userDim);
    h = mixKey(h, tempF);
    h = mixKey(h, gLang);
  } else if (cardOpen) {
    for (uint32_t v : { (uint32_t)pet.speciesId, (uint32_t)pet.shiny, (uint32_t)pet.ageMinutes, (uint32_t)pet.streak,
                        (uint32_t)pet.bestStreak, (uint32_t)pet.bond, (uint32_t)pet.medals, (uint32_t)pet.weight,
                        (uint32_t)pet.trAtk, (uint32_t)pet.trDef, (uint32_t)pet.trSpe, (uint32_t)pet.geneAtk,
                        (uint32_t)pet.geneDef, (uint32_t)pet.geneSpe, (uint32_t)pet.careMistakes,
                        (uint32_t)pet.fullness, (uint32_t)pet.joy, (uint32_t)pet.energy, (uint32_t)pet.hygiene,
                        (uint32_t)pet.stepsToday, (uint32_t)gLang })
      h = mixKey(h, v);
    for (const char *c = pet.nick; *c; c++) h = mixKey(h, (uint8_t)*c);
  }
  return h;
}
