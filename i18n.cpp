#include "i18n.h"
#include "pet.h"        // MED_COUNT
#include <Preferences.h>

Lang gLang = LANG_DEFAULT;

// Tabla de cadenas [idioma][id]. Sin acentos ni enes: la fuente bitmap del
// firmware no los tiene (por eso el espanol ya iba "Esta", "bano", etc.).
static const char *const STRINGS[LANG_COUNT][STR_COUNT] = {
  // ---------------- EN ----------------
  {
    "Evolving!", "Yum yum!", "It likes it!", "It's hungry!", "Needs a bath!",
    "Worn out...", "Feeling sad...", "A bit chubby...", "It's SHINY!!", "It's happy",
    "THANKS! Farewell", "It ran away...", "Bye! Waving goodbye...",
    "EGG", "Legendary egg!?", "Rare egg!", "Tap the egg...", "It moves!", "Almost there!",
    "POKEDEX %u/1025",
    "%s%s Lv.%u",
    "Release %s?", "YES", "NO",
    "SETTINGS", "swipe up: back", "Lang",
    "MEDAL!", "AWESOME!", "%u DAY STREAK!",
    "STREAK %u  best %u", "BOND",
    "AGE %lu days", "tap name: rename",
    "ABILITY", "STA", "RES", "VIT", "WGT",
    "MEDALS %d/%d", "tap: back",
    "NAME:", "tap to go back",
    "FOOD", "JOY", "ENE", "HYG",
    "PROGRESS", "Lv.%u", "%u min to Lv.%u", "EVOLUTION", "Final form",
    "Ready to evolve!", "All needs >=40 to evolve",
    "Evolves in %ud %uh", "Slip-ups: %u",
    "SND ON", "SND OFF",
    "EVOLVE!", "%s wants to tell you...", "%s feels abandoned...",
    "Evolve?", "Keep form", "Say goodbye?", "Goodbye", "Stay together",
    "Choose your starter",
    "No sprites", "Load them onto the SD",

    "Feeling sick...",

    "Collect", "Release", "FARM %u", "Farm is empty", "Swap in", "Hatch the egg first",
    "Off to the farm!",

    "Needs an SD card",

    "KANTO", "JOHTO", "HOENN", "SINNOH", "UNOVA", "KALOS", "ALOLA", "GALAR", "PALDEA",

    "SHOP  %ld", "BAG",
    "CANDY", "MEDICINE", "STA POTION", "VIT POTION", "SHINY CHARM",
    "EGG", "RARE EGG", "LEGEND EGG",
    "Bought!", "Not enough points", "None left - shop", "Done!", "Already at max", "Not sick",
    "Used on next egg",
    "Only with no partner", "No partner", "Pick an egg", "SHOP",

    "%u games %u wins", "+%ld points", "New record!",

    "WI-FI", "Phone: join TamaPoke", "open 192.168.4.1", "Wi-Fi saved", "Syncing chess...",
    "Sync failed",
    "Happy New Year!", "Happy Lunar New Year!", "Lantern Festival!", "Happy Valentine's!",
    "Happy Easter!", "Happy Mother's Day!", "Dragon Boat Festival!", "Happy Father's Day!",
    "Happy Canada Day!", "Mid-Autumn Festival!", "Happy Thanksgiving!", "Happy Halloween!",
    "Merry Christmas!", "Happy Birthday %s!",
    "%s DEX DONE!", "+%d pts  Shiny egg!", "SHINY LEGEND",
    "Loves the rain!", "Hates the rain...", "Snow! Yay!", "Brr... so cold", "Lovely sunshine!",
    "STEPS TODAY %lu",
    "ALBUM", "ALBUM %d", "No memories yet",
    "Farewell", "Ran away", "Released", "To the farm",
    "Easygoing", "Glutton", "Playful", "Tidy", "Lazy",
    "Good morning!", "Getting sleepy...", "I want a berry...", "Bath time?", "Play with me!",
    "So tired...", "You won %u games!", "Good games yesterday!", "I love you, %s!", "Hi!",
    "My tummy rumbles", "Let's play!", "I like being clean", "Five more minutes", "What a nice day",
    "Bow", "Flower", "Cap", "Witch hat", "Top hat", "Crown",
    "ACCESSORIES", "OWNED", "WEARING", "Put on!", "Taken off",
    "%s came to visit!",
    "RESTORE %u/%u", "No backup yet", "Restore this backup?", "Restoring...",
    "So comfy~", "FARM HARVEST!", "+%u pts",
    "Hehe~", "Giggle giggle!", "Haha, that tickles!", "Hey, stop it!", "Wheee!",
    "Yummy!", "So fun!", "*big stretch*", "Took my medicine!",
    "EVO POTION", "Not needed now",
    "LOCKED", "Register every egg Pokemon of", "NEW REGION!",
    "energy drains %u%% slower", "joy drains %u%% slower", "%u%% less likely to get sick",
    "%u/%u", "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT",
    "STATS", "Raised", "Pokedex", "Shiny", "Regions open", "Regions done", "Best streak", "Medals",
    "Steps", "Chess points", "Farm",
    "tap to open",
    "SYNC NOW", "No new games",
  },
  // ---------------- ZH ----------------
  // Chino simplificado. Los especificadores van en el orden de la fila
  // inglesa: T(id) se rellena con varargs en un orden fijo en cada llamada. Todo debe estar en GB2312, que es lo que
  // trae la fuente (lo comprueba test/test_tools.py).
  {
    "正在进化！", "好吃！", "它很喜欢！", "肚子饿了！", "该洗澡了！",
    "累坏了...", "好难过...", "有点胖了...", "是异色！！", "心情很好",
    "谢谢！再见", "它离家出走了...", "拜拜！挥手告别...",

    "蛋", "传说之蛋！？", "稀有的蛋！", "摸摸蛋...", "动了！", "快孵出来了！",

    "图鉴 %u/1025",

    "%s%s Lv.%u",

    "放生%s？", "是", "否",

    "设置", "上滑: 返回", "语言",

    "奖章！", "太棒了！", "连续 %u 天！",

    "连续 %u  最高 %u", "羁绊",

    "年龄 %lu 天", "点名字: 改名",

    "能力", "体力", "抵抗力", "活力", "体重",

    "奖章 %d/%d", "点击: 返回",

    "名字:", "点击返回",

    "饱食", "心情", "精力", "清洁",

    "成长", "Lv.%u", "%u分钟后 Lv.%u", "进化", "最终形态",

    "可以进化了！", "全部40以上才能进化",

    "%u天%u小时后可进化", "照顾失误: %u",

    "声音开", "声音关",

    "进化！", "%s有话想说...", "%s觉得被冷落了...",

    "要进化吗？", "保持原样", "要告别吗？", "再见", "继续在一起",

    "选择你的初始宝可梦",

    "没有图片", "请放进SD卡",

    "生病了...",

    "收藏", "放生", "农场 %u", "农场还是空的", "换出来", "先把蛋孵出来",
    "去农场啦！",

    "需要SD卡",

    "关都", "城都", "丰缘", "神奥", "合众", "卡洛斯", "阿罗拉", "伽勒尔", "帕底亚",

    "商店  %ld分", "背包",
    "糖果", "药", "体力药剂", "活力药剂", "异色符",
    "普通蛋", "稀有蛋", "传说蛋",
    "买好了！", "积分不够", "没有了，去商店买", "用好了！", "已经满了", "没有生病",
    "下一颗蛋自动生效",
    "没有伙伴时才能孵", "没有伙伴", "选一颗蛋来孵", "去商店",

    "%u盘 赢%u盘", "+%ld分", "创纪录了！",

    "Wi-Fi", "手机连 TamaPoke", "打开 192.168.4.1", "Wi-Fi 已保存", "正在同步棋局...",
    "同步失败",

    "新年快乐！", "春节快乐！", "元宵节快乐！", "情人节快乐！",
    "复活节快乐！", "母亲节快乐！", "端午安康！", "父亲节快乐！",
    "加拿大国庆日快乐！", "中秋节快乐！", "感恩节快乐！", "万圣节快乐！",
    "圣诞快乐！", "%s 生日快乐！",

    "%s图鉴集齐！", "+%d分 送异色传说蛋", "异色传说蛋",

    "下雨真开心！", "讨厌下雨…", "下雪啦，好开心！", "好冷…", "晒太阳真舒服！",

    "今日步数 %lu",

    "回忆录", "回忆录 %d", "还没有回忆",

    "告别", "离家出走", "放生", "进了农场",

    "性格：乖巧", "性格：贪吃", "性格：爱玩", "性格：爱干净", "性格：懒洋洋",

    "早上好！", "有点困了…", "想吃树果…", "想洗澡…", "陪我玩嘛！",
    "好累呀…", "昨天赢了%u盘，好厉害！", "昨天下棋辛苦啦！", "最喜欢 %s 了！", "嗨！",
    "肚子咕咕叫…", "我们来玩吧！", "我爱干干净净的", "再躺一会儿…", "今天也很开心",

    "蝴蝶结", "小花", "棒球帽", "巫师帽", "礼帽", "皇冠",
    "饰品", "已拥有", "戴着", "戴上了", "摘下了",

    "%s来串门啦！",

    "恢复备份 %u/%u", "还没有备份", "用这份备份覆盖现在的进度？", "正在恢复…",

    "好舒服～", "农场收获！", "+%u分",

    "嘿嘿～", "咯咯咯！", "哈哈哈，好痒！", "别闹啦！", "转圈圈～",

    "好吃！", "好开心！", "伸个懒腰～", "自己吃了药！",

    "进化药水", "现在用不上",

    "未解锁", "集齐上一地区蛋里能孵的宝可梦", "新地区解锁！",

    "精力消耗慢 %u%%", "心情消耗慢 %u%%", "生病几率低 %u%%",

    "%u月%u日", "周日", "周一", "周二", "周三", "周四", "周五", "周六",

    "统计", "养过的宝可梦", "图鉴登记", "异色", "已解锁地区", "集齐地区", "最长连续照顾", "获得奖章",
    "总步数", "下棋赚的积分", "农场",
    "点击查看",
    "立即同步", "没有新对局",
  },
};

// Nombres de medalla en sus tres longitudes [idioma][medalla].
static const char *const MED_NAME[LANG_COUNT][MED_COUNT] = {
  { "Lv.10", "Lv.25", "Lv.50", "7 STREAK", "BOND", "TOP FORM", "IN SHAPE",
    "CURED", "SHINY", "LEGEND", "TIRELESS", "LIVELY", "30 STREAK", "CHESS WIN", "CHESS RECORD" },
  { "Lv.10", "Lv.25", "Lv.50", "连续7天", "羁绊", "最终形态", "精神饱满",
    "康复", "异色", "传说", "精力充沛", "活力满满", "连续30天", "棋局胜利", "棋艺新高" },
};
static const char *const MED_LBL[LANG_COUNT][MED_COUNT] = {
  { "Lv10", "Lv25", "Lv50", "7DAYS", "BOND", "TOP", "FIT",
    "CURED", "SHINY", "LEGEND", "STA", "VIT", "30DAYS", "WIN", "RECORD" },
  { "Lv10", "Lv25", "Lv50", "7天", "羁绊", "最终", "健康",
    "康复", "异色", "传说", "体力", "活力", "30天", "胜利", "新高" },
};
static const char *const MED_DSC[LANG_COUNT][MED_COUNT] = {
  { "LEVEL 10", "LEVEL 25", "LEVEL 50", "7 DAY STREAK", "MAX BOND", "FINAL FORM", "IN SHAPE",
    "GOT CURED", "SHINY FORM", "LEGENDARY", "STAMINA 50", "VITALITY 50",
    "30DAY STREAK", "WON CHESS", "CHESS RECORD" },
  { "等级 10", "等级 25", "等级 50", "连续 7 天", "羁绊 MAX", "最终进化", "精神饱满",
    "吃药治好病", "异色伙伴", "传说宝可梦", "体力练到 50", "活力练到 50",
    "连续 30 天", "陪伴时下棋赢了", "陪伴时棋艺新高" },
};

const char *T(StrId id) { return STRINGS[gLang][id]; }
const char *medalName(int i)  { return MED_NAME[gLang][i]; }
const char *medalLabel(int i) { return MED_LBL[gLang][i]; }
const char *medalDesc(int i)  { return MED_DSC[gLang][i]; }

// La clave es "lang2" y no "lang": con 9 idiomas el ingles era el 1 y el chino
// el 8. Releer esos numeros con la tabla nueva cambiaria el idioma de golpe.
void loadLang() {
  Preferences p;
  p.begin("tamapoke", true);  // solo lectura
  uint8_t v = p.getUChar("lang2", LANG_DEFAULT);
  p.end();
  gLang = (v < LANG_COUNT) ? (Lang)v : LANG_DEFAULT;
}

void setLang(Lang l) {
  if (l >= LANG_COUNT) return;
  gLang = l;
  Preferences p;
  p.begin("tamapoke", false);
  p.putUChar("lang2", (uint8_t)l);
  p.end();
}
