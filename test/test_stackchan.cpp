// Herramientas de Stack-chan (stackchan.cpp): hacen la accion y contestan.
#include "framework.h"
#include "shim/Arduino.h"
#include "../stackchan.h"
#include "../chess.h"
#include "shim/Preferences.h"
#include <string.h>
#include <string>

static void nacer(Pet &p, int16_t dex) {
  mockNvsReset();
  mockSetMillis(0);
  p.begin();
  if (p.awaitingStarter()) p.chooseStarter(dex);
  p.eggTap(); p.eggTap(); p.eggTap();
}

static std::string usar(Pet &p, uint8_t tool, void (*bath)() = nullptr, uint8_t a = 0, uint8_t b = 0,
                        const char *granja = nullptr) {
  char out[800];
  StackchanCtx ctx;
  ctx.name = "CHARMANDER";
  ctx.bath = bath;
  ctx.farmList = granja;
  stackchanTool(p, tool, a, b, ctx, out, sizeof(out));
  return out;
}
static bool tiene(const std::string &s, const char *x) { return s.find(x) != std::string::npos; }

static bool empieza(const std::string &s, const char *pre) { return s.rfind(pre, 0) == 0; }

TEST(stackchan, dar_de_comer_y_contar_el_estado) {
  Pet p;
  nacer(p, 4);
  p.fullness = 50;
  std::string r = usar(p, MCP_FEED);
  CHECK_EQ(p.fullness, (uint8_t)75);
  CHECK_MSG(empieza(r, "喂了一颗树果。"), r);
  CHECK_MSG(r.find("CHARMANDER Lv.1") != std::string::npos, r);
  CHECK_MSG(r.find("饱食 75") != std::string::npos, r);
}

TEST(stackchan, dormido_no_come_y_se_le_puede_despertar) {
  Pet p;
  nacer(p, 4);
  p.fullness = 50;
  CHECK(empieza(usar(p, MCP_SLEEP), "关灯了"));
  CHECK(p.sleeping);
  CHECK(empieza(usar(p, MCP_FEED), "它在睡觉"));
  CHECK_EQ(p.fullness, (uint8_t)50);
  CHECK(empieza(usar(p, MCP_WAKE), "开灯了"));
  CHECK(!p.sleeping);
}

TEST(stackchan, sin_chuches_ni_medicina_lo_dice) {
  Pet p;
  nacer(p, 4);
  CHECK(empieza(usar(p, MCP_CANDY), "背包里没有糖果"));
  p.bag[ITEM_CANDY] = 1;
  usar(p, MCP_CANDY);
  CHECK_EQ(p.bag[ITEM_CANDY], (uint16_t)0);
  CHECK(empieza(usar(p, MCP_MEDICINE), "它没有生病"));
  p.sick = true;
  CHECK(empieza(usar(p, MCP_MEDICINE), "背包里没有药"));
  p.bag[ITEM_MEDICINE] = 1;
  usar(p, MCP_MEDICINE);
  CHECK(!p.sick);
}

static int banos = 0;
static void banar() { banos++; }

TEST(stackchan, el_bano_usa_la_animacion_del_sketch) {
  Pet p;
  nacer(p, 4);
  banos = 0;
  usar(p, MCP_BATH, banar);
  CHECK_EQ(banos, 1);
}

TEST(stackchan, huevo_y_sin_compañero) {
  mockNvsReset();
  mockSetMillis(0);
  Pet p;
  p.begin();
  p.chooseStarter(7);
  CHECK(empieza(usar(p, MCP_FEED), "现在还是一颗蛋"));
  usar(p, MCP_PET);
  usar(p, MCP_PET);
  CHECK(empieza(usar(p, MCP_PET), "摸了摸蛋，孵出来了"));
  CHECK(!p.isEgg());

  p.vacant = true;
  p.speciesId = -1;
  CHECK(empieza(usar(p, MCP_STATUS), "现在没有伙伴"));
}

TEST(stackchan, comprar_solo_lo_barato_y_con_puntos) {
  Pet p;
  nacer(p, 4);
  p.points = 7;
  JsonDocument d;
  deserializeJson(d, R"({"item":"candy","count":5})");
  uint8_t a, b;
  stackchanArgs(MCP_BUY, d.as<JsonVariantConst>(), a, b);
  CHECK_EQ(a, (uint8_t)ITEM_CANDY);
  CHECK_EQ(b, (uint8_t)5);
  std::string r = usar(p, MCP_BUY, nullptr, a, b);
  CHECK_EQ(p.bag[ITEM_CANDY], (uint16_t)3);  // 7 puntos dan para 3
  CHECK_EQ(p.points, (int32_t)1);
  CHECK_MSG(empieza(r, "买了 3 个糖果"), r);
  CHECK_MSG(tiene(r, "积分只够"), r);

  deserializeJson(d, R"({"item":"legend_egg"})");
  stackchanArgs(MCP_BUY, d.as<JsonVariantConst>(), a, b);
  CHECK(empieza(usar(p, MCP_BUY, nullptr, a, b), "语音只能买"));
  CHECK(empieza(usar(p, MCP_BUY, nullptr, ITEM_POTION_STR, 1), "积分不够"));
}

TEST(stackchan, pokedex_ajedrez_y_granja) {
  Pet p;
  nacer(p, 4);
  std::string r = usar(p, MCP_POKEDEX);
  CHECK_MSG(empieza(r, "图鉴登记了 1/1025 只"), r);
  CHECK_MSG(tiene(r, "关都 1/151"), r);
  CHECK_MSG(empieza(usar(p, MCP_CHESS), "还没有同步过"), "sin sincronizar");
  ChessReport rep;
  rep.games = 2;
  rep.wins = 1;
  rep.gamePts = 11;
  chessSaveReport(rep, 20763);  // 6 de noviembre de 2026
  r = usar(p, MCP_CHESS);
  CHECK_MSG(empieza(r, "最近一次同步是 11月6日：下了 2 盘"), r);
  CHECK(empieza(usar(p, MCP_FARM), "没有插 SD 卡"));
  CHECK(empieza(usar(p, MCP_FARM, nullptr, 0, 0, ""), "农场里还没有"));
  CHECK(empieza(usar(p, MCP_FARM, nullptr, 0, 0, "皮卡丘 Lv.3"), "农场里有：皮卡丘"));
}

TEST(stackchan, la_lista_de_herramientas) {
  JsonDocument d;
  JsonArray t = d["tools"].to<JsonArray>();
  stackchanToolList(t);
  CHECK_EQ((int)t.size(), (int)MCP_TOOL_COUNT);
  CHECK_EQ(stackchanFind("tamapoke_buy"), (int)MCP_BUY);
  CHECK_EQ(stackchanFind("nada"), -1);
  CHECK(d["tools"][(int)MCP_BUY]["inputSchema"]["properties"]["item"]["enum"].size() == 4);
}
