// Amigos lejos (mailbox.h): la pagina "好友" (ver "见面" y escribir a un amigo) y el mensajero
// que llega a la pantalla principal. Postales y regalos los lleva un pokemon que elige quien manda
// (el compañero o uno de la granja: solo se ve, no se va de su granja); se queda esperando hasta que
// lo tocan, enseña lo que trae, se queda unos minutos y se va. Una visita es el compañero del amigo
// que viene a saludar. Nada da puntos.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

enum : uint8_t { MU_LIST = 0, MU_FRIEND, MU_MESSENGER, MU_PHRASE, MU_STICKER, MU_ITEM };
enum : uint8_t { MUR_VISIT = 0, MUR_CARD, MUR_GIFT, MUR_ROWS };
static uint8_t muStage = MU_LIST, muFi = 0, muKind = MT_CARD, muPage = 0;
static int16_t muDex = 0;        // el mensajero elegido
static uint8_t muShiny = 0;
static uint16_t muLevel = 1;
static uint8_t muPhrase = 0, muSticker = 0, muItem = 0;
static bool muAsk = false;       // pregunta "¿lo mando?"
static uint16_t muVisitDay = 0;  // visitas de hoy a cada amigo (como mucho MAIL_VISITS_DAY)
static uint8_t muVisits[LINK_FRIENDS];
#define MAIL_VISITS_DAY 3
#define MU_STAY_MS (3UL * 60 * 1000)  // lo que se queda el mensajero despues de abrirlo

static const StrId PHRASE[MAIL_PHRASES] = { S_PH_HELLO, S_PH_MISS, S_PH_PLAY, S_PH_TOMORROW, S_PH_MORNING, S_PH_DREAMS,
                                            S_PH_THANKS, S_PH_CHEER, S_PH_BIRTHDAY, S_PH_HAPPY, S_PH_CHESS, S_PH_EVOLVED };
static const char *const *const STICKER[MAIL_STICKERS] = { SPR_ICON_LINK, SPR_ICON_STAR, SPR_ICON_PLAY, SPR_ICON_TROPHY,
                                                          SPR_ICON_FOOD, SPR_ICON_CANDY, SPR_ICON_FUN, SPR_ICON_FARM };

static uint64_t muPid() { return muFi < linkFriendCount() ? linkFriendAt(muFi).pid : 0; }

// nombre del amigo que manda algo (por su numero de jugador)
static const char *mailFromName(uint64_t pid) {
  for (uint8_t i = 0; i < linkFriendCount(); i++)
    if (linkFriendAt(i).pid == pid) return shownName(linkFriendAt(i).name);
  return "?";
}

void openFriends() {
  if (pet.ceremony) return;
  linkFriendsLoad();
  friendsOpen = true;
  muStage = MU_LIST;
  muAsk = false;
}

static void closeFriends() { friendsOpen = false; }

// los que pueden llevar algo: el compañero (si hay) y los de la granja
static int muCandidates() { return (pet.isEgg() ? 0 : 1) + (farmReady() ? farmCount() : 0); }
static void muCandidate(int i, int16_t &dex, uint8_t &shiny, uint16_t &lv) {
  if (!pet.isEgg()) {
    if (i == 0) {
      dex = pet.speciesId;
      shiny = pet.shiny;
      lv = pet.level();
      return;
    }
    i--;
  }
  const FarmEntry &e = farmAt(i);
  dex = e.speciesId;
  shiny = e.shiny;
  lv = farmLevel(e, pet.lastSeenEpoch);
}

static int muItems(uint8_t out[ITEM_COUNT]) {
  int n = 0;
  for (int i = 0; i < ITEM_COUNT; i++)
    if (pet.bag[i]) out[n++] = (uint8_t)i;
  return n;
}

// ---------- dibujar la pagina ----------

static void muTitle(const char *t) {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  uint8_t ts = textW(t, 3) <= 260 ? 3 : 2;  // arriba el circulo es estrecho: si no cabe, mas pequeño
  setSize(ts);
  setCur(centerX(t, ts), ts == 3 ? 44 : 50);
  printT(t);
}

static void muFooter(const char *t, bool pill) {
  setSize(2);
  if (pill) {
    int w = textW(t, 2) + 36;
    gfx->fillRoundRect(CX - w / 2, 388, w, 34, 14, UI_BAR_OK);
    gfx->setTextColor(UI_WHITE);
    setCur(CX - textW(t, 2) / 2, 397);
  } else {
    gfx->setTextColor(UI_HINT);
    setCur(centerX(t, 2), 396);
  }
  printT(t);
}

static void renderFriendsList() {
  muTitle(T(S_HUB_LINK));
  hubRow(0, C565(0xd8, 0xd8, 0xf8), SPR_ICON_LINK, true, T(S_LINK_TITLE), T(S_HUB_LINK_HINT));
  uint8_t n = linkFriendCount();
  if (!n) {
    gfx->setTextColor(UI_HINT);
    setSize(2);
    setCur(centerX(T(S_ML_NO_FRIENDS), 2), 190);
    printT(T(S_ML_NO_FRIENDS));
  }
  for (uint8_t i = 0; i < n && i < 4; i++) {
    const LinkFriend &f = linkFriendAt(i);
    bool listo = mailFriendReady(f.pid);
    char sub[40];
    uint8_t enCamino = mailOutCount(f.pid);
    if (!listo) snprintf(sub, sizeof(sub), "%s", T(S_ML_NEED_MEET));
    else if (enCamino) snprintf(sub, sizeof(sub), T(S_ML_ONWAY_FMT), (unsigned)enCamino);
    else snprintf(sub, sizeof(sub), "%s", T(S_ML_READY));
    hubRow(i + 1, 0xB6F2, SPR_ICON_PLAY, listo, shownName(f.name), sub);
  }
  muFooter(T(S_ML_SYNC), true);
}

static void renderFriendMail() {
  muTitle(shownName(linkFriendAt(muFi).name));
  hubRow(MUR_VISIT, C565(0xf8, 0xd0, 0xc8), SPR_ICON_LINK, !pet.isEgg(), T(S_ML_VISIT), T(S_ML_VISIT_SUB));
  hubRow(MUR_CARD, 0xFF2E, SPR_ICON_STAR, true, T(S_ML_CARD), T(S_ML_CARD_SUB));
  hubRow(MUR_GIFT, 0xB6F2, SPR_ICON_CANDY, true, T(S_ML_GIFT), T(S_FR_GIFT_SUB));
  uint8_t enCamino = mailOutCount(muPid());
  if (enCamino) {
    char t[40];
    snprintf(t, sizeof(t), T(S_ML_ONWAY_FMT), (unsigned)enCamino);
    gfx->setTextColor(UI_HINT);
    setSize(2);
    setCur(centerX(t, 2), 300);
    printT(t);
  }
  muFooter(T(S_ML_SYNC), true);
}

static void renderMessengers() {
  muTitle(T(S_ML_PICK_MSGR));
  int n = muCandidates();
  if (muPage * 9 >= n) muPage = 0;
  for (int k = 0; k < 9 && muPage * 9 + k < n; k++) {
    int16_t dex;
    uint8_t sh;
    uint16_t lv;
    muCandidate(muPage * 9 + k, dex, sh, lv);
    int x = FARM_X0 + (k % 3) * FARM_CELL, y = FARM_Y0 + 12 + (k / 3) * FARM_CELL;
    gfx->fillRoundRect(x + 4, y + 4, FARM_CELL - 8, FARM_CELL - 8, 14, UI_WHITE);
    const uint8_t *t = thumbs.get(dex);
    if (t) drawThumbIn(t, x, y, 2, false, FARM_CELL);
    if (sh) {
      gfx->setTextColor(UI_BAR_WARN);
      setSize(2);
      setCur(x + FARM_CELL - 22, y + 8);
      printT("*");
    }
  }
  muFooter(T(S_SWIPE_R_BACK), false);
}

static void renderPhrases() {
  muTitle(T(S_ML_PICK_PHRASE));
  for (int k = 0; k < 6; k++) {
    int i = muPage * 6 + k;
    if (i >= MAIL_PHRASES) break;
    int y = 86 + k * 48;
    gfx->fillRoundRect(CX - 165, y, 330, 42, 12, UI_WHITE);
    gfx->drawRoundRect(CX - 165, y, 330, 42, 12, UI_TRACK);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(CX - textW(T(PHRASE[i]), 2) / 2, y + 12);
    printT(T(PHRASE[i]));
  }
  muFooter(T(S_SWIPE_R_BACK), false);
}

static void renderStickers() {
  muTitle(T(S_ML_PICK_STICKER));
  for (int k = 0; k < MAIL_STICKERS; k++) {
    int x = CX - 168 + (k % 4) * 84, y = 130 + (k / 4) * 100;
    gfx->fillRoundRect(x + 4, y + 4, 76, 76, 14, UI_WHITE);
    drawMap(STICKER[k], 16, x + 18, y + 18, 3, false);
  }
  muFooter(T(S_SWIPE_R_BACK), false);
}

static void renderMailItems() {
  muTitle(T(S_FR_GIFT_PICK));
  uint8_t its[ITEM_COUNT];
  int n = muItems(its);
  if (!n) {
    gfx->setTextColor(UI_HINT);
    setSize(2);
    setCur(centerX(T(S_FR_GIFT_EMPTY), 2), CY - 8);
    printT(T(S_FR_GIFT_EMPTY));
  }
  if (muPage * 9 >= n) muPage = 0;
  for (int k = 0; k < 9 && muPage * 9 + k < n; k++) {
    uint8_t it = its[muPage * 9 + k];
    int x = CX - 144 + (k % 3) * 96, y = 96 + (k / 3) * 96;
    gfx->fillRoundRect(x + 4, y + 4, 88, 88, 12, UI_WHITE);
    drawItemIcon(it, x + 24, y + 10);
    char c[8];
    snprintf(c, sizeof(c), "x%u", pet.bag[it]);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(x + 48 - textW(c, 2) / 2, y + 64);
    printT(c);
  }
  muFooter(T(S_SWIPE_R_BACK), false);
}

static const char *muMessengerName(char *b, size_t cap) {
  if (muKind != MT_VISIT && muDex == pet.speciesId && pet.nick[0]) return pet.nick;
  snprintf(b, cap, "%s", dexName(muDex));
  return b;
}

void renderFriends() {
  switch (muStage) {
    case MU_FRIEND: renderFriendMail(); break;
    case MU_MESSENGER: renderMessengers(); break;
    case MU_PHRASE: renderPhrases(); break;
    case MU_STICKER: renderStickers(); break;
    case MU_ITEM: renderMailItems(); break;
    default: renderFriendsList();
  }
  if (muAsk) {  // ¿lo mando?
    char l1[64], nb[24];
    if (muKind == MT_VISIT) {
      snprintf(l1, sizeof(l1), T(S_ML_VISIT_Q_FMT), shownName(linkFriendAt(muFi).name));
      drawAskBox(l1, pet.nick[0] ? pet.nick : dexName(pet.speciesId));
    } else {
      snprintf(l1, sizeof(l1), T(S_ML_CARRIES_FMT), muMessengerName(nb, sizeof(nb)));
      drawAskBox(l1, T(S_ML_SEND_Q));
    }
  }
  drawToast();
  gfx->flush();
}

// ---------- tocar ----------

static void muSend() {
  MailMsg m;
  m.type = muKind;
  m.species = muDex;
  m.shiny = muShiny;
  m.level = muLevel;
  if (muKind == MT_CARD) {
    m.phrase = muPhrase;
    m.sticker = muSticker;
  } else if (muKind == MT_GIFT) {
    if (!pet.bag[muItem]) { toast(T(S_NONE_LEFT)); return; }
    m.item = muItem;
    m.qty = 1;
  }
  if (!wifiHasCreds()) { toast(T(S_ML_NO_WIFI)); return; }  // sin Wi-Fi no saldria nunca (y el regalo, 14 dias fuera)
  if (!mailSend(muPid(), m)) { toast(T(S_ML_FULL)); return; }
  if (muKind == MT_GIFT && !friendTestMode()) {  // sale de la mochila ya (si no llega en 14 dias, vuelve)
    pet.bag[muItem]--;
    pet.markDirty();
    // guardado ya, no al atenuar: la cola de salida ya esta en la NVS, y un reinicio antes de guardar
    // la mochila dejaba el objeto en las dos (regalo repetido)
    pet.flushSave();
  }
  if (muKind == MT_VISIT) muVisits[muFi]++;
  sfxPlay(SFX_MEDAL);
  toast(T(S_ML_SENT));
  muStage = MU_FRIEND;
}

void friendsTap(int16_t x, int16_t y) {
  if (muAsk) {
    muAsk = false;
    if (askTap(x, y) == 1) muSend();
    return;
  }
  bool footer = y >= 384 && abs(x - CX) < 110;
  if ((muStage == MU_LIST || muStage == MU_FRIEND) && footer) {  // "立即收发"
    sfxPlay(SFX_TAP);
    if (!wifiHasCreds()) { toast(T(S_ML_NO_WIFI)); return; }  // la tarea de red no hace nada sin Wi-Fi
    mailSyncSoon();
    toast(T(S_ML_SYNCING));
    return;
  }
  if (muStage == MU_LIST) {
    if (x < HUB_ROW_X || x >= HUB_ROW_X + HUB_ROW_W || y < HUB_ROW_Y0) return;
    int row = (y - HUB_ROW_Y0) / HUB_ROW_STEP;
    if (row >= HUB_ROWS || y - HUB_ROW_Y0 - row * HUB_ROW_STEP > HUB_ROW_H) return;
    sfxPlay(SFX_TAP);
    if (row == 0) { openLink(); return; }  // "见面": las placas de al lado
    if (row - 1 >= linkFriendCount()) return;
    if (!mailFriendReady(linkFriendAt(row - 1).pid)) { toast(T(S_ML_NEED_MEET)); return; }
    muFi = (uint8_t)(row - 1);
    muStage = MU_FRIEND;
    return;
  }
  if (muStage == MU_FRIEND) {
    if (x < HUB_ROW_X || x >= HUB_ROW_X + HUB_ROW_W || y < HUB_ROW_Y0) return;
    int row = (y - HUB_ROW_Y0) / HUB_ROW_STEP;
    if (row >= MUR_ROWS || y - HUB_ROW_Y0 - row * HUB_ROW_STEP > HUB_ROW_H) return;
    sfxPlay(SFX_TAP);
    muPage = 0;
    if (!wifiHasCreds()) { toast(T(S_ML_NO_WIFI)); return; }  // antes de elegir nada: sin Wi-Fi no saldria
    if (row == MUR_VISIT) {
      if (pet.isEgg()) { toast(T(S_NO_PARTNER)); return; }
      if (muVisitDay != pet.dayNow()) {
        muVisitDay = pet.dayNow();
        memset(muVisits, 0, sizeof(muVisits));
      }
      if (muVisits[muFi] >= MAIL_VISITS_DAY) { toast(T(S_ML_LIMIT)); return; }
      muKind = MT_VISIT;
      muDex = pet.speciesId;
      muShiny = pet.shiny;
      muLevel = pet.level();
      muAsk = true;
    } else {
      if (!muCandidates()) { toast(T(S_NO_PARTNER)); return; }
      muKind = row == MUR_CARD ? MT_CARD : MT_GIFT;
      muStage = MU_MESSENGER;
    }
    return;
  }
  if (muStage == MU_MESSENGER) {
    if (x < FARM_X0 || y < FARM_Y0 + 12) return;
    int c = (x - FARM_X0) / FARM_CELL, r = (y - FARM_Y0 - 12) / FARM_CELL;
    if (c > 2 || r > 2) return;
    int i = muPage * 9 + r * 3 + c;
    if (i >= muCandidates()) return;
    sfxPlay(SFX_TAP);
    muCandidate(i, muDex, muShiny, muLevel);
    muPage = 0;
    muStage = muKind == MT_CARD ? MU_PHRASE : MU_ITEM;
    return;
  }
  if (muStage == MU_PHRASE) {
    if (y < 86 || abs(x - CX) > 165) return;
    int k = (y - 86) / 48, i = muPage * 6 + k;
    if (k > 5 || i >= MAIL_PHRASES || (y - 86) % 48 > 42) return;
    sfxPlay(SFX_TAP);
    muPhrase = (uint8_t)i;
    muStage = MU_STICKER;
    return;
  }
  if (muStage == MU_STICKER) {
    if (y < 130 || y >= 330 || x < CX - 168 || x >= CX + 168) return;
    int k = (x - (CX - 168)) / 84 + ((y - 130) / 100) * 4;
    if (k >= MAIL_STICKERS) return;
    sfxPlay(SFX_TAP);
    muSticker = (uint8_t)k;
    muAsk = true;
    return;
  }
  if (muStage == MU_ITEM) {
    if (x < CX - 144 || y < 96) return;
    int c = (x - (CX - 144)) / 96, r = (y - 96) / 96;
    if (c > 2 || r > 2) return;
    uint8_t its[ITEM_COUNT];
    int n = muItems(its), k = muPage * 9 + r * 3 + c;
    if (k >= n) return;
    sfxPlay(SFX_TAP);
    muItem = its[k];
    muAsk = true;
  }
}

void friendsSwipe(int dir) {
  if (muAsk) { muAsk = false; return; }
  if (dir < 0) {  // izquierda: mas
    if (muStage == MU_MESSENGER) muPage = (uint8_t)((muPage + 1) * 9 < muCandidates() ? muPage + 1 : 0);
    else if (muStage == MU_PHRASE) muPage = (uint8_t)((muPage + 1) * 6 < MAIL_PHRASES ? muPage + 1 : 0);
    else if (muStage == MU_ITEM) {
      uint8_t its[ITEM_COUNT];
      muPage = (uint8_t)((muPage + 1) * 9 < muItems(its) ? muPage + 1 : 0);
    }
    return;
  }
  muPage = 0;  // derecha: atras
  switch (muStage) {
    case MU_FRIEND: muStage = MU_LIST; break;
    case MU_MESSENGER: muStage = MU_FRIEND; break;
    case MU_PHRASE: case MU_ITEM: muStage = MU_MESSENGER; break;
    case MU_STICKER: muStage = MU_PHRASE; break;
    default: closeFriends();
  }
}

// ---------- el mensajero en la pantalla principal ----------

enum : uint8_t { MV_IN = 0, MV_WAIT, MV_OPEN, MV_STAY, MV_OUT };
static struct {
  bool on = false;
  uint8_t phase = MV_IN;
  int8_t dir = 1;
  float x = 0, goal = 0;
  uint32_t t0 = 0, until = 0, last = 0, wanderAt = 0;
  bool voiced = false;  // una visita: su grabacion ya sono (al primer toque)
  MailMsg m;
} mv;
static uint32_t mvNextTry = 0;

bool mailVisitorOn() { return mv.on; }

void mailVisitorInfo() {  // consola MAIL INFO
  if (mv.on) Serial.printf("correo: mensajero #%d tipo %u fase %u x=%d meta=%d (companero x=%d)\n", mv.m.species, mv.m.type,
                           mv.phase, (int)mv.x, (int)mv.goal, (int)beh.x);
}

static void mvLeave() {
  mv.on = false;
  mailPmd.unload();
}

// que no se quede detras del compañero: el que espera puede estar horas y el bicho va y viene.
// Si estan pegados (y no va ya hacia otro sitio), se aparta por su lado (sin cruzar por delante);
// si por ese lado no cabe, por el otro
static void mvKeepClear() {
  if (fabsf(mv.x - beh.x) >= 90 || fabsf(mv.goal - beh.x) >= 90) return;
  bool dcha = mv.x >= beh.x;
  if (dcha && beh.x + 120 > 396) dcha = false;
  else if (!dcha && beh.x - 120 < 70) dcha = true;
  mv.goal = dcha ? beh.x + 120 : beh.x - 120;
  if (mv.goal < 70) mv.goal = 70;
  if (mv.goal > 396) mv.goal = 396;
}

void mailVisitTick(uint32_t now) {
  if (!mv.on) {
    if ((int32_t)(now - mvNextTry) < 0) return;
    mvNextTry = now + 3000;
    if (!mainScreenFree() || dimStage || pet.ceremony || pet.isEgg() || vis.on) return;
    MailMsg m;
    if (!mailPeekArrival(m)) return;
    linkFriendsLoad();
    if (m.species < 1 || m.species > PET_DEX_MAX) {  // estropeado: fuera
      mailPopArrival();
      return;
    }
    mailPmd.load(m.species, m.shiny);
    mv.on = true;
    mv.m = m;
    mv.voiced = false;
    mv.phase = MV_IN;
    mv.dir = random(2) ? 1 : -1;
    mv.x = mv.dir > 0 ? -50 : 516;
    mv.goal = beh.x - mv.dir * 110;
    if (mv.goal < 80) mv.goal = 80;
    if (mv.goal > 386) mv.goal = 386;
    mv.t0 = mv.last = now;
    Serial.printf("correo: llega el mensajero #%d (tipo %u)\n", m.species, m.type);
    return;
  }
  if (!mainScreenFree() || pet.ceremony) {  // otra pantalla: se va; si no se abrio, volvera
    Serial.println("correo: el mensajero se va (otra pantalla)");
    mvLeave();
    return;
  }
  float dt = (now - mv.last) / 1000.0f;
  if (dt > 0.3f) dt = 0.3f;
  mv.last = now;
  float step = 40.0f * dt;
  if (mv.phase == MV_IN) {
    float d = mv.goal - mv.x;
    if (fabsf(d) <= step) {
      mv.x = mv.goal;
      mv.t0 = now;
      Serial.printf("correo: el mensajero ya esta (x=%d)\n", (int)mv.x);
      if (mv.m.type == MT_VISIT) {  // una visita: ya ha llegado (no hay nada que abrir)
        mailPopArrival();
        mv.phase = MV_STAY;
        mv.until = now + MU_STAY_MS;
        mv.wanderAt = now + 6000;
        sfxPlay(SFX_TAP);
      } else {
        mv.phase = MV_WAIT;  // espera a que le toquen
      }
    } else {
      mv.x += d > 0 ? step : -step;
    }
  } else if (mv.phase == MV_WAIT) {
    mvKeepClear();
    float d = mv.goal - mv.x;
    if (fabsf(d) > step) mv.x += d > 0 ? step * 0.6f : -step * 0.6f;
  } else if (mv.phase == MV_OPEN) {
    if ((int32_t)(now - mv.until) >= 0) {
      mv.phase = MV_STAY;
      mv.until = now + MU_STAY_MS;
      mv.wanderAt = now;
    }
  } else if (mv.phase == MV_STAY) {
    if ((int32_t)(now - mv.wanderAt) >= 0) {  // da un paseito (no a pararse pegado al compañero)
      float g = 80 + random(306);
      if (fabsf(g - beh.x) >= 100) mv.goal = g;
      mv.wanderAt = now + 5000 + random(6000);
    }
    mvKeepClear();
    float d = mv.goal - mv.x;
    if (fabsf(d) > step) mv.x += d > 0 ? step * 0.6f : -step * 0.6f;
    if ((int32_t)(now - mv.until) >= 0) {
      mv.phase = MV_OUT;
      mv.dir = mv.x < CX ? -1 : 1;
      Serial.println("correo: el mensajero se despide");
    }
  } else if (mv.phase == MV_OUT) {
    mv.x += mv.dir * step;
    if (mv.x < -60 || mv.x > 526) mvLeave();
  }
}

// sobre el mensajero: un sobre o un paquete que se mueve un poco (esperando que lo abran)
static void mvBadge(int x, int y, uint8_t type) {
  if (type == MT_CARD) {
    gfx->fillRect(x - 16, y - 11, 32, 22, UI_WHITE);
    gfx->drawRect(x - 16, y - 11, 32, 22, UI_INK);
    gfx->drawLine(x - 16, y - 11, x, y + 2, UI_INK);
    gfx->drawLine(x + 16, y - 11, x, y + 2, UI_INK);
    gfx->fillCircle(x, y + 2, 3, UI_BAR_BAD);
  } else {
    gfx->fillRect(x - 14, y - 10, 28, 22, C565(0xf0, 0x80, 0x90));
    gfx->drawRect(x - 14, y - 10, 28, 22, UI_INK);
    gfx->fillRect(x - 2, y - 10, 5, 22, C565(0xf8, 0xe0, 0x60));
    gfx->fillRect(x - 14, y - 1, 28, 5, C565(0xf8, 0xe0, 0x60));
    gfx->fillCircle(x - 5, y - 13, 4, C565(0xf8, 0xe0, 0x60));
    gfx->fillCircle(x + 5, y - 13, 4, C565(0xf8, 0xe0, 0x60));
  }
}

void drawMailVisitor(uint32_t now) {
  if (!mv.on || !mailPmd.loaded) return;
  bool walk = mv.phase == MV_IN || mv.phase == MV_OUT ||
              ((mv.phase == MV_STAY || mv.phase == MV_WAIT) && fabsf(mv.goal - mv.x) > 2);
  bool right = mv.phase == MV_OUT ? mv.dir > 0 : mv.goal > mv.x;
  uint8_t act = walk ? (right ? PMD_WALKR : PMD_WALKL) : PMD_IDLE;
  if (!mailPmd.has(act)) act = PMD_IDLE;
  drawPmdActM(mailPmd, act, (int)mv.x, PET_GROUND - 4, now - mv.t0, true, false, 3);
  int top = gPmdTopY;
  if (mv.phase == MV_WAIT) {
    int bob = (int)(sinf(now / 220.0f) * 4);
    mvBadge((int)mv.x, top - 26 + bob, mv.m.type);
    const char *t = T(S_ML_TAP);  // "点一下打开"
    setSize(2);
    int w = textW(t, 2) + 20, x = (int)mv.x - w / 2;
    if (x < 50) x = 50;
    if (x + w > 416) x = 416 - w;
    gfx->fillRoundRect(x, top - 76, w, 28, 10, UI_WHITE);
    gfx->setTextColor(UI_INK);
    setCur(x + 10, top - 70);
    printT(t);
  } else if (mv.phase == MV_STAY && mv.m.type == MT_VISIT && now - mv.t0 < 8000) {
    char t[64], nb[24];  // "Ash的妙蛙种子来看你了！"; si no cabe, en dos lineas (por el espacio mas centrado)
    snprintf(nb, sizeof(nb), "%s", dexName(mv.m.species));
    snprintf(t, sizeof(t), T(S_ML_VISITED_FMT), mailFromName(mv.m.from), nb);
    setSize(2);
    const char *l2 = nullptr;
    if (textW(t, 2) > 366) {
      int n = strlen(t), best = -1;
      for (int i = 1; i < n - 1; i++)
        if (t[i] == ' ' && (best < 0 || abs(i - n / 2) < abs(best - n / 2))) best = i;
      if (best > 0) {
        t[best] = 0;
        l2 = t + best + 1;
      }
    }
    int tw = textW(t, 2);
    if (l2 && textW(l2, 2) > tw) tw = textW(l2, 2);
    int w = tw + 20, h = l2 ? 52 : 30, x = (int)mv.x - w / 2, y = top - 20 - h;
    if (x < 40) x = 40;
    if (x + w > 426) x = 426 - w;
    gfx->fillRoundRect(x, y, w, h, 10, UI_WHITE);
    gfx->drawRoundRect(x, y, w, h, 10, UI_INK);
    gfx->setTextColor(UI_INK);
    setCur(x + 10, y + 7);
    printT(t);
    if (l2) {
      setCur(x + 10, y + 29);
      printT(l2);
    }
  } else if (mv.phase == MV_STAY && (now / 2500) % 3 == 0) {
    drawMap(SPR_HEART, 32, (int)mv.x - 16, top - 36, 1, false);
  }
}

// lo que trae, en un panel encima de todo (MV_OPEN)
void drawMailPanel() {
  if (!mv.on || mv.phase != MV_OPEN) return;
  char l1[48];
  snprintf(l1, sizeof(l1), T(mv.m.type == MT_GIFT ? S_ML_FROM_GIFT_FMT : S_ML_FROM_CARD_FMT), mailFromName(mv.m.from));
  gfx->fillRoundRect(64, 120, 338, 200, 18, UI_WHITE);
  gfx->drawRoundRect(64, 120, 338, 200, 18, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(l1, 2), 138);
  printT(l1);
  if (mv.m.type == MT_CARD) {
    const char *ph = T(PHRASE[mv.m.phrase < MAIL_PHRASES ? mv.m.phrase : 0]);
    int ts = textW(ph, 3) <= 310 ? 3 : 2;
    setSize(ts);
    setCur(centerX(ph, ts), 176);
    printT(ph);
    drawMap(STICKER[mv.m.sticker < MAIL_STICKERS ? mv.m.sticker : 0], 16, CX - 32, 228, 4, false);
  } else {
    uint8_t it = mv.m.item < ITEM_COUNT ? mv.m.item : 0;
    drawItemIcon(it, CX - 24, 176);
    char t[40];
    snprintf(t, sizeof(t), "%s x%u", T(ITEM_NAME[it]), (unsigned)(mv.m.qty ? mv.m.qty : 1));
    setSize(2);
    setCur(centerX(t, 2), 248);
    printT(t);
  }
}

// la grabacion del amigo para este mensaje, si la hay en la SD (voice.h)
static void mailVoice(const MailMsg &m) {
  if (!audioEnabled()) return;
  uint32_t n;
  int16_t *pcm = sdLoadVoice(m.from, voiceClip(m.type, m.phrase), n);
  if (pcm && !voicePlay(pcm, n)) free(pcm);
}

// un toque en la principal: si es para el mensajero, true
bool mailVisitorTap(int16_t x, int16_t y) {
  if (!mv.on) return false;
  uint32_t now = millis();
  if (mv.phase == MV_OPEN) {  // cerrar el panel: se queda un rato
    mv.until = now;
    return true;
  }
  bool cerca = abs(x - (int)mv.x) < 70 && y > 120 && y < PET_GROUND + 10;
  if (!cerca) return false;
  if (mv.phase == MV_WAIT) {  // abrirlo: lo que trae, ahora (un regalo, a la mochila)
    bool regalo = mv.m.type == MT_GIFT && mv.m.item < ITEM_COUNT && !friendTestMode();
    if (regalo) pet.give(mv.m.item, mv.m.qty ? (mv.m.qty > 5 ? 5 : mv.m.qty) : 1);
    // la mochila a la NVS antes de borrar la llegada: un corte entre las dos escrituras repite el
    // regalo (vuelve a llegar), al reves se perdia
    if (regalo) pet.flushSave();
    mailPopArrival();
    mv.phase = MV_OPEN;
    mv.until = now + 8000;
    sfxPlay(SFX_MEDAL);
    mailVoice(mv.m);  // y, si grabo una para esto, la voz del amigo (voice.h)
    wakeScreen(now);
    Serial.printf("correo: abierto (tipo %u, objeto %u)%s\n", mv.m.type, mv.m.item, friendTestMode() ? " (prueba)" : "");
    return true;
  }
  sfxPlay(SFX_TAP);  // de visita o despues: un corazon
  showEmote(EMO_HEART, 1200);
  // una visita no se "abre" (llega y se queda): su grabacion suena al primer toque del que la mira
  if (mv.phase == MV_STAY && mv.m.type == MT_VISIT && !mv.voiced) {
    mv.voiced = true;
    mailVoice(mv.m);
  }
  return true;
}
