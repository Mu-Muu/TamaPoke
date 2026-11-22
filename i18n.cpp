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
    "THANKS! See you at the farm", "It ran away...", "Bye! Waving goodbye...",
    "EGG", "Legendary egg!?", "Rare egg!", "Tap the egg...", "It moves!", "Almost there!",
    "POKEDEX %u/1025",
    "%s%s Lv.%u",
    "Release %s?", "YES", "NO",
    "SETTINGS", "swipe up: back", "Lang",
    "MEDAL!", "AWESOME!", "%u DAY STREAK!",
    "STREAK %u  best %u", "BOND",
    "AGE %lu days", "tap name: rename",
    "ABILITY", "STR", "END", "SPD", "WGT",
    "MEDALS %d/%d", "tap: back",
    "NAME:", "tap: back",
    "FOOD", "JOY", "ENE", "HYG",
    "PROGRESS", "Lv.%u", "%u min to Lv.%u", "EVOLUTION", "Final form",
    "Ready to evolve!", "All needs >=40 to evolve",
    "Evolves in %ud %uh", "Slip-ups: %u",
    "SND ON", "SND OFF",
    "EVOLVE!", "%s wants to retire", "%s feels abandoned...",
    "Evolve?", "Keep form", "Retire to the farm?", "To the farm", "Stay together",
    "Choose your starter",
    "No sprites", "Load them onto the SD",

    "Feeling sick...",

    "Collect", "Release", "FARM %u", "Farm is empty", "Swap in", "Hatch the egg first",
    "Off to the farm!",

    "Needs an SD card",

    "KANTO", "JOHTO", "HOENN", "SINNOH", "UNOVA", "KALOS", "ALOLA", "GALAR", "PALDEA",

    "SHOP  %ld", "BAG",
    "CANDY", "MEDICINE", "STR POTION", "SPD POTION", "CHARM",
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
    "Retired to the farm", "Ran away", "Released", "To the farm",
    "Easygoing", "Glutton", "Playful", "Tidy", "Lazy",
    "Good morning!", "Getting sleepy...", "I want a berry...", "Bath time?", "Play with me!",
    "So tired...", "You won %u games!", "Good games yesterday!", "I love you, %s!", "Hi!",
    "My tummy rumbles", "Let's play!", "I like being clean", "Five more minutes", "What a nice day",
    "Bow", "Flower", "Cap", "Witch hat", "Top hat", "Crown",
    "ACCESSORIES", "OWNED", "WEARING", "Put on!", "Taken off",
    "%s came to visit!",
    "No backup yet", "Restore this backup?", "Restoring...",
    "So comfy~", "FARM HARVEST!", "+%u pts",
    "Hehe~", "Giggle giggle!", "Haha, that tickles!", "Hey, stop it!", "Wheee!",
    "Yummy!", "So fun!", "*big stretch*", "Took my medicine!",
    "EVO POTION", "Not needed now",
    "LOCKED", "Register every egg Pokemon of", "NEW REGION!",
    "energy drains %u%% slower", "joy drains %u%% slower", "gets sick %u%% less",
    "%u/%u", "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT",
    "STATS", "Raised", "Pokedex", "Shiny", "Regions open", "Regions done", "Best streak", "Medals",
    "Steps", "Chess points", "Farm",
    "tap: open",
    "SYNC NOW", "No new games",
    "also your chess.com name", "Set chess.com name in WI-FI",
    "10,000 steps! +10 pts",
    "ACTIVITIES", "TOURNAMENT", "Win a Mega Stone", "Asleep...", "swipe right: back",
    "MEGA STONE", "MEGA %s%s", "No Mega form", "Already Mega", "MEGA EVOLVED!", "Back to normal",
    "100m DASH", "HURDLES", "50m SWIM", "LONG JUMP", "HIGH JUMP",
    "Speed counts", "Speed and spring", "Speed and stamina", "Power and speed", "Power counts",
    "tap: start", "GO!", "GOLD!", "SILVER", "BRONZE",
    "Got a Mega Stone!", "Today's stone is won", "%u.%u s", "%u.%02u m", "vs %s & %s", "Too tired", "YOU",
    "Bonus: Electric, Fighting", "Bonus: Fighting, Bug", "Bonus: Water, Ice", "Bonus: Fighting, Flying",
    "Bonus: Flying, Psychic", "You +%u%%", "You -%u%%",
    "In the grass: ",
    "Phone steps code",
    "WEIGHTS", "CLIMBING", "DIVING", "SKATING",
    "TORCH RUN", "TRAIL RUN", "Power counts", "Power and speed",
    "Technique counts", "Speed counts", "Stamina counts", "A bit of everything",
    "Bonus: Fighting, Steel, Rock", "Bonus: Rock, Ground, Bug", "Bonus: Water, Psychic, Flying", "Bonus: Ice, Fairy",
    "Bonus: Fire, Dragon", "Bonus: Dark, Ghost, Poison", "%u kg", "%u.%u pts",
    "%u:%02u", "GYMS", "%s GYMS", "Badges %u/8",
    "LEAGUE", "CHAMPION", "swipe: regions", "Win all 8 badges first",
    "Already tried today", "Win the badge before", "Need 2 in the farm", "Elite Four",
    "Champion", "CHALLENGE", "REMATCH", "PICK 2",
    "NEXT", "LINEUP", "Tap yours to swap", "START",
    "%s  Round %u/3", "You win!", "You lose", "tap: continue",
    "Got the %s!", "Also: %s", "Lost. Try again tomorrow", "CHAMPION!",
    "League over. Try tomorrow", "Normal", "Fire", "Water",
    "Grass", "Electric", "Ice", "Fighting",
    "Poison", "Ground", "Flying", "Psychic",
    "Bug", "Rock", "Ghost", "Dragon",
    "Dark", "Steel", "Fairy",
    "%u badges",
    "FUN", "Fish, grow, dig...", "FUN  %ld", "ORCHARD",
    "FISHING", "MINING", "TREASURE", "Grass Bug Ground Poison Fairy",
    "Needs Water Ice Dragon", "Needs Rock Steel Fighting Fire", "Normal Fly Elec Psy Ghost Dark", "%u can go",
    "%u pts", "Won at chess: double luck!", "Played chess: +1 loot", "Play chess for good luck",
    "WHO GOES?", "Out treasure hunting", "%u min left", "%u h left",
    "%u ready", "Back! Collect", "PLANT", "WATER",
    "HARVEST", "Watered", "Ripe!", "Clock not set yet",
    "CAST", "Wait for a bite...", "Now! Tap!", "Great catch!",
    "Just a small one", "DIG", "Tap the rock!", "SET OFF",
    "Back in 1 hour", "COLLECT", "Someone is already out", "You got",
    "STR FRUIT", "END FRUIT", "SPD FRUIT", "GOLD FRUIT",
    "FISH", "PEARL", "MEGA SHARD", "WATER EGG",
    "FOSSIL EGG", "Full today: just a snack", "10 shards = 1 Mega Stone", "STR +3",
    "END +3", "SPD +3", "All +3, so happy!", "So happy!",
    "Yum, so full!",
    "FARM", "%u Pokemon", "RESTORE", "x%u",
    "Good at: %s, %s", "HP %u  ATK %u  DEF %u  SPD %u", "STR %u  END %u  SPD %u", "Try other teammates,",
    "or train with fruits", "Badges", "League titles", "Fun finds",
    "%u h", "%u min",
    "Buy for %u pts?", "Spend %u pts?",
    "POINTS  %ld", "This week", "Chess +%u (won %u)", "Steps +%u  Other +%u", "Shop -%u  Fun -%u",
    "Last week: +%u  -%u", "Most points came from chess!", "Play chess to earn more", "LAST WEEK", "Chess +%u, spent %u",
    "HOCKEY", "%u min today, +%u pts", "%u min today (max reached)",
    "FRIENDS", "Put boards together", "Looking for friends...", "Nobody here yet", "Tap: be friends",
    "Friend, connected", "Friend", "Pair again", "Too far", "Needs update",
    "%s's trainer", "Be friends?", "wants to be friends", "Waiting for YES...",
    "Same on both boards?", "Friends with %s!", "Didn't work, try again", "They said no", "Pair again?",
    "Friend list is full",
    "PLAY", "Hop around together", "GIFT", "From your bag", "RACE",
    "Tournament events", "GYM", "You 2 + friend 1", "PHOTOS", "%u so far",
    "Friend needs update", "Both tap your Pokemon!", "High five!", "Pick a gift", "Your bag is empty",
    "Give it away?", "Waiting for them...", "%s sent you", "%s. Take it?", "Gift delivered!",
    "Not taken: back in your bag", "Got %s!", "%s wants to race", "%s wants a gym", "%s - go?",
    "Waiting for your friend...", "Your friend is busy", "Their Pokemon can't go now", "Your friend left", "Pick 1 from the farm",
    "Won together!", "Lost this time", "No photos yet", "Run!", "Catch!",
    "Dance!",
    "MEET", "Meet and mail", "Visit, card, gift", "Meet once to mail", "No friends yet",
    "SYNC NOW", "Syncing soon", "VISIT", "Send your partner", "POSTCARD",
    "A messenger takes it", "GIFT", "%u on the way", "Pick a messenger", "What to say?",
    "Pick a picture", "%s will take it", "Send it?", "Visit %s?", "Sent! Arrives at the next sync",
    "Too much mail, wait a bit", "Enough visits today", "From %s:", "%s sent you:", "%s's %s came to see you!",
    "A gift came back to your bag", "Tap to open",
    "Hello!", "Miss you!", "Let's play!", "See you tomorrow!", "Good morning!", "Sweet dreams!",
    "Thank you!", "You can do it!", "Happy birthday!", "Had a great day!", "Won at chess today!", "My Pokemon evolved!",
    "BACKUP", "SAVE NOW", "Save a new backup?", "Backup saved", "Backup failed",
    "REWARD", "+%u pts (%u today)", "Today's %u pts are all given", "Hockey +%u  Reward +%u",
    "Can't evolve while asleep", "Its next form is in the farm",
    "City updated", "City not found",
    "END POTION",
    "HIGH", "MEDIUM", "LOW",
    "%uh %umin to Lv.%u",
    "Set up WI-FI first",
  },
  // ---------------- ZH ----------------
  // Chino simplificado. Los especificadores van en el orden de la fila
  // inglesa: T(id) se rellena con varargs en un orden fijo en cada llamada. Todo debe estar en GB2312, que es lo que
  // trae la fuente (lo comprueba test/test_tools.py).
  {
    "正在进化！", "好吃！", "它很喜欢！", "肚子饿了！", "该洗澡了！",
    "累坏了...", "好难过...", "有点胖了...", "是异色！！", "心情很好",
    "谢谢！农场见", "它离家出走了...", "拜拜！挥手告别...",

    "蛋", "传说之蛋！？", "稀有的蛋！", "摸摸蛋...", "动了！", "快孵出来了！",

    "图鉴 %u/1025",

    "%s%s Lv.%u",

    "放生%s？", "是", "否",

    "设置", "上滑：返回", "语言",

    "奖章！", "太棒了！", "连续 %u 天！",

    "连续 %u  最高 %u", "羁绊",

    "年龄 %lu 天", "点名字：改名",

    "能力", "力量", "耐力", "速度", "体重",

    "奖章 %d/%d", "点一下：返回",

    "名字：", "点一下：返回",

    "饱食", "心情", "精力", "清洁",

    "成长", "Lv.%u", "%u分钟后 Lv.%u", "进化", "最终形态",

    "可以进化了！", "全部40以上才能进化",

    "%u天%u小时后可进化", "照顾失误：%u",

    "声音开", "声音关",

    "进化！", "%s想去农场养老了", "%s觉得被冷落了...",

    "要进化吗？", "保持原样", "要送它去农场养老吗？", "去农场养老", "继续在一起",

    "选择你的初始宝可梦",

    "没有图片", "请放进SD卡",

    "生病了...",

    "收藏", "放生", "农场 %u", "农场还是空的", "换出来", "先把蛋孵出来",
    "去农场啦！",

    "需要SD卡",

    "关都", "城都", "丰缘", "神奥", "合众", "卡洛斯", "阿罗拉", "伽勒尔", "帕底亚",

    "商店  %ld分", "背包",
    "糖果", "药", "力量药剂", "速度药剂", "异色符",
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

    "去农场养老了", "离家出走", "放生", "进了农场",

    "性格：乖巧", "性格：贪吃", "性格：爱玩", "性格：爱干净", "性格：懒洋洋",

    "早上好！", "有点困了…", "想吃树果…", "想洗澡…", "陪我玩嘛！",
    "好累呀…", "昨天赢了%u盘，好厉害！", "昨天下棋辛苦啦！", "最喜欢 %s 了！", "嗨！",
    "肚子咕咕叫…", "我们来玩吧！", "我爱干干净净的", "再躺一会儿…", "今天也很开心",

    "蝴蝶结", "小花", "棒球帽", "巫师帽", "礼帽", "皇冠",
    "饰品", "已拥有", "戴着", "戴上了", "摘下了",

    "%s来串门啦！",

    "还没有备份", "用这份备份覆盖现在的进度？", "正在恢复…",

    "好舒服～", "农场收获！", "+%u分",

    "嘿嘿～", "咯咯咯！", "哈哈哈，好痒！", "别闹啦！", "转圈圈～",

    "好吃！", "好开心！", "伸个懒腰～", "自己吃了药！",

    "进化药水", "现在用不上",

    "未解锁", "集齐上一地区蛋里能孵的宝可梦", "新地区解锁！",

    "精力消耗慢 %u%%", "心情消耗慢 %u%%", "生病几率低 %u%%",

    "%u月%u日", "周日", "周一", "周二", "周三", "周四", "周五", "周六",

    "统计", "养过的宝可梦", "图鉴登记", "异色", "已解锁地区", "集齐地区", "最长连续照顾", "获得奖章",
    "总步数", "下棋赚的积分", "农场",
    "点一下：查看",
    "立即同步", "没有新对局",
    "还能设置 chess.com 名字", "先在 Wi-Fi 里设置 chess.com 名字",
    "走满一万步！+10 分",
    "活动", "锦标赛", "赢了得Mega石", "在睡觉...", "右滑：返回",
    "Mega石", "超级%s%s", "它没有超级进化", "已经是超级形态", "超级进化！", "变回来了",
    "100米短跑", "跨栏", "50米游泳", "跳远", "跳高",
    "比速度", "比速度和弹跳", "比速度和耐力", "比力量和速度", "比力量",
    "点一下：开始", "开始！", "金牌！", "银牌", "铜牌",
    "获得Mega石！", "今天的Mega石已拿过", "%u.%u 秒", "%u.%02u 米", "对手：%s、%s", "太累了", "你",
    "电、格斗属性有加成", "格斗、虫属性有加成", "水、冰属性有加成", "格斗、飞行属性有加成",
    "飞行、超能力属性有加成", "你 +%u%%", "你 -%u%%",
    "草丛里有：",
    "手机步数配对码",
    "举重", "攀岩", "跳水", "速度滑冰",
    "火炬长跑", "越野障碍", "比力量", "比力量和速度",
    "比技巧", "比速度", "比耐力", "什么都比一点",
    "格斗、钢、岩石属性有加成", "岩石、地面、虫属性有加成", "水、超能力、飞行属性有加成", "冰、妖精属性有加成",
    "火、龙属性有加成", "恶、幽灵、毒属性有加成", "%u 公斤", "%u.%u 分",
    "%u:%02u", "道馆", "%s道馆", "徽章 %u/8",
    "联盟大赛", "冠军", "左右滑：换地区", "先集齐 8 枚徽章",
    "今天已经挑战过了", "先拿到上一枚徽章", "农场里要有 2 只宝可梦", "四天王",
    "冠军", "挑战", "再挑战", "选两只队友",
    "下一步", "排阵容", "点自己的宝可梦换人", "开始",
    "%s  第 %u/3 局", "赢了！", "输了", "点一下：继续",
    "获得%s！", "还得到：%s", "这次输了，明天再来", "你是冠军！",
    "联盟挑战结束，明天再来", "一般属性", "火属性", "水属性",
    "草属性", "电属性", "冰属性", "格斗属性",
    "毒属性", "地面属性", "飞行属性", "超能力属性",
    "虫属性", "岩石属性", "幽灵属性", "龙属性",
    "恶属性", "钢属性", "妖精属性",
    "已有 %u 枚徽章",
    "趣玩", "钓鱼、种果子、挖宝……", "趣玩  %ld分", "果园",
    "钓鱼", "挖矿", "寻宝", "要草、虫、地面、毒、妖精属性",
    "要水、冰、龙属性", "要岩石、钢、格斗、火属性", "要一般、飞行、电、超能力、幽灵、恶属性", "%u 只能去",
    "%u 分", "下棋赢了：好运翻倍！", "下了棋：收获多 1 个", "下盘棋会带来好运",
    "派谁去？", "出门寻宝去了", "还要 %u 分钟", "还要 %u 小时",
    "%u 块熟了", "回来了，快领取", "种下", "浇水",
    "收获", "浇过水了", "熟了！", "时间还没同步",
    "抛竿", "等鱼上钩……", "快点屏幕收竿！", "钓得好！",
    "只钓到一条小鱼", "开挖", "点石头把它敲开！", "出发",
    "1 小时后回来", "领取", "已经有宝可梦出门了", "收获",
    "力量果", "耐力果", "速度果", "金果",
    "鱼", "珍珠", "Mega石碎片", "水属性蛋",
    "化石蛋", "今天吃够了，只填饱肚子", "集齐 10 片自动变成 Mega石", "力量 +3",
    "耐力 +3", "速度 +3", "三项都 +3，好开心！", "好开心！",
    "好吃，吃得好饱！",
    "农场", "%u 只宝可梦", "恢复备份", "有 %u",
    "擅长：%s、%s", "HP %u  攻 %u  防 %u  速 %u", "力量 %u  耐力 %u  速度 %u", "试试换别的队友，",
    "或者吃果子练训练值", "徽章总数", "联盟冠军", "趣玩收获",
    "%u 小时", "%u 分钟",
    "花 %u 分购买？", "花 %u 分？",
    "积分本  %ld分", "本周", "下棋 +%u（赢 %u 盘）", "走路 +%u  其他 +%u", "商店 -%u  趣玩 -%u",
    "上周：赚 %u  花 %u", "积分大多来自下棋，加油！", "多下几盘棋，积分会更多", "上周积分", "下棋赚 %u 分，花了 %u 分",
    "冰球练习", "今天 %u 分钟，+%u 分", "今天 %u 分钟，今天的分已拿满",
    "好友", "两块板子放在一起", "正在等好友…", "还没有看到别人", "点一下成好友",
    "好友，已连上", "好友", "要重新配对", "离得远", "要更新",
    "%s的主人", "要成好友吗？", "想和你成好友", "等对面点\"是\"…",
    "两边一样才对", "和%s成好友了！", "没成功，再试一次", "对面说不要", "要重新配对吗？",
    "好友满了",
    "一起玩", "一起跳一跳", "送礼物", "背包里的东西", "一起比赛",
    "锦标赛的项目", "一起去道馆", "你出两只，好友出一只", "合照", "%u 次",
    "好友要更新", "两人一起点自己的宝可梦！", "击掌！", "挑一样礼物", "背包是空的",
    "要送出去吗？", "等对面收下…", "%s送你", "%s，收下吗？", "礼物送到了！",
    "对面没收，放回背包了", "收到%s！", "%s想一起比赛", "%s想一起去道馆", "%s，去吗？",
    "等好友…", "好友现在没空", "好友的宝可梦现在不能去", "好友走了", "农场里选1只",
    "一起赢了！", "这次输了", "还没有合照", "一起跑！", "传球！",
    "一起跳舞！",
    "见面", "见面、寄信", "串门、明信片、礼物", "见面一次后才能寄", "还没有好友",
    "立即收发", "马上收发", "去串门", "派伙伴过去", "寄明信片",
    "信使送过去", "寄礼物", "还有 %u 个在路上", "挑一个信使", "想说什么？",
    "挑一个小图", "%s来送", "寄出吗？", "去%s家串门吗？", "寄出了！下次收发时送到",
    "信太多了，等一会儿", "今天串门够多了", "%s寄来：", "%s送来：", "%s的%s来看你了！",
    "有个礼物回到背包了", "点一下打开",
    "你好呀！", "想你了！", "一起玩吧！", "明天见！", "早上好！", "好梦！",
    "谢谢你！", "加油！", "生日快乐！", "今天很开心！", "今天下棋赢了！", "我的宝可梦进化了！",
    "备份", "立即备份", "保存一份新的备份？", "备份好了", "备份失败",
    "家长加分", "+%u 分（今天已加 %u 分）", "今天的加分已满 %u 分", "冰球 +%u  家长 +%u",
    "睡着时不能进化", "进化形态在农场里",
    "城市已更新", "没查到这个城市",
    "耐力药剂",
    "大声", "中等", "小声",
    "%u小时%u分钟后 Lv.%u",
    "先设置 Wi-Fi",
  },
};

// Nombres de medalla en sus tres longitudes [idioma][medalla].
static const char *const MED_NAME[LANG_COUNT][MED_COUNT] = {
  { "Lv.10", "Lv.20", "Lv.30", "7 STREAK", "BOND", "TOP FORM", "IN SHAPE",
    "CURED", "SHINY", "LEGEND", "STRONG", "SPEEDY", "30 STREAK", "CHESS WIN", "CHESS RECORD" },
  { "Lv.10", "Lv.20", "Lv.30", "连续7天", "羁绊", "最终形态", "精神饱满",
    "康复", "异色", "传说", "大力士", "飞毛腿", "连续30天", "棋局胜利", "棋艺新高" },
};
static const char *const MED_LBL[LANG_COUNT][MED_COUNT] = {
  { "Lv10", "Lv20", "Lv30", "7DAYS", "BOND", "TOP", "FIT",
    "CURED", "SHINY", "LEGEND", "STR", "SPD", "30DAYS", "WIN", "RECORD" },
  { "Lv10", "Lv20", "Lv30", "7天", "羁绊", "最终", "健康",
    "康复", "异色", "传说", "力量", "速度", "30天", "胜利", "新高" },
};
static const char *const MED_DSC[LANG_COUNT][MED_COUNT] = {
  { "LEVEL 10", "LEVEL 20", "LEVEL 30", "7 DAY STREAK", "MAX BOND", "FINAL FORM", "IN SHAPE",
    "GOT CURED", "SHINY FORM", "LEGENDARY", "STRENGTH 50", "SPEED 50",
    "30DAY STREAK", "WON CHESS", "CHESS RECORD" },
  { "等级 10", "等级 20", "等级 30", "连续 7 天", "羁绊 MAX", "最终进化", "精神饱满",
    "吃药治好病", "异色伙伴", "传说宝可梦", "力量练到 50", "速度练到 50",
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
