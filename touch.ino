// Entrada tactil: toques, deslizamientos y gestos sobre el bicho.
// (pestaña de TamaPoke.ino: Arduino la une al final del principal)

// ---------- entrada tactil ----------

bool inPetZone(int16_t x, int16_t y) {
  return x > 110 && x < 356 && y > 95 && y < 310;
}

// gesto en curso que cuenta como pulsacion larga sobre el bicho (sin mirar el
// tiempo). NO mira swallowGesture: el toque que despierta la pantalla atenuada
// no debe accionar botones, pero sostenerlo 3 s sobre el bicho es a proposito
// (antes se tragaba y "la pulsacion larga no hacia nada" al volver a la placa).
bool holdOnPet() {
  return !holdFired && !gestureMoved && !strokeFired && !dragFired && !galleryOpen && !farmOpen && !cardOpen && !kbOpen && !clockOpen &&
         abs(tXl - tX0) < 30 && abs(tYl - tY0) < 30 && inPetZone(tX0, tY0) &&
         !pet.isEgg() && !confirmUntil && !actionUntil && !pet.ceremony;
}

// caricia: el dedo empezo sobre el bicho y va y viene en horizontal; cada dos
// vueltas (cambios de sentido de mas de 15 px) es una caricia (como mucho una
// cada 1,2 s)
void strokeTrack(int16_t x, int16_t y) {
  if (!inPetZone(tX0, tY0) || !mainScreenFree() || pet.isEgg() || pet.sleeping || pet.ceremony || swallowGesture)
    return;
  int8_t d = x > strokeLastX ? 1 : x < strokeLastX ? -1 : 0;
  if (d && d == strokeDir && ((d > 0 && x > strokeExt) || (d < 0 && x < strokeExt))) strokeExt = x;
  if (d && d != strokeDir && abs(x - strokeExt) > 15) {  // vuelta
    if (strokeDir) strokeTurns++;
    strokeDir = d;
    strokeExt = x;
  } else if (!strokeDir && abs(x - tX0) > 15) {
    strokeDir = d;
    strokeExt = x;
  }
  strokeLastX = x;
  uint32_t now = millis();
  if (strokeTurns >= 2 && now - lastStroke > 1200) {
    strokeTurns = 0;
    lastStroke = now;
    strokeFired = true;
    pet.stroke();
    if (!pet.sleeping) sfxPlay(SFX_HEART);
    react(PMD_NOD, 1600, T(S_SAY_STROKE));
  }
}

// arrastrar: el dedo empezo sobre el bicho, se quedo quieto 250 ms y luego se
// mueve en horizontal: el bicho va (rapido) hacia donde este el dedo
void dragTrack(int16_t x, int16_t y) {
  uint32_t dt = millis() - tStart;
  if (dt < 250) {
    if (abs(x - tX0) > 20 || abs(y - tY0) > 20) earlyMove = true;  // eso es deslizar
    return;
  }
  if (!dragOn) {
    if (earlyMove || strokeFired || swallowGesture || !mainScreenFree() || pet.isEgg() || pet.sleeping ||
        pet.ceremony || !pmd.loaded || abs(tX0 - (int)beh.x) > 70 || tY0 < gPmdTopY - 20 || abs(x - tX0) < 25)
      return;
    dragOn = dragFired = true;
    walkSpeed = 160.0f;
  }
  beh.mode = 1;
  beh.targetX = x < 80 ? 80 : x > 386 ? 386 : x;
  beh.until = millis() + 4000;
}

// circulo alrededor del bicho (lejos de el, 60-200 px): una vuelta entera = gira
void circleTrack(int16_t x, int16_t y) {
  if (circFired || dragOn || swallowGesture || !mainScreenFree() || pet.isEgg() || pet.sleeping || pet.ceremony)
    return;
  float cx = beh.x, cy = (gPmdTopY + PET_GROUND) / 2.0f, dx = x - cx, dy = y - cy;
  float r = sqrtf(dx * dx + dy * dy);
  if (r < 60 || r > 200) return;
  float a = atan2f(dy, dx);
  if (circHave) {
    float d = a - circLast;
    if (d > PI) d -= 2 * PI;
    if (d < -PI) d += 2 * PI;
    circAng += d;
  }
  circHave = true;
  circLast = a;
  if (fabsf(circAng) > 2 * PI * 0.9f) {
    circFired = true;
    pet.caress();
    react(REACT_SPIN, 1600, T(S_SAY_SPIN));
  }
}

// lado de la pantalla hacia arriba: se aprende cada vez que la tocan (se toca de cara)
void learnFaceUp() {
  static AccelerometerData a;
  if (imuOk && imu.readAccel(a)) {
    float gz = a.mps2.z / 9.80665f;
    if (fabsf(gz) > 0.7f) faceUpSign = gz > 0 ? 1 : -1;
  }
}

// el toque se resuelve al LEVANTAR el dedo para distinguir tap de deslizar
void handleTouch() {
  static uint32_t lastPoll = 0;
  if (millis() - lastPoll < 20) return;  // 50 Hz le sobra a un dedo
  lastPoll = millis();
  // solo tocamos el bus si el chip aviso por INT o si el dedo sigue abajo (hay
  // que detectar el levantamiento). Leer el CST9217 dormido se colgaba ~1s y
  // congelaba el loop entero; SensorLib no respeta el timeout de Wire.
  if (!gTouchIrq && !wasPressed) return;
  gTouchIrq = false;
  // Un ciclo de solo direccion antes de leer. Sin esto, getPoint() se colgaba
  // exactamente 1000 ms (el timeout por defecto del driver I2C, que SensorLib no
  // acota) y congelaba el loop entero: es lo que se percibia como "el minijuego
  // se congela 2-3 segundos al tocar la bola" (issue #16), porque ahi los toques
  // son rapidos y seguidos.
  //
  // Medido en placa, jugando lo mismo (~470 lecturas, marcador ~50):
  //   sin esta linea: 5 parones de 1000 ms en 60 s
  //   con ella:       0
  // El contador de rechazos salio 0 en ambos casos, asi que NO funciona
  // saltandose lecturas cuando el chip no contesta: lo que hace es despertarlo,
  // para que la lectura siguiente no se encuentre el CST9217 dormido.
  Wire.beginTransmission(TOUCH_ADDR);
  if (Wire.endTransmission() != 0) return;  // no responde: se reintenta en 20 ms
  int16_t x, y;
  bool pressed = touch.getPoint(&x, &y, 1) > 0;

  if (pressed && !wasPressed) {  // empieza el gesto
    tX0 = tXl = x;
    tY0 = tYl = y;
    tStart = millis();
    holdFired = false;
    gestureMoved = false;
    strokeFired = false;
    dragOn = dragFired = earlyMove = false;
    circHave = circFired = false;
    circAng = 0;
    learnFaceUp();
    strokeTurns = 0;
    strokeDir = 0;
    strokeLastX = strokeExt = x;
    swallowGesture = (dimStage > 0) || screenOff;  // si estaba a oscuras, solo despierta
    if (screenOff) pet.screenNap(false);
    screenOff = false;
    wakeUI();
    if (gTouchLog) Serial.printf("down %d,%d swallow=%d pet=%d\n", x, y, swallowGesture, inPetZone(x, y));
  } else if (pressed) {  // sigue apoyado
    tXl = x;
    tYl = y;
    if (abs(x - tX0) > 30 || abs(y - tY0) > 30) gestureMoved = true;
    if (!dragOn) strokeTrack(x, y);
    dragTrack(x, y);
    circleTrack(x, y);
    // pulsacion larga sin moverse sobre el bicho -> dialogo de soltar
    if (holdOnPet() && millis() - tStart > 3000) {
      actionUntil = millis() + 10000;
      holdFired = true;
    }
  } else if (wasPressed) {  // levanta el dedo: resolver gesto
    wakeUI();
    int dx = tXl - tX0, dy = tYl - tY0;
    uint32_t dt = millis() - tStart;
    if (gTouchLog) Serial.printf("up dt=%lu dx=%d dy=%d hold=%d\n", (unsigned long)dt, dx, dy, holdFired);
    if (dragOn) {  // al soltarlo vuelve a su paso normal
      dragOn = false;
      walkSpeed = 30.0f;
    }
    if (!holdFired && !swallowGesture && !strokeFired && !dragFired && !circFired) {
      if (abs(dx) > 80 && abs(dy) < 70 && dt < 800) onSwipe(dx > 0 ? 1 : -1);
      else if (abs(dy) > 80 && abs(dx) < 70 && dt < 800) onSwipeV(dy > 0 ? 1 : -1);
      else if (dt < 1500 && abs(dx) < 40 && abs(dy) < 40) onTap(tX0, tY0);
    }
  }
  wasPressed = pressed;
}

// deslizar vertical: abre/cierra la ficha del bicho
void openClock();  // prototipo

void onSwipeV(int dir) {
  if (pet.awaitingStarter()) return;  // bloqueado durante la eleccion de inicial
  if (galleryOpen) {
    gallerySwipeV(dir);
    return;
  }
  if (farmOpen || shopOpen || kbOpen || pet.ceremony) return;
  if (clockOpen) { clockOpen = statsOpen = false; return; }
  if (cardOpen) {
    if (dir < 0) cardOpen = false;  // arriba cierra la ficha
    return;
  }
  if (dir > 0) {                    // deslizar abajo: ajustar hora
    if (!confirmUntil && !actionUntil && !feedMenuUntil) openClock();
  } else if (!pet.isEgg() && !confirmUntil && !actionUntil && !feedMenuUntil) {
    cardOpen = true;                // deslizar arriba: ficha
    cardPage = 0;
  }
}

// deslizar: dir +1 = hacia la derecha
void onSwipe(int dir) {
  if (pet.awaitingStarter()) return;  // bloqueado durante la eleccion de inicial
  if (kbOpen || clockOpen) return;
  if (shopOpen) {
    shopSwipe(dir);
    return;
  }
  if (farmOpen) {
    farmSwipe(dir);
    return;
  }
  if (cardOpen) {  // dentro de la ficha: cambiar entre las CARD_PAGES paginas
    int p = (int)cardPage + (dir > 0 ? -1 : 1);  // izquierda avanza
    cardPage = p < 0 ? 0 : (p > CARD_PAGES - 1 ? CARD_PAGES - 1 : p);
    return;
  }
  // desde la pantalla principal: derecha = pokedex, izquierda = granja
  // (como tres paginas en fila: [pokedex] [principal] [granja])
  if (!galleryOpen) {
    if (!pet.ceremony && !confirmUntil && !actionUntil) {
      if (dir > 0) {
        galleryOpen = true;
        galleryRegion = 0;  // la portada de Kanto, la que esta al lado de la principal
        galleryGrid = false;
        galleryPage = 0;
        galleryDetail = 0;
        galleryDirty = true;
      } else {
        openFarm();
      }
    }
    return;
  }
  if (galleryDetail) {  // en detalle: volver a la rejilla
    galleryDetail = 0;
    galleryPmd.unload();
    galleryDirty = true;
    return;
  }
  if (galleryGrid) {  // en la rejilla: volver a la portada de la region
    galleryGrid = false;
    galleryDirty = true;
    return;
  }
  // las portadas estan a la izquierda de la principal, en fila: a la derecha
  // la siguiente region, a la izquierda la anterior; de Kanto a la izquierda, a casa
  int nr = galleryRegion + (dir > 0 ? 1 : -1);
  if (nr < 0) {
    galleryOpen = false;
    return;
  }
  if (nr > REGION_COUNT - 1) return;
  galleryRegion = nr;
  galleryPage = 0;
  galleryDirty = true;
}

// dentro de una region (rejilla), arriba/abajo pasa sus paginas
void gallerySwipeV(int dir) {
  if (galleryDetail || !galleryGrid) return;
  int np = galleryPage + (dir < 0 ? 1 : -1);  // deslizar hacia arriba = la siguiente
  if (np < 0 || np > regionPages(galleryRegion) - 1) return;
  galleryPage = np;
  galleryDirty = true;
}

void onTap(int16_t x, int16_t y) {
  // Serial.printf("TOUCH %d %d\n", x, y);  // diagnostico (silenciado: satura el log)
  if (pet.awaitingStarter()) {  // primera partida: elegir inicial
    for (int i = 0; i < 3; i++) {
      int ry = STARTER_ROW_Y + i * (STARTER_ROW_H + STARTER_ROW_GAP);
      if (x >= 70 && x <= 396 && y >= ry && y <= ry + STARTER_ROW_H) {
        pet.chooseStarter(STARTER_DEX[i]);
        sfxPlay(SFX_TAP);
        break;
      }
    }
    return;
  }
  if (wifiPortalActive()) {  // tocar = cancelar la configuracion
    wifiPortalStop();
    return;
  }
  if (shopOpen) {
    shopTap(x, y);
    return;
  }
  if (farmOpen) {
    farmTap(x, y);
    return;
  }
  if (galleryOpen) {
    galleryTap(x, y);
    return;
  }
  if (kbOpen) {
    keyboardTap(x, y);
    return;
  }
  if (clockOpen) {
    clockTap(x, y);
    return;
  }
  if (pet.ceremony) return;  // durante la despedida no hay botones
  if (cardOpen) {
    if (cardPage == 0 && y < 84) openKeyboard();  // tocar el nombre = renombrar
    else cardOpen = false;
    return;
  }
  if (choiceKind) {          // dialogo de decision: boton accion (arriba) / mantener (abajo)
    bool b1 = (x >= 93 && x <= 373 && y >= 206 && y <= 258);  // accion
    bool b2 = (x >= 93 && x <= 373 && y >= 268 && y <= 320);  // mantener / quedaros
    if (choiceKind == 1) {                 // evolucion
      if (b1) { int16_t old = pet.speciesId; pet.evolve(); evoPmd.load(old, pet.shiny); }
      else if (b2) pet.declineEvolve();
    } else if (choiceKind == 2) {          // despedida
      if (b1) pet.startFarewell();
      else if (b2) pet.declineFarewell();
    }
    choiceKind = 0;
    return;
  }
  if (actionUntil) {         // dialogo de pulsacion larga: RECOGER / SOLTAR
    if (timeLeft(actionUntil) && y >= 252 && y <= 304) {
      if (x >= 118 && x <= 218) collectCurrent();
      else if (x >= 248 && x <= 348) confirmUntil = millis() + 10000;  // soltar: confirmar
    }
    actionUntil = 0;
    return;
  }
  if (confirmUntil) {        // dialogo "soltar?": SI / NO
    if (timeLeft(confirmUntil) && x >= 118 && x <= 218 && y >= 252 && y <= 304) {
      pet.release();
    }
    confirmUntil = 0;
    return;
  }
  if (feedMenuUntil) {       // selector de comida
    if (timeLeft(feedMenuUntil) && y >= 288 && y <= 352 && x >= FEED_X && x < FEED_X + 3 * 66) {
      int item = (x - FEED_X) / 66;  // 0 baya, 1 chuche, 2 medicina
      if (item == 2 && pet.bag[ITEM_MEDICINE] == 0) toast(T(S_NONE_LEFT));
      else if (item == 2 && !pet.sick) toast(T(S_NOT_SICK));
      else if (item == 1 && pet.bag[ITEM_CANDY] == 0) toast(T(S_NONE_LEFT));
      else {
        if (item == 2) pet.giveMedicine();
        else if (item == 1) pet.feedCandy();
        else pet.feed();
        sfxPlay(SFX_EAT);
      }
    }
    feedMenuUntil = 0;
    return;
  }
  if (reportPending) {        // el resumen del ajedrez se cierra con un toque
    reportPending = false;
    return;
  }
  if (y < 44 && x >= 250 && x <= 360) {  // contador de puntos: abre la tienda
    openShop(0);
    return;
  }
  if (pet.vacant) {
    vacantTap(x, y);
    return;
  }
  if (pet.isEgg()) {
    pet.eggTap();
    sfxPlay(SFX_TAP);
    return;
  }
  // boton de evolucion: abre el dialogo evolucionar/mantener
  if (pet.wantEvolveButton() && x >= EVO_BTN_X && x <= EVO_BTN_X + EVO_BTN_W &&
      y >= EVO_BTN_Y && y <= EVO_BTN_Y + EVO_BTN_H) {
    choiceKind = 1; choiceUntil = millis() + 12000;
    return;
  }
  // botones de final (mismo recuadro): escapada directa; despedida abre dialogo
  if (x >= FAR_BTN_X && x <= FAR_BTN_X + FAR_BTN_W &&
      y >= FAR_BTN_Y && y <= FAR_BTN_Y + FAR_BTN_H) {
    if (pet.canRunawayNow()) { pet.startRunaway(); return; }
    if (pet.wantFarewellButton()) { choiceKind = 2; choiceUntil = millis() + 12000; return; }
  }
  for (int i = 0; i < 4; i++) {
    int dx = x - buttons[i].cx, dy = y - buttons[i].cy;
    if (dx * dx + dy * dy <= BTN_HIT * BTN_HIT) {
      Serial.printf("BTN %d\n", i);
      sfxPlay(SFX_TAP);
      if (i == 0) {
        if (!pet.sleeping) feedMenuUntil = millis() + 6000;
      } else if (i == 1) {
        if (!pet.sick) pet.play();  // jugar al momento (sin minijuego); enfermo no juega
      } else if (i == 2) {
        pet.toggleLight();
      } else {
        startBath(true);
      }
      return;
    }
  }
  // tocar al bicho = caricia (y segun como: doble toque, cosquillas, cabeza o barriga)
  if (inPetZone(x, y)) {
    Serial.println("PET");
    petTap(x, y);
  }
}

// cuantos toques al bicho en los ultimos `ms`
static int tapsWithin(uint32_t now, uint32_t ms) {
  int n = 0;
  for (int i = 0; i < tapCount; i++)
    if (now - tapTimes[i] <= ms) n++;
  return n;
}

void petTap(int16_t x, int16_t y) {
  uint32_t now = millis();
  if (pet.sleeping) {  // dormido: una caricia y nada mas
    pet.caress();
    return;
  }
  if (tapCount == 16) {
    memmove(tapTimes, tapTimes + 1, 15 * sizeof(uint32_t));
    tapCount--;
  }
  tapTimes[tapCount++] = now;
  bool doble = lastPetTap && now - lastPetTap < 400;
  lastPetTap = doble ? 0 : now;
  if (tapsWithin(now, 5000) >= 14 && now - lastAnnoy > 6000) {  // demasiadas: harto
    lastAnnoy = now;
    pet.pester();
    react(PMD_HURT, 1400, T(S_SAY_ANNOYED));
    return;
  }
  pet.caress();
  sfxPlay(SFX_HEART);
  if (tapsWithin(now, 2000) >= 6 && now - lastTickle > 3000) {  // cosquillas
    lastTickle = now;
    react(REACT_BOUNCE, 2000, T(S_SAY_TICKLE));
  } else if (doble) {  // doble toque: salta
    react(REACT_BOUNCE, 900, nullptr);
  } else if (abs(x - (int)beh.x) < 70 && !timeLeft(reactUntil)) {  // cabeza o barriga
    int cabeza = gPmdTopY + (PET_GROUND - gPmdTopY) * 2 / 5;
    if (y < cabeza) react(PMD_NOD, 1200, T(S_SAY_HEAD));
    else react(REACT_BOUNCE, 1200, T(S_SAY_BELLY));
  }
}

// reaccion visible a un gesto (accion PMD y, si se da, lo que dice)
void react(uint8_t act, uint32_t ms, const char *say) {
  uint32_t now = millis();
  reactAct = act;
  reactUntil = now + ms;
  beh.t0 = now;
  if (say) {
    bubbleText = say;
    bubbleUntil = now + ms + 400;
  }
}
