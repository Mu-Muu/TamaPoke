// Consola serie: provision de la SD y ordenes de depuracion (SHOT, PTS, ...).
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

// ---------- consola serie (provision de SD + depuracion) ----------

// acelerometro cada 20 ms: agitar el huevo cuenta como tocarlo (3 = eclosiona);
// agitar con ganas al compañero lo marea un rato (no le baja el animo)
// ---------- acelerometro: FIFO del QMI8658 ----------
//
// Se lee a mano y como mucho 30 bytes por transferencia I2C. SensorLib pide
// hasta 240 de golpe, y una lectura de mas de 32 bytes (la FIFO del controlador
// I2C) se vacia por interrupcion a mitad: si el otro nucleo escribe en la flash
// justo entonces (el wifi al conectar), el ESP-IDF aborta (spinlock_release en
// i2c_isr_receive_handler) y la placa se queda reiniciando. Leyendo de 6 en 6
// bytes, como antes, eso nunca paso.
bool imuReg(uint8_t reg, uint8_t *buf, uint8_t n) {
  Wire.beginTransmission(imuAddr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)imuAddr, (int)n) != n) return false;
  for (uint8_t k = 0; k < n; k++) buf[k] = Wire.read();
  return true;
}
static bool imuPut(uint8_t reg, uint8_t v) {
  Wire.beginTransmission(imuAddr);
  Wire.write(reg);
  Wire.write(v);
  return Wire.endTransmission() == 0;
}
// orden CTRL9 con su apreton de manos (STATUSINT.bit7 = hecho; luego ACK)
static bool imuCtrl9(uint8_t cmd) {
  if (!imuPut(0x0A, cmd)) return false;
  bool hecho = false;
  uint8_t st;
  for (int k = 0; k < 50 && !hecho; k++) {
    if (imuReg(0x2D, &st, 1) && (st & 0x80)) hecho = true;
    else delayMicroseconds(100);
  }
  imuPut(0x0A, 0x00);  // CTRL_CMD_ACK
  for (int k = 0; k < 50; k++) {
    if (imuReg(0x2D, &st, 1) && !(st & 0x80)) break;
    delayMicroseconds(100);
  }
  return hecho;
}

// Las muestras de la FIFO, cada una con su instante (la ultima es la de ahora):
// con la pantalla apagada y en bateria el bucle solo despierta cada 200 ms, y
// leyendo solo el dato del momento se perdian casi todos los pasos.
void handleShake(uint32_t now) {
  static uint32_t last = 0, lastLog = 0, lastT = 0;
  if (!imuOk || !imuFifoOk || now - last < 50) return;
  last = now;
  uint8_t c[2];
  if (!imuReg(0x15, c, 2)) return;  // FIFO_SMPL_CNT y FIFO_STATUS[1:0]: palabras de 2 bytes
  uint16_t n = (uint16_t)(((c[1] & 0x03) << 8) | c[0]) * 2 / 6;
  if (!n) return;
  if (n > 128) n = 128;  // no caben mas: seria una lectura mala
  bool leido = imuCtrl9(0x05);  // CTRL_CMD_REQ_FIFO: modo lectura (mientras, no entran muestras)
  uint8_t ev = 0;
  float gx = 0, gy = 0, gz = 0;
  uint8_t buf[30];
  for (uint16_t i = 0; leido && i < n;) {
    uint8_t k = n - i > 5 ? 5 : (uint8_t)(n - i);
    if (!imuReg(0x17, buf, k * 6)) break;  // FIFO_DATA
    for (uint8_t j = 0; j < k; j++, i++) {
      const uint8_t *b = buf + j * 6;
      gx = (int16_t)(b[0] | b[1] << 8) / IMU_LSB_PER_G;
      gy = (int16_t)(b[2] | b[3] << 8) / IMU_LSB_PER_G;
      gz = (int16_t)(b[4] | b[5] << 8) / IMU_LSB_PER_G;
      uint32_t t = now - (uint32_t)(n - 1 - i) * IMU_SAMPLE_MS;
      if ((int32_t)(t - lastT) <= 0) t = lastT + 1;  // sin volver atras entre tandas
      lastT = t;
      ev |= shaker.feed(gx, gy, gz, t);
      stepper.feed(sqrtf(gx * gx + gy * gy + gz * gz), t);
    }
  }
  imuPut(0x14, imuFifoCtrl);  // fuera del modo lectura: vuelven a entrar muestras
  if (!leido || (gx == 0 && gy == 0 && gz == 0)) return;
  if (gImuLog && (ev || now - lastLog >= 200)) {
    lastLog = now;
    Serial.printf("imu |a|=%.2f x=%.2f y=%.2f z=%.2f ev=%u\n", sqrtf(gx * gx + gy * gy + gz * gz), gx, gy, gz, ev);
  }
  orientation(gy, gz, now);
  if (!ev || screenOff || pet.awaitingStarter() || pet.ceremony) return;
  if (galleryOpen || farmOpen || hubOpen || chOpen || gymsOpen || funOpen || linkOpen || friendsOpen || shopOpen || cardOpen || kbOpen || clockOpen || wifiPortalActive()) return;
  wakeUI();
  if (pet.isEgg()) {
    if (!pet.vacant && (ev & SHAKE_SOFT)) {
      pet.eggTap();
      sfxPlay(SFX_TAP);
    }
  } else if ((ev & SHAKE_HARD) && !pet.sleeping) {
    if (!timeLeft(dizzyUntil)) sfxPlay(SFX_DENY);
    dizzyUntil = now + DIZZY_MS;
  }
}

void openClock();  // prototipo (consola CLOCK)

// boca abajo 1,5 s: se apaga la pantalla y se duerme (como el boton PWR); al darle
// la vuelta se enciende. Inclinada de lado mas de ~25 grados 0,6 s: el bicho va
// hacia el lado de abajo, una vez (no vuelve a ir hasta que se ponga recta).
// En esta placa el eje x va de abajo arriba de la pantalla (de pie en su soporte
// marca x ~ 0,75) y el y de izquierda a derecha: el lado es el y.
void orientation(float gy, float gz, uint32_t now) {
  static uint32_t downSince = 0, tiltSince = 0, tiltWalkUntil = 0;
  static bool tiltDone = false;
  bool bocaAbajo = gz * faceUpSign < -0.75f;
  if (bocaAbajo) {
    if (!downSince) downSince = now;
    if (now - downSince > 1500 && !screenOff) {
      screenOff = true;
      faceDownOff = true;
      pet.screenNap(true);
    }
  } else {
    downSince = 0;
    if (faceDownOff && gz * faceUpSign > 0.3f) {  // otra vez de cara
      faceDownOff = false;
      if (screenOff) {
        screenOff = false;
        pet.screenNap(false);
        wakeUI();
      }
    }
  }
  bool inclinada = fabsf(gy) > 0.42f && gz * faceUpSign > 0.2f;
  if (tiltWalkUntil && (int32_t)(now - tiltWalkUntil) > 0) {  // acabo la carrera cuesta abajo
    tiltWalkUntil = 0;
    if (!dragOn && walkSpeed > 30.0f) walkSpeed = 30.0f;
  }
  if (!inclinada || screenOff || dragOn || !mainScreenFree() || pet.isEgg() || pet.sleeping || pet.ceremony) {
    tiltSince = 0;
    if (!inclinada) tiltDone = false;
    return;
  }
  if (tiltDone) return;
  if (!tiltSince) tiltSince = now;
  if (now - tiltSince < 600) return;
  tiltDone = true;
  walkSpeed = 60.0f;
  beh.mode = 1;
  // inclinada a la derecha, lo de arriba queda hacia la izquierda: y < 0
  beh.targetX = (gy * TILT_SIGN < 0) ? 386 : 80;
  beh.until = now + 3000;
  tiltWalkUntil = now + 3000;
}

void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  if (sdSerialCommand(line)) return;

  if (line == "HATCH") {
    pet.eggTap(); pet.eggTap(); pet.eggTap();
    Serial.println("DONE");
  } else if (line.startsWith("SPEC ")) {
    int n = line.substring(5).toInt();
    if (n >= 1 && n <= DEX_COUNT) {
      pet.prevSpeciesId = pet.speciesId;
      pet.speciesId = n;
      Serial.printf("especie #%d %s\n", n, DEX_TBL[n].name);
    }
    Serial.println("DONE");
  } else if (line.startsWith("LVL ")) {
    pet.ageMinutes = (uint32_t)line.substring(4).toInt() * MINUTES_PER_LEVEL;
    Serial.println("DONE");
  } else if (line.startsWith("TIME ")) {
    uint32_t e = (uint32_t)line.substring(5).toInt();
    rtcSetEpoch(e);
    pet.setClock(e);
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line.startsWith("RTCSET ")) {  // solo RTC (simular apagados en pruebas)
    rtcSetEpoch((uint32_t)line.substring(7).toInt());
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line == "TIME") {
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line == "GAL") {
    galleryOpen = !galleryOpen;
    galleryRegion = 0;
    galleryGrid = false;
    galleryPage = 0;
    galleryDetail = 0;
    galleryDirty = true;
    if (!galleryOpen) galleryPmd.unload();
    Serial.println("DONE");
  } else if (line == "SICK") {  // enferma / cura al actual (pruebas de la medicina)
    pet.sick = !pet.sick;
    pet.sickMinutes = 0;
    Serial.printf("sick=%d\n", pet.sick);
    Serial.println("DONE");
  } else if (line.startsWith("TAP ")) {  // un toque en x y (pruebas, como el dedo)
    int sp = line.indexOf(' ', 4);
    if (sp > 0) onTap(line.substring(4, sp).toInt(), line.substring(sp + 1).toInt());
    Serial.println("DONE");
  } else if (line.startsWith("SWIPE ")) {  // deslizar: 1 derecha, -1 izquierda (pruebas)
    onSwipe(line.substring(6).toInt() > 0 ? 1 : -1);
    Serial.println("DONE");
  } else if (line.startsWith("MISTAKES ")) {  // fija los descuidos (mantenimiento)
    pet.dbgSetMistakes((uint8_t)constrain(line.substring(9).toInt(), 0, 255));
    Serial.printf("desc=%u\n", pet.careMistakes);
    Serial.println("DONE");
  } else if (line.startsWith("BOND ")) {  // fija el vinculo (mantenimiento)
    pet.dbgSetBond((uint8_t)constrain(line.substring(5).toInt(), 0, 100));
    Serial.printf("bond=%u\n", pet.bond);
    Serial.println("DONE");
  } else if (line.startsWith("NEEDS ")) {  // NEEDS comida animo energia limpieza (pruebas)
    char *p = (char *)line.c_str() + 6;
    long v[4];
    for (int i = 0; i < 4; i++) v[i] = strtol(p, &p, 10);
    pet.dbgNeeds(v[0], v[1], v[2], v[3]);
    Serial.println("DONE");
  } else if (line == "IDLE10") {  // como si nadie lo tocara hace 10 min (prueba del cuidarse solo)
    lastInteract = lastUserAt = millis() - 10UL * 60 * 1000 - 1000;
    Serial.println("DONE");
  } else if (line == "HEAL") {  // todo al maximo: sano, limpio, sin cacas (mantenimiento)
    pet.dbgHeal();
    Serial.println("DONE");
  } else if (line.startsWith("PTS ")) {  // suma (o resta) puntos a mano
    pet.addPoints(line.substring(4).toInt());
    Serial.printf("puntos=%ld\n", (long)pet.points);
    Serial.println("DONE");
  } else if (line == "SYNC") {  // sincroniza el ajedrez ya
    syncRequested = true;
    Serial.println("DONE");
  } else if (line == "WIFI") {
    Serial.printf("wifi=%d sday=%u hoy=%u puntos=%ld\n", wifiHasCreds(), chessSyncDay(),
                  rtcEpoch() / 86400, (long)pet.points);
    Serial.println("DONE");
  } else if (line == "SHOP") {
    if (shopOpen) closeShop(); else openShop(0);
    Serial.println("DONE");
  } else if (line.startsWith("SHOPPG ")) {  // tienda en la pagina n (pruebas)
    openShop(line.substring(7).toInt());
    Serial.println("DONE");
  } else if (line == "CLOCK") {  // ajustes (pruebas)
    statsOpen = false;
    if (clockOpen) clockOpen = false; else openClock();
    Serial.println("DONE");
  } else if (line == "STATSPG") {  // estadisticas (pruebas)
    openClock();
    statsOpen = true;
    Serial.println("DONE");
  } else if (line.startsWith("ACCTEST ")) {  // ver un complemento puesto sin comprarlo (solo en RAM)
    pet.accWorn = (uint8_t)line.substring(8).toInt();
    Serial.println("DONE");
  } else if (line == "WX") {  // tiempo actual y pedir uno nuevo
    Weather w = weatherNow();
    Serial.printf("wx valid=%d kind=%u %dC sol=%d-%d fase=%u mcp=%d online=%d usb=%d\n", w.valid, w.kind,
                  w.tempC, w.sunriseMin, w.sunsetMin, gPhase, mcpConfigured(), mcpOnline(), usbPresent());
    weatherRefresh();
    Serial.println("DONE");
  } else if (line.startsWith("WXSET ")) {  // forzar el tiempo para verlo (-1 = el real)
    gWxForce = line.substring(6).toInt();
    Serial.println("DONE");
  } else if (line.startsWith("PHASE ")) {  // forzar 0 dia, 1 amanecer/atardecer, 2 noche (-1 = real)
    gPhaseForce = line.substring(6).toInt();
    Serial.println("DONE");
  } else if (line.startsWith("BDAY ")) {  // cumpleaños del dueño: BDAY mes dia (BDAY 0 0 = ninguno)
    int sp = line.indexOf(' ', 5);
    int m = line.substring(5, sp).toInt(), d = sp > 0 ? line.substring(sp + 1).toInt() : 0;
    if (m < 0 || m > 12 || d < 0 || d > 31) m = d = 0;
    gBdayM = m;
    gBdayD = d;
    Preferences f;
    f.begin("fest", false);
    f.putUChar("bm", gBdayM);
    f.putUChar("bd", gBdayD);
    f.end();
    Serial.printf("bday=%u/%u\n", gBdayM, gBdayD);
    Serial.println("DONE");
  } else if (line.startsWith("SEASON ")) {  // ver una estacion: SEASON n (-1 = la de hoy)
    gSeasonForce = line.substring(7).toInt();
    if (gSeasonForce > 3) gSeasonForce = -1;
    Serial.println("DONE");
  } else if (line.startsWith("HOLI ")) {  // ver una fiesta: HOLI n (-1 = la de hoy)
    gHolForce = line.substring(5).toInt();
    if (gHolForce >= HOL_COUNT) gHolForce = -1;
    Serial.printf("fiesta=%d\n", gHolForce);
    Serial.println("DONE");
  } else if (line.startsWith("REGTEST ")) {  // solo el cartel de region completa (no da premio)
    pet.dbgRegionBanner(line.substring(8).toInt());
    Serial.println("DONE");
  } else if (line == "SHOT") {  // captura de pantalla: "SHOT w h\n" + w*h*2 bytes RGB565 (para revisar en el PC)
    render();  // un frame fresco en el lienzo
    uint16_t *fb = gfx->getFramebuffer();
    Serial.setTxTimeoutMs(500);  // aqui si se espera al PC (si no, se pierden trozos)
    Serial.printf("SHOT %d %d\n", LCD_WIDTH, LCD_HEIGHT);
    Serial.flush();
    const uint8_t *b = (const uint8_t *)fb;
    size_t n = (size_t)LCD_WIDTH * LCD_HEIGHT * 2;
    for (size_t i = 0; i < n;) {
      size_t w = Serial.write(b + i, n - i < 512 ? n - i : 512);
      if (!w) delay(2);  // bufer lleno: se reintenta el mismo trozo
      i += w;
    }
    Serial.flush();
    Serial.setTxTimeoutMs(0);
    Serial.println("DONE");
  } else if (line.startsWith("MCPURL ")) {  // punto MCP sin pasar por el portal (pruebas; "-" lo borra)
    String u = line.substring(7);
    u.trim();
    Preferences w;
    w.begin("wifi", false);
    if (u == "-") w.remove("mcp");
    else w.putString("mcp", u);
    w.end();
    Serial.println("DONE");
  } else if (line == "BACKUP") {  // copia de seguridad ya
    pet.flushSave();
    Serial.printf("copia %s\n", backupSave(pet.lastSeenEpoch) ? "ok" : "FALLO");
    Serial.println("DONE");
  } else if (line == "BKINFO") {
    uint32_t e = 0;
    bool hay = backupInfo(e);
    Serial.printf("copia=%d hora=%lu\n", hay, (unsigned long)e);
    Serial.println("DONE");
  } else if (line.startsWith("SPORT ")) {  // abre la prueba n de entrenamiento: sin premio ni cansancio (pruebas)
    int n = line.substring(6).toInt();
    if (n >= 0 && n < EV_COUNT) sportsPractice(n);
    else if (chOpen) sportsSwipe(1);
    Serial.println("DONE");
  } else if (line.startsWith("LUCK ")) {  // LUCK 0/1/2: hoy "sin / jugo / gano" al ajedrez; LUCK -1 deja lo de antes
    static uint16_t d0, w0;
    static bool guardado = false;
    int n = line.substring(5).toInt();
    if (n < 0) {
      if (guardado) { pet.chessDay = d0; pet.chessWinDay = w0; guardado = false; }
    } else {
      if (!guardado) { d0 = pet.chessDay; w0 = pet.chessWinDay; guardado = true; }
      pet.chessDay = n >= 1 ? pet.dayNow() : 0;
      pet.chessWinDay = n >= 2 ? pet.dayNow() : 0;
    }
    Serial.printf("suerte=%u\n", pet.chessLuck());
    Serial.println("DONE");
  } else if (line.startsWith("BKFARM")) {  // BKFARM: quien hay en la copia de la SD; BKFARM 1: a la granja
    PetRec r;
    if (!backupPetRec(r, pet.lastSeenEpoch)) {
      Serial.println("copia: no se puede leer");
    } else {
      Serial.printf("copia: dex=%d nv=%u edad=%lu min genes=%u/%u/%u tr=%u/%u/%u vinculo=%u medallas=0x%X shiny=%u apodo=%s\n",
                    r.speciesId, levelOfAge(r.ageMinutes), (unsigned long)r.ageMinutes, r.geneAtk, r.geneDef,
                    r.geneSpe, r.trAtk, r.trDef, r.trSpe, r.bond, r.medals, r.shiny, r.nick);
      if (line.endsWith(" 1")) {
        bool ya = pet.speciesId == r.speciesId;
        for (int i = 0; i < farmCount() && !ya; i++) ya = farmAt(i).speciesId == r.speciesId;
        if (ya) Serial.println("ya hay uno de esa especie (companero o granja): no se toca nada");
        else Serial.printf("a la granja: %s\n", farmPut(pet, r) ? "ok" : "ERROR");
      }
    }
    Serial.println("DONE");
  } else if (line == "HKCLEAR") {  // borra los minutos de hockey de hoy (pruebas; los puntos ya dados no)
    pet.hockeyMin = 0;
    pet.hockeyPts = 0;
    Serial.println("DONE");
  } else if (line == "WEEKFX") {  // el cartel del resumen de la semana, otra vez (pruebas)
    pet.ptReport = true;
    Serial.println("DONE");
  } else if (line == "FARMUNDO") {  // deshace la ultima fruta dada a uno de la granja (pruebas)
    Serial.printf("granja deshecha=%d\n", farmUndoFruit());
    Serial.println("DONE");
  } else if (line.startsWith("GRASSFX ")) {  // GRASSFX s: la mata de la hierba se ve s segundos (solo se ve)
    grassFxUntil = millis() + line.substring(8).toInt() * 1000UL;
    Serial.println("DONE");
  } else if (line.startsWith("BAG ")) {  // BAG i n: deja n del objeto i en la mochila (pruebas)
    int i = line.substring(4).toInt(), sp = line.indexOf(' ', 4);
    if (sp > 0 && i >= 0 && i < ITEM_COUNT) pet.bag[i] = (uint16_t)line.substring(sp + 1).toInt();
    Serial.println("DONE");
  } else if (line.startsWith("AWAY ")) {  // AWAY s: el compañero "de expedicion" s segundos (cartel); 0 lo quita
    long s = line.substring(5).toInt();
    pet.questEnd = s > 0 ? pet.lastSeenEpoch + s : 0;
    pet.questDex = pet.speciesId;
    Serial.println("DONE");
  } else if (line.startsWith("FUN ")) {  // FUN a: actividad a del ocio, de prueba (sin puntos ni premios, 600x); -1 sale
    funPractice(line.substring(4).toInt());
    Serial.println("DONE");
  } else if (line.startsWith("GYM ")) {  // GYM r k: ficha del gimnasio k (8 = liga) de la region r, de prueba
    int r = line.substring(4).toInt(), sp = line.indexOf(' ', 4);
    if (sp > 0) gymsPractice((uint8_t)r, (uint8_t)line.substring(sp + 1).toInt());
    Serial.println("DONE");
  } else if (line.startsWith("MEGA ")) {  // se ve megaevolucionado n segundos (0 = ya no), sin gastar piedra (pruebas)
    long s = line.substring(5).toInt();
    pet.megaUntil = s > 0 && pet.lastSeenEpoch ? pet.lastSeenEpoch + s : 0;
    Serial.printf("mega=%d hasta=%lu\n", pet.isMega(), (unsigned long)pet.megaUntil);
    Serial.println("DONE");
  } else if (line.startsWith("SHOTNAME")) {  // capturas: SHOTNAME Ash = ese nombre en pantalla; SHOTNAME = los de verdad
    String n = line.substring(8);
    n.trim();
    strncpy(shotName, n.c_str(), sizeof(shotName) - 1);
    shotName[sizeof(shotName) - 1] = 0;
    Serial.printf("shotname=%s\n", shotName);
    Serial.println("DONE");
  } else if (line.startsWith("FRIENDTEST")) {  // FRIENDTEST 1/0: juegos con un amigo sin efectos (solo RAM)
    friendSetTest(line.substring(10).toInt() != 0);
    Serial.printf("friendtest=%d\n", friendTestMode());
    Serial.println("DONE");
  } else if (line.startsWith("MAIL")) {  // correo con amigos lejos (mailbox.h): pruebas
    consoleMail(line.substring(4));
  } else if (line.startsWith("LINK")) {  // "见面" sin la pagina (pruebas con dos placas; ver link.h)
    consoleLink(line.substring(4));
  } else if (line == "VISIT") {  // que venga ya alguien de la granja (pruebas)
    nextVisit = millis();
    Serial.println("DONE");
  } else if (line.startsWith("CLOCKAT ")) {  // los relojes enseñan esta hora: CLOCKAT hh mm (CLOCKAT -1 = la real)
    int h = line.substring(8).toInt(), sp = line.indexOf(' ', 8);
    int m = sp > 0 ? line.substring(sp + 1).toInt() : 0;
    gClockForce = (h >= 0 && h < 24 && m >= 0 && m < 60) ? (int16_t)(h * 60 + m) : -1;
    Serial.println("DONE");
  } else if (line == "BATHFX") {  // solo la animacion del baño, sin limpiar (capturas)
    startBath(false);
    Serial.println("DONE");
  } else if (line == "WAKE") {  // como un toque: enciende la pantalla y quita el reloj de espera (capturas)
    wakeUI();
    Serial.println("DONE");
  } else if (line == "SAY") {  // que diga algo ya (pruebas)
    nextBubble = millis();
    Serial.println("DONE");
  } else if (line == "ALBUM") {  // abre la granja en el album (pruebas)
    openFarm();
    albumOpen = true;
    albumPage = 0;
    Serial.println("DONE");
  } else if (line.startsWith("CARD ")) {  // abre la ficha en la pagina n (pruebas; -1 la cierra)
    int n = line.substring(5).toInt();
    cardOpen = n >= 0 && n < CARD_PAGES;
    if (cardOpen) cardPage = n;
    Serial.println("DONE");
  } else if (line == "STEPS") {
    Serial.printf("pasos hoy=%lu puntos dados=%u detectados=%lu\n", (unsigned long)pet.stepsToday, pet.stepPts,
                  (unsigned long)stepper.total);
    Serial.println("DONE");
  } else if (line.startsWith("ACT ")) {  // que haga ya una actividad propia (pruebas)
    forceAct = line.substring(4).toInt();
    beh.until = 0;
    Serial.println("DONE");
  } else if (line == "BATH") {  // baño (pruebas)
    startBath(true);
    Serial.println("DONE");
  } else if (line.startsWith("REACT ")) {  // ver una reaccion a un gesto sin tocar (pruebas)
    int n = line.substring(6).toInt();
    if (n == 1) react(PMD_NOD, 3000, T(S_SAY_HEAD));
    else if (n == 2) react(REACT_BOUNCE, 3000, T(S_SAY_BELLY));
    else if (n == 3) react(REACT_BOUNCE, 3000, T(S_SAY_TICKLE));
    else if (n == 4) react(PMD_HURT, 3000, T(S_SAY_ANNOYED));
    else if (n == 5) react(REACT_SPIN, 3000, T(S_SAY_SPIN));
    Serial.println("DONE");
  } else if (line.startsWith("FXTEST ")) {  // ver una celebracion sin que pase de verdad (pruebas)
    int n = line.substring(7).toInt();
    if (n == 1) {  // subir de nivel
      levelUpShow = pet.level() + 1;
      levelUpUntil = millis() + 3000;
    } else if (n == 2) {  // cosecha
      char p1[16];
      snprintf(p1, sizeof(p1), T(S_HARVEST_PTS_FMT), 1);
      snprintf(harvestMsg, sizeof(harvestMsg), "%s  %s", p1, T(ITEM_NAME[ITEM_CANDY]));
      harvestUntil = millis() + 6000;
    }
    Serial.println("DONE");
  } else if (line == "LIGHT") {  // dormir / despertar (pruebas)
    pet.toggleLight();
    Serial.println("DONE");
  } else if (line == "IMULOG") {
    gImuLog = !gImuLog;
    Serial.printf("imu=%d imulog=%d\n", imuOk, gImuLog);
    Serial.println("DONE");
  } else if (line == "TOUCHLOG") {
    gTouchLog = !gTouchLog;
    Serial.printf("touchlog=%d\n", gTouchLog);
    Serial.println("DONE");
  } else if (line == "FARM") {  // abre/cierra la granja
    if (farmOpen) closeFarm(); else openFarm();
    Serial.printf("granja=%d\n", farmCount());
    Serial.println("DONE");
  } else if (line.startsWith("DEXSHOW ")) {  // ficha de la pokedex de una especie (pruebas; con DEXALL)
    int16_t d = line.substring(8).toInt();
    if (d >= 1 && d <= DEX_COUNT) {
      galleryOpen = true;
      galleryRegion = regionOf(d);
      galleryGrid = true;
      galleryPage = (d - REGIONS[galleryRegion].first) / 16;
      galleryDetail = d;
      galleryPmd.load(d, false);
      galleryDirty = true;
    }
    Serial.println("DONE");
  } else if (line.startsWith("REGPAGE ")) {  // pokedex en la portada de una region (pruebas)
    int r = line.substring(8).toInt();
    if (r >= 0 && r < REGION_COUNT) {
      galleryOpen = true;
      galleryRegion = r;
      galleryGrid = false;
      galleryPage = 0;
      galleryDetail = 0;
      galleryDirty = true;
    }
    Serial.println("DONE");
  } else if (line == "DEXALL") {  // ver toda la pokedex sin registrarla (pruebas)
    galleryShowAll = !galleryShowAll;
    galleryDirty = true;
    Serial.printf("dexall=%d\n", galleryShowAll);
    Serial.println("DONE");
  } else if (line == "EGGS") {
    // simula 20 tiradas de huevo (no cambia el estado del juego)
    for (int i = 0; i < 20; i++) {
      int16_t d = pet.pickEggSpecies();
      Serial.printf("%d:%s(r%u) ", d, DEX_TBL[d].name, DEX_TBL[d].rarity);
    }
    Serial.println();
    Serial.println("DONE");
  } else if (line == "SHINY") {  // alterna shiny del actual (pruebas)
    pet.shiny = !pet.shiny;
    Serial.printf("shiny=%d\n", pet.shiny);
    Serial.println("DONE");
  } else if (line.startsWith("NICK ")) {
    pet.rename(line.substring(5).c_str());
    Serial.printf("nick=%s\n", pet.nick);
    Serial.println("DONE");
  } else if (line == "CAREDAY") {  // simula un dia nuevo cuidado (pruebas)
    pet.setClock(pet.lastSeenEpoch + 86400);
    pet.caress();
    Serial.printf("streak=%u bond=%u medals=0x%X\n", pet.streak, pet.bond, pet.medals);
    Serial.println("DONE");
  } else if (line == "BYE") {
    farewellToFarm();
    Serial.println("DONE");
  } else if (line == "RUN") {
    pet.startRunaway();
    Serial.println("DONE");
  } else if (line == "BEEP") {
    sfxPlay(SFX_HATCH);  // prueba de audio
    Serial.println("DONE");
  } else if (line == "ABANDON") {
    pet.dbgRunawayReady();  // fuerza el estado "lista para escaparse" (test del boton)
    Serial.println("DONE");
  } else if (line == "WIPE") {
    pet.factoryReset();     // borra NVS y reinicia -> partida nueva (eleccion de inicial)
    Serial.println("DONE");
    delay(100);
    ESP.restart();
  } else if (line == "REG") {
    Serial.printf("pokedex %u/%d:", pet.registeredCount(), DEX_COUNT);
    for (int i = 1; i <= DEX_COUNT; i++)
      if (pet.isRegistered(i)) Serial.printf(" %d", i);
    Serial.println();
    Serial.println("DONE");
  } else if (line == "HEALTH") {
    Serial.printf("up=%lus heap=%u min=%u sd=%d mon=%d\n",
                  (unsigned long)(millis() / 1000), ESP.getFreeHeap(),
                  ESP.getMinFreeHeap(), sdReady, pmd.loaded || mon.loaded);
    Serial.println("DONE");
  } else if (line == "STATS") {
    Serial.printf("spec=%d nv=%u com=%u fel=%u ene=%u lim=%u desc=%u sd=%d mon=%d bat=%d usb=%d rtc=%u\n",
                  pet.speciesId, pet.level(), pet.fullness, pet.joy, pet.energy,
                  pet.hygiene, pet.careMistakes, sdReady, mon.loaded,
                  batPercent(), usbPresent(), rtcEpoch());
    Serial.printf("peso=%u fue=%u def=%u vel=%u genes=%u/%u/%u tr=%u/%u/%u\n",
                  pet.weight, pet.atkStat(), pet.defStat(), pet.speStat(),
                  pet.geneAtk, pet.geneDef, pet.geneSpe,
                  pet.trAtk, pet.trDef, pet.trSpe);
    Serial.printf("beh x=%d act=%u mode=%u walk=%.0f slp=%d idle=%lus user=%lus\n", (int)beh.x, beh.activity,
                  beh.mode, walkSpeed, pet.sleeping, (unsigned long)(idleSince(millis(), lastInteract) / 1000),
                  (unsigned long)(idleSince(millis(), lastUserAt) / 1000));
    Serial.printf("shiny=%d streak=%u/%u bond=%u medals=0x%X(%u) nick=%s\n",
                  pet.shiny, pet.streak, pet.bestStreak, pet.bond, pet.medals,
                  pet.totalMedals, pet.nick);
    Serial.println("DONE");
  }
}

// LINK ON [min] / OFF / INFO / PAIRTEST / PING n [bytes] / BIG n / VER [a.b] / FORGET i|ALL / PAGE
// PAIRTEST y VER solo cambian la RAM; FORGET si borra amigos guardados (solo para limpiar)
void consoleLink(String a) {
  a.trim();
  if (a.startsWith("ON")) {
    int min = a.substring(2).toInt();
    bool ok = linkStart((uint32_t)(min > 0 ? min : 10) * 60000UL);
    Serial.println(ok ? "link: ON" : "link: no se pudo");
  } else if (a == "OFF") {
    if (linkOpen) closeLink();
    else linkStop();
  } else if (a == "INFO") {
    linkInfo();
  } else if (a == "PAIRTEST") {
    Serial.println(linkPairTest() ? "link: emparejando (solo RAM)..." : "link: no hay ninguna placa nueva cerca");
  } else if (a.startsWith("PING") || a.startsWith("BIG")) {
    bool big = a.startsWith("BIG");
    a = a.substring(big ? 3 : 4);
    a.trim();
    int n = a.toInt(), sp = a.indexOf(' ');
    int len = big ? LINK_BODY_MAX : sp > 0 ? a.substring(sp + 1).toInt() : LINK_PING_MIN;
    if (linkPingTest(n > 0 ? n : 20, len)) return;  // el resultado (y DONE) sale al acabar
    Serial.println("link: no hay ningun amigo que conteste cerca");
  } else if (a.startsWith("VER")) {
    a = a.substring(3);
    a.trim();
    int dot = a.indexOf('.');
    if (a.length() && dot > 0) linkSetVersion(a.substring(0, dot).toInt(), a.substring(dot + 1).toInt());
    else linkSetVersion(LINK_VER_MAJOR, LINK_VER_MINOR);
    linkInfo();
  } else if (a.startsWith("FORGET")) {
    a = a.substring(6);
    a.trim();
    if (!linkActive()) Serial.println("link: primero LINK ON (los amigos se leen al abrir)");
    else if (a == "ALL") while (linkFriendCount()) linkForget(0);
    else if (a.length()) Serial.println(linkForget(a.toInt()) ? "link: olvidado" : "link: no existe");
  } else if (a == "PAGE") {
    if (linkOpen) closeLink();
    else { hubOpen = true; openLink(); }
  } else {
    Serial.println("LINK ON [min] | OFF | INFO | PAIRTEST | PING n [bytes] | BIG n | VER [a.b] | FORGET i|ALL | PAGE");
  }
  Serial.println("DONE");
}

// MAIL INFO | SYNC | FAKE v|c|g (solo se ve: un mensajero de mentira en la principal) |
// SEND v|c|g i [frase/objeto] (de verdad: al amigo i; con FRIENDTEST un regalo no sale de la mochila)
void consoleMail(String a) {
  a.trim();
  if (a == "INFO") {
    mailInfo();
    mailVisitorInfo();
  } else if (a == "SYNC") {
    mailSyncSoon();
    Serial.println("correo: se recoge en la proxima vuelta de la red");
  } else if (a.startsWith("FAKE")) {
    MailMsg m;
    char k = a.length() > 5 ? a[5] : 'c';
    m.type = k == 'v' ? MT_VISIT : k == 'g' ? MT_GIFT : MT_CARD;
    linkFriendsLoad();
    m.from = linkFriendCount() ? linkFriendAt(0).pid : 0;
    m.species = 25;
    m.level = 12;
    m.phrase = 1;
    m.sticker = 0;
    m.item = ITEM_CANDY;
    m.qty = 1;
    m.id = esp_random();
    Serial.println(mailFakeArrival(m) ? "correo: mensajero de prueba en camino (solo se ve)" : "correo: lleno");
    if (m.type == MT_GIFT && !friendTestMode()) Serial.println("correo: OJO, sin FRIENDTEST el regalo de prueba si va a la mochila");
  } else if (a.startsWith("SEND")) {
    a = a.substring(4);
    a.trim();
    char k = a.length() ? a[0] : 'c';
    int sp = a.indexOf(' '), sp2 = sp > 0 ? a.indexOf(' ', sp + 1) : -1;
    int fi = sp > 0 ? a.substring(sp + 1).toInt() : 0;
    int arg = sp2 > 0 ? a.substring(sp2 + 1).toInt() : 0;
    linkFriendsLoad();
    if (fi >= linkFriendCount() || !mailFriendReady(linkFriendAt(fi).pid)) {
      Serial.println("correo: ese amigo no esta o aun no se sabe su buzon");
    } else {
      MailMsg m;
      m.type = k == 'v' ? MT_VISIT : k == 'g' ? MT_GIFT : MT_CARD;
      m.species = pet.isEgg() ? 25 : pet.speciesId;
      m.shiny = pet.shiny;
      m.level = pet.level();
      m.phrase = (uint8_t)(arg % MAIL_PHRASES);
      m.sticker = (uint8_t)(arg % MAIL_STICKERS);
      m.item = (uint8_t)(arg < ITEM_COUNT ? arg : 0);
      m.qty = 1;
      bool deMochila = m.type == MT_GIFT && !friendTestMode();
      if (deMochila && !pet.bag[m.item]) {  // si no, el regalo salia de la nada (y a los 14 dias volvia)
        Serial.println("correo: ese objeto no esta en la mochila (FRIENDTEST 1 para probar sin gastarlo)");
      } else {
        bool ok = mailSend(linkFriendAt(fi).pid, m);
        if (ok && deMochila) {
          pet.bag[m.item]--;
          pet.markDirty();
          pet.flushSave();
        }
        Serial.println(ok ? "correo: a la cola (sale en la proxima vuelta de la red)" : "correo: cola llena");
      }
    }
  } else {
    Serial.println("MAIL INFO | SYNC | FAKE v|c|g | SEND v|c|g amigo [frase|objeto]");
  }
  Serial.println("DONE");
}
