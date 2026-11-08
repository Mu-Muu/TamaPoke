#include "chess.h"
#include <Preferences.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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
  for (JsonVariantConst g : archive["games"].as<JsonArrayConst>()) {
    uint32_t end = g["end_time"] | 0u;
    if (end <= st.watermark) continue;  // ya contada en otra sincronizacion
    if (!chessCountsTimeControl(g["time_control"] | "")) continue;
    const char *lado = nullptr;
    if (strcasecmp(g["white"]["username"] | "", CHESS_USER) == 0) lado = "white";
    else if (strcasecmp(g["black"]["username"] | "", CHESS_USER) == 0) lado = "black";
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
  p.end();
}
