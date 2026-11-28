// Gimnasios: torneos por equipos, nada de peleas (gym.h: reglas; gyms.h: lideres, medallas y
// equipos de cada region). 8 por region, en orden, solo en regiones abiertas; al final, la
// liga (alto mando y campeon, uno tras otro). Se juega a 3 contra 3: el compañero y dos de
// la granja contra los tres del lider; en cada prueba uno contra uno, gana quien se lleve 2.
// Un intento al dia por gimnasio (y por liga). Las pruebas las juega sports.ino.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

enum : uint8_t { GY_LIST = 0, GY_DETAIL, GY_TEAM, GY_ORDER, GY_END, GY_WAIT };  // WAIT: a que conteste el amigo
static uint8_t gyStage = GY_LIST, gyRegion = 0, gySel = 0;  // gySel 0..7 gimnasio, 8 liga
static bool gyLeague = false;
static uint8_t lgMember = 0;             // liga: 0..3 alto mando, 4 campeon
static int16_t teamDex[3];               // 0 = el compañero, 1 y 2 de la granja
static Athlete teamA[3];
static int16_t farmPick[2] = { -1, -1 };  // indices en la granja (pasan de 127)
static uint8_t gyPage = 0;
static int *gyOrder = nullptr;           // la granja al elegir: indices, los favoritos primero (PSRAM)
static int gyOrderN = 0;
static uint8_t gyEv[3], gyPerm[3], foePerm[3];  // pruebas; quien de cada equipo hace cada una
static uint8_t gyRound = 0, gyWins = 0, gyLost = 0;
static bool gyWon = false;
static uint8_t gyItem = 0xFF;            // regalo con la medalla
static bool gyNewBadge = false;
static uint32_t gyEndAt = 0;
static bool gyPractice = false;         // consola GYM: sin gastar el intento ni ganar nada
// con un amigo (friend_play.ino): yo pongo el compañero y uno de la granja, el su compañero (el 3o
// del equipo). El intento y la medalla son mios; yo calculo cada prueba y se la mando (FM_ROUND)
static int8_t gyFriend = -1;
static FAthlete gyFriendAth;
static bool gyHelper = false;           // soy el amigo: solo se ve lo que manda el
static uint32_t gyWaitAt = 0;

static const char *gtx(const char *en, const char *zh) { return gLang == LANG_ZH ? zh : en; }

static const GymPerson &foe() { return gyLeague ? LEAGUE[gyRegion][lgMember] : GYMS[gyRegion][gySel].leader; }
static uint8_t foeDifficulty() {
  return gyLeague ? leagueDifficulty(gyRegion, lgMember) : gymDifficulty(gyRegion, gySel);
}
static Athlete foeAthlete(int i) { return leaderAthlete(foe().team[i], foeDifficulty()); }

void openGyms() {
  if (pet.ceremony) return;
  gymsOpen = true;
  gyPractice = false;
  gyFriend = -1;
  gyStage = GY_LIST;
  // la primera region abierta en la que aun falte algo
  gyRegion = 0;
  for (int r = 0; r < pet.unlockedRegions(); r++)
    if (!pet.leagueChampion(r)) { gyRegion = r; break; }
}

static void closeGyms() {
  if (gyFriend >= 0 && gyStage == GY_WAIT) friendCancel(gyFriend);
  gyFriend = -1;
  gymsOpen = false;
  for (int i = 0; i < 2; i++) rivalPmd[i].unload();
}

// ---------- medallas ----------

// una medalla: aro dorado, el color del tipo dentro y un rombo brillante en medio; sin
// ganar, solo el contorno
void drawBadge(int cx, int cy, int r, uint8_t type, bool won) {
  uint16_t col = TYPE_COL[type < TY_COUNT ? type : 0];
  if (!won) {
    gfx->drawCircle(cx, cy, r, UI_TRACK);
    gfx->drawCircle(cx, cy, r - 1, UI_TRACK);
    return;
  }
  gfx->fillCircle(cx, cy, r, 0xE5CC);  // aro
  gfx->drawCircle(cx, cy, r, UI_INK);
  int ri = r * 3 / 4, d = ri * 2 / 3;
  gfx->fillCircle(cx, cy, ri, col);
  gfx->drawCircle(cx, cy, ri, UI_INK);
  gfx->fillTriangle(cx, cy - d, cx - d / 2, cy, cx + d / 2, cy, UI_WHITE);  // rombo: arriba claro,
  gfx->fillTriangle(cx, cy + d, cx - d / 2, cy, cx + d / 2, cy, 0xC618);    // abajo en sombra
  gfx->fillCircle(cx - r * 3 / 5, cy - r * 3 / 5, r / 8 + 1, UI_WHITE);     // brillo del aro
}

static void drawGymLock(int cx, int cy) {
  gfx->fillRoundRect(cx - 8, cy - 2, 16, 13, 3, UI_INK);
  gfx->drawCircle(cx, cy - 4, 6, UI_INK);
  gfx->drawCircle(cx, cy - 4, 5, UI_INK);
}

// ---------- lista: los 8 de una region y su liga (es tambien la vitrina de medallas) ----------

#define GL_Y0 96
#define GL_H 56
static void renderGymList() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  bool abierta = gyRegion < pet.unlockedRegions();
  char t[48];
  snprintf(t, sizeof(t), T(S_GYMS_TITLE_FMT), T((StrId)(S_REGION_1 + gyRegion)));
  gfx->setTextColor(abierta ? UI_INK : UI_HINT);
  setSize(3);
  setCur(centerX(t, 3), 34);
  printT(t);
  snprintf(t, sizeof(t), T(S_BADGES_FMT), (unsigned)pet.badgeCount(gyRegion));
  setSize(2);
  setCur(centerX(t, 2), 70);
  printT(t);
  for (int k = 0; k < GYMS_PER_REGION; k++) {
    int col = k % 2, row = k / 2;
    int x = col ? CX + 4 : CX - 170, y = GL_Y0 + row * GL_H;
    const GymInfo &g = GYMS[gyRegion][k];
    bool won = pet.hasBadge(gyRegion, k), open = pet.gymOpen(gyRegion, k);
    gfx->fillRoundRect(x, y, 166, 48, 12, won ? 0xFFDF : UI_WHITE);
    gfx->drawRoundRect(x, y, 166, 48, 12, UI_TRACK);
    drawBadge(x + 26, y + 24, 16, g.leader.type, won);
    if (!open && !won) drawGymLock(x + 26, y + 26);
    gfx->setTextColor(open || won ? UI_INK : UI_HINT);
    const char *n = gtx(g.leader.en, g.leader.zh);
    int ts = textW(n, 2) <= 108 ? 2 : 1;  // "Professor Kukui", "Crasher Wake"...
    setSize(ts);
    setCur(x + 50, y + (ts == 2 ? 16 : 20));
    printT(n);
  }
  // la liga: con las 8 medallas
  bool liga = pet.leagueOpen(gyRegion), campeon = pet.leagueChampion(gyRegion);
  int y = GL_Y0 + 4 * GL_H + 4;
  gfx->fillRoundRect(CX - 110, y, 220, 42, 14, campeon ? 0xE5CC : (liga ? 0x8B9A : UI_TRACK));
  gfx->setTextColor(campeon || liga ? UI_WHITE : UI_HINT);  // apagado: gris sobre gris claro
  setSize(2);
  const char *lt = T(campeon ? S_CHAMPION : S_LEAGUE);
  setCur(CX - textW(lt, 2) / 2, y + 13);
  printT(lt);
  gfx->setTextColor(UI_HINT);
  setCur(centerX(T(S_GYMS_HINT), 2), 396);
  printT(T(S_GYMS_HINT));
  drawToast();
}

static void gymListTap(int16_t x, int16_t y) {
  if (y >= GL_Y0 && y < GL_Y0 + 4 * GL_H) {
    int row = (y - GL_Y0) / GL_H, k = row * 2 + (x >= CX ? 1 : 0);
    gySel = k;
    gyLeague = false;
    gyStage = GY_DETAIL;
    sfxPlay(SFX_TAP);
  } else if (y >= GL_Y0 + 4 * GL_H && y < GL_Y0 + 4 * GL_H + 50 && abs(x - CX) < 110) {
    if (!pet.leagueOpen(gyRegion)) { toast(T(S_LEAGUE_LOCKED)); return; }
    gySel = 8;
    gyLeague = true;
    lgMember = 0;
    gyStage = GY_DETAIL;
    sfxPlay(SFX_TAP);
  }
}

// ---------- ficha de un gimnasio (o de la liga) ----------

// por que no se puede retar ahora (nullptr = si se puede)
static const char *gymBlocked() {
  uint8_t granja = gyFriend >= 0 ? 1 : 2;  // con un amigo, el tercero lo pone el
  if (gyPractice) return farmCount() < granja ? T(S_NEED_FARM2) : nullptr;
  const char *s = sportsBlocked();
  if (s) return s;
  if (gyRegion >= pet.unlockedRegions()) return T(S_REGION_LOCKED);
  if (gyLeague) {
    if (!pet.leagueOpen(gyRegion)) return T(S_LEAGUE_LOCKED);
    if (pet.leagueTriedToday(gyRegion)) return T(S_GYM_TRIED);
  } else {
    if (!pet.gymOpen(gyRegion, gySel)) return T(S_GYM_NEED_BADGE);
    if (pet.gymTriedToday(gyRegion, gySel)) return T(S_GYM_TRIED);
  }
  if (farmCount() < granja) return T(S_NEED_FARM2);
  return nullptr;
}

static void drawTeamThumbs(const int16_t dex[3], int y) {
  for (int i = 0; i < 3; i++) {
    const uint8_t *t = thumbs.get(dex[i]);
    if (t) drawThumb(t, CX - 140 + i * 100, y, 2, false);
  }
}

static void renderGymDetail() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  const GymPerson &p = foe();
  uint8_t type = p.type < TY_COUNT ? p.type : TY_NORMAL;
  char t[64];
  if (gyLeague) {  // liga: quien toca (alto mando 1-4, luego campeon)
    snprintf(t, sizeof(t), "%s  %u/%u", T(lgMember < 4 ? S_ELITE : S_CHAMP_ROLE), (unsigned)lgMember + 1,
             (unsigned)LEAGUE_SIZE);
  } else {
    const GymInfo &g = GYMS[gyRegion][gySel];
    snprintf(t, sizeof(t), "%s", gtx(g.cityEn, g.cityZh));
  }
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(t, 2), 34);
  printT(t);
  setSize(3);
  setCur(centerX(gtx(p.en, p.zh), 3), 60);
  printT(gtx(p.en, p.zh));
  if (p.type < TY_COUNT) {
    gfx->setTextColor(TYPE_COL[type]);
    setSize(2);
    setCur(centerX(T((StrId)(S_TY_NORMAL + type)), 2), 98);
    printT(T((StrId)(S_TY_NORMAL + type)));
  }
  if (!gyLeague) {  // su medalla
    const GymInfo &g = GYMS[gyRegion][gySel];
    bool won = pet.hasBadge(gyRegion, gySel);
    drawBadge(CX - 90, 140, 14, type, true);
    gfx->setTextColor(won ? UI_BAR_OK : UI_INK);
    setSize(2);
    setCur(CX - 68, 132);
    printT(gtx(g.badgeEn, g.badgeZh));
  }
  // sus tres pruebas y sus tres pokemon
  gymEvents(type, gyEv);
  for (int i = 0; i < 3; i++) {
    gfx->fillRoundRect(CX - 186 + i * 126, 166, 120, 30, 10, EV_COL[gyEv[i]]);
    gfx->setTextColor(UI_WHITE);
    setSize(2);
    const char *n = T(EV_NAME[gyEv[i]]);
    int ts = textW(n, 2) <= 114 ? 2 : 1;
    setSize(ts);
    setCur(CX - 126 + i * 126 - textW(n, ts) / 2, 174);
    printT(n);
  }
  drawTeamThumbs(p.team, 206);
  const char *no = gymBlocked();
  bool repetir = !gyLeague && pet.hasBadge(gyRegion, gySel);
  gfx->fillRoundRect(CX - 100, 300, 200, 46, 14, no ? UI_TRACK : UI_BAR_OK);
  gfx->setTextColor(no ? UI_HINT : UI_WHITE);
  setSize(2);
  const char *b = T(repetir ? S_REMATCH : S_CHALLENGE);
  setCur(CX - textW(b, 2) / 2, 315);
  printT(b);
  if (no) {
    gfx->setTextColor(UI_BAR_BAD);
    setCur(centerX(no, 2), 360);
    printT(no);
  }
  drawToast();
}

// a elegir de la granja: en el orden de la lista (los favoritos primero), fijo mientras se elige
static void openTeam() {
  if (!gyOrder) gyOrder = (int *)ps_malloc(PET_DEX_MAX * sizeof(int));
  gyOrderN = gyOrder ? farmFavOrder(gyOrder) : 0;
  farmPick[0] = farmPick[1] = -1;
  gyPage = 0;
  gyStage = GY_TEAM;
}

static void gymDetailTap(int16_t x, int16_t y) {
  if (y < 300 || y > 346 || abs(x - CX) > 100) {  // fuera del boton: a la lista
    gyStage = GY_LIST;
    return;
  }
  const char *no = gymBlocked();
  if (no) { toast(no); return; }
  sfxPlay(SFX_TAP);
  if (gyFriend >= 0) {  // con un amigo: se le pregunta y se espera a que traiga su compañero
    FInvite v;
    v.kind = FK_GYM;
    v.a = gyRegion;
    v.b = gyLeague ? 8 : gySel;
    if (!friendInvite(gyFriend, v)) { toast(T(S_FR_GONE)); return; }
    gyStage = GY_WAIT;
    gyWaitAt = millis();
    return;
  }
  openTeam();
}

// ---------- equipo: el compañero y dos de la granja ----------

#define FT_X0 (CX - 3 * 46)
#define FT_Y0 150
#define FT_CELL 92
static int farmPages9() { return (farmCount() + 5) / 6; }  // 6 por pagina (3 x 2)
// la casilla k (contando las paginas de antes) -> su indice en la granja (-1: ninguno)
static int teamAt(int k) { return k < gyOrderN && gyOrder[k] < farmCount() ? gyOrder[k] : -1; }

// en una casilla de lado w: su tipo (punto, abajo a la izquierda) y una estrella (arriba a la
// derecha) si su tipo tiene ventaja en alguna de las pruebas de este gimnasio
static void drawFitMarks(int16_t dex, int x, int y, int w) {
  uint8_t ty = typeOf(dex);
  gfx->fillCircle(x + 12, y + w - 12, 6, TYPE_COL[ty]);
  gfx->drawCircle(x + 12, y + w - 12, 6, UI_INK);
  for (int i = 0; i < 3; i++)
    if (typeBonus(gyEv[i], ty) > 1.0f) {
      drawMap(SPR_ICON_STAR, 16, x + w - 22, y + 4, 1, false);
      break;
    }
}

static void renderGymTeam() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  bool juntos = gyFriend >= 0;
  const char *h = T(juntos ? S_PICK_ONE : S_PICK_TWO);
  setSize(3);
  setCur(centerX(h, 3), 34);
  printT(h);
  // el compañero siempre va (y con un amigo, el suyo)
  int xc = juntos ? CX - 88 : CX - 32;
  const uint8_t *t = thumbs.get(pet.speciesId);
  if (t) drawThumb(t, xc, 74, 2, false);
  drawFitMarks(pet.speciesId, xc - 8, 74, 80);
  if (juntos) {
    const uint8_t *f = thumbs.get(gyFriendAth.dex);
    if (f) drawThumb(f, CX + 8, 74, 2, false);
    drawFitMarks(gyFriendAth.dex, CX, 74, 80);
  }
  for (int i = 0; i < 6; i++) {
    int idx = teamAt(gyPage * 6 + i);
    if (idx < 0) continue;
    int x = FT_X0 + (i % 3) * FT_CELL, y = FT_Y0 + (i / 3) * FT_CELL;
    bool sel = farmPick[0] == idx || farmPick[1] == idx;
    if (sel) gfx->fillRoundRect(x, y, FT_CELL - 6, FT_CELL - 6, 12, 0xB6F2);
    gfx->drawRoundRect(x, y, FT_CELL - 6, FT_CELL - 6, 12, sel ? UI_BAR_OK : UI_TRACK);
    const uint8_t *th = thumbs.get(farmAt(idx).speciesId);
    if (th) drawThumbIn(th, x, y, 2, false, FT_CELL - 6);
    drawFitMarks(farmAt(idx).speciesId, x, y, FT_CELL - 6);
    if (pet.isFav(farmAt(idx).speciesId)) drawFavHeart(x + 12, y + 12, 6, true);
  }
  bool listo = farmPick[0] >= 0 && (juntos || farmPick[1] >= 0);
  gfx->fillRoundRect(CX - 90, 340, 180, 42, 14, listo ? UI_BAR_OK : UI_TRACK);
  gfx->setTextColor(listo ? UI_WHITE : UI_HINT);
  setSize(2);
  setCur(CX - textW(T(S_NEXT), 2) / 2, 353);
  printT(T(S_NEXT));
  drawToast();
}

// el atleta de uno de la granja: sus genes y su entrenamiento (de su ficha en la SD)
static Athlete farmAthlete(int idx) {
  PetRec r;
  Athlete a = athleteOf(farmAt(idx).speciesId);
  if (farmPeek(farmAt(idx).speciesId, r)) {
    a.geneAtk = r.geneAtk;
    a.geneDef = r.geneDef;
    a.geneSpe = r.geneSpe;
    a.trAtk = r.trAtk;
    a.trDef = r.trDef;
    a.trSpe = r.trSpe;
    a.mega = recIsMega(r, pet.lastSeenEpoch);
  }
  return a;
}

static void toOrder() {
  teamDex[0] = pet.speciesId;
  teamA[0] = petAthlete();
  for (int i = 0; i < 2; i++) {
    if (i == 1 && gyFriend >= 0) {  // el tercero es el compañero del amigo
      teamDex[2] = gyFriendAth.dex;
      teamA[2] = friendAthlete(gyFriendAth);
      continue;
    }
    teamDex[i + 1] = farmAt(farmPick[i]).speciesId;
    teamA[i + 1] = farmAthlete(farmPick[i]);
  }
  const GymPerson &p = foe();
  gymEvents(p.type < TY_COUNT ? p.type : TY_NORMAL, gyEv);
  Athlete f[3] = { foeAthlete(0), foeAthlete(1), foeAthlete(2) };
  bestAssign(f, gyEv, foePerm);      // el lider pone a cada uno donde mejor va
  bestAssign(teamA, gyEv, gyPerm);   // y al jugador se le propone lo mismo (lo puede cambiar)
  gyStage = GY_ORDER;
}

static void gymTeamTap(int16_t x, int16_t y) {
  if (y >= 340 && y <= 382 && abs(x - CX) < 90) {
    if (farmPick[0] < 0 || (gyFriend < 0 && farmPick[1] < 0)) { toast(T(gyFriend >= 0 ? S_PICK_ONE : S_PICK_TWO)); return; }
    sfxPlay(SFX_TAP);
    toOrder();
    return;
  }
  if (x < FT_X0 || y < FT_Y0) return;
  int c = (x - FT_X0) / FT_CELL, r = (y - FT_Y0) / FT_CELL;
  if (c > 2 || r > 1) return;
  int idx = teamAt(gyPage * 6 + r * 3 + c);
  if (idx < 0) return;
  sfxPlay(SFX_TAP);
  if (gyFriend >= 0) {  // con un amigo solo se elige uno
    farmPick[0] = farmPick[0] == idx ? -1 : idx;
    return;
  }
  if (farmPick[0] == idx) farmPick[0] = -1;       // otra vez: se quita
  else if (farmPick[1] == idx) farmPick[1] = -1;
  else if (farmPick[0] < 0) farmPick[0] = idx;
  else if (farmPick[1] < 0) farmPick[1] = idx;
  else farmPick[1] = idx;                          // ya habia dos: cambia el segundo
}

// ---------- alineacion: quien hace cada prueba ----------

static const char *memberName(int i) {
  if (i == 0) return pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  if (i == 2 && gyFriend >= 0) return dexName(gyFriendAth.dex);
  const FarmEntry &e = farmAt(farmPick[i - 1]);
  return e.nick[0] ? e.nick : dexName(e.speciesId);
}

#define OR_Y0 92
#define OR_H 74
static void renderGymOrder() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  char t[64];
  if (gyLeague) snprintf(t, sizeof(t), "%s  %s", T(S_LINEUP), gtx(foe().en, foe().zh));
  else snprintf(t, sizeof(t), "%s", T(S_LINEUP));
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(t, 2), 40);
  printT(t);
  setCur(centerX(T(S_LINEUP_HINT), 2), 66);
  gfx->setTextColor(UI_HINT);
  printT(T(S_LINEUP_HINT));
  for (int i = 0; i < 3; i++) {
    int y = OR_Y0 + i * OR_H;
    gfx->fillRoundRect(CX - 175, y, 350, OR_H - 6, 12, UI_WHITE);
    gfx->drawRoundRect(CX - 175, y, 350, OR_H - 6, 12, UI_TRACK);
    gfx->fillRoundRect(CX - 60, y + 6, 120, 26, 8, EV_COL[gyEv[i]]);
    gfx->setTextColor(UI_WHITE);
    const char *n = T(EV_NAME[gyEv[i]]);
    int ts = textW(n, 2) <= 114 ? 2 : 1;
    setSize(ts);
    setCur(CX - textW(n, ts) / 2, y + 12);
    printT(n);
    const uint8_t *mine = thumbs.get(teamDex[gyPerm[i]]), *his = thumbs.get(foe().team[foePerm[i]]);
    if (mine) drawThumb(mine, CX - 170, y + 2, 2, false);
    if (his) drawThumb(his, CX + 106, y + 2, 2, false);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(CX - textW("vs", 2) / 2, y + 40);
    printT("vs");
  }
  gfx->fillRoundRect(CX - 90, OR_Y0 + 3 * OR_H + 4, 180, 42, 14, UI_BAR_OK);
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(CX - textW(T(S_START), 2) / 2, OR_Y0 + 3 * OR_H + 17);
  printT(T(S_START));
  drawToast();
}

static void startRound();

static void gymOrderTap(int16_t x, int16_t y) {
  if (y >= OR_Y0 + 3 * OR_H && y <= OR_Y0 + 3 * OR_H + 50 && abs(x - CX) < 90) {  // empezar
    bool ok = gyPractice || (gyLeague ? (lgMember > 0 || pet.startLeague(gyRegion)) : pet.startGym(gyRegion, gySel));
    if (!ok) { toast(T(S_GYM_TRIED)); return; }
    sfxPlay(SFX_TAP);
    gyRound = gyWins = gyLost = 0;
    startRound();
    return;
  }
  if (y < OR_Y0 || y >= OR_Y0 + 3 * OR_H || x > CX - 60) return;
  int i = (y - OR_Y0) / OR_H;  // tocar el suyo: lo cambia por el de la siguiente prueba
  int j = (i + 1) % 3;
  uint8_t t = gyPerm[i];
  gyPerm[i] = gyPerm[j];
  gyPerm[j] = t;
  sfxPlay(SFX_TAP);
}

// ---------- la serie: tres pruebas, gana quien se lleve dos ----------

static char roundTitle[48];
static void startRound() {
  uint8_t ev = gyEv[gyRound], mine = gyPerm[gyRound], his = foePerm[gyRound];
  PmdMon *pm = &pmd;
  if (mine != 0) {  // uno de la granja (o el compañero del amigo)
    if (mine == 2 && gyFriend >= 0) rivalPmd[0].load(teamDex[mine], gyFriendAth.shiny);
    else rivalPmd[0].load(teamDex[mine], false, teamA[mine].mega);
    pm = &rivalPmd[0];
  }
  rivalPmd[1].load(foe().team[his], false);
  snprintf(roundTitle, sizeof(roundTitle), T(S_ROUND_FMT), gtx(foe().en, foe().zh), (unsigned)gyRound + 1);
  sportsRound(ev, pm, memberName(mine), teamA[mine], &rivalPmd[1], dexName(foe().team[his]), foeAthlete(his),
              roundTitle, gyFriend >= 0, nullptr);
  if (gyFriend >= 0) {  // el amigo ve la misma prueba, con las mismas marcas
    FRound r;
    r.round = gyRound;
    r.ev = ev;
    r.wins = gyWins;
    r.lost = gyLost;
    r.league = gyLeague;
    r.member = lgMember;
    r.region = gyRegion;
    r.gym = gyLeague ? 8 : gySel;
    r.mineDex = teamDex[mine];
    r.foeDex = foe().team[his];
    r.mineShiny = mine == 0 ? pet.shiny : mine == 2 ? gyFriendAth.shiny : 0;
    r.mineHelper = mine == 2;
    sportsMarks(r.marks);
    friendSendRound(gyFriend, r);
  }
}

// el marcador de la serie, en el resultado de cada prueba (ya contando esta)
void gymScoreLine(int y, bool won) {
  char t[48];
  snprintf(t, sizeof(t), "%s %u : %u %s", T(S_YOU), (unsigned)(gyWins + won), (unsigned)(gyLost + !won),
           gtx(foe().en, foe().zh));
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(t, 2), y);
  printT(t);
}

void gymRoundDone(bool won) {
  if (won) gyWins++;
  else gyLost++;
  if (gyWins < 2 && gyLost < 2) {  // aun no se ha decidido: la siguiente
    gyRound++;
    startRound();
    return;
  }
  gyWon = gyWins >= 2;
  gyItem = 0xFF;
  gyNewBadge = false;
  if (gyPractice) {
    if (gyLeague && gyWon && lgMember < LEAGUE_SIZE - 1) {
      lgMember++;
      toOrder();
      return;
    }
  } else if (gyLeague) {
    if (gyWon && lgMember < LEAGUE_SIZE - 1) {  // siguiente del alto mando (o el campeon)
      lgMember++;
      toOrder();
      return;
    }
    if (gyWon) gyNewBadge = pet.winLeague(gyRegion);
  } else if (gyWon) {
    gyNewBadge = pet.winGym(gyRegion, gySel, gyItem);
  }
  if (!gyPractice) pet.sportsDone(gyWon);
  sfxPlay(gyWon ? SFX_MEDAL : SFX_TAP);
  gyEndAt = millis();
  gyStage = GY_END;
  if (gyFriend >= 0) {  // el amigo: como acabo (el no se lleva medalla; su compañero se alegra)
    FGymEnd e;
    e.won = gyWon;
    e.region = gyRegion;
    e.gym = gyLeague ? 8 : gySel;
    e.member = lgMember;
    friendSendGymEnd(gyFriend, e);
  }
  Serial.printf("gimnasio r=%u g=%u liga=%d %s medalla=%d regalo=%d\n", gyRegion, gySel, gyLeague, gyWon ? "gana" : "pierde",
                gyNewBadge, gyItem);
}

static void renderGymEnd() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  char t[64];
  if (!gyWon) {  // con quien se perdio, el marcador y que probar la proxima vez
    gfx->setTextColor(UI_INK);
    setSize(2);
    const char *m = T(gyLeague ? S_LEAGUE_LOST : S_GYM_LOST);
    setCur(centerX(m, 2), 84);
    printT(m);
    drawTeamThumbs(foe().team, 120);
    snprintf(t, sizeof(t), "%s %u : %u %s", T(S_YOU), (unsigned)gyWins, (unsigned)gyLost, gtx(foe().en, foe().zh));
    setCur(centerX(t, 2), 212);
    printT(t);
    gfx->setTextColor(UI_HINT);
    setCur(centerX(T(S_GYM_TIP1), 2), 262);
    printT(T(S_GYM_TIP1));
    setCur(centerX(T(S_GYM_TIP2), 2), 286);
    printT(T(S_GYM_TIP2));
  } else if (gyLeague) {
    drawBadge(CX, 150, 40, TY_DRAGON, true);
    gfx->setTextColor(0xBC23);  // oro oscuro: se lee sobre el fondo claro
    setSize(3);
    setCur(centerX(T(S_LEAGUE_WON), 3), 220);
    printT(T(S_LEAGUE_WON));
    if (gyNewBadge) {  // primera vez: huevo legendario y amuleto iris
      gfx->setTextColor(UI_BAR_OK);
      setSize(2);
      for (int k = 0; k < 2; k++) {
        snprintf(t, sizeof(t), T(S_ALSO_GOT_FMT), T(ITEM_NAME[k ? ITEM_SHINY_CHARM : ITEM_EGG_LEGEND]));
        setCur(centerX(t, 2), 270 + k * 28);
        printT(t);
      }
    }
  } else {
    const GymInfo &g = GYMS[gyRegion][gySel];
    drawBadge(CX, 150, 40, g.leader.type, true);
    snprintf(t, sizeof(t), T(S_GOT_BADGE_FMT), gtx(g.badgeEn, g.badgeZh));
    gfx->setTextColor(UI_INK);
    int ts = gyNewBadge && textW(t, 3) <= 380 ? 3 : 2;
    setSize(ts);
    setCur(centerX(t, ts), 220);
    printT(t);
    if (gyItem != 0xFF) {
      snprintf(t, sizeof(t), T(S_ALSO_GOT_FMT), T(ITEM_NAME[gyItem]));
      gfx->setTextColor(UI_BAR_OK);
      setSize(2);
      setCur(centerX(t, 2), 270);
      printT(t);
    }
  }
  gfx->setTextColor(UI_HINT);
  setSize(2);
  setCur(centerX(T(S_TAP_NEXT), 2), 372);
  printT(T(S_TAP_NEXT));
}

// consola: la ficha de un gimnasio (k = 8: la liga) para probar, sin requisitos ni premios
void gymsPractice(uint8_t r, uint8_t k) {
  if (r >= REGION_COUNT || k > 8) return;
  hubOpen = gymsOpen = true;
  gyPractice = true;
  gyFriend = -1;
  gyRegion = r;
  gySel = k;
  gyLeague = k == 8;
  lgMember = 0;
  gyStage = GY_DETAIL;
}

// ---------- pantalla, toques y deslizar ----------

void renderGyms() {
  switch (gyStage) {
    case GY_LIST: renderGymList(); break;
    case GY_DETAIL: renderGymDetail(); break;
    case GY_TEAM: renderGymTeam(); break;
    case GY_ORDER: renderGymOrder(); break;
    case GY_WAIT:  // esperando al amigo (con un plazo)
      if (millis() - gyWaitAt > 35000) {
        toast(T(S_FR_GONE));
        gyStage = GY_DETAIL;
      }
      renderGymDetail();
      lkPanel(nullptr, T(S_FR_WAIT_FRIEND), nullptr);
      break;
    default: renderGymEnd();
  }
  gfx->flush();
}

void gymsTap(int16_t x, int16_t y) {
  switch (gyStage) {
    case GY_LIST: gymListTap(x, y); break;
    case GY_DETAIL: gymDetailTap(x, y); break;
    case GY_TEAM: gymTeamTap(x, y); break;
    case GY_ORDER: gymOrderTap(x, y); break;
    case GY_WAIT: break;
    default:
      if (millis() - gyEndAt < 1500) break;  // no con un toque que aun llega
      for (int i = 0; i < 2; i++) rivalPmd[i].unload();
      gyStage = GY_LIST;
  }
}

// en la lista: izquierda/derecha cambia de region (de la primera, a la derecha, se sale);
// en las demas pantallas, a la derecha se vuelve atras
void gymsSwipe(int dir) {
  if (gyStage == GY_WAIT) {  // ya no se espera al amigo
    if (dir > 0) {
      friendCancel(gyFriend);
      gyStage = GY_DETAIL;
    }
    return;
  }
  if (gyStage == GY_LIST) {
    int r = (int)gyRegion + (dir > 0 ? -1 : 1);
    if (r < 0) { closeGyms(); return; }
    if (r < REGION_COUNT) gyRegion = r;
    return;
  }
  if (dir <= 0) {
    if (gyStage == GY_TEAM && farmPages9() > 1) gyPage = (gyPage + 1) % farmPages9();
    return;
  }
  if (gyStage == GY_DETAIL || gyStage == GY_END) gyStage = GY_LIST;
  else if (gyStage == GY_TEAM) gyStage = GY_DETAIL;
  else if (gyStage == GY_ORDER && !(gyLeague && lgMember > 0)) gyStage = GY_TEAM;  // la liga, ya empezada, sigue
}

// ---------- con un amigo (friend_play.ino) ----------

bool gymsWithFriendNow() { return (gymsOpen && gyFriend >= 0) || gyHelper; }

void gymsWithFriend(uint8_t fi) {
  openGyms();
  gyFriend = fi;
  gyPractice = friendTestMode();  // consola FRIENDTEST: sin gastar el intento ni ganar nada
}

// yo invite: si viene, a elegir uno de la granja (el tercero es su compañero)
void gymsFriendAnswer(uint8_t fi, const FInviteAns &a) {
  if (!gymsOpen || gyFriend != (int8_t)fi || gyStage != GY_WAIT) return;
  if (a.answer != FW_OK) {
    toast(T(a.answer == FW_BUSY ? S_FR_BUSY : a.answer == FW_CANT ? S_FR_CANT : S_LINK_SAID_NO));
    gyStage = GY_DETAIL;
    return;
  }
  gyFriendAth = a.ath;
  openTeam();
}

// como sportsLeaveForFriend: en la lista, la ficha o el final con ese amigo se deja; si los dos
// esperan la respuesta del otro, cede el de numero mas alto (sin avisar: la invitacion del otro sigue)
bool gymsLeaveForFriend(uint8_t fi) {
  if (!gymsOpen || gyFriend != (int8_t)fi || fi >= linkFriendCount()) return false;
  bool cede = gyStage == GY_LIST || gyStage == GY_DETAIL || gyStage == GY_END ||
              (gyStage == GY_WAIT && linkSelfId() > linkFriendAt(fi).pid);
  if (!cede) return false;
  gyStage = GY_LIST;  // closeGyms no le manda "lo dejo"
  closeGyms();
  return true;
}

void gymsFriendGone(uint8_t fi) {
  if (gymsOpen && gyFriend == (int8_t)fi && gyStage == GY_WAIT) {
    toast(T(S_FR_GONE));
    gyStage = GY_DETAIL;
  }
}

// soy el amigo que ayuda: una prueba tal como la mando el (mi compañero sale como "YOU")
void gymsHelperRound(const FRound &r) {
  gyHelper = true;
  gyRegion = r.region < REGION_COUNT ? r.region : 0;
  gyLeague = r.gym >= 8;
  gySel = gyLeague ? 8 : r.gym;
  lgMember = r.member < LEAGUE_SIZE ? r.member : 0;
  gyRound = r.round;
  gyWins = r.wins;
  gyLost = r.lost;
  PmdMon *pm = &pmd;
  if (!r.mineHelper) {
    rivalPmd[0].unload();
    rivalPmd[0].load(r.mineDex, r.mineShiny);
    pm = &rivalPmd[0];
  }
  rivalPmd[1].unload();
  rivalPmd[1].load(r.foeDex, false);
  snprintf(roundTitle, sizeof(roundTitle), T(S_ROUND_FMT), gtx(foe().en, foe().zh), (unsigned)gyRound + 1);
  float m[2] = { r.marks[0], r.marks[1] };
  sportsRound(r.ev < EV_COUNT ? r.ev : 0, pm, r.mineHelper ? T(S_YOU) : dexName(r.mineDex), athleteOf(r.mineDex),
              &rivalPmd[1], dexName(r.foeDex), athleteOf(r.foeDex), roundTitle, true, m);
}

void gymsHelperEnd() { gyHelper = false; }
