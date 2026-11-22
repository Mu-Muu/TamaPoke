// The player's guide (/guide/): every chapter in English and Simplified Chinese.
// The rules and numbers come from the firmware and the READMEs; when a rule changes
// there, change it here too. **bold** is the only markup inside a text.
export type L = { en: string; zh: string };
const t = (en: string, zh: string): L => ({ en, zh });

export type Block =
  | { k: "p"; t: L }
  | { k: "h"; t: L }
  | { k: "list"; items: L[] }
  | { k: "steps"; items: L[] }
  | { k: "table"; head: L[]; rows: L[][]; tight?: boolean } // tight: short cells, no minimum width
  | { k: "tip"; t: L }
  | { k: "warn"; t: L }
  | { k: "shots"; items: { name: string; cap: L }[] }
  | { k: "swipes" }
  | { k: "links"; items: { t: L; href: string; local?: boolean }[] }
  | { k: "qa"; items: { q: L; a: L }[] };

export type Chapter = { id: string; title: L; lead: L; blocks: Block[] };

export const GUIDE_UI = {
  kicker: t("Player's guide", "玩法教程"),
  title: t("How to play", "怎么玩"),
  lead: t(
    "Everything the board can do, from the first egg to the League. Read it in order the first time; after that, jump to the chapter you need.",
    "板子能做的所有事情，从第一颗蛋到联盟大赛。第一次建议按顺序读一遍，以后需要哪章就跳到哪章。",
  ),
  quick: t("The short version", "先看这四步"),
  toc: t("Chapters", "目录"),
  top: t("Back to top", "回到顶部"),
  home: t("Back to the home page", "回到首页"),
  // the swipe map of the "screens" chapter
  swipeUp: t("Swipe up: your pet's card", "向上滑：宠物卡片"),
  swipeDown: t("Swipe down: settings", "向下滑：设置"),
  swipeLeft: t("Swipe left: activities", "向左滑：活动"),
  swipeRight: t("Swipe right: Pokédex", "向右滑：图鉴"),
  swipeMain: t("Main screen", "主屏幕"),
};

export const GUIDE_QUICK: { to: string; t: L; d: L }[] = [
  { to: "start", t: t("Install", "安装"), d: t("Copy the files to an SD card and flash the board from a browser.", "把文件拷进 SD 卡，用浏览器给板子刷机。") },
  { to: "first", t: t("Hatch", "孵蛋"), d: t("Pick a starter, tap the egg three times, set up Wi-Fi.", "选一只初始宝可梦，点三下蛋，连上 Wi-Fi。") },
  { to: "care", t: t("Care", "照顾"), d: t("Feed it, play, bathe it and turn the light off at night. All free.", "喂食、玩耍、洗澡，晚上关灯。全部免费。") },
  { to: "points", t: t("Play chess", "下棋"), d: t("Real chess.com games earn the points that pay for everything else.", "在 chess.com 下真正的棋，赚到的积分用来换其他所有东西。") },
];

export const GUIDE: Chapter[] = [
  {
    id: "start",
    title: t("Before you start", "准备工作"),
    lead: t("What you need, and how to get the game onto the board. About fifteen minutes.", "需要准备什么，以及怎么把游戏装进板子。大约十五分钟。"),
    blocks: [
      {
        k: "list",
        items: [
          t("A **Waveshare ESP32-S3-Touch-AMOLED-1.75** board (standard or -G; the -B comes in its own case and the 1.75C is a different board).", "一块 **Waveshare ESP32-S3-Touch-AMOLED-1.75** 板子（标准版或 -G 版；-B 版自带外壳，1.75C 是另一块板子）。"),
          t("A **microSD card** formatted as FAT32. It holds the sprites, the farm, the album and the backups.", "一张格式化成 FAT32 的 **microSD 卡**，用来存精灵图、农场、相册和备份。"),
          t("A USB-C cable and a computer with **Chrome or Edge**.", "一根 USB-C 线，一台装了 **Chrome 或 Edge** 的电脑。"),
          t("Optional: a 3.7 V battery (MX1.25 plug, check the polarity first), a small speaker, and the printed Poké Ball case with 3 M2×4 mm screws and 4 magnets (6×2 mm).", "可选：3.7 V 锂电池（MX1.25 插头，先确认正负极）、小喇叭，以及 3D 打印的精灵球外壳（另需 3 颗 M2×4 mm 螺丝和 4 颗 6×2 mm 磁铁）。"),
        ],
      },
      {
        k: "steps",
        items: [
          t("Download **tamapoke-sd.zip** from the latest release, unzip it and copy the **mons** folder to the root of the SD card (about 290 MB). Put the card in the board.", "从最新发布版本下载 **tamapoke-sd.zip**，解压后把 **mons** 文件夹拷到 SD 卡根目录（约 290 MB），再把卡插进板子。"),
          t("Open the **web installer** on the computer, plug the board in and press Install. No Arduino needed.", "在电脑上打开**网页安装器**，插上板子，点安装。不需要装 Arduino。"),
          t("When it restarts you see the starter screen. Go on to the next chapter.", "板子重启后会出现选初始宝可梦的画面，接着看下一章。"),
        ],
      },
      {
        k: "links",
        items: [
          { t: t("Open the web installer", "打开网页安装器"), href: "/web/", local: true },
          { t: t("Download the SD files", "下载 SD 卡文件"), href: "https://github.com/Mu-Muu/TamaPoke/releases/latest" },
        ],
      },
      { k: "warn", t: t("If the board can't read the card, it **formats** it. Back up anything on the card first.", "如果板子读不了这张卡，会直接把它**格式化**。卡里有东西的话先备份。") },
      { k: "tip", t: t("Updating later is the same click in the web installer, and it keeps your game: only the program is replaced.", "以后更新也是在网页安装器里点一下，游戏进度会保留，只替换程序。") },
    ],
  },
  {
    id: "first",
    title: t("The first ten minutes", "第一次开机"),
    lead: t("A starter, an egg, your language and Wi-Fi.", "选初始宝可梦、孵蛋、切换语言、连 Wi-Fi。"),
    blocks: [
      {
        k: "steps",
        items: [
          t("**Pick a starter**: Bulbasaur, Charmander or Squirtle. This choice only appears in a brand-new game.", "**选初始宝可梦**：妙蛙种子、小火龙或杰尼龟。只有全新的游戏才会出现这个选择。"),
          t("**Hatch the egg**: tap it three times (or give the board a gentle shake). Left alone it hatches by itself after 3 minutes.", "**孵蛋**：点三下蛋（轻轻摇一下板子也行）。不管它的话，3 分钟后会自己孵出来。"),
          t("**Language**: the installer's firmware starts in English. Swipe down for settings and tap the language button to switch to 简体中文.", "**语言**：网页安装器刷的固件默认是英文。向下滑打开设置，点语言按钮切成简体中文。"),
          t("**Wi-Fi**: in settings tap Wi-Fi. The board opens a hotspot called **TamaPoke**; join it with a phone and open **192.168.4.1**. Enter up to three networks (2.4 GHz only) and, if you play, your **chess.com username**. The clock takes your phone's time zone, and once online the board finds where it is from the connection for the weather; if the place is wrong, enter your **city** there too (add the state or country after a comma if the name is shared, like Springfield, IL).", "**Wi-Fi**：在设置里点 Wi-Fi，板子会开一个叫 **TamaPoke** 的热点。用手机连上，打开 **192.168.4.1**，最多填三个 Wi-Fi（只支持 2.4 GHz），下棋的话再填上 **chess.com 用户名**。时钟按手机的时区走，联网后板子按网络自动定位，用来取天气；地点不对的话，在这一页填上**城市**（有重名的城市，在逗号后面加上省、州或国家，比如 Springfield, IL）。"),
        ],
      },
      { k: "tip", t: t("You start with **10 points**. Feeding, playing, bathing and sleeping never cost anything.", "开局有 **10 积分**。喂食、玩耍、洗澡、睡觉永远不花积分。") },
      {
        k: "list",
        items: [
          t("With three networks saved (say home, school and a phone hotspot) the board joins the strongest one it can see. On an iPhone hotspot turn on Maximize Compatibility.", "存了三个 Wi-Fi（比如家里、学校、手机热点）时，板子会连能看到的信号最强的那个。iPhone 热点要打开“最大兼容性”。"),
          t("The Wi-Fi details stay on the board and are never shown again. Leave a field empty to keep what was saved; enter **-** to delete it.", "Wi-Fi 信息只存在板子上，之后不会再显示。某一栏留空表示不改，填一个减号 **-** 表示删除。"),
          t("Wi-Fi also sets the clock and brings the weather. Without it the pet still works; you just get no chess points.", "连上 Wi-Fi 后还会自动校时、更新天气。不连也能养，只是没有下棋的积分。"),
        ],
      },
    ],
  },
  {
    id: "screens",
    title: t("Finding your way", "认识屏幕"),
    lead: t("Everything starts from the main screen: four swipes, four buttons and the pet itself.", "一切从主屏幕开始：四个方向的滑动、四个按钮，还有宠物本身。"),
    blocks: [
      { k: "swipes" },
      {
        k: "table",
        head: [t("On the main screen", "在主屏幕上"), t("What it does", "作用")],
        rows: [
          [t("Tap the **points** at the top", "点顶部的**积分**"), t("Opens the shop", "打开商店")],
          [t("**Long press** the pet", "**长按**宠物"), t("Send it to the farm, or release it", "送去农场，或者放生")],
          [t("**Feed**", "**喂食**"), t("Berry (free), candy, medicine; fish and fruits when you have some", "树果（免费）、糖果、药；有鱼和果子时会多一排")],
          [t("**Play**", "**玩耍**"), t("Cheers it up, costs a little energy", "让它开心，会消耗一点精力")],
          [t("**Light**", "**开关灯**"), t("Puts it to sleep or wakes it", "让它睡觉或起床")],
          [t("**Bath**", "**洗澡**"), t("Washes it and cleans up the poops", "洗干净，顺便清理便便")],
        ],
      },
      { k: "h", t: t("Touching the pet", "和宠物互动") },
      { k: "p", t: t("Tap to pet it, stroke back and forth, double tap to make it hop, poke its head or belly, tickle it (too much and it gets annoyed), drag it to walk, or draw a circle to spin it.", "点一下是摸摸它，来回划是抚摸，双击让它跳一下，可以戳头或戳肚子，挠痒痒（挠太多它会烦），拖着它走，画个圈让它转圈。") },
      { k: "h", t: t("Moving the board", "动一动板子") },
      { k: "p", t: t("Tilt the board and it walks downhill. Lay it face down and it goes to sleep. Shake it hard and it gets dizzy.", "把板子倾斜，它会往低处走。屏幕朝下放，它会睡觉。用力摇晃，它会头晕。") },
      { k: "h", t: t("The PWR button", "电源键") },
      {
        k: "list",
        items: [
          t("**Short press**: the screen turns off and the pet goes to sleep. Press again to wake it.", "**短按**：屏幕关闭，宠物去睡觉。再按一下叫醒。"),
          t("While the screen is off, **touching it does nothing**. That is on purpose, so it doesn't wake up in a pocket.", "屏幕关着时**碰屏幕没有反应**。这是故意的，放在口袋里不会被蹭亮。"),
          t("**Hold for about 4 seconds**: the board switches off completely (steps are not counted then).", "**按住约 4 秒**：彻底关机（关机后不计步数）。"),
        ],
      },
      {
        k: "shots",
        items: [
          { name: "card", cap: t("The pet's card", "宠物卡片") },
          { name: "activities", cap: t("Activities", "活动") },
          { name: "settings", cap: t("Settings", "设置") },
        ],
      },
    ],
  },
  {
    id: "care",
    title: t("Daily care", "每天的照顾"),
    lead: t("Four needs from 0 to 100. Keep them up and your friend stays healthy, happy and on time for its evolution.", "四项需求，每项 0 到 100。照顾好了，它就健康、开心，也能按时进化。"),
    blocks: [
      {
        k: "table",
        head: [t("Need", "需求"), t("How it drops", "怎么掉"), t("How to raise it", "怎么补")],
        rows: [
          [t("**Food**", "**饱食**"), t("The fastest: about 2 a minute while awake", "掉得最快：醒着时每分钟约 2 点"), t("Feed a berry: +25, free", "喂树果：+25，免费")],
          [t("**Joy**", "**心情**"), t("About 1 a minute, faster when hungry or dirty", "每分钟约 1 点，饿了或脏了掉得更快"), t("Play: +25. Petting +5, stroking +8", "玩耍：+25。摸摸 +5，抚摸 +8")],
          [t("**Energy**", "**精力**"), t("Slowly while awake; playing costs 10", "醒着时慢慢掉；玩一次耗 10 点"), t("Sleep: it comes back in a few minutes", "睡觉：几分钟就能回满")],
          [t("**Hygiene**", "**清洁**"), t("About 1 a minute, much faster with poops around", "每分钟约 1 点，有便便时快很多"), t("Bath: back to 100, poops gone", "洗澡：回到 100，便便清掉")],
        ],
      },
      { k: "h", t: t("What goes wrong", "照顾不好会怎样") },
      {
        k: "list",
        items: [
          t("A need at **10 or below** is a **care mistake** (at most one an hour). Each mistake delays evolution by 2 hours.", "任何一项降到 **10 或以下**算一次**照顾失误**（每小时最多一次）。每次失误让进化推迟 2 小时。"),
          t("Two or more poops, being overweight, or a need at 20 or below can make it **sick**. Give medicine; every 2 hours sick is another mistake.", "便便两坨以上、太胖，或者任何一项降到 20 或以下，都可能**生病**。要喂药；生病期间每 2 小时再算一次失误。"),
          t("If it stays sick and awake for 10 minutes it takes a medicine from the bag by itself. It never spends your points: with no medicine in the bag it waits for you.", "它醒着病了 10 分钟，会自己从背包里拿药吃。它不会花你的积分：背包里没药，就一直等你。"),
          t("All four needs at 0 for an hour: a **run-away** button appears.", "四项全部为 0 并持续一小时，会出现**逃走**按钮。"),
        ],
      },
      { k: "h", t: t("Sleep protects it", "睡着时有保护") },
      { k: "p", t: t("While it sleeps, energy comes back, the other needs drop about four times slower and they stop at a floor: food 30, joy 35, hygiene 45. Asleep it makes no care mistakes and doesn't get sick. So whenever you leave, let it sleep.", "睡着时精力会回复，其他需求掉得比醒着慢约四倍，而且有下限：饱食 30、心情 35、清洁 45。睡着时不记照顾失误，也不会生病。所以离开之前，让它去睡。") },
      {
        k: "table",
        head: [t("Situation", "情况"), t("What happens", "会发生什么")],
        rows: [
          [t("Night, light still on", "夜里灯还开着"), t("It falls asleep by itself at 23:00 and wakes at 07:00", "23:00 自己睡着，07:00 自己醒来")],
          [t("You press PWR", "按一下电源键"), t("Screen off, pet asleep. Only PWR wakes it", "关屏，宠物入睡。只有电源键能叫醒")],
          [t("On battery, untouched for 5 minutes", "用电池，5 分钟没人碰"), t("The same as pressing PWR, by itself", "自动关屏入睡，效果和按电源键一样")],
          [t("Plugged in, untouched for 90 seconds", "插着电，90 秒没人碰"), t("The standby clock: time, date and weather", "显示待机时钟：时间、日期和天气")],
          [t("Plugged in, untouched for 10 minutes", "插着电，10 分钟没人碰"), t("Self-care: any need below 40 goes back to 50, one a minute, silently (no animation, no sound), and it can't get sick. This doesn't count as your care", "自己照顾自己：低于 40 的需求补到 50，每分钟补一项，全程静默（没有动画和声音），这期间不会生病。这不算你的照顾")],
        ],
      },
      { k: "h", t: t("Natures", "性格") },
      { k: "p", t: t("Each pet hatches with a nature: easygoing, glutton, playful, tidy or lazy. One need drops a little faster, and its favourite care gives more (a glutton gets +30 from a berry, a playful one +30 from play, a tidy one enjoys its bath).", "每只宝可梦孵出来都有一种性格：乖巧、贪吃、爱玩、爱干净、懒洋洋。某一项需求会掉得稍快，它喜欢的那种照顾效果也更好（贪吃的吃树果 +30，爱玩的玩耍 +30，爱干净的洗澡会开心）。") },
      { k: "tip", t: t("A simple routine: berry, bath and play in the morning and after school, and PWR (or the light) when you leave. Candy is tasty but fattening, and being overweight makes it sick.", "一个简单的习惯：早上和放学后各喂一次、洗一次、玩一次，离开时按电源键（或关灯）。糖果好吃但会长胖，太胖会生病。") },
      {
        k: "shots",
        items: [
          { name: "care_feed", cap: t("The feed menu", "喂食菜单") },
          { name: "care_bath", cap: t("Bath time", "洗澡") },
          { name: "care_pat", cap: t("Petting", "摸摸它") },
          { name: "sleep", cap: t("Asleep at night", "夜里睡着了") },
        ],
      },
    ],
  },
  {
    id: "points",
    title: t("Earning points", "赚积分"),
    lead: t("Points buy items, eggs and fun. They come from real life: mostly chess, a little from walking.", "积分用来换道具、蛋和趣玩。积分来自真实生活：主要靠下棋，走路也有一点。"),
    blocks: [
      { k: "h", t: t("Chess", "下棋") },
      { k: "p", t: t("Once a chess.com username is saved on the Wi-Fi page, the board reads that account's public games once a day at midnight (or at the next start if it was off). **Sync now** in settings does it right away.", "在 Wi-Fi 设置页填了 chess.com 用户名后，板子每天午夜读一次这个账号公开的对局（当时没开机就在下次开机时读）。设置里的**立即同步**可以马上读。") },
      {
        k: "table",
        head: [t("What you did", "做了什么"), t("Points", "积分")],
        rows: [
          [t("Lost a game (10 minutes or more on the clock, or a daily game)", "输了一盘（基础时间 10 分钟以上，或每日棋）"), t("5", "5")],
          [t("Drew a game", "和了一盘"), t("7", "7")],
          [t("Won a game", "赢了一盘"), t("10", "10")],
          [t("A new best rapid or daily rating", "快棋或每日棋等级分创新高"), t("20", "20")],
          [t("Each new hundred in rapid (1000, 1100...)", "快棋每突破一个百位整数（1000、1100……）"), t("50", "50")],
          [t("A new best in tactics or Puzzle Rush", "战术题或 Puzzle Rush 创新纪录"), t("10", "10")],
        ],
      },
      { k: "warn", t: t("Bullet, blitz and games against bots don't count. The first sync (and the first after changing the username) only records where you stand: no points for old games.", "子弹棋、闪电棋和跟电脑下的棋不算。第一次同步（以及换用户名后的第一次）只记录当前成绩，不补发以前的分。") },
      { k: "h", t: t("Other ways", "其他来源") },
      {
        k: "list",
        items: [
          t("**Steps**: 1 point per 1,000 steps, up to 5 a day, and 10 more the day you reach 10,000. The board counts them itself, also with the screen off.", "**走路**：每 1000 步 1 分，每天最多 5 分；当天走满 1 万步再加 10 分。板子自己计步，关屏时也算。"),
          t("**Hockey practice** (from an iPhone shortcut): 1 point every 5 minutes, up to 6 a day.", "**冰球练习**（用 iPhone 快捷指令上报）：每 5 分钟 1 分，每天最多 6 分。"),
          t("**Rewards from home** (from a parent's iPhone shortcut): 1 to 10 points at a time for anything the parent likes, up to 20 a day.", "**家长加分**（用家长 iPhone 上的快捷指令）：奖励什么都行，每次 1 到 10 分，每天最多 20 分。"),
          t("**Completing a region's Pokédex**: 200 points, a crown and a shiny legendary egg of that region.", "**集齐一个地区的图鉴**：200 分、一顶皇冠，还有一个该地区的异色传说蛋。"),
          t("**The farm** gives a small harvest every day after 07:00.", "**农场**每天 07:00 后有一点小收获。"),
        ],
      },
      { k: "h", t: t("The points book", "积分本") },
      { k: "p", t: t("The last page of the shop (or tap the points at the top of the shop) shows this week day by day: earned in green, spent in orange, where it came from and where it went, plus last week. Every Monday the main screen shows a short summary.", "商店的最后一页（或点商店顶部的积分）是积分本：本周每天赚了多少（绿色）、花了多少（橙色），从哪来、花在哪，还有上周的对比。每周一主屏幕会弹出上周小结。") },
      {
        k: "shots",
        items: [
          { name: "points_book", cap: t("The points book", "积分本") },
          { name: "stats", cap: t("The stats page", "统计页") },
        ],
      },
    ],
  },
  {
    id: "shop",
    title: t("Shop and bag", "商店和背包"),
    lead: t("Tap the points at the top of the main screen.", "点主屏幕顶部的积分就能打开。"),
    blocks: [
      { k: "p", t: t("Swipe left to move through the pages: shop, bag, harvest (what you got from Fun), accessories and the points book. Buying always asks first (YES / NO).", "向左滑依次是：商店、背包、收获（趣玩得到的东西）、饰品、积分本。买东西前都会先问一下（是 / 否）。") },
      {
        k: "table",
        head: [t("Item", "道具"), t("Points", "积分"), t("What it's for", "用途")],
        rows: [
          [t("Candy", "糖果"), t("2", "2"), t("Food +10 and joy +12, but fattening", "饱食 +10、心情 +12，但会长胖")],
          [t("Medicine", "药"), t("3", "3"), t("Cures a sick pet", "治病")],
          [t("Strength potion", "力量药剂"), t("5", "5"), t("Raises strength: energy drops more slowly", "提高力量：精力掉得更慢")],
          [t("Speed potion", "速度药剂"), t("5", "5"), t("Raises speed: joy drops more slowly", "提高速度：心情掉得更慢")],
          [t("Common / rare / legendary egg", "普通蛋 / 稀有蛋 / 传说蛋"), t("10 / 30 / 100", "10 / 30 / 100"), t("Your next companion", "下一位伙伴")],
          [t("Shiny charm", "异色符"), t("50", "50"), t("Makes the next egg four times as likely to be shiny", "让下一颗蛋出异色的机会变成四倍")],
          [t("Evolution potion", "进化药水"), t("100", "100"), t("Skips the wait in days (needs still 40+)", "省掉要等的天数（四项仍要 40 以上）")],
          [t("Mega Stone", "Mega石"), t("150", "150"), t("Mega Evolution for 24 hours", "超级进化 24 小时")],
          [t("Accessories", "饰品"), t("15-80", "15-80"), t("Bow, flower, cap, witch hat, top hat, crown", "蝴蝶结、小花、棒球帽、巫师帽、礼帽、皇冠")],
        ],
      },
      { k: "tip", t: t("One win at chess is 10 points: five candies, or three medicines and a candy. A Mega Stone is also the prize for the first tournament gold of the day, so you rarely need to buy one.", "赢一盘棋是 10 分：能买五颗糖果，或者三颗药加一颗糖果。每天第一块锦标赛金牌也送 Mega石，一般不用花积分买。") },
      { k: "warn", t: t("An egg can only be opened when you have **no companion**. Send the current one to the farm first (long press it).", "只有**没有伙伴**的时候才能开蛋。先把现在这只送去农场（长按它）。") },
      {
        k: "shots",
        items: [
          { name: "shop", cap: t("The shop", "商店") },
          { name: "shop_confirm", cap: t("It asks first", "买之前先问") },
          { name: "bag", cap: t("The bag", "背包") },
          { name: "accessories", cap: t("Accessories", "饰品") },
        ],
      },
    ],
  },
  {
    id: "grow",
    title: t("Growing up", "成长"),
    lead: t("Levels, evolution, abilities, and what happens when a companion grows old.", "等级、进化、能力，以及伙伴长大以后的事。"),
    blocks: [
      {
        k: "list",
        items: [
          t("**Level**: one level for every hour of its life, asleep or awake.", "**等级**：每活一小时升一级，睡着醒着都算。"),
          t("**Evolution** goes by age, the same for every Pokémon: the first at **3 days**, the second at **5 days**. All four needs must be at 40 or more, and each care mistake adds 2 hours.", "**进化**看天数，所有宝可梦都一样：第一次在**第 3 天**，第二次在**第 5 天**。四项需求都要在 40 以上，每次照顾失误多等 2 小时。"),
          t("When it is ready a button appears: evolve, or keep the form you like. An **evolution potion** skips the wait.", "时间到了会出现按钮：可以进化，也可以保持喜欢的样子。**进化药水**可以跳过等待。"),
          t("**Retiring**: 7 days after reaching its final form it asks to retire (you can stay together a while longer). It moves to the farm, the album keeps its page, and you open a new egg.", "**养老**：到达最终形态满 7 天后，它会想去农场养老（也可以再陪它一段时间）。它搬进农场，相册里留下一页，然后你开一颗新蛋。"),
        ],
      },
      { k: "h", t: t("Abilities", "能力") },
      { k: "p", t: t("Three abilities from 0 to 100 (there are no battles): **strength** slows energy loss by up to half, **speed** slows joy loss by up to half, and **endurance** cuts the chance of getting sick by up to 60%. Potions raise all three. The strength and speed potions are in the shop; the endurance potion is not sold: it turns up in the grass and on Fun treasure hunts. Endurance also grows by 1 for every 3 hours of good care (all needs at 40 or more; the screen being off counts in the daytime, night sleep and lights off pause it). Fruits from the orchard raise them as training, which also counts in the tournament and the gyms: strength where Attack counts, speed where Speed counts and endurance where HP and Defense count.", "三项能力，每项 0 到 100（没有对战）：**力量**让精力最多慢一半下降，**速度**让心情最多慢一半下降，**耐力**最多让生病几率降低 60%。三项都能靠药剂提高：力量和速度药剂在商店买，耐力药剂不卖，在草丛和趣玩的寻宝里得到。耐力也会靠好好照顾增长，四项需求都在 40 以上时每 3 小时加 1（白天关屏也算，晚上睡觉和关灯时暂停）。果园的果子可以当训练来加能力，对锦标赛和道馆也有用：力量帮看重攻击的项目，速度帮看重速度的项目，耐力帮看重 HP 和防御的项目。") },
      { k: "h", t: t("Eggs and shinies", "蛋和异色") },
      {
        k: "table",
        tight: true,
        head: [t("Egg", "蛋"), t("Rare species", "出稀有"), t("Legendary", "出传说")],
        rows: [
          [t("Common", "普通蛋"), t("10%", "10%"), t("3%", "3%")],
          [t("Rare", "稀有蛋"), t("20%", "20%"), t("6%", "6%")],
          [t("Legendary", "传说蛋"), t("30%", "30%"), t("10%", "10%")],
        ],
      },
      { k: "p", t: t("These are the base chances; good care of the companion before it improves them. Eggs only bring species from regions you have opened, and never one that is already in your farm. About 1 egg in 48 is **shiny** (1 in 24 after a proper farewell); a shiny charm in the bag is used on the next egg and makes it four times as likely.", "这是基础几率，上一位伙伴照顾得好会提高。蛋里只会孵出已开放地区的宝可梦，而且不会和农场里已有的重复。大约每 48 颗蛋出一只**异色**（好好告别之后是 24 颗）；背包里有异色符时，下一颗蛋会用掉它，机会变成四倍。") },
      { k: "h", t: t("Mega Evolution", "超级进化") },
      { k: "p", t: t("Use a **Mega Stone** from the bag: 41 species (Charizard X, Mewtwo Y, Gengar, Lucario, Rayquaza...) Mega Evolve for 24 hours and then change back. It adds 15% in the tournament and the gyms. If your companion has no Mega form the stone is not used up.", "在背包里用一颗 **Mega石**：41 种宝可梦（喷火龙 X、超梦 Y、耿鬼、路卡利欧、烈空坐……）会超级进化 24 小时，然后变回来。超级进化在锦标赛和道馆里有 15% 的加成。伙伴没有超级进化形态时，石头不会被用掉。") },
      {
        k: "shots",
        items: [
          { name: "card", cap: t("Profile", "资料") },
          { name: "abilities", cap: t("Abilities", "能力") },
          { name: "medals", cap: t("Medals", "奖章") },
        ],
      },
    ],
  },
  {
    id: "dex",
    title: t("Pokédex, farm and album", "图鉴、农场和相册"),
    lead: t("1,025 Pokémon in nine regions, and a home for everyone you have raised.", "九个地区、1025 种宝可梦，还有一个给所有养过的伙伴住的地方。"),
    blocks: [
      { k: "h", t: t("Pokédex", "图鉴") },
      {
        k: "list",
        items: [
          t("Swipe right from the main screen: one cover per region. Swipe on for the next region, tap a cover for its grid (up and down for pages), tap a Pokémon for its page.", "在主屏幕向右滑：每个地区一张封面。继续滑换地区，点封面进入列表（上下滑翻页），点某只看详情。"),
          t("A species is registered when you raise it: by hatching it or by evolving into it.", "养过才算登记：孵出来的，或者进化成的。"),
          t("The nine regions open **in order**: the next one opens once every common and rare species of the current one is registered.", "九个地区**按顺序**开放：当前地区所有普通和稀有的宝可梦都登记后，下一个地区才开放。"),
          t("Completing a region gives 200 points, a crown and a shiny legendary egg of that region.", "集齐一个地区送 200 分、一顶皇冠和一个该地区的异色传说蛋。"),
        ],
      },
      { k: "h", t: t("Farm", "农场") },
      {
        k: "list",
        items: [
          t("**Long press** your companion to send it to the farm. Swipe left for activities and tap **Farm** to visit.", "**长按**伙伴可以把它送去农场。向左滑打开活动，点**农场**去看它们。"),
          t("Farm Pokémon keep growing at a tenth of the speed (without evolving), come back healthy, and sometimes walk across the main screen.", "农场里的宝可梦以十分之一的速度继续长大（不会进化），接回来时是健康的，偶尔还会走到主屏幕上串门。"),
          t("You can take one back at any time. You need **two** in the farm to enter the gyms, and different types open more of Fun.", "随时可以把它换回来。道馆要求农场里至少有**两只**；属性越多，趣玩能玩的也越多。"),
        ],
      },
      { k: "h", t: t("Album and backup", "相册和备份") },
      { k: "p", t: t("The album button is at the bottom of the farm: one page for every companion that has left, with its dates, level, medals and how it ended. The whole game is backed up to the SD card once a day (Wi-Fi details are not included). In settings, **BACKUP** saves a fresh copy at any moment or restores it; both ask first.", "相册按钮在农场底部：每位离开的伙伴一页，记着日期、等级、奖章和离开的方式。整个游戏每天备份到 SD 卡一次（不包括 Wi-Fi 信息）。在设置里点**备份**，可以随时存一份新的，也可以恢复，两种操作都会先问一下。") },
      {
        k: "shots",
        items: [
          { name: "dex_cover", cap: t("A region's cover", "地区封面") },
          { name: "dex_grid", cap: t("The grid", "列表") },
          { name: "dex_detail", cap: t("A Pokémon's page", "详情") },
          { name: "farm", cap: t("The farm", "农场") },
          { name: "album", cap: t("The album", "相册") },
        ],
      },
    ],
  },
  {
    id: "tournament",
    title: t("Tournament", "锦标赛"),
    lead: t("Eleven sports events. Nobody fights and nobody taps: you choose the event and watch.", "十一个运动项目。不打架，也不用点屏幕：你选项目，然后看比赛。"),
    blocks: [
      { k: "p", t: t("Swipe left, tap **Tournament** and pick an event. Your companion competes against two Pokémon drawn at random. The result depends on its species' stats (each event weighs them its own way), its training, its evolution stage, Mega Evolution (+15%), its type and a little luck, so the strongest doesn't always win.", "向左滑，点**锦标赛**，选一个项目。你的伙伴和随机抽到的两只宝可梦比赛。成绩看种族能力（每个项目看重的不一样）、训练、进化阶段、超级进化（+15%）、属性，再加一点运气，所以最强的不一定每次赢。") },
      {
        k: "table",
        head: [t("Event", "项目"), t("What counts", "比什么"), t("Type bonus", "属性加成")],
        rows: [
          [t("100m dash", "100米短跑"), t("Speed", "速度"), t("Electric +15%, Fighting +10% (Rock, Steel -10%)", "电 +15%、格斗 +10%（岩石、钢 -10%）")],
          [t("Hurdles", "跨栏"), t("Speed and spring", "速度和弹跳"), t("Fighting +15%, Bug +10% (Rock, Steel -10%)", "格斗 +15%、虫 +10%（岩石、钢 -10%）")],
          [t("50m swim", "50米游泳"), t("Speed and stamina", "速度和耐力"), t("Water +25%, Ice +10% (Fire, Rock -15%)", "水 +25%、冰 +10%（火、岩石 -15%）")],
          [t("Long jump", "跳远"), t("Power and speed", "力量和速度"), t("Fighting, Flying +15% (Rock, Steel -10%)", "格斗、飞行 +15%（岩石、钢 -10%）")],
          [t("High jump", "跳高"), t("Power", "力量"), t("Flying +25%, Psychic +15% (Rock, Steel -15%)", "飞行 +25%、超能力 +15%（岩石、钢 -15%）")],
          [t("Weights", "举重"), t("Power and steadiness", "力量和稳定"), t("Fighting +20%, Steel, Rock +15% (Flying, Fairy, Bug -10%)", "格斗 +20%、钢、岩石 +15%（飞行、妖精、虫 -10%）")],
          [t("Climbing", "攀岩"), t("Power and speed", "力量和速度"), t("Rock +20%, Ground +15%, Bug +10% (Water, Steel -10%)", "岩石 +20%、地面 +15%、虫 +10%（水、钢 -10%）")],
          [t("Diving", "跳水"), t("Technique", "技巧"), t("Water +20%, Psychic, Flying, Fairy +10% (Fire, Rock -15%)", "水 +20%、超能力、飞行、妖精 +10%（火、岩石 -15%）")],
          [t("Skating", "速度滑冰"), t("Speed", "速度"), t("Ice +25%, Fairy +10% (Fire -15%)", "冰 +25%、妖精 +10%（火 -15%）")],
          [t("Torch run", "火炬长跑"), t("Stamina", "耐力"), t("Fire +25%, Dragon +10% (Water, Ice -10%)", "火 +25%、龙 +10%（水、冰 -10%）")],
          [t("Trail run", "越野障碍"), t("A bit of everything", "什么都比一点"), t("Dark, Ghost +15%, Poison, Grass +10% (Steel, Rock -10%)", "恶、幽灵 +15%、毒、草 +10%（钢、岩石 -10%）")],
        ],
      },
      { k: "tip", t: t("Pick the event that suits its type: a Water type in the pool, a Fire type in the torch run. The first **gold** of the day gives a **Mega Stone**.", "选适合它属性的项目：水属性去游泳，火属性去火炬长跑。每天第一块**金牌**送一颗 **Mega石**。") },
      { k: "p", t: t("Competing tires it a little. It can't take part while asleep, sick or very tired.", "比赛会消耗一点精力。睡觉、生病或太累时不能参加。") },
      {
        k: "shots",
        items: [
          { name: "tournament_menu", cap: t("Choosing an event", "选项目") },
          { name: "tournament_race", cap: t("The race", "比赛中") },
          { name: "tournament_podium", cap: t("The podium", "领奖台") },
        ],
      },
    ],
  },
  {
    id: "gyms",
    title: t("Gyms and the League", "道馆和联盟大赛"),
    lead: t("72 gyms with the leaders and badges from the games. Still sport: a three-against-three team contest.", "72 个道馆，馆主和徽章都是原作里的。比的还是运动：三对三团体赛。"),
    blocks: [
      {
        k: "steps",
        items: [
          t("Swipe left and tap **Gyms**. The list is also your badge case; swipe left and right to change region.", "向左滑，点**道馆**。道馆列表也是徽章盒，左右滑换地区。"),
          t("Your team is your companion plus **two Pokémon from the farm**. Each gym picks the three events that suit its type best.", "你的队伍是伙伴加上**农场里的两只**。每个道馆按自己的属性挑最拿手的三个项目。"),
          t("Before starting you can swap who does which event. The leader puts each of theirs where it does best.", "开始前可以调整谁比哪个项目。馆主会把自己的宝可梦排在最拿手的项目上。"),
          t("Each event is one against one. **Win two of the three** and the badge is yours.", "每个项目一对一，**三局两胜**，赢了就拿到徽章。"),
        ],
      },
      {
        k: "list",
        items: [
          t("Gyms open with their region and go in order: you need the badge of the one before. **One try per gym per day.**", "道馆跟着地区一起开放，要按顺序挑战：拿到上一枚徽章才能挑战下一个。**每个道馆每天只能挑战一次。**"),
          t("Each gym is harder than the last: the leaders' Pokémon get better genes and more training.", "越往后越难：馆主宝可梦的基因和训练越来越好。"),
          t("Badges 2, 4 and 6 of a region also give a rare egg; badge 8 gives a legendary egg.", "每个地区的第 2、4、6 枚徽章还送稀有蛋，第 8 枚送传说蛋。"),
          t("With all 8 badges the region's **League** opens: the Elite Four and then the Champion, one after the other (one try a day). The first win gives a legendary egg and a shiny charm.", "集齐 8 枚徽章后开放这个地区的**联盟大赛**：先四天王、再冠军，一个接一个比（每天一次）。第一次夺冠送传说蛋和异色符。"),
        ],
      },
      { k: "tip", t: t("Teammates use their own training, so feed orchard fruits to the farm Pokémon you take to the gyms.", "队友用的是它们自己的训练值，所以要给带去道馆的农场宝可梦喂果园的果子。") },
      {
        k: "shots",
        items: [
          { name: "gyms_list", cap: t("Gyms and badges", "道馆和徽章") },
          { name: "gyms_lineup", cap: t("Choosing the line-up", "排阵容") },
          { name: "gyms_detail", cap: t("A gym", "道馆详情") },
        ],
      },
    ],
  },
  {
    id: "fun",
    title: t("Fun", "趣玩"),
    lead: t("Four little activities, one for every type. They cost points and never give any.", "四种小玩法，每个属性都有一种可以玩。只花积分，不产生积分。"),
    blocks: [
      { k: "p", t: t("Swipe left and tap **Fun**. Your companion goes if it is the right type (and awake, healthy and not too tired); otherwise you pick one of the right type from the farm. That is why collecting different types pays off.", "向左滑，点**趣玩**。伙伴属性对得上（而且醒着、没生病、不太累）就由它去；不然就从农场里挑一只属性对的。所以收集不同属性的宝可梦很有用。") },
      {
        k: "table",
        head: [t("Activity", "玩法"), t("Types", "属性"), t("Cost", "花费"), t("How", "怎么玩"), t("What you get", "收获")],
        rows: [
          [t("Orchard", "果园"), t("Grass, Bug, Ground, Poison, Fairy", "草、虫、地面、毒、妖精"), t("8 per plot", "每块地 8 分"), t("3 plots in real time: plant, water once (ripe in 12 h, or 24 h without water), harvest", "3 块地，按真实时间长：种下、浇一次水（浇了 12 小时熟，不浇 24 小时），熟了收获"), t("2-3 fruits", "2-3 个果子")],
          [t("Fishing", "钓鱼"), t("Water, Ice, Dragon", "水、冰、龙"), t("5", "5 分"), t("Cast, then tap when the float dips", "抛竿，浮漂沉下去时点屏幕"), t("Fish, pearls, now and then a water-type egg", "鱼、珍珠，偶尔钓到水属性蛋")],
          [t("Mining", "挖矿"), t("Rock, Steel, Fighting, Fire", "岩石、钢、格斗、火"), t("10", "10 分"), t("Tap the rock until it breaks", "一直点石头直到敲开"), t("Mega Stone shards (10 make a stone), sometimes a fossil egg", "Mega石碎片（10 片合成 1 颗），有时是化石蛋")],
          [t("Treasure", "寻宝"), t("Normal, Flying, Electric, Psychic, Ghost, Dark", "一般、飞行、电、超能力、幽灵、恶"), t("8", "8 分"), t("It sets off for an hour, then you collect", "出门 1 小时，回来后领取"), t("Candy, medicine, potions, an accessory, rarely a shiny charm", "糖果、药、药剂、饰品，极少数时候是异色符")],
        ],
      },
      { k: "tip", t: t("Chess brings luck the same day: if you **played**, one more thing from every activity; if you **won**, rare finds are twice as likely.", "当天下棋会带来好运：**下过棋**，每种玩法多收获 1 个；**赢过棋**，稀有东西的概率翻倍。") },
      {
        k: "list",
        items: [
          t("**Fruits** raise training: strength, endurance and speed fruit +3 each; the gold fruit +3 to all three and full joy. Up to 3 a day per Pokémon count (after that they are just a snack).", "**果子**加训练值：力量果、耐力果、速度果各 +3；金果三项都 +3，心情加满。每只宝可梦每天最多 3 个果子有效（再吃只是填肚子）。"),
          t("Farm Pokémon can eat fruits too: open one in the farm and tap a fruit at the top of its page.", "农场里的宝可梦也能吃果子：在农场点开它，点上面那排果子就喂了。"),
          t("**Fish** fills it up and a **pearl** makes it very happy. Both, and the fruits, appear as a second row in the Feed menu.", "**鱼**能吃得很饱，**珍珠**让它特别开心。鱼、珍珠和果子都会出现在喂食菜单的第二排。"),
          t("While your companion is out on a treasure hunt, the main screen shows a sign with the minutes left.", "伙伴出门寻宝时，主屏幕上会挂一块牌子，写着还要多少分钟回来。"),
        ],
      },
      {
        k: "shots",
        items: [
          { name: "fun_list", cap: t("The four activities", "四种玩法") },
          { name: "fun_orchard", cap: t("The orchard", "果园") },
          { name: "fun_fishing", cap: t("Fishing", "钓鱼") },
        ],
      },
    ],
  },
  {
    id: "friends",
    title: t("Friends: meet and mail", "好友：见面和寄信"),
    lead: t("When a friend has a board too. Meeting and mail never give points.", "朋友也有一块板子的时候。见面和寄信都不产生积分。"),
    blocks: [
      { k: "h", t: t("Becoming friends", "成为好友") },
      {
        k: "steps",
        items: [
          t("Put the two boards side by side. On both, swipe left, tap **Friends** and then **Meet**.", "把两块板子放在一起。两边都向左滑，点**好友**，再点**见面**。"),
          t("Each board finds the other. Tap the other board and say yes on both.", "板子会互相找到对方。点一下对方，两边都同意。"),
          t("Both screens show the same **4-digit code**. If it matches, you are friends.", "两块屏幕会显示同一个 **4 位数**，核对一样就成了好友。"),
        ],
      },
      { k: "p", t: t("While the Meet page is open the boards talk to each other directly by radio and home Wi-Fi pauses; it comes back when you close the page or after 3 idle minutes.", "见面页开着时，两块板子用无线电直接连接，家里的 Wi-Fi 先暂停；关掉页面或 3 分钟没操作就恢复。") },
      { k: "h", t: t("With a friend nearby", "好友就在旁边时") },
      {
        k: "list",
        items: [
          t("**Play**: the two Pokémon run, catch and dance together. Tap your own Pokémon at the same moment and they high-five.", "**一起玩**：两只宝可梦一起跑、接球、跳舞。两人同时点自己的宝可梦，它们会击掌。"),
          t("**Gift**: pick something from the bag. It only goes if your friend accepts; otherwise it comes back.", "**送礼物**：从背包里挑一样。对方收下才送出，不收就退回背包。"),
          t("**Race and gyms together**: a tournament race with a third random rival; at a gym the one who invites brings two and the friend's companion is the third.", "**一起比赛、打道馆**：锦标赛里再加一只随机对手；道馆由邀请的一方出两只，好友的伙伴当第三只。"),
          t("**Photo**: one per friend per day, the first time you meet, kept on the SD card. The Pokédex also marks the species a friend has raised.", "**合影**：每个好友每天第一次见面时拍一张，存在 SD 卡里。图鉴里也会标出好友养过的宝可梦。"),
        ],
      },
      { k: "h", t: t("Mail, when you are apart", "不在一起时寄信") },
      {
        k: "list",
        items: [
          t("**Visit**: your companion goes to their screen, says hello and stays a few minutes.", "**串门**：伙伴去对方的屏幕上打个招呼，待几分钟。"),
          t("**Postcard**: a messenger (your companion or a farm Pokémon) carries one of 12 phrases and one of 8 pictures.", "**明信片**：挑一只宝可梦当信使（伙伴或农场里的），从 12 句话和 8 张图里各选一个。"),
          t("**Gift**: an item from the bag, taken out when it is sent.", "**礼物**：背包里的一样东西，寄出时从背包里扣掉。"),
        ],
      },
      { k: "p", t: t("Mail arrives at the next sync: every 15 minutes when plugged in, with the weather on battery, or right away with **Sync now** on the friend's page. The messenger walks onto your friend's main screen and waits with an envelope or a gift box until it is tapped. A gift that never arrives comes back after 14 days. Everything is encrypted with the key the two boards made when they paired.", "信件在下次同步时送到：插着电时每 15 分钟一次，用电池时跟天气一起更新，也可以在好友页点**立即收发**。信使会走到好友的主屏幕上，拿着信封或礼物盒等对方点开。礼物 14 天还没送到会退回背包。所有内容都用两块板子配对时生成的密钥加密。") },
      {
        k: "shots",
        items: [
          { name: "friends", cap: t("Friends", "好友") },
          { name: "friends_meet", cap: t("Meeting", "见面") },
          { name: "friends_play", cap: t("Playing together", "一起玩") },
          { name: "friends_mail", cap: t("Mail", "寄信") },
        ],
      },
    ],
  },
  {
    id: "world",
    title: t("More to discover", "更多发现"),
    lead: t("Surprises in the grass, real weather, holidays, and points from a phone.", "草丛彩蛋、真实天气、节日，还有从手机来的积分。"),
    blocks: [
      { k: "h", t: t("Surprises in the grass", "草丛彩蛋") },
      { k: "p", t: t("Now and then a tuft of grass rustles on the main screen. Tap it. There is one a day (two candies, a medicine, a potion or a common egg), one a week from Monday (a rare egg, a shiny charm or two potions, endurance plus strength or speed; blue sparkles) and one a month (a legendary egg, a Mega Stone or an evolution potion; gold sparkles). They never give points.", "主屏幕地上有时会有一丛草在晃，点它。每天一个（两颗糖果、一颗药、一瓶药剂或一个普通蛋），每周一个（从周一算起；稀有蛋、异色符或两瓶药剂（耐力药剂加一瓶力量或速度药剂），闪蓝光），每月一个（传说蛋、Mega石或进化药水，闪金光）。彩蛋不送积分。") },
      { k: "h", t: t("Weather, seasons and holidays", "天气、季节和节日") },
      {
        k: "list",
        items: [
          t("With Wi-Fi the board shows the real weather where you live: rain, snow, fog and storms, with sunrise and sunset. Weather changes moods by type: Water types enjoy rain, Ice types enjoy snow, Grass types enjoy sun.", "连着 Wi-Fi 时，屏幕会显示你所在地的真实天气：雨、雪、雾、雷暴，还有日出日落。天气按属性影响心情：水属性喜欢下雨，冰属性喜欢下雪，草属性喜欢晴天。"),
          t("The scenery follows the seasons and thirteen holidays, from New Year and Lunar New Year to Halloween and Christmas, and there is a birthday too.", "风景会跟着季节变化，还有十三个节日，从元旦、春节到万圣节、圣诞节，另外还有生日。"),
          t("**Standby clock**: plugged in and untouched for 90 seconds, the screen becomes a large clock with the date and the weather. Tap to go back.", "**待机时钟**：插着电、90 秒没人碰时，屏幕变成大时钟，带日期和天气。点一下回到主屏幕。"),
        ],
      },
      { k: "h", t: t("Steps from an iPhone", "从 iPhone 同步步数") },
      { k: "p", t: t("Handy when the board stays at home. The board must be plugged in and on the home Wi-Fi, the phone on the same network. Your code is on the stats page (settings, then stats).", "板子留在家里时很有用。板子要插着电、连着家里的 Wi-Fi，手机在同一个网络。配对码在统计页（设置 → 统计）。") },
      {
        k: "steps",
        items: [
          t("In the Shortcuts app: Automation, New Automation, **Time of Day** (for example 9:00 PM, daily), Run Immediately.", "在“快捷指令”App 里：自动化 → 新建自动化 → **特定时间**（比如每天 21:00）→ 立即运行。"),
          t("Add **Find Health Samples**: Type is Steps, Start Date is Today.", "添加**查找健康样本**：类型是步数，开始日期是今天。"),
          t("Add **Calculate Statistics**: Sum of the Health Samples.", "添加**计算统计数据**：健康样本的总和。"),
          t("Add **Get Contents of URL**: http://tamapoke.local/steps?n= + the Sum + &k= + your code.", "添加**获取 URL 内容**：http://tamapoke.local/steps?n= + 总和 + &k= + 你的配对码。"),
        ],
      },
      { k: "p", t: t("The first time, allow local network access. The board keeps the higher of the phone's count and its own, so nothing is counted twice.", "第一次运行时选择允许访问本地网络。板子取手机和自己计步中较多的那个，不会重复算。") },
      { k: "h", t: t("Hockey practice from an iPhone", "从 iPhone 记录冰球练习") },
      {
        k: "steps",
        items: [
          t("A new shortcut with **Ask for Input**: type Number, prompt “Minutes of hockey practice?”.", "新建一个快捷指令，添加**请求输入**：类型选数字，提示写“练了几分钟冰球？”。"),
          t("Add **Get Contents of URL**: http://tamapoke.local/hockey?min= + Provided Input + &k= + your code (the same one as for steps).", "添加**获取 URL 内容**：http://tamapoke.local/hockey?min= + 提供的输入 + &k= + 你的配对码（和步数用同一个）。"),
        ],
      },
      { k: "p", t: t("1 point every 5 minutes, up to 6 a day. Keep the shortcut on a parent's phone so the points stay honest.", "每 5 分钟 1 分，每天最多 6 分。快捷指令放在家长的手机上，积分才可靠。") },
      { k: "h", t: t("Rewards from a parent's iPhone", "家长用 iPhone 加分") },
      {
        k: "steps",
        items: [
          t("A new shortcut with **Ask for Input**: type Number, prompt “How many points? (1-10)”.", "新建一个快捷指令，添加**请求输入**：类型选数字，提示写“加几分？（1-10）”。"),
          t("Add **Get Contents of URL**: http://tamapoke.local/reward?pts= + Provided Input + &k= + your code (the same one as for steps).", "添加**获取 URL 内容**：http://tamapoke.local/reward?pts= + 提供的输入 + &k= + 你的配对码（和步数用同一个）。"),
        ],
      },
      { k: "p", t: t("1 to 10 points at a time, for chores, reading or a good day, up to 20 a day. The board wakes up and shows the reward; the points book has a line for it. Same conditions: board plugged in, phone on the same Wi-Fi.", "每次 1 到 10 分，做家务、读书、表现好都可以奖励，每天最多 20 分。板子会亮屏显示加分，积分本里也有一项。条件一样：板子插着电，手机和板子连着同一个 Wi-Fi。") },
      { k: "h", t: t("Voice control", "语音控制") },
      { k: "p", t: t("With a Stack-chan or another xiaozhi voice assistant, the board can be asked how the pet is and told to feed it, play, bathe it or put it to bed. The setup is in the README.", "有 Stack-chan 或其他小智语音设备时，可以用语音问宠物的状态，让它吃饭、玩耍、洗澡或睡觉。设置方法见说明文档。") },
      {
        k: "links",
        items: [{ t: t("Read the README on GitHub", "去 GitHub 看说明文档"), href: "https://github.com/Mu-Muu/TamaPoke#readme" }],
      },
      {
        k: "shots",
        items: [
          { name: "grass", cap: t("Something in the grass", "草丛里有东西") },
          { name: "scene_snow_night", cap: t("A snowy night", "下雪的夜晚") },
          { name: "scene_christmas", cap: t("Christmas", "圣诞节") },
          { name: "standby", cap: t("The standby clock", "待机时钟") },
        ],
      },
    ],
  },
  {
    id: "faq",
    title: t("If something looks wrong", "遇到问题时"),
    lead: t("The questions that come up most.", "最常遇到的几个问题。"),
    blocks: [
      {
        k: "qa",
        items: [
          {
            q: t("The screen is black and touching it does nothing.", "屏幕是黑的，碰它没反应。"),
            a: t("Press **PWR**. The screen was turned off with the button, or by itself after 5 minutes on battery, and then only the button turns it back on. If nothing happens, the battery may be empty or the board switched off: plug it in.", "按一下**电源键**。屏幕可能是被电源键关掉的，也可能是用电池时 5 分钟没人碰自动关的，这时只有电源键能点亮。如果还是没反应，可能是没电了或者关机了，插上电再试。"),
          },
          {
            q: t("I played chess and got no points.", "下了棋，但是没有积分。"),
            a: t("Check four things: the game had 10 minutes or more on the clock (bullet, blitz and bots don't count); the username on the Wi-Fi page is right; the board has synced since (at midnight, at the next start, or with Sync now); and it wasn't the very first sync, which only records where you stand.", "检查四点：这盘棋的基础时间是不是 10 分钟以上（子弹棋、闪电棋和跟电脑下的不算）；Wi-Fi 设置页里的用户名对不对；下完之后板子同步过没有（午夜、下次开机，或点立即同步）；是不是第一次同步，第一次只记录当前成绩。"),
          },
          {
            q: t("It won't connect to Wi-Fi.", "连不上 Wi-Fi。"),
            a: t("The board only uses 2.4 GHz networks. On an iPhone hotspot turn on Maximize Compatibility. To change a network, open the Wi-Fi page again and fill in only the field you want to change.", "板子只支持 2.4 GHz 的网络。iPhone 热点要打开“最大兼容性”。要改某个 Wi-Fi，重新打开 Wi-Fi 设置页，只填要改的那一栏。"),
          },
          {
            q: t("I bought an egg but can't open it.", "买了蛋，但是打不开。"),
            a: t("An egg opens only when you have no companion. Long press your current one and send it to the farm; you can take it back later.", "只有没有伙伴时才能开蛋。长按现在这只，把它送去农场，以后可以再换回来。"),
          },
          {
            q: t("The gyms won't let me in.", "道馆进不去。"),
            a: t("You need two Pokémon in the farm for the team, and the badge of the gym before. Each gym allows one try a day.", "队伍需要农场里有两只宝可梦，还要有上一个道馆的徽章。每个道馆每天只能挑战一次。"),
          },
          {
            q: t("It is sick.", "它生病了。"),
            a: t("Feed, then Medicine (3 points in the shop). If there is one in the bag, it takes it by itself after 10 minutes awake. Clean up poops and go easy on candy to keep it healthy.", "喂食 → 药（商店里 3 分一颗）。背包里有药的话，它醒着病 10 分钟后会自己吃。及时清理便便、少喂糖果，就不容易生病。"),
          },
          {
            q: t("The evolution button doesn't appear.", "到时间了，进化按钮没出现。"),
            a: t("All four needs must be at 40 or more, and each care mistake adds 2 hours to the wait. An evolution potion (100 points) skips the wait in days, but the four needs still have to be at 40 or more.", "四项需求都要在 40 以上，而且每次照顾失误要多等 2 小时。进化药水（100 分）能省掉要等的天数，但四项需求仍要在 40 以上。"),
          },
          {
            q: t("I got a new board. Can I move my game to it?", "换了一块新板子，存档能搬过去吗？"),
            a: t(
              "Yes. On the old board open settings, tap **BACKUP** and **SAVE NOW**. Put its SD card in the new board (the card also holds the farm, the album and the friend photos). Flash the new board, pick any starter, set up Wi-Fi so the clock is right, then tap BACKUP and **RESTORE**. Wi-Fi details are the only thing that does not move. When the new board looks right, erase the old one with the web installer so two boards don't share one game.",
              "可以。在旧板子的设置里点**备份** → **立即备份**，再把它的 SD 卡插到新板子上（卡里还有农场、相册和好友合影）。给新板子刷机，随便选一只初始宝可梦，连上 Wi-Fi 让时钟校准，然后点备份 → **恢复备份**。只有 Wi-Fi 信息不会跟过去。确认新板子没问题后，用网页安装器把旧板子清空，免得两块板子共用一份存档。",
            ),
          },
          {
            q: t("Will an update erase my game?", "更新固件会丢进度吗？"),
            a: t("No. The web installer only replaces the program. The backup on the SD card is a second copy: tap **BACKUP** and **SAVE NOW** in settings before updating if you want it fresh, and **RESTORE** brings it back.", "不会。网页安装器只替换程序。SD 卡上的备份是第二份保险：想要最新的，更新前在设置里点**备份** → **立即备份**；需要时点**恢复备份**就能找回来。"),
          },
        ],
      },
    ],
  },
];
