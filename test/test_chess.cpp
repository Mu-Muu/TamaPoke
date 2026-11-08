// Tests de las reglas de puntos de ajedrez (chess.cpp) con JSON como el de
// la API publica de chess.com.
#include "framework.h"
#include "shim/Arduino.h"
#include "shim/Preferences.h"
#include "../chess.h"
#include <ctype.h>
#include <string>

// un archivo mensual en miniatura: lo justo de cada partida
// el jugador es CHESS_USER (config.h): @ME@ se cambia por el y @ME_UP@ por el
// mismo en mayusculas (se compara sin mayusculas)
static const char *ARCHIVO_T = R"({"games":[
  {"end_time":1000,"time_control":"600","white":{"username":"@ME_UP@","result":"win"},
   "black":{"username":"rival1","result":"checkmated"}},
  {"end_time":2000,"time_control":"600","white":{"username":"rival2","result":"win"},
   "black":{"username":"@ME@","result":"resigned"}},
  {"end_time":3000,"time_control":"900+10","white":{"username":"rival3","result":"agreed"},
   "black":{"username":"@ME@","result":"agreed"}},
  {"end_time":4000,"time_control":"300","white":{"username":"@ME@","result":"win"},
   "black":{"username":"rival4","result":"timeout"}},
  {"end_time":5000,"time_control":"-","white":{"username":"@ME@","result":"win"},
   "black":{"username":"Coach-BotezSisters","result":"resigned"}},
  {"end_time":6000,"time_control":"1/259200","white":{"username":"@ME@","result":"win"},
   "black":{"username":"rival5","result":"resigned"}}
]})";

static std::string conJugador(const char *t) {
  std::string s(t), yo(CHESS_USER), yoUp(CHESS_USER);
  for (char &c : yoUp) c = (char)toupper((unsigned char)c);
  for (size_t i; (i = s.find("@ME_UP@")) != std::string::npos;) s.replace(i, 7, yoUp);
  for (size_t i; (i = s.find("@ME@")) != std::string::npos;) s.replace(i, 4, yo);
  return s;
}
static const std::string ARCHIVO_S = conJugador(ARCHIVO_T);
static const char *ARCHIVO = ARCHIVO_S.c_str();

static void cargar(JsonDocument &doc, const char *json) {
  JsonDocument filtro;
  chessArchiveFilter(filtro);
  DeserializationError e = deserializeJson(doc, json, DeserializationOption::Filter(filtro));
  CHECK_MSG(!e, e.c_str());
}

TEST(chess, que_partidas_cuentan) {
  CHECK(chessCountsTimeControl("600"));
  CHECK(chessCountsTimeControl("600+5"));
  CHECK(chessCountsTimeControl("900+10"));
  CHECK(chessCountsTimeControl("1800"));
  CHECK(chessCountsTimeControl("1/259200"));  // diaria
  CHECK(!chessCountsTimeControl("300"));
  CHECK(!chessCountsTimeControl("180+2"));
  CHECK(!chessCountsTimeControl("599"));
  CHECK(!chessCountsTimeControl("-"));        // sin reloj: bots
  CHECK(!chessCountsTimeControl(""));
  CHECK(!chessCountsTimeControl(nullptr));
}

TEST(chess, ganar_10_y_cualquier_otro_final_1) {
  JsonDocument doc;
  cargar(doc, ARCHIVO);
  ChessState st;
  ChessReport rep;
  chessScoreArchive(doc.as<JsonVariantConst>(), st, rep);
  // cuentan: 600 gana (10), 600 pierde (1), 900+10 tablas (1), diaria gana (10)
  // no cuentan: la de 5 min ni la del bot
  CHECK_EQ(rep.games, (uint16_t)4);
  CHECK_EQ(rep.wins, (uint16_t)2);
  CHECK_EQ(rep.gamePts, (int32_t)22);
  CHECK_EQ(rep.newestEnd, (uint32_t)6000);
}

TEST(chess, no_cuenta_dos_veces_la_misma_partida) {
  JsonDocument doc;
  cargar(doc, ARCHIVO);
  ChessState st;
  ChessReport a;
  chessScoreArchive(doc.as<JsonVariantConst>(), st, a);
  chessCommit(st, a);
  CHECK_EQ(st.watermark, (uint32_t)6000);
  ChessReport b;
  chessScoreArchive(doc.as<JsonVariantConst>(), st, b);
  CHECK_EQ(b.games, (uint16_t)0);
  CHECK_EQ(b.total(), (int32_t)0);
  CHECK_EQ(st.watermark, (uint32_t)6000);  // sin partidas nuevas no retrocede
}

TEST(chess, solo_cuenta_lo_posterior_a_la_marca) {
  JsonDocument doc;
  cargar(doc, ARCHIVO);
  ChessState st;
  st.watermark = 2500;  // ya se contaron las dos primeras
  ChessReport rep;
  chessScoreArchive(doc.as<JsonVariantConst>(), st, rep);
  CHECK_EQ(rep.games, (uint16_t)2);  // tablas 900+10 y la diaria
  CHECK_EQ(rep.gamePts, (int32_t)11);
}

static void stats(JsonDocument &doc, int rapid, int daily, int tactics, int rush) {
  char json[256];
  snprintf(json, sizeof(json),
           R"({"chess_rapid":{"best":{"rating":%d}},"chess_daily":{"best":{"rating":%d}},)"
           R"("tactics":{"highest":{"rating":%d}},"puzzle_rush":{"best":{"score":%d}}})",
           rapid, daily, tactics, rush);
  deserializeJson(doc, json);
}

TEST(chess, la_primera_vez_solo_apunta_las_marcas) {
  JsonDocument doc;
  stats(doc, 978, 976, 1582, 27);
  ChessState st;
  ChessReport rep;
  chessScoreStats(doc.as<JsonVariantConst>(), st, rep);
  CHECK(st.baseline);
  CHECK_EQ(rep.bonusPts, (int32_t)0);
  CHECK_EQ(st.bestRapid, (uint16_t)978);
  CHECK_EQ(st.hundredRapid, (uint16_t)900);
}

TEST(chess, records_y_centenas_dan_puntos_una_vez) {
  JsonDocument doc;
  ChessState st;
  ChessReport base;
  stats(doc, 978, 976, 1582, 27);
  chessScoreStats(doc.as<JsonVariantConst>(), st, base);

  JsonDocument doc2;
  stats(doc2, 1012, 976, 1600, 28);  // pasa de 1000 en rapidas, record de tacticas y rush
  ChessReport rep;
  chessScoreStats(doc2.as<JsonVariantConst>(), st, rep);
  CHECK(rep.newBestRapid);
  CHECK(!rep.newBestDaily);
  CHECK(rep.newTactics);
  CHECK(rep.newRush);
  CHECK_EQ(rep.hundreds, (uint16_t)1);
  CHECK_EQ(rep.bonusPts, (int32_t)(PTS_BEST_RATING + PTS_HUNDRED + PTS_TACTICS_BEST + PTS_RUSH_BEST));

  ChessReport otra;  // mismas marcas: nada
  chessScoreStats(doc2.as<JsonVariantConst>(), st, otra);
  CHECK_EQ(otra.bonusPts, (int32_t)0);
}

TEST(chess, saltar_dos_centenas_paga_las_dos) {
  JsonDocument doc;
  ChessState st;
  ChessReport base;
  stats(doc, 950, 0, 0, 0);
  chessScoreStats(doc.as<JsonVariantConst>(), st, base);
  JsonDocument doc2;
  stats(doc2, 1105, 0, 0, 0);
  ChessReport rep;
  chessScoreStats(doc2.as<JsonVariantConst>(), st, rep);
  CHECK_EQ(rep.hundreds, (uint16_t)2);  // 1000 y 1100
  CHECK_EQ(st.hundredRapid, (uint16_t)1100);
}

TEST(chess, sin_partidas_diarias_no_falla) {
  JsonDocument doc;
  deserializeJson(doc, R"({"chess_rapid":{"best":{"rating":900}}})");
  ChessState st;
  ChessReport rep;
  chessScoreStats(doc.as<JsonVariantConst>(), st, rep);
  CHECK_EQ(st.bestDaily, (uint16_t)0);
  CHECK_EQ(st.tacticsHigh, (uint16_t)0);
}

TEST(chess, el_estado_se_guarda) {
  mockNvsReset();
  ChessState st;
  st.watermark = 1793922634u;
  st.baseline = true;
  st.bestRapid = 978;
  st.hundredRapid = 900;
  st.tacticsHigh = 1582;
  st.rushBest = 27;
  chessSave(st);
  ChessState q;
  chessLoad(q);
  CHECK_EQ(q.watermark, st.watermark);
  CHECK(q.baseline);
  CHECK_EQ(q.bestRapid, (uint16_t)978);
  CHECK_EQ(q.hundredRapid, (uint16_t)900);
  CHECK_EQ(q.tacticsHigh, (uint16_t)1582);
  CHECK_EQ(q.rushBest, (uint16_t)27);
}

TEST(chess, el_ultimo_resumen_se_guarda) {
  mockNvsReset();
  ChessReport r;
  uint32_t day = 0;
  CHECK(!chessLoadReport(r, day));
  ChessReport a;
  a.games = 3;
  a.wins = 2;
  a.gamePts = 21;
  a.bonusPts = 20;
  chessSaveReport(a, 20763);
  CHECK(chessLoadReport(r, day));
  CHECK_EQ(day, (uint32_t)20763);
  CHECK_EQ(r.games, (uint16_t)3);
  CHECK_EQ(r.wins, (uint16_t)2);
  CHECK_EQ(r.total(), (int32_t)41);
}
