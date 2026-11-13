#include "chess.h"
#include <Preferences.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

bool chessNormUser(const char *in, char *out, size_t cap) {
  if (!out || !cap) return false;
  out[0] = 0;
  if (!in || cap < CHESS_USER_MAX + 1) return false;
  size_t n = 0;
  for (; in[n]; n++) {
    char c = in[n];
    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    bool vale = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
    if (!vale || n >= CHESS_USER_MAX) {
      out[0] = 0;
      return false;
    }
    out[n] = c;
  }
  out[n] = 0;
  if (n >= CHESS_USER_MIN) return true;
  out[0] = 0;
  return false;
}

bool chessAccount(char *out, size_t cap) {
  char guardado[CHESS_USER_MAX + 2] = "";
  Preferences p;
  if (p.begin("chess", true)) {
    p.getString("user", guardado, sizeof(guardado));
    p.end();
  }
  return chessNormUser(guardado, out, cap) || chessNormUser(CHESS_USER, out, cap);
}

void chessSaveUser(const char *user) {
  Preferences p;
  p.begin("chess", false);
  if (user && user[0]) p.putString("user", user);
  else p.remove("user");
  p.end();
}

bool chessPrepare(ChessState &st, const char *user, const char *legacyOwner, uint32_t todayUtc) {
  char viejo[CHESS_USER_MAX + 1];
  const char *antes = st.owner;
  if (!antes[0]) {  // marcas de antes de apuntar la cuenta: eran de CHESS_USER
    chessNormUser(legacyOwner, viejo, sizeof(viejo));
    antes = viejo;
  }
  bool cambia = strcmp(antes, user ? user : "") != 0;
  if (cambia) {  // marcas de otra cuenta: fuera (la marca de agua se queda)
    uint32_t wm = st.watermark;
    st = ChessState();
    st.watermark = wm;
  }
  strncpy(st.owner, user ? user : "", sizeof(st.owner) - 1);
  st.owner[sizeof(st.owner) - 1] = 0;
  if (!st.baseline && st.watermark < todayUtc) st.watermark = todayUtc;
  return cambia;
}

bool chessCountsTimeControl(const char *tc) {
  if (!tc || !tc[0] || tc[0] == '-') return false;  // sin reloj: bots
  if (strncmp(tc, "1/", 2) == 0) return true;        // diaria (dias por jugada)
  return atoi(tc) >= CHESS_MIN_BASE_SECS;            // "600", "900+10"...
}

void chessArchiveFilter(JsonDocument &filter) {
  JsonObject g = filter["games"].add<JsonObject>();
  g["end_time"] = true;
  g["time_control"] = true;
  g["white"]["username"] = true;
  g["white"]["result"] = true;
  g["black"]["username"] = true;
  g["black"]["result"] = true;
}

void chessScoreArchive(JsonVariantConst archive, const ChessState &st, ChessReport &rep) {
  if (!st.owner[0]) return;  // sin cuenta no hay partidas suyas
  for (JsonVariantConst g : archive["games"].as<JsonArrayConst>()) {
    uint32_t end = g["end_time"] | 0u;
    if (end <= st.watermark) continue;  // ya contada en otra sincronizacion
    if (!chessCountsTimeControl(g["time_control"] | "")) continue;
    const char *lado = nullptr;
    if (strcasecmp(g["white"]["username"] | "", st.owner) == 0) lado = "white";
    else if (strcasecmp(g["black"]["username"] | "", st.owner) == 0) lado = "black";
    if (!lado) continue;
    bool gana = strcmp(g[lado]["result"] | "", "win") == 0;
    rep.games++;
    if (gana) rep.wins++;
    rep.gamePts += gana ? PTS_WIN : PTS_GAME;
    if (end > rep.newestEnd) rep.newestEnd = end;
  }
}

void chessScoreStats(JsonVariantConst stats, ChessState &st, ChessReport &rep) {
  uint16_t rapid = stats["chess_rapid"]["best"]["rating"] | 0;
  uint16_t daily = stats["chess_daily"]["best"]["rating"] | 0;
  uint16_t tactics = stats["tactics"]["highest"]["rating"] | 0;
  uint16_t rush = stats["puzzle_rush"]["best"]["score"] | 0;
  if (!st.baseline) {  // primera vez: los records de antes no se pagan
    st.bestRapid = rapid;
    st.bestDaily = daily;
    st.hundredRapid = rapid / 100 * 100;
    st.tacticsHigh = tactics;
    st.rushBest = rush;
    st.baseline = true;
    return;
  }
  if (rapid > st.bestRapid) {
    rep.newBestRapid = true;
    rep.bonusPts += PTS_BEST_RATING;
    st.bestRapid = rapid;
  }
  if (daily > st.bestDaily) {
    rep.newBestDaily = true;
    rep.bonusPts += PTS_BEST_RATING;
    st.bestDaily = daily;
  }
  while (st.hundredRapid + 100 <= rapid) {  // cada centena nueva: 1000, 1100...
    st.hundredRapid += 100;
    rep.hundreds++;
    rep.bonusPts += PTS_HUNDRED;
  }
  if (tactics > st.tacticsHigh) {
    rep.newTactics = true;
    rep.bonusPts += PTS_TACTICS_BEST;
    st.tacticsHigh = tactics;
  }
  if (rush > st.rushBest) {
    rep.newRush = true;
    rep.bonusPts += PTS_RUSH_BEST;
    st.rushBest = rush;
  }
}

void chessCommit(ChessState &st, const ChessReport &rep) {
  if (rep.newestEnd > st.watermark) st.watermark = rep.newestEnd;
}

void chessLoad(ChessState &st) {
  Preferences p;
  p.begin("chess", true);
  st.watermark = p.getUInt("wm", 0);
  st.baseline = p.getBool("base", false);
  st.bestRapid = p.getUShort("rap", 0);
  st.bestDaily = p.getUShort("day", 0);
  st.hundredRapid = p.getUShort("hun", 0);
  st.tacticsHigh = p.getUShort("tac", 0);
  st.rushBest = p.getUShort("rush", 0);
  st.owner[0] = 0;
  p.getString("own", st.owner, sizeof(st.owner));
  p.end();
}

void chessSaveReport(const ChessReport &rep, uint32_t day) {
  Preferences p;
  p.begin("chess", false);
  p.putUInt("rday", day);
  p.putUShort("rg", rep.games);
  p.putUShort("rw", rep.wins);
  p.putInt("rp", rep.gamePts);
  p.putInt("rb", rep.bonusPts);
  p.end();
}

bool chessLoadReport(ChessReport &rep, uint32_t &day) {
  Preferences p;
  p.begin("chess", true);
  day = p.getUInt("rday", 0);
  rep = ChessReport();
  rep.games = p.getUShort("rg", 0);
  rep.wins = p.getUShort("rw", 0);
  rep.gamePts = p.getInt("rp", 0);
  rep.bonusPts = p.getInt("rb", 0);
  p.end();
  return day != 0;
}

void chessSave(const ChessState &st) {
  Preferences p;
  p.begin("chess", false);
  p.putUInt("wm", st.watermark);
  p.putBool("base", st.baseline);
  p.putUShort("rap", st.bestRapid);
  p.putUShort("day", st.bestDaily);
  p.putUShort("hun", st.hundredRapid);
  p.putUShort("tac", st.tacticsHigh);
  p.putUShort("rush", st.rushBest);
  p.putString("own", st.owner);
  p.end();
}
