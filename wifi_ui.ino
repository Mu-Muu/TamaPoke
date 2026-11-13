// Portal Wi-Fi, sincronizacion del ajedrez y ordenes de Stack-chan.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

// ---------- wi-fi: configuracion y sincronizacion ----------

void renderWifiPortal() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(3);
  setCur(centerX(T(S_WIFI_SETUP), 3), 120);
  printT(T(S_WIFI_SETUP));
  setSize(2);
  setCur(centerX(T(S_WIFI_HINT1), 2), 190);
  printT(T(S_WIFI_HINT1));
  setCur(centerX(T(S_WIFI_HINT2), 2), 230);
  printT(T(S_WIFI_HINT2));
  setCur(centerX(T(S_WIFI_HINT3), 2), 270);
  printT(T(S_WIFI_HINT3));
  gfx->setTextColor(UI_TRACK);
  setCur(centerX(T(S_DETAIL_BACK), 2), 330);
  printT(T(S_DETAIL_BACK));
  gfx->flush();
}

// sincroniza con chess.com (bloquea unos segundos: aviso en pantalla mientras)
bool runChessSync(bool manual) {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(T(S_SYNCING), 2), CY - 8);
  printT(T(S_SYNCING));
  gfx->flush();
  ChessReport rep;
  bool ok = chessSync(pet, rep);
  bool nuevo = ok && (rep.games > 0 || rep.bonusPts > 0);
  if (nuevo) {
    lastReport = rep;
    reportPending = true;
  }
  if (!ok && manual) toast(T(S_SYNC_FAIL));
  if (ok && manual && !nuevo) toast(T(S_SYNC_NONE));
  return ok;
}

// ---------- Stack-chan (xiaozhi MCP): sus ordenes, en el bucle principal ----------

void stackchanService() {
  NetCmd c;
  if (!netTakeCommand(c)) return;
  if (c.tool != MCP_STATUS) {  // que se vea lo que hace: enciende la pantalla
    if (screenOff) {
      screenOff = false;
      pet.screenNap(false);
    }
    wakeUI();
  }
  static char out[800];
  static char granja[600];
  StackchanCtx ctx;
  ctx.name = pet.isEgg() ? "" : (pet.nick[0] ? pet.nick : dexName(pet.speciesId));
  ctx.bath = startBathClean;
  if (c.tool == MCP_FARM && farmReady()) {  // "皮卡丘 Lv.3、异色伊布 Lv.12 ..." (como mucho 25)
    size_t n = 0;
    granja[0] = 0;
    for (int i = 0; i < farmCount() && i < 25 && n < sizeof(granja) - 60; i++) {
      const FarmEntry &e = farmAt(i);
      n += snprintf(granja + n, sizeof(granja) - n, "%s%s%s Lv.%u", i ? ", " : "", e.shiny ? "*" : "",
                    e.nick[0] ? e.nick : dexName(e.speciesId), farmLevel(e, pet.lastSeenEpoch));
    }
    if (farmCount() > 25) snprintf(granja + n, sizeof(granja) - n, " (+%d)", farmCount() - 25);
    ctx.farmList = granja;
  }
  stackchanTool(pet, c.tool, c.a, c.b, ctx, out, sizeof(out));
  netGiveResult(c.seq, out);
}
