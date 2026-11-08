// El bicho en pantalla: baño, sprites PMD, comportamiento autonomo,
// emociones, barras y botones.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

// ---------- escena de bano ----------

#define BATH_MS 3800

// baño: ducha desde arriba, espuma en la cabeza, pompas de jabon (transparentes,
// con brillo) que suben y se mecen, un patito al lado y destellos de limpio
void startBathClean() { startBath(true); }  // baño de verdad (Stack-chan)

// limpiar = false: solo la animacion (cuando se lava el solo, ya subio la limpieza)
void startBath(bool limpiar) {
  if (pet.isEgg() || pet.sleeping || pet.ceremony || bathUntil) return;
  uint32_t now = millis();
  bathUntil = now + BATH_MS;
  bathPending = limpiar;
  for (auto &b : bubbles) {  // posiciones relativas al bicho
    b.x = -80 + random(160);
    b.y = random(140);
    b.r = 5 + random(12);
    b.ph = random(64);
  }
  react(PMD_NOD, BATH_MS - 400, nullptr);  // se deja hacer, a gusto
}

static void drawShine(int x, int y, int r, uint16_t c) {  // destello de 4 puntas
  gfx->fillTriangle(x, y - r, x - r / 4 - 1, y, x + r / 4 + 1, y, c);
  gfx->fillTriangle(x, y + r, x - r / 4 - 1, y, x + r / 4 + 1, y, c);
  gfx->fillTriangle(x - r, y, x, y - r / 4 - 1, x, y + r / 4 + 1, c);
  gfx->fillTriangle(x + r, y, x, y - r / 4 - 1, x, y + r / 4 + 1, c);
}

static void drawDuck(int x, int y) {  // patito de goma
  uint16_t am = C565(0xf8, 0xd0, 0x30);
  gfx->fillEllipse(x, y, 13, 8, am);
  gfx->fillCircle(x + 9, y - 9, 6, am);
  gfx->fillTriangle(x + 14, y - 10, x + 21, y - 8, x + 14, y - 6, C565(0xf0, 0x80, 0x20));
  gfx->fillCircle(x + 10, y - 11, 1, UI_INK);
  gfx->fillTriangle(x - 13, y - 2, x - 19, y - 8, x - 9, y - 4, am);
}

void drawBath() {
  uint32_t now = millis();
  if (!timeLeft(bathUntil)) {
    bathUntil = 0;
    if (bathPending) {
      bathPending = false;
      pet.clean();
      // pose de alegria al quedar limpio
      if (pmd.has(PMD_POSE)) {
        beh.mode = 2;
        beh.act = PMD_POSE;
        beh.t0 = now;
        beh.until = now + pmdActTotalMs(pmd.acts[PMD_POSE]) * 2;
      }
      showEmote(EMO_SPARK, 1500);
    }
    return;
  }
  uint32_t el = BATH_MS - timeLeft(bathUntil);  // tiempo de baño transcurrido
  int cx = (int)beh.x, top = gPmdTopY;
  uint16_t agua = C565(0x8c, 0xd0, 0xf8), metal = C565(0xb4, 0xc0, 0xcc);

  // patito que se balancea al lado
  drawDuck(cx - 78, PET_GROUND - 8 + (int)(3 * sinf(now / 260.0f)));

  // ducha (los 2,6 primeros segundos): alcachofa arriba y chorritos inclinados
  if (el < 2600) {
    int hx = cx + 26, hy = top - 64;
    if (hy < 112) hy = 112;
    gfx->fillRect(hx + 24, hy - 26, 5, 26, metal);  // tubo
    gfx->fillRoundRect(hx - 24, hy - 6, 50, 12, 6, metal);
    for (int i = 0; i < 8; i++) gfx->fillCircle(hx - 18 + i * 5, hy + 6, 1, C565(0x60, 0x70, 0x80));
    int largo = PET_GROUND - hy - 10;
    for (int i = 0; i < 20; i++) {
      int y = hy + 10 + (int)((now / 2 + i * 41) % (largo > 20 ? largo : 20));
      int x = hx - 20 + (i * 9) % 42 - (y - hy) / 10;
      gfx->drawFastVLine(x, y, 7, agua);
      gfx->drawFastVLine(x + 1, y + 1, 5, UI_WHITE);
    }
    for (int i = 0; i < 6; i++) {  // salpicaduras en el suelo
      int k = (int)((now / 80 + i * 3) % 6);
      gfx->fillCircle(cx - 40 + i * 16, PET_GROUND - 2 - k, 2, agua);
    }
  }

  // espuma sobre la cabeza (de 0,5 s a 3,2 s)
  if (el > 500 && el < 3200) {
    static const int8_t ESPUMA[][3] = { { -20, 4, 10 }, { -8, -2, 13 }, { 6, -4, 14 }, { 19, 2, 11 },
                                        { -14, 12, 9 }, { 12, 11, 10 }, { 0, 8, 12 } };
    for (auto &e : ESPUMA) {
      int r = e[2] + (int)(1.5f * sinf(now / 200.0f + e[0]));
      gfx->fillCircle(cx + e[0], top + e[1], r, UI_WHITE);
      gfx->drawCircle(cx + e[0], top + e[1], r, C565(0xd4, 0xe8, 0xf8));
    }
  }

  // pompas de jabon: solo el contorno de color, un brillo blanco, y suben meciendose
  if (el > 300 && el < 3500) {
    for (auto &b : bubbles) {
      int sube = (int)((el - 300) / 18) + b.y;
      int by = PET_GROUND - 30 - sube;
      if (by < 112 || by > PET_GROUND) continue;
      int bx = cx + b.x + (int)(7 * sinf(now / 300.0f + b.ph));
      uint16_t c = (b.ph & 1) ? C565(0xa8, 0xd8, 0xf8) : C565(0xe0, 0xc4, 0xf8);
      gfx->drawCircle(bx, by, b.r, c);
      gfx->drawCircle(bx, by, b.r - 1, c);
      gfx->fillCircle(bx - b.r / 3, by - b.r / 3, b.r / 4 + 1, UI_WHITE);
    }
  }

  // limpio: destellos alrededor (el ultimo segundo)
  if (el > 2800) {
    float f = (el - 2800) / 1000.0f;
    for (int i = 0; i < 6; i++) {
      int x = cx - 60 + (i * 47) % 124, y = top + 10 + (i * 37) % (PET_GROUND - top > 30 ? PET_GROUND - top - 20 : 20);
      int r = (int)(4 + 9 * sinf((f + i * 0.15f) * PI));
      if (r > 2) drawShine(x, y, r, (i & 1) ? UI_WHITE : C565(0xff, 0xe8, 0x70));
    }
  }
}

// ---------- mascota PMD: comportamiento ----------

uint32_t pmdActTotalMs(const PmdAct &a) {
  uint32_t t = 0;
  for (uint8_t i = 0; i < a.frames; i++) t += a.ms[i];
  return t ? t : 100;
}

uint8_t pmdFrameAt(const PmdAct &a, uint32_t t, bool loop) {
  uint32_t total = pmdActTotalMs(a);
  if (!loop && t >= total) return a.frames - 1;
  t %= total;
  uint8_t i = 0;
  // guarda: nunca mas vueltas que frames, por si un .bin corrupto trae ms[]=0
  // pese al saneo de la carga (defensa en profundidad)
  for (uint8_t guard = 0; guard < a.frames && t >= a.ms[i]; guard++) {
    t -= a.ms[i];
    i = (i + 1) % a.frames;
  }
  return i;
}

// dibuja una accion anclada por la base (centro-x, suelo) y devuelve su escala
// dibuja una accion de un PmdMon concreto (m); drawPmdAct usa el global pmd
void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS) {
  const PmdAct &a = m.acts[actId];
  if (!a.frames) return;
  uint8_t sBase = m.acts[PMD_IDLE].h ? 170 / m.acts[PMD_IDLE].h : 5;
  if (sBase < 2) sBase = 2;
  if (sBase > maxS) sBase = maxS;
  uint8_t s = sBase;
  while (s > 2 && a.h * s > 250) s--;  // acciones con frame grande (ataque)
  uint8_t fi = pmdFrameAt(a, t, loop);
  const uint8_t *fr = a.data + (uint32_t)fi * a.w * a.h;
  // anclar por los pies (a.base), no por el alto del lienzo: asi las acciones
  // con padding distinto (Hurt, Eat...) quedan todas a la misma altura de suelo
  int x0 = cx - a.w * s / 2, y0 = groundY - (a.base ? a.base : a.h) * s;
  bool top = false;
  for (int r = 0; r < a.h; r++) {
    const uint8_t *row = fr + r * a.w;
    forRuns(a.w, [&](int c) { return indexedKey(row[c], sil); },
            [&](int c0, int len, int k) {
              if (!top) {  // primera fila con pixeles: la cima
                top = true;
                gPmdTopY = y0 + r * s;
                gPmdTopX = x0 + c0 * s + len * s / 2;
              }
              gfx->fillRect(x0 + c0 * s, y0 + r * s, len * s, s, sil ? INK_K : m.pal[k]);
            });
  }
}
void drawPmdAct(uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS) {
  drawPmdActM(pmd, actId, cx, groundY, t, loop, sil, maxS);
}

void showEmote(uint8_t e, uint32_t ms) {
  emote = e;
  emoteUntil = millis() + ms;
}

// gestos de "contento" disponibles en este sprite (Hop fuera: salta demasiado
// alto; Sit va aparte)
static uint8_t flairPick(uint8_t *pick) {
  static const uint8_t flair[] = { PMD_POSE, PMD_NOD, PMD_BREATH };
  uint8_t n = 0;
  for (uint8_t f : flair)
    if (pmd.has(f)) pick[n++] = f;
  return n;
}

// lo que deja la actividad que acaba (siesta: algo de energia; jugar solo: animo)
static void behEnd(uint32_t now) {
  if (beh.activity == ACT_DOZE) {
    pet.selfRest();
    bubbleText = T(S_SAY_STRETCH);
    bubbleUntil = now + 1800;
  } else if (beh.activity == ACT_BALL) {
    pet.selfPlay();
  }
  ball.on = false;
}

// elige lo siguiente que hace por su cuenta. El peso de cada cosa depende de
// como esta (energia, animo, hambre, peso), de su caracter, de la hora y del
// tiempo de fuera: el perezoso se echa siestas, el juguetón juega a la pelota,
// el gloton va a buscar comida, de noche se adormila, con frio se queda quieto...
void behNext() {
  uint32_t now = millis();
  behEnd(now);
  bool anda = pmd.has(PMD_WALKL) || pmd.has(PMD_WALKR);
  uint8_t gestos[3];
  uint8_t nGestos = flairPick(gestos);
  int w[ACT_COUNT] = { 0 };
  w[ACT_IDLE] = 14;
  w[ACT_LOOK] = anda ? 10 : 0;
  w[ACT_WALK] = anda ? 20 : 0;
  w[ACT_GESTURE] = nGestos ? 14 : 0;
  w[ACT_HOP] = 7;
  w[ACT_BALL] = anda ? 6 : 0;
  w[ACT_DOZE] = 2;
  w[ACT_SIT] = pmd.has(PMD_SIT) ? 4 : 0;
  if (pet.energy < 50) { w[ACT_DOZE] += 10; w[ACT_SIT] += 5; w[ACT_HOP] /= 2; w[ACT_BALL] /= 3; }
  if (pet.energy < 25) { w[ACT_DOZE] += 20; w[ACT_WALK] /= 2; }
  if (pet.joy > 70 && pet.energy >= 40) { w[ACT_HOP] += 6; w[ACT_BALL] += anda ? 6 : 0; }
  if (pet.fullness < 40 && anda) w[ACT_SEEK] += 15;
  switch (pet.nature) {
    case NAT_LAZY: w[ACT_DOZE] += 8; w[ACT_SIT] += 6; w[ACT_WALK] /= 2; break;
    case NAT_PLAYFUL: w[ACT_BALL] += anda ? 10 : 0; w[ACT_HOP] += 6; break;
    case NAT_GLUTTON: if (anda) w[ACT_SEEK] += 6; break;
    case NAT_TIDY: w[ACT_GESTURE] += nGestos ? 6 : 0; break;
    default: w[ACT_IDLE] += 4; break;
  }
  int h = sceneHour();
  if (h >= 21 || h < 6) w[ACT_DOZE] += 8;
  if (pet.weatherFeel() > 0) w[ACT_HOP] += 8;
  if (pet.feelsCold()) { w[ACT_SIT] += 6; w[ACT_BALL] = 0; }
  if (pet.weight > 60) { w[ACT_SIT] += 6; w[ACT_BALL] /= 2; }
  if (vis.on) w[ACT_LOOK] += 20;  // mira al que ha venido
  if (!pmd.has(PMD_SLEEP)) w[ACT_DOZE] = 0;
  int total = 0;
  for (int v : w) total += v;
  int r = random(total > 0 ? total : 1), a = 0;
  while (a < ACT_COUNT - 1 && r >= w[a]) r -= w[a++];
  if (forceAct >= 0 && forceAct < ACT_COUNT) a = forceAct;
  forceAct = -1;

  beh.activity = a;
  beh.t0 = now;
  beh.mode = 0;
  switch (a) {
    case ACT_LOOK:
      beh.until = now + 2600;
      if (!random(3)) showEmote(EMO_QUESTION, 1500);
      break;
    case ACT_WALK:
      beh.mode = 1;
      beh.targetX = 90 + random(287);
      beh.until = now + 15000;
      break;
    case ACT_SEEK:  // va hacia el boton de la comida y la echa de menos
      beh.mode = 1;
      beh.targetX = 120 + random(30);
      beh.until = now + 15000;
      showEmote(EMO_QUESTION, 1500);
      break;
    case ACT_GESTURE:
      beh.mode = 2;
      beh.act = gestos[random(nGestos)];
      beh.until = now + pmdActTotalMs(pmd.acts[beh.act]);
      if (!random(3)) showEmote(EMO_HEART, 1500);
      break;
    case ACT_HOP:
      beh.until = now + 1800 + random(1000);
      showEmote(EMO_NOTE, 1800);
      break;
    case ACT_BALL:
      ball.on = true;
      ball.x = beh.x + (random(2) ? 60 : -60);
      ball.vx = 0;
      ball.last = now;
      beh.until = now + 9000 + random(5000);
      showEmote(EMO_EXCLAIM, 1000);
      break;
    case ACT_DOZE:
      beh.until = now + (pet.nature == NAT_LAZY ? 25000 : 15000) + random(15000);
      break;
    case ACT_SIT:
      beh.until = now + 5000 + random(5000);
      if (pet.energy < 30) showEmote(EMO_SWEAT, 2000);
      else if (!random(3)) showEmote(EMO_DOTS, 2000);
      break;
    default:  // ACT_IDLE: a veces dice como esta con una expresion
      beh.until = now + 2000 + random(3000);
      if (!random(3)) showEmote(pet.joy > 70 ? EMO_HEART : pet.hygiene < 40 ? EMO_ANGRY : EMO_NOTE, 1600);
      break;
  }
}

// pelota: rueda, se frena, rebota en los lados; el bicho va detras y le da
static uint8_t ballStep(uint32_t now) {
  float dt = (now - ball.last) / 1000.0f;
  if (dt > 0.3f) dt = 0.3f;
  ball.last = now;
  ball.x += ball.vx * dt;
  ball.vx *= powf(0.35f, dt);
  if (ball.x < 70) { ball.x = 70; ball.vx = -ball.vx * 0.7f; }
  if (ball.x > 396) { ball.x = 396; ball.vx = -ball.vx * 0.7f; }
  float d = ball.x - beh.x;
  if (fabsf(d) > 36) {
    float step = 55.0f * dt;
    if (step > fabsf(d) - 32) step = fabsf(d) - 32;
    beh.x += d > 0 ? step : -step;
    return d > 0 ? PMD_WALKR : PMD_WALKL;
  }
  if (fabsf(ball.vx) < 25) {  // la tiene al lado y parada: patada
    ball.vx = (d >= 0 ? 1 : -1) * (110 + pet.trAtk + random(80));  // con mas aguante, mas lejos
    if (!random(3)) showEmote(EMO_NOTE, 900);
  }
  return PMD_IDLE;
}

// expresion en un bocadillo redondo junto a la cabeza
void drawEmote(int x, int y, uint32_t now) {
  if (!emote || !timeLeft(emoteUntil)) return;
  if (y < 112) y = 112;
  gfx->fillCircle(x, y, 15, UI_WHITE);
  gfx->drawCircle(x, y, 15, UI_INK);
  gfx->fillCircle(x - 12, y + 16, 3, UI_WHITE);
  gfx->drawCircle(x - 12, y + 16, 3, UI_INK);
  switch (emote) {
    case EMO_HEART: heartShape(x, y - 3, 8, C565(0xe8, 0x40, 0x68)); break;
    case EMO_NOTE:
      gfx->fillCircle(x - 4, y + 5, 4, UI_INK);
      gfx->fillRect(x - 1, y - 9, 2, 14, UI_INK);
      gfx->fillTriangle(x + 1, y - 9, x + 8, y - 5, x + 1, y - 3, UI_INK);
      break;
    case EMO_EXCLAIM:
      gfx->fillRoundRect(x - 2, y - 10, 5, 13, 2, C565(0xe8, 0x50, 0x30));
      gfx->fillCircle(x, y + 8, 3, C565(0xe8, 0x50, 0x30));
      break;
    case EMO_QUESTION:
      gfx->setTextColor(0x4C98);
      setSize(3);
      setCur(x - textW("?", 3) / 2, y - 10);
      printT("?");
      break;
    case EMO_SWEAT:
      gfx->fillCircle(x, y + 3, 6, C565(0x60, 0xb0, 0xf0));
      gfx->fillTriangle(x - 5, y + 1, x + 5, y + 1, x, y - 11, C565(0x60, 0xb0, 0xf0));
      break;
    case EMO_DOTS:
      for (int k = -1; k <= 1; k++) gfx->fillCircle(x + k * 8, y + 1, 2, UI_INK);
      break;
    case EMO_ZZZ: {
      gfx->setTextColor(0x4C98);
      int sube = (int)((now / 120) % 6);
      setSize(2);
      setCur(x - 9, y - 4 - sube);
      printT("z");
      setCur(x + 1, y - 12 - sube / 2);
      printT("z");
      break;
    }
    case EMO_ANGRY: {
      uint16_t c = C565(0xd8, 0x30, 0x30);
      gfx->fillRect(x - 7, y - 7, 5, 2, c); gfx->fillRect(x - 7, y - 7, 2, 5, c);
      gfx->fillRect(x + 3, y - 7, 5, 2, c); gfx->fillRect(x + 6, y - 7, 2, 5, c);
      gfx->fillRect(x - 7, y + 5, 5, 2, c); gfx->fillRect(x - 7, y + 2, 2, 5, c);
      gfx->fillRect(x + 3, y + 5, 5, 2, c); gfx->fillRect(x + 6, y + 2, 2, 5, c);
      break;
    }
    case EMO_SPARK: {
      uint16_t c = C565(0xf0, 0xb8, 0x20);
      gfx->fillTriangle(x, y - 11, x - 3, y, x + 3, y, c);
      gfx->fillTriangle(x, y + 11, x - 3, y, x + 3, y, c);
      gfx->fillTriangle(x - 11, y, x, y - 3, x, y + 3, c);
      gfx->fillTriangle(x + 11, y, x, y - 3, x, y + 3, c);
      break;
    }
    case EMO_FOOD: drawMap(SPR_ICON_FOOD, 16, x - 8, y - 8, 1, false); break;
  }
}

// comida delante de la boca, a mordiscos (3 tamaños) y migas cayendo
static void drawFood(int cx, uint32_t now) {
  uint32_t t = now - eatStart;
  if (t >= EAT_ANIM_MS) return;
  int mordisco = (int)(t * 3 / EAT_ANIM_MS);  // 0, 1, 2
  int s = 3 - mordisco;
  const char *const *ic = pet.lastFood == FOOD_CANDY ? SPR_ICON_CANDY : SPR_ICON_FOOD;
  int fx = cx + 34, fy = PET_GROUND - 34;
  drawMap(ic, 16, fx - 8 * s, fy - 8 * s, s, false);
  for (int i = 0; i < 4; i++) {  // migas
    int k = (int)((t / 90 + i * 5) % 12);
    gfx->fillRect(fx - 10 + i * 6, fy + k * 2, 3, 3, C565(0xc8, 0x70, 0x40));
  }
}

void drawPetPMD() {
  uint32_t now = millis();

  if (pet.evolving()) {
    drawEvolveFX(now);
    return;
  }
  if (evoPmd.loaded) evoPmd.unload();  // termino la evolucion: libera la forma anterior

  PetMood m = pet.mood();
  uint8_t act;
  bool loop = true;
  bool dizzy = timeLeft(dizzyUntil) > 0 && m != MOOD_SLEEPING;
  if (dizzy) {
    act = pmd.has(PMD_HURT) ? PMD_HURT : PMD_IDLE;
    beh.mode = 0;
  } else if (timeLeft(reactUntil) && m != MOOD_SLEEPING) {  // reacciona a un gesto
    if (reactAct == REACT_SPIN) act = ((now / 200) & 1) ? PMD_WALKL : PMD_WALKR;
    else if (reactAct == REACT_BOUNCE) act = PMD_IDLE;
    else act = pmd.has(reactAct) ? reactAct : PMD_IDLE;
    if (!dragOn) beh.mode = 0;
  } else if (m == MOOD_SLEEPING && pmd.has(PMD_SLEEP)) {
    act = PMD_SLEEP;
    beh.mode = 0;
  } else if (m == MOOD_EATING) {
    // cada comida a su manera: masticar, masticar dando saltitos, asentir
    // ("mmm, que rico") o sentarse a comer
    beh.mode = 0;
    act = PMD_EAT;
    if (eatStyle == 2 && pmd.has(PMD_NOD)) act = PMD_NOD;
    if (eatStyle == 3 && pmd.has(PMD_SIT)) act = PMD_SIT;
    if (!pmd.has(act)) act = PMD_IDLE;
    beh.t0 = eatStart;
  } else if (m == MOOD_SAD && pmd.has(PMD_HURT)) {
    act = PMD_HURT;
    if (!timeLeft(emoteUntil) && now % 9000 < 40)  // de vez en cuando dice que no esta bien
      showEmote(pet.sick ? EMO_SWEAT : pet.fullness < 25 ? EMO_FOOD : EMO_DOTS, 2000);
  } else {
    // contento: el planificador decide (idle / paseo / gesto)
    if (now > beh.until) behNext();
    if (beh.mode != 1) walkLast = 0;  // al volver a caminar no se cuenta el tiempo parado
    if (beh.mode == 1) {
      float d = beh.targetX - beh.x;
      if (fabsf(d) < 4) {
        if (beh.activity == ACT_SEEK) {  // llego a la comida: la echa de menos
          beh.activity = ACT_IDLE;
          beh.mode = 0;
          beh.until = now + 2500;
          showEmote(EMO_FOOD, 2500);
        } else {
          behNext();
        }
        act = PMD_IDLE;
      } else {
        // 30 px/s de diseno (3 px por frame a 10 fps), midiendo el tiempo real: con menos frames
        // por segundo camina igual de rapido
        uint32_t dt = walkLast ? now - walkLast : 100;
        uint32_t cap = frameIntervalMs(powerCtx(now)) + 100;  // un hueco mayor (otra pantalla de por medio) no es un paso largo
        if (dt > cap) dt = cap;
        walkLast = now;
        // a paso normal, la vitalidad lo hace un poco mas agil (hasta +50%)
        float vel = walkSpeed == 30.0f ? 30.0f * (1.0f + pet.trSpe / 200.0f) : walkSpeed;
        float step = vel * dt / 1000.0f;
        if (step > fabsf(d)) step = fabsf(d);  // un frame lento no sobrepasa el destino
        beh.x += d > 0 ? step : -step;
        act = (d > 0) ? PMD_WALKR : PMD_WALKL;
      }
    } else if (beh.mode == 2) {
      act = beh.act;
      loop = false;
    } else {
      switch (beh.activity) {
        case ACT_LOOK:  // mira a un lado y luego al otro (quieto)
          act = timeLeft(beh.until) > 1300 ? PMD_WALKL : PMD_WALKR;
          beh.t0 = now;  // fotograma fijo: no camina
          break;
        case ACT_BALL: act = ballStep(now); break;
        case ACT_DOZE: act = PMD_SLEEP; break;
        case ACT_SIT: act = PMD_SIT; break;
        default: act = PMD_IDLE; break;
      }
    }
    if (!pmd.has(act)) act = PMD_IDLE;
  }

  // de frio tirita a ratos (1,5 s de cada 4)
  int tirita = (pet.feelsCold() && !pet.sleeping && now % 4000 < 1500) ? (((now / 70) & 1) ? 2 : -2) : 0;
  bool brinca = (reactAct == REACT_BOUNCE && timeLeft(reactUntil) && m != MOOD_SLEEPING) ||
                (m == MOOD_HAPPY && !timeLeft(reactUntil) && beh.activity == ACT_HOP && beh.mode == 0) ||
                (m == MOOD_EATING && eatStyle == 1);
  int salto = brinca ? (int)((m == MOOD_EATING ? 10 : 22) * fabsf(sinf(now * 0.012f))) : 0;
  drawPmdAct(act, (int)beh.x + tirita, PET_GROUND - salto, now - beh.t0, loop || act == PMD_IDLE, false, 5);
  if (gHoliday == HOL_CHRISTMAS) drawSantaHat(gPmdTopX, gPmdTopY + 8);
  else if (gHoliday == HOL_BIRTHDAY) drawPartyHat(gPmdTopX, gPmdTopY + 8);
  else if (pet.accWorn) drawAccessory(pet.accWorn - 1, gPmdTopX, gPmdTopY + 8);
  if (ball.on && beh.activity == ACT_BALL) drawMap(SPR_ICON_PLAY, 16, (int)ball.x - 16, PET_GROUND - 30, 2, false);
  if (m == MOOD_EATING) drawFood((int)beh.x, now);
  if (beh.activity == ACT_DOZE && m == MOOD_HAPPY && beh.mode == 0 && !timeLeft(reactUntil)) showEmote(EMO_ZZZ, 300);
  drawEmote(gPmdTopX + 40, gPmdTopY - 4, now);
  if (pet.shiny && !pet.sleeping) drawSparkles(gPmdTopX, gPmdTopY, now);
  if (timeLeft(levelUpUntil)) drawLevelUp(gPmdTopX, gPmdTopY, now);
  drawBubble(gPmdTopX, gPmdTopY);
  if (dizzy) drawDizzy(gPmdTopX, gPmdTopY, now);

  if (pet.showHeart()) drawMap(SPR_HEART, 32, (int)beh.x + 50, PET_GROUND - 190, 2, false);
}

// sprite animado desde la SD: zoom entero por pixel, frames a su ritmo
void drawPetSD() {
  int s = mon.scale;
  int w = mon.w * s, h = mon.h * s;
  int x = CX - w / 2;
  int y = PET_CY - h / 2;

  bool sil = false;
  if (pet.evolving()) {
    sil = (millis() / 300) % 2;
  } else if (pet.mood() == MOOD_HAPPY && (millis() / 500) % 2) {
    y -= 6;  // saltito
  }

  uint16_t fm = mon.frameMs ? mon.frameMs : 100;
  uint16_t fi = pet.sleeping ? 0 : (millis() / fm) % mon.frames;
  const uint8_t *fr = mon.data + (uint32_t)fi * mon.w * mon.h;
  for (int r = 0; r < mon.h; r++) {
    const uint8_t *row = fr + r * mon.w;
    forRuns(mon.w, [&](int c) { return indexedKey(row[c], sil); },
            [&](int c0, int len, int k) { gfx->fillRect(x + c0 * s, y + r * s, len * s, s, sil ? INK_K : mon.pal[k]); });
  }

  // emotes en vez de expresiones (los sprites importados no tienen anclas)
  if (pet.showHeart()) drawMap(SPR_HEART, 32, x + w - 30, y - 50, 2, false);
  if (timeLeft(dizzyUntil) && !pet.sleeping) drawDizzy(CX, y, millis());
}

// ojo cerrado: borra el ojo 3x4 y dibuja el parpado
void overlayEye(const Species &sp, int x, int y, int s, int col) {
  gfx->fillRect(x + col * s, y + sp.eyeRow * s, 3 * s, 4 * s, sp.bodyColor);
  gfx->fillRect(x + col * s, y + (sp.eyeRow + 2) * s, 3 * s, s, INK_K);
}

// borra la sonrisa base y pinta boca abierta (comer) o ceno (triste)
void overlayMouth(const Species &sp, int x, int y, int s, bool open) {
  int mc = sp.mouthCol, mr = sp.mouthRow;
  gfx->fillRect(x + (mc - 3) * s, y + mr * s, 7 * s, 2 * s, sp.bodyColor);
  if (open) {
    gfx->fillRect(x + (mc - 2) * s, y + mr * s, 5 * s, 2 * s, INK_K);
  } else {
    gfx->fillRect(x + (mc - 2) * s, y + mr * s, 5 * s, s, INK_K);
    gfx->fillRect(x + (mc - 3) * s, y + (mr + 1) * s, s, s, INK_K);
    gfx->fillRect(x + (mc + 3) * s, y + (mr + 1) * s, s, s, INK_K);
  }
}

void drawPoops() {
  for (int i = 0; i < pet.poops; i++) {
    drawMap(SPR_POOP, 32, 36 + i * 46, 244, 2, false);
  }
}

void drawBars() {
  drawBar(78, 318, T(S_BAR_FOOD), pet.fullness);
  drawBar(244, 318, T(S_BAR_JOY), pet.joy);
  drawBar(78, 346, T(S_BAR_ENE), pet.energy);
  drawBar(244, 346, T(S_BAR_HYG), pet.hygiene);
}

// Separacion entre la etiqueta y su barra en la fila de necesidades. Estaba fija
// en 48 px, justo lo que ocupan 4 letras latinas a escala 2, sin margen. Las
// etiquetas japonesas son mas anchas (ごきげん son 4 kana, ~64 px) y se metian
// dentro de la barra. Se deriva de la etiqueta mas larga, con el 48 de suelo
// para que en los idiomas latinos no cambie nada.
// Requiere el tamano de texto 2 ya puesto, porque textW() lo necesita.
static int barLabelGap() {
  const StrId ids[] = { S_BAR_FOOD, S_BAR_JOY, S_BAR_ENE, S_BAR_HYG };
  int ancho = 0;
  for (StrId id : ids) {
    int w = textW(T(id), 2);
    if (w > ancho) ancho = w;
  }
  // por debajo del umbral original se deja EXACTAMENTE 48, para no mover nada en
  // los idiomas latinos; por encima se anade aire para que la etiqueta respire
  return ancho <= 48 ? 48 : ancho + 8;
}

void drawBar(int x, int y, const char *label, uint8_t val) {
  gfx->setTextColor(inkColor());
  setSize(2);
  setCur(x, y);
  printT(label);
  int gap = barLabelGap();
  // las dos columnas empiezan en 78 y 244: la barra de la primera tiene que
  // acabar antes de la etiqueta de la segunda, y ambas se pintan igual de anchas
  int bw = 232 - (78 + gap);
  if (bw > 100) bw = 100;   // con etiquetas latinas sale 100, como estaba
  int bx = x + gap, bh = 15;
  uint16_t fill = (val >= 50) ? UI_BAR_OK : (val >= 25) ? UI_BAR_WARN : UI_BAR_BAD;
  gfx->fillRoundRect(bx, y, bw, bh, 4, UI_TRACK);
  int fw = (bw - 4) * val / 100;
  if (fw > 0) gfx->fillRoundRect(bx + 2, y + 2, fw, bh - 4, 3, fill);
}

void drawButtons() {
  for (int i = 0; i < 4; i++) {
    bool off = (pet.sleeping && i != 2) || (pet.sick && i == 1);  // durmiendo solo LUZ; enfermo no juega
    int bx = buttons[i].cx - BTN_HALF, by = buttons[i].cy - BTN_HALF;
    if (!pet.sleeping) gfx->fillRoundRect(bx, by, 2 * BTN_HALF, 2 * BTN_HALF, 14, UI_WHITE);
    gfx->drawRoundRect(bx, by, 2 * BTN_HALF, 2 * BTN_HALF, 14, inkColor());
    if (!off) drawMap(buttons[i].icon, 16, buttons[i].cx - 16, buttons[i].cy - 16, 2, false);
  }
}

const char *eggMsg() {
  switch (pet.eggCracks()) {
    case 0: return T(S_EGG_TOUCH);
    case 1: return T(S_EGG_MOVES);
    default: return T(S_EGG_ALMOST);
  }
}

const char *statusMsg() {
  if (pet.evolving()) return T(S_EVOLVING);
  if (bathUntil) return "Splish splash!";  // onomatopeya universal
  if (pet.sleeping) return "Zzz...";
  if (pet.eating()) return T(S_EATING);
  if (pet.showHeart()) return T(S_LIKES);
  if (pet.sick) return T(S_SICK);
  if (pet.fullness < 25) return T(S_HUNGRY);
  if (pet.hygiene < 25) return T(S_NEEDS_BATH);
  if (pet.energy < 25) return T(S_EXHAUSTED);
  if (pet.joy < 25) return T(S_SAD);
  if (pet.weight > 60) return T(S_CHUBBY);
  if (pet.shiny && pet.ageMinutes < 15) return T(S_IS_SHINY);
  static char visitMsg[64];
  if (vis.on) {  // ha venido alguien de la granja
    snprintf(visitMsg, sizeof(visitMsg), T(S_VISIT_FMT), vis.name);
    return visitMsg;
  }
  if (gHoliday == HOL_BIRTHDAY) {  // con el nombre del dueño (config.h)
    snprintf(visitMsg, sizeof(visitMsg), T(S_HOL_BIRTHDAY), OWNER_NAME);
    return visitMsg;
  }
  if (gHoliday) return T((StrId)(S_HOL_NEWYEAR + gHoliday - 1));  // dia de fiesta: felicita
  // el tiempo de fuera: frio, lluvia que no le gusta, o el tiempo que le gusta
  if (pet.feelsCold()) return T(S_WX_COLD);
  int8_t wf = pet.weatherFeel();
  if (wf < 0) return T(S_WX_HATE_RAIN);
  if (wf > 0) {
    uint8_t b = DEX_TBL[pet.speciesId].biome;
    return T(b == 5 ? S_WX_LIKE_SNOW : b == 2 ? S_WX_LIKE_SUN : S_WX_LIKE_RAIN);
  }
  return T(S_HAPPY);
}

// dibuja un mapa de n x n pixeles escalado; silhouette=true lo pinta en tinta
void drawMap(const char *const *map, int n, int x, int y, int s, bool silhouette) {
  for (int r = 0; r < n; r++) {
    const char *row = map[r];
    forRuns(n, [&](int c) { return charKey(row[c], silhouette); },
            [&](int c0, int len, int k) { gfx->fillRect(x + c0 * s, y + r * s, len * s, s, silhouette ? INK_K : spriteColor((char)k)); });
  }
}
