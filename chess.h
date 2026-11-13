#pragma once
#include <stdint.h>
#include <ArduinoJson.h>

// Puntos de la tienda ganados jugando al ajedrez en chess.com (API publica,
// sin login). La placa sincroniza una vez al dia (medianoche): baja las stats y
// el archivo del mes, y suma lo nuevo desde la ultima vez.
//
// Partidas: solo las de 10 min o mas de base (y las diarias por correo); las
// de sin reloj ("-", contra los bots de chess.com) no cuentan. Ganar = 10,
// cualquier otro final = 1. Records: nueva mejor rapida o diaria +20, cada
// centena nueva en rapidas +50, record de tacticas +10, record de Puzzle Rush
// +10. La primera sincronizacion solo apunta las marcas (no paga el pasado).
//
// La cuenta se escribe en el portal Wi-Fi (NVS "chess"/"user"); si no hay,
// vale la de config_private.h (CHESS_USER). Al cambiar de cuenta se vuelve a
// empezar como la primera vez.

#include <stddef.h>
#include "config.h"  // CHESS_USER

#define CHESS_USER_MIN 3   // usuarios de chess.com: de 3 a 25 caracteres
#define CHESS_USER_MAX 25

#define PTS_GAME 1
#define PTS_WIN 10
#define PTS_BEST_RATING 20
#define PTS_HUNDRED 50
#define PTS_TACTICS_BEST 10
#define PTS_RUSH_BEST 10
#define CHESS_MIN_BASE_SECS 600

// lo ya contado (se guarda en la NVS)
struct ChessState {
  uint32_t watermark = 0;  // end_time (UTC) de la ultima partida contada
  bool baseline = false;   // ya se tomaron las marcas iniciales de las stats
  uint16_t bestRapid = 0, bestDaily = 0, hundredRapid = 0, tacticsHigh = 0, rushBest = 0;
  char owner[CHESS_USER_MAX + 1] = "";  // cuenta de la que son estas marcas ("" = de antes de apuntarla)
};

// resumen de una sincronizacion (se enseña al dia siguiente)
struct ChessReport {
  uint16_t games = 0, wins = 0;
  int32_t gamePts = 0, bonusPts = 0;
  bool newBestRapid = false, newBestDaily = false, newTactics = false, newRush = false;
  uint16_t hundreds = 0;   // centenas nuevas superadas en rapidas
  uint32_t newestEnd = 0;  // end_time de la partida contada mas reciente
  int32_t total() const { return gamePts + bonusPts; }
};

// usuario tal como se escribio -> en minusculas en out (cap >= CHESS_USER_MAX + 1).
// Solo letras, numeros, _ y -, de CHESS_USER_MIN a CHESS_USER_MAX. false si no vale.
bool chessNormUser(const char *in, char *out, size_t cap);
// la cuenta que se sincroniza: la del portal o, si no hay, CHESS_USER. false si ninguna
bool chessAccount(char *out, size_t cap);
inline bool chessHasAccount() {
  char u[CHESS_USER_MAX + 1];
  return chessAccount(u, sizeof(u));
}
// la del portal (ya normalizada); "" la borra y vuelve a valer CHESS_USER
void chessSaveUser(const char *user);
// antes de sincronizar. Si las marcas son de otra cuenta, se empieza otra vez
// como la primera vez (solo se apuntan las marcas, sin pagarlas). Sin marcas
// tomadas se cuenta desde todayUtc (la medianoche local de hoy, en UTC), pero
// la marca de agua nunca retrocede: lo ya contado no se paga dos veces. Las
// marcas de antes de apuntar la cuenta eran de legacyOwner (CHESS_USER).
// true si la cuenta ha cambiado.
bool chessPrepare(ChessState &st, const char *user, const char *legacyOwner, uint32_t todayUtc);

// ¿cuenta una partida con este time_control? ("600", "900+10", "1/259200", "-")
bool chessCountsTimeControl(const char *tc);
// filtro de ArduinoJson para el archivo mensual (solo los campos que se usan)
void chessArchiveFilter(JsonDocument &filter);
// suma las partidas nuevas (end_time > st.watermark) de st.owner en un archivo mensual
void chessScoreArchive(JsonVariantConst archive, const ChessState &st, ChessReport &rep);
// marcas de /stats: la primera vez solo las apunta; luego paga los records
void chessScoreStats(JsonVariantConst stats, ChessState &st, ChessReport &rep);
// tras sumar los puntos: avanza la marca de agua para no contar dos veces
void chessCommit(ChessState &st, const ChessReport &rep);

void chessLoad(ChessState &st);
void chessSave(const ChessState &st);
// lo de la ultima sincronizacion buena (para contarlo despues, p. ej. Stack-chan);
// day = dia local (epoch de pared / 86400). false si aun no hubo ninguna.
void chessSaveReport(const ChessReport &rep, uint32_t day);
bool chessLoadReport(ChessReport &rep, uint32_t &day);
