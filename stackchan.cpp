#include "stackchan.h"
#include "audio.h"
#include "chess.h"
#include "config.h"  // OWNER_NAME
#include "regions.h"  // rangos de cada region (los paisajes no se usan: el compilador los quita)
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *const NAME[MCP_TOOL_COUNT] = {
  "tamapoke_status", "tamapoke_feed", "tamapoke_play", "tamapoke_bath", "tamapoke_pet",
  "tamapoke_sleep", "tamapoke_wake", "tamapoke_candy", "tamapoke_medicine", "tamapoke_chess",
  "tamapoke_pokedex", "tamapoke_buy", "tamapoke_farm",
};
static const char *const DESC[MCP_TOOL_COUNT] = {
  "查看 " OWNER_NAME " 的电子宠物 TamaPoke（一只宝可梦）现在的状态：名字、等级、饱食、心情、精力、清洁（0-100）、"
  "是否在睡觉或生病、便便、积分、背包、今天走了多少步。Check the virtual pet's current status.",
  "给宠物喂一颗树果（免费），饱食 +25。睡觉时不能吃。Feed the pet a free berry.",
  "陪宠物玩一会儿：心情 +25，精力 -10。睡觉或生病时不能玩。Play with the pet.",
  "给宠物洗澡，清掉便便，清洁回到 100。Give the pet a bath and clean up.",
  "摸摸宠物，心情 +5。Pet / stroke the pet.",
  "关灯让宠物去睡觉，睡觉时精力恢复。Turn off the light so the pet sleeps.",
  "开灯叫醒宠物。Turn on the light and wake the pet up.",
  "喂一颗糖果（用背包里的，没有要先买）：心情 +12、饱食 +10，但会变胖。Feed a candy from the bag.",
  "宠物生病时喂一颗药治好它（用背包里的）。Give medicine from the bag when the pet is sick.",
  OWNER_NAME " 在 chess.com 下棋赚积分：查最近一次同步（每晚 12 点）的结果：下了几盘、赢了几盘、"
  "得了多少分，还有现在的积分和记录过的最好等级分。Last chess.com sync result and points.",
  "查宝可梦图鉴：一共登记了多少只（共 1025 只）、每个地区集了多少、哪些地区集齐了、异色有几只。"
  "Pokedex progress.",
  "用积分在商店买东西放进背包。只能买：candy 糖果(2分)、medicine 药(3分)、strength_potion 体力药剂(5分，"
  "喝了精力掉得慢)、speed_potion 活力药剂(5分，喝了心情掉得慢)；蛋、异色护符和进化药水要在板子上买。"
  "Buy cheap items with points.",
  "查农场里收藏了哪些宝可梦。List the Pokemon kept in the farm.",
};
static const char *const BUY_ID[4] = { "candy", "medicine", "strength_potion", "speed_potion" };
static const uint8_t BUY_ITEM[4] = { ITEM_CANDY, ITEM_MEDICINE, ITEM_POTION_STR, ITEM_POTION_SPD };
static const char *const ITEM_ZH[ITEM_COUNT] = { "糖果", "药", "体力药剂", "活力药剂", "异色护符",
                                                 "普通蛋", "稀有蛋", "传说蛋", "进化药水" };
static const char *const NAT_ZH[NAT_COUNT] = { "乖巧", "贪吃", "爱玩", "爱干净", "懒洋洋" };
static const char *const REGION_ZH[REGION_COUNT] = { "关都", "城都", "丰缘", "神奥", "合众",
                                                     "卡洛斯", "阿罗拉", "伽勒尔", "帕底亚" };

void stackchanToolList(JsonArray tools) {
  for (int i = 0; i < MCP_TOOL_COUNT; i++) {
    JsonObject t = tools.add<JsonObject>();
    t["name"] = NAME[i];
    t["description"] = DESC[i];
    JsonObject sch = t["inputSchema"].to<JsonObject>();
    sch["type"] = "object";
    JsonObject props = sch["properties"].to<JsonObject>();
    if (i == MCP_BUY) {
      JsonObject item = props["item"].to<JsonObject>();
      item["type"] = "string";
      JsonArray en = item["enum"].to<JsonArray>();
      for (const char *id : BUY_ID) en.add(id);
      item["description"] = "要买的东西";
      JsonObject n = props["count"].to<JsonObject>();
      n["type"] = "integer";
      n["minimum"] = 1;
      n["maximum"] = MCP_BUY_MAX;
      n["description"] = "买几个，默认 1";
      sch["required"].to<JsonArray>().add("item");
    }
  }
}

int stackchanFind(const char *name) {
  for (int i = 0; i < MCP_TOOL_COUNT; i++)
    if (name && !strcmp(name, NAME[i])) return i;
  return -1;
}

void stackchanArgs(uint8_t tool, JsonVariantConst args, uint8_t &a, uint8_t &b) {
  a = b = 0;
  if (tool != MCP_BUY) return;
  const char *item = args["item"] | "";
  a = 0xFF;  // desconocido
  for (int i = 0; i < 4; i++)
    if (!strcmp(item, BUY_ID[i])) a = BUY_ITEM[i];
  int n = args["count"] | 1;
  b = (uint8_t)(n < 1 ? 1 : n > MCP_BUY_MAX ? MCP_BUY_MAX : n);
}

// "糖果 2、药 1"; "空的" si no hay nada
static void mochila(const Pet &pet, char *out, size_t cap) {
  out[0] = 0;
  size_t n = 0;
  for (int i = 0; i < ITEM_COUNT; i++) {
    if (!pet.bag[i]) continue;
    n += snprintf(out + n, n < cap ? cap - n : 0, "%s%s %u", n ? "、" : "", ITEM_ZH[i], pet.bag[i]);
  }
  if (!n) snprintf(out, cap, "空的");
}

// "PIKACHU Lv.6：饱食 75，心情 90，精力 40，清洁 100，醒着，积分 12。"
static void resumen(const Pet &pet, const char *name, char *out, size_t cap) {
  char bolsa[120];
  mochila(pet, bolsa, sizeof(bolsa));
  snprintf(out, cap, "%s%s Lv.%u（性格%s）：饱食 %u，心情 %u，精力 %u，清洁 %u%s%s%s。积分 %ld，背包：%s，今天走了 %lu 步。",
           pet.shiny ? "异色" : "", name, pet.level(), NAT_ZH[pet.nature < NAT_COUNT ? pet.nature : 0],
           pet.fullness, pet.joy, pet.energy, pet.hygiene,
           pet.poops ? "，有便便" : "", pet.sleeping ? "，在睡觉" : "，醒着", pet.sick ? "，生病了" : "",
           (long)pet.points, bolsa, (unsigned long)pet.stepsToday);
}

// dia local -> "11月6日"
static void fecha(uint32_t day, char *out, size_t cap) {
  time_t t = (time_t)day * 86400;
  struct tm tm;
  gmtime_r(&t, &tm);
  snprintf(out, cap, "%d月%d日", tm.tm_mon + 1, tm.tm_mday);
}

static void ajedrez(const Pet &pet, char *out, size_t cap) {
  ChessReport rep;
  uint32_t day;
  ChessState st;
  chessLoad(st);
  if (!chessLoadReport(rep, day)) {
    snprintf(out, cap, "还没有同步过 chess.com（每晚 12 点自动同步）。现在积分 %ld。", (long)pet.points);
    return;
  }
  char f[24];
  fecha(day, f, sizeof(f));
  int n = snprintf(out, cap, "最近一次同步是 %s：下了 %u 盘（10 分钟以上才算），赢了 %u 盘，对局得 %ld 分",
                   f, rep.games, rep.wins, (long)rep.gamePts);
  if (rep.bonusPts > 0 && n > 0 && (size_t)n < cap)
    n += snprintf(out + n, cap - n, "，破纪录奖励 %ld 分", (long)rep.bonusPts);
  if (n > 0 && (size_t)n < cap)
    snprintf(out + n, cap - n, "。现在积分 %ld。记录过的最好成绩：快棋 %u，每日棋 %u，谜题 %u，Puzzle Rush %u。",
             (long)pet.points, st.bestRapid, st.bestDaily, st.tacticsHigh, st.rushBest);
}

static void pokedex(const Pet &pet, char *out, size_t cap) {
  unsigned shiny = 0;
  for (int d = 1; d <= PET_DEX_MAX; d++)
    if (pet.isShinyRegistered(d)) shiny++;
  int n = snprintf(out, cap, "图鉴登记了 %u/%d 只，异色 %u 只。各地区：", pet.registeredCount(), PET_DEX_MAX, shiny);
  for (int r = 0; r < REGION_COUNT && n > 0 && (size_t)n < cap; r++) {
    unsigned got = 0;
    for (int d = REGIONS[r].first; d <= REGIONS[r].last; d++)
      if (pet.isRegistered(d)) got++;
    n += snprintf(out + n, cap - n, "%s%s %u/%d%s", r ? "，" : "", REGION_ZH[r], got,
                  REGIONS[r].last - REGIONS[r].first + 1, (pet.regionDone & (1 << r)) ? "（集齐了）" : "");
  }
  uint8_t abiertas = pet.unlockedRegions();
  if (n > 0 && (size_t)n < cap)
    snprintf(out + n, cap - n, "。蛋里现在能孵出：从关都到%s（下一个地区要先把%s蛋里能孵的都登记）。",
             REGION_ZH[abiertas - 1], REGION_ZH[abiertas - 1]);
}

static void comprar(Pet &pet, uint8_t item, uint8_t count, char *out, size_t cap) {
  if (item == 0xFF || item >= ITEM_COUNT) {
    snprintf(out, cap, "语音只能买糖果、药、力量药剂和速度药剂；蛋和异色护符请在板子上的商店买。");
    return;
  }
  uint8_t ok = 0;
  while (ok < count && pet.buy(item)) ok++;
  char bolsa[120];
  mochila(pet, bolsa, sizeof(bolsa));
  if (!ok)
    snprintf(out, cap, "积分不够：%s要 %u 分，现在只有 %ld 分。", ITEM_ZH[item], ITEM_PRICE[item], (long)pet.points);
  else
    snprintf(out, cap, "买了 %u 个%s，花了 %u 分%s。还剩 %ld 分，背包：%s。", ok, ITEM_ZH[item],
             ok * ITEM_PRICE[item], ok < count ? "（积分只够买这么多）" : "", (long)pet.points, bolsa);
}

void stackchanTool(Pet &pet, uint8_t tool, uint8_t a, uint8_t b, const StackchanCtx &ctx, char *out,
                   size_t cap) {
  // las que no dependen del compañero
  if (tool == MCP_CHESS) { ajedrez(pet, out, cap); return; }
  if (tool == MCP_POKEDEX) { pokedex(pet, out, cap); return; }
  if (tool == MCP_BUY) { comprar(pet, a, b, out, cap); return; }
  if (tool == MCP_FARM) {
    if (!ctx.farmList) snprintf(out, cap, "没有插 SD 卡，没有农场。");
    else if (!ctx.farmList[0]) snprintf(out, cap, "农场里还没有宝可梦。");
    else snprintf(out, cap, "农场里有：%s。", ctx.farmList);
    return;
  }

  if (pet.awaitingStarter()) { snprintf(out, cap, "还没有选初始宝可梦，要先在板子上选一只。"); return; }
  if (pet.vacant) { snprintf(out, cap, "现在没有伙伴，要先在板子上打开一个蛋。积分 %ld。", (long)pet.points); return; }
  if (pet.ceremony) { snprintf(out, cap, "宝可梦正在告别，现在不能操作。"); return; }
  if (pet.isEgg()) {
    if (tool == MCP_PET) {
      pet.eggTap();
      sfxPlay(SFX_TAP);
      snprintf(out, cap, pet.isEgg() ? "摸了摸蛋，它动了一下。" : "摸了摸蛋，孵出来了！");
    } else {
      snprintf(out, cap, "现在还是一颗蛋，摸三下（或者摇一摇）就会孵出来。");
    }
    return;
  }
  const char *msg = "";
  switch (tool) {
    case MCP_FEED:
      if (pet.sleeping) msg = "它在睡觉，吃不了。";
      else { pet.feed(); sfxPlay(SFX_EAT); msg = "喂了一颗树果。"; }
      break;
    case MCP_PLAY:
      if (pet.sleeping) msg = "它在睡觉。";
      else if (pet.sick) msg = "它生病了，先喂药吧。";
      else { pet.play(); sfxPlay(SFX_HEART); msg = "陪它玩了一会儿。"; }
      break;
    case MCP_BATH:
      if (pet.sleeping) msg = "它在睡觉，醒了再洗。";
      else { if (ctx.bath) ctx.bath(); msg = "开始洗澡了。"; }
      break;
    case MCP_PET:
      if (pet.sleeping) msg = "它睡得正香。";
      else { pet.caress(); sfxPlay(SFX_HEART); msg = "摸了摸它，它很开心。"; }
      break;
    case MCP_SLEEP:
      if (pet.sleeping) msg = "它已经在睡了。";
      else { pet.toggleLight(); msg = "关灯了，它去睡觉了。"; }
      break;
    case MCP_WAKE:
      if (!pet.sleeping) msg = "它本来就醒着。";
      else { pet.toggleLight(); msg = "开灯了，它醒了。"; }
      break;
    case MCP_CANDY:
      if (pet.sleeping) msg = "它在睡觉，吃不了。";
      else if (!pet.bag[ITEM_CANDY]) msg = "背包里没有糖果了，可以用积分买（2 分一颗）。";
      else { pet.feedCandy(); sfxPlay(SFX_EAT); msg = "喂了一颗糖果。"; }
      break;
    case MCP_MEDICINE:
      if (!pet.sick) msg = "它没有生病，不用吃药。";
      else if (pet.sleeping) msg = "它在睡觉，醒了再吃药。";
      else if (!pet.bag[ITEM_MEDICINE]) msg = "背包里没有药了，可以用积分买（3 分一颗）。";
      else { pet.giveMedicine(); sfxPlay(SFX_EAT); msg = "喂了药，病好了。"; }
      break;
    default: break;  // MCP_STATUS: solo el estado
  }
  char st[400];
  resumen(pet, ctx.name, st, sizeof(st));
  snprintf(out, cap, "%s%s", msg, st);
}
