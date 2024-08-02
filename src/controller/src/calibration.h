/**********************************************
 *
 * MANUAL CALIBRATION
 *
 **********************************************/
void manualCalibration(bool upLimit) {
  if (!engineOn())
    return;
  struct S_MOD {
    bool mod[4] = {false, false, false, false};
  };
  std::vector<S_MOD> mod;
  uint8_t modPos = 0;
#define axle(a) (data.axles & (a))
  if (axle(MF_FRONT_AXLE) && axle(MF_REAR_AXLE))
    mod.push_back({true, true, true, true});
  if (axle(MF_FRONT_AXLE))
    mod.push_back({true, true, false, false});
  if (axle(MF_REAR_AXLE))
    mod.push_back({false, false, true, true});
  if (axle(MF_FRONT_AXLE)) {
    mod.push_back({true, false, false, false});
    mod.push_back({false, true, false, false});
  }
  if (axle(MF_REAR_AXLE)) {
    mod.push_back({false, false, true, false});
    mod.push_back({false, false, false, true});
  }

  constexpr uint8_t border = 7;
  uint32_t lcdTimer = TICKS();
  MFLCD_FONTDATA fontData = {0};
  bool exit = false;

  lcd.noRefresh();
  lcd.clear(LCD_BLACK);
  lcd.boxwh(BW + 0, BW + 0, lcd.width - BW, 20, HEADER_COLOR);
  lcd.setFont(1, &fontData);
  lcd.printC(BW + 0, lcd.width - BW * 2, BW + 13,
             getTranslation(upLimit ? TR_upLimit : TR_downLimit, LANG),
             LCD_WHITE);

  lcdClear();
  lcd.refresh();
  exit = false;
  float top[4] = {4095, 4095, 4095, 4095};
  float bottom[4] = {0, 0, 0, 0};
  float range[4] = {0, 0, 0, 0};

  int16_t up[4] = {200, 200, 200, 200};
  int16_t down[4] = {-100, -100, -100, -100};
  float oldAutoMin[4], oldRawMin[4], oldAutoMax[4], oldRawMax[4],
      oldAdaptivesUp[4], oldAdaptivesDown[4];
  float oldPreset[4] = {0, 0, 0, 0};
  bool oldCalibration = data.calibration;
  data.calibration = false;
  for (uint8_t i = 0; i < 4; i++) { // reset calibrations
    oldAutoMin[i] = data.sensorAutoMin[i];
    oldAutoMax[i] = data.sensorAutoMax[i];
    oldRawMin[i] = data.sensorRawMin[i];
    oldRawMax[i] = data.sensorRawMax[i];
    data.sensorAutoMax[i] = data.sensorRawMax[i] = top[i];
    data.sensorAutoMin[i] = data.sensorRawMin[i] = bottom[i];
    oldAdaptivesUp[i] = data.adaptivesUp[i];
    oldAdaptivesDown[i] = data.adaptivesDown[i];
    data.adaptivesUp[i] = 0;
    data.adaptivesDown[i] = 0;
    oldPreset[i] = svInfo.curPreset.value[i];
    svInfo.doNotStop[i] = true;
  }
  // restartCCM(false);
  if (svInfo.svComp) {
    svInfo.svComp->resume();
    setupCCM();
  }
  restartSV();
  bool locked = true;
  for (auto dl = 0; dl < 1000; dl++) {
    locked = false;
    for (uint8_t i = 0; i < 4; i++) {
      if (svInfo.locked[i])
        locked = true;
    }
    if (!locked)
      break;
    DELAY(1);
  }
  DELAY(100);
  svInfo.autoCalibration = true;
  svInfo.enableSupervisor = true;
  DELAY(100);
  stopUp(false);
  while (!exit) {
    checkACC();
    DELAY(10);
    for (uint8_t b = 0; b < lcdButtons.size(); b++) {
      mfButtonFlags state = lcdButtons[b]->getState();
      if (state != MFBUTTON_NOSIGNAL) {
        switch (state) {
        case MFBUTTON_PRESSED:
          if (b == B_UP) {
            int16_t lev[4] = {
                mod.at(modPos).mod[0] ? up[0] : (int16_t)svInfo.level[0],
                mod.at(modPos).mod[1] ? up[1] : (int16_t)svInfo.level[1],
                mod.at(modPos).mod[2] ? up[2] : (int16_t)svInfo.level[2],
                mod.at(modPos).mod[3] ? up[3] : (int16_t)svInfo.level[3]};
            for (uint8_t w = 0; w < 4; w++)
              while (svInfo.wheel[w]->config.up->isLocked())
                svInfo.wheel[w]->config.up->unlock();
            setLevel(lev, 4, false);
          }
          if (b == B_DOWN) {
            int16_t lev[4] = {
                mod.at(modPos).mod[0] ? down[0] : (int16_t)svInfo.level[0],
                mod.at(modPos).mod[1] ? down[1] : (int16_t)svInfo.level[1],
                mod.at(modPos).mod[2] ? down[2] : (int16_t)svInfo.level[2],
                mod.at(modPos).mod[3] ? down[3] : (int16_t)svInfo.level[3]};
            for (uint8_t w = 0; w < 4; w++)
              while (svInfo.wheel[w]->config.down->isLocked())
                svInfo.wheel[w]->config.down->unlock();
            setLevel(lev, 4, false);
          }

          break;
        case MFBUTTON_PRESSED_LONG:
          if (b == B_OK) {
            debug.printf("Manual calibs: ");
            bool saveValues = false;

            auto answer =
                mfMenuItem<bool>({{"Save?"}, {"Сохранить?"}}, &saveValues,
                                 {{false, TR_no}, {true, TR_yes}});
            processValue(lcd, &answer);       // ask save or not?
            for (uint8_t i = 0; i < 4; i++) { // restore calibrations
              float tenPercents = 0;
              if (saveValues) {
                if (upLimit) {
                  data.sensorAutoMin[i] = oldAutoMin[i];
                  data.sensorRawMin[i] = oldRawMin[i];

                  data.sensorRawMax[i] = svInfo.raw[i];
                  range[i] = data.sensorRawMax[i] - data.sensorRawMin[i];
                  tenPercents = range[i] / 9;
                  data.sensorAutoMax[i] = svInfo.raw[i] - tenPercents;
                } else {
                  data.sensorAutoMax[i] = oldAutoMax[i];
                  data.sensorRawMax[i] = oldRawMax[i];

                  data.sensorRawMin[i] = svInfo.raw[i];
                  range[i] = data.sensorRawMax[i] - data.sensorRawMin[i];
                  tenPercents = range[i] / 9;
                  data.sensorAutoMin[i] = svInfo.raw[i] + tenPercents;
                }
              } else {
                data.sensorAutoMin[i] = oldAutoMin[i];
                data.sensorRawMin[i] = oldRawMin[i];
                data.sensorAutoMax[i] = oldAutoMax[i];
                data.sensorRawMax[i] = oldRawMax[i];
                data.adaptivesUp[i] = oldAdaptivesUp[i];
                data.adaptivesDown[i] = oldAdaptivesDown[i];
              }
              svInfo.curPreset.value[i] = oldPreset[i];
              debug.printf(
                  "%d:[%i(%i)]-[%i(%i)]//(%i/%i) ", i,
                  int(data.sensorRawMin[i]), int(data.sensorAutoMin[i]),
                  int(data.sensorRawMax[i]), int(data.sensorAutoMax[i]),
                  int(tenPercents), int(range[i]));
              svInfo.doNotStop[i] = false;
            }
            debug.println();
            data.calibration = saveValues ? oldCalibration : true;
            exit = true;
          }
          break;
        case MFBUTTON_RELEASED:
          if (b == B_OK) {
            if (((size_t)modPos + 1) >= mod.size())
              modPos = 0;
            else
              modPos++;
          }
          if (b == B_UP)
            stopUp(false);
          if (b == B_DOWN)
            stopDown(false);
          break;
        case MFBUTTON_RELEASED_LONG:
          if (b == B_UP)
            stopUp(false);
          if (b == B_DOWN)
            stopDown(false);
          break;
        default:;
        }
      }
    }
    float delta[4] = {0.0, 0.0, 0.0, 0.0};
    for (uint8_t i = 0; i < 4; i++) {
      delta[i] =
          fabsf(svInfo.raw[i] - (upLimit ? oldRawMin[i] : oldRawMax[i])) *
          vMult;
    }
    if (TICKS() - lcdTimer > 70) {
      lcd.noRefresh();
      lcdClear();
      lcd.setFont(1, &fontData);
      if (svInfo.present[0]) {
        lcd.print(border, border + 20 + 13, svInfo.names[0],
                  mod.at(modPos).mod[0] ? LCD_GREEN : LCD_WHITE);
        lcd.print(border, border + 20 + 28, printV(svInfo.volt[0]),
                  delta[0] > 2 ? LCD_WHITE : LCD_RED);
        if (svInfo.movingDir[0] == 2)
          lcd.image(25, 35, "image_arrowUp");
        if (svInfo.movingDir[0] == 1)
          lcd.image(25, 35, "image_arrowDown");
      }
      if (svInfo.present[1]) {
        lcd.printR(lcd.width - border, border + 20 + 13, svInfo.names[1],
                   mod.at(modPos).mod[1] ? LCD_GREEN : LCD_WHITE);
        lcd.printR(lcd.width - border, border + 20 + 28, printV(svInfo.volt[1]),
                   delta[1] > 2 ? LCD_WHITE : LCD_RED);
        if (svInfo.movingDir[1] == 2)
          lcd.image(lcd.width - 36, 35, "image_arrowUp");
        if (svInfo.movingDir[1] == 1)
          lcd.image(lcd.width - 36, 35, "image_arrowDown");
      }
      if (svInfo.present[2]) {
        lcd.print(border, border + 20 + lcd.height / 2, svInfo.names[2],
                  mod.at(modPos).mod[2] ? LCD_GREEN : LCD_WHITE);
        lcd.print(border, border + 38 + lcd.height / 2, printV(svInfo.volt[2]),
                  delta[2] > 2 ? LCD_WHITE : LCD_RED);
        if (svInfo.movingDir[2] == 2)
          lcd.image(25, lcd.height - 42, "image_arrowUp");
        if (svInfo.movingDir[2] == 1)
          lcd.image(25, lcd.height - 42, "image_arrowDown");
      }
      if (svInfo.present[3]) {
        lcd.printR(lcd.width - border, border + 20 + lcd.height / 2,
                   svInfo.names[3],
                   mod.at(modPos).mod[3] ? LCD_GREEN : LCD_WHITE);
        lcd.printR(lcd.width - border, border + 38 + lcd.height / 2,
                   printV(svInfo.volt[3]), delta[3] > 2 ? LCD_WHITE : LCD_RED);
        if (svInfo.movingDir[3] == 2)
          lcd.image(lcd.width - 35, lcd.height - 42, "image_arrowUp");
        if (svInfo.movingDir[3] == 1)
          lcd.image(lcd.width - 35, lcd.height - 42, "image_arrowDown");
      }
      lcd.setFont(0, &fontData);
      if (data.receiverPresent && svInfo.svComp) {
        uint16_t compColor;
        if (svInfo.svComp->isCompressorRunning()) {
          compColor = LCD_YELLOW;
        } else {
          if (svInfo.svComp->isCompressorLocked()) {
            compColor = LCD_RED;
          } else {
            compColor = LCD_GREEN;
          }
        }
        lcd.printC(0, lcd.width, 76,
                   printfString("%u psi", svInfo.svComp->getPressure()),
                   compColor);
      }
      lcd.refresh();
      resetTimer(lcdTimer);
    }
    DELAY(10);
  }
  // svInfo.svComp->disable();
  if (svInfo.svComp) {
    svInfo.svComp->pause();
  }
  svInfo.autoCalibration = false;
  svInfo.enableSupervisor = false;
}

/**********************************************
 *
 * AUTOMATIC CALIBRATION
 *
 **********************************************/

void autoCalibration(bool &quitOnExit) {
  if (!engineOn())
    return;
  constexpr uint8_t border = 7;
  constexpr uint32_t up1stDelay = 300000, up2ndDelay = 1000,
                     down1stDelay = 5000, down2ndDelay = 1000;
  bool breakFlag = false;
  uint32_t lcdTimer = TICKS();
  MF_TRANSLATION autoCalibrationWarning = {
      "Do you really\nwant to start\nan automatic\ncalibration?",
      "Вы хотите\nзапустить\nавтоматическую\nкалибровку?"};

  MF_TRANSLATION autoCalibrationSuccess = {"Calibration\ncompleted!",
                                           "Калибровка\nзавершена!"};

  MF_TRANSLATION autoCalibrationLowRange = {
      "Error: Range\nis to low!\nCheck: ",
      "Ошибка, слишком\nузкий диапазон!\nКаналы: "};

  MFLCD_FONTDATA fontData = {0};
  bool exit = false;
  uint32_t exitTimer = 0;
  uint32_t startTimer;
  lcd.noRefresh();
  lcd.clear(LCD_BLACK);
  lcd.boxwh(BW + 0, BW + 0, lcd.width - BW, 20, HEADER_COLOR);
  lcd.setFont(1, &fontData);
  lcd.printC(BW + 0, lcd.width - BW * 2, BW + 13,
             getTranslation(TR_autoCalibration, LANG), LCD_WHITE);
  lcd.refresh();

  if (buttonMenu != 22) {
    lcd.noRefresh();
    lcdClear();
    lcd.setFont(0, &fontData);
    lcd.printC(border, lcd.width - border * 2, border + 20 + 30,
               getTranslation(autoCalibrationWarning, LANG), LCD_WHITE);
    lcd.refresh();
    while (!exit) {
      checkACC();
      DELAY(10);
      for (uint8_t b = 0; b < lcdButtons.size(); b++) {
        mfButtonFlags state = lcdButtons[b]->getState();
        if (state != MFBUTTON_NOSIGNAL) {
          switch (state) {
          case MFBUTTON_PRESSED:
            break;
          case MFBUTTON_PRESSED_LONG:
            if (b == B_OK)
              return;
            break;
          case MFBUTTON_RELEASED:
            if (b == B_OK)
              exit = true;
            break;
          case MFBUTTON_RELEASED_LONG:
            break;
          default:;
          }
        }
      }
    }
  }
  exit = false;
  float startV[4] = {0, 0, 0, 0};
  float topV[4] = {0, 0, 0, 0};
  float top[4] = {4095, 4095, 4095, 4095};
  float bottomV[4] = {0, 0, 0, 0};
  float bottom[4] = {0, 0, 0, 0};
  float range[4] = {0, 0, 0, 0};
  // unused
  (void)startV;
  (void)topV;

  bool present[4] = {svInfo.wheel[0]->present, svInfo.wheel[1]->present,
                     svInfo.wheel[2]->present, svInfo.wheel[3]->present};
#define restorePresent()                                                       \
  do {                                                                         \
    for (uint8_t r = 0; r < 4; r++)                                            \
      svInfo.wheel[r]->present = present[r];                                   \
  } while (0)
  uint8_t calibStep = 0;
  uint8_t maxStep = (axle(MF_ALL_AXLES) > 0) ? 3 : 2;
  float frontRange = 0.0, rearRange = 0.0;
  bool valid = true;
#define axle(a) (data.axles & (a))
  int16_t up[4] = {200, 200, 200, 200};
  int16_t down[4] = {-100, -100, -100, -100};
  float oldPreset[4];
  float oldAutoMin[4], oldRawMin[4], oldAutoMax[4], oldRawMax[4];
  bool oldCalibration = data.calibration;
  data.calibration = false;
  for (uint8_t i = 0; i < 4; i++) { // reset calibrations
    oldAutoMin[i] = data.sensorAutoMin[i];
    oldAutoMax[i] = data.sensorAutoMax[i];
    oldRawMin[i] = data.sensorRawMin[i];
    oldRawMax[i] = data.sensorRawMax[i];
    data.sensorAutoMax[i] = data.sensorRawMax[i] = top[i];
    data.sensorAutoMin[i] = data.sensorRawMin[i] = bottom[i];
    data.adaptivesUp[i] = 0;
    data.adaptivesDown[i] = 0;
    oldPreset[i] = svInfo.curPreset.value[i];
  }
  // restartCCM(false);
  if (svInfo.svComp) {
    svInfo.svComp->resume();
    setupCCM();
  }
  restartSV();
  bool locked = true;
  for (auto dl = 0; dl < 1000; dl++) {
    locked = false;
    for (uint8_t i = 0; i < 4; i++) {
      if (svInfo.locked[i])
        locked = true;
    }
    if (!locked)
      break;
    DELAY(1);
  }

  DELAY(100);
  svInfo.autoCalibration = true;
  svInfo.enableSupervisor = true;
  for (uint8_t i = 0; i < 4; i++) {
    if (svInfo.present[i]) {
      startV[i] = svInfo.volt[i];
    }
  }

  /* moving down */
  if (buttonMenu == 22) {
    ledDOWN.setColor(MF_WHITE);
    ledOK.setColor(MF_WHITE);
    ledUP.setColor(MF_WHITE);
    ledUP.off();
    ledOK.blink(~0UL, 1000, 0);
    ledDOWN.blink(~0UL, 1000, 500);
  }
  calibStep = 0;
  while ((calibStep < maxStep) && !breakFlag) {
    exit = false;
    exitTimer = 0;
    startTimer = TICKS();
    for (uint8_t i = 0; i < 4; i++) {
      svInfo.doNotStop[i] = true;
    }
    switch (calibStep) {
    case 2:
      restorePresent();
      if (frontRange >= rearRange) {
        svInfo.wheel[2]->present = false;
        svInfo.wheel[3]->present = false;
      } else {
        svInfo.wheel[0]->present = false;
        svInfo.wheel[1]->present = false;
      }
      debug.printf("Step2: %d %d\n", svInfo.wheel[0]->present,
                   svInfo.wheel[2]->present);
      break;
    case 1:
      restorePresent();
      if (frontRange < rearRange) {
        svInfo.wheel[2]->present = false;
        svInfo.wheel[3]->present = false;
      } else {
        svInfo.wheel[0]->present = false;
        svInfo.wheel[1]->present = false;
      }
      debug.printf("Step1: %d %d\n", svInfo.wheel[0]->present,
                   svInfo.wheel[2]->present);
      break;
    default:
    case 0:
      restorePresent();
      debug.printf("Step0\n");
      break;
    }
    setLevel(down, 4, false);
    for (auto dl = 0; dl < 1000; dl++) {
      if (svInfo.moving)
        break;
      DELAY(1);
    }
    while (!exit) {
      checkACC();
      if (buttonMenu == 22)
        for (uint8_t b = 0; b < buttons.size(); b++) {
          mfButtonFlags state = buttons[b]->getState();
          if (state != MFBUTTON_NOSIGNAL) {
            if ((state == MFBUTTON_PRESSED_LONG) && (b == B_OK))
              breakFlag = true;
          }
        }
      for (uint8_t b = 0; b < lcdButtons.size(); b++) {
        mfButtonFlags state = lcdButtons[b]->getState();
        if (state != MFBUTTON_NOSIGNAL) {
          if ((state == MFBUTTON_PRESSED_LONG) && (b == B_OK))
            breakFlag = true;
        }
      }
      if (breakFlag) {
        valid = false;
        exit = true;
      }
      for (uint8_t i = 0; i < 4; i++) {
        bool dns = svInfo.doNotStop[i];
        if (svInfo.wheel[i]->isStartedMoving())
          if (dns) {
            svInfo.doNotStop[i] = false;
            debug.printf("release [%d]\n", i);
          }
        if (!svInfo.forceStop[i])
          if (svInfo.wheel[i]->isStoppedMoving()) {
            svInfo.forceStop[i] = true;
          }
      }
      uint32_t doNotStop = (calibStep > 0) ? down2ndDelay : down1stDelay;
      if (TICKS() - startTimer > doNotStop) {
        for (uint8_t i = 0; i < 4; i++)
          svInfo.doNotStop[i] = false;
      }
      if (TICKS() - lcdTimer > 70) {
        lcd.noRefresh();
        lcdClear();
        lcd.setFont(1, &fontData);
        if (svInfo.present[0]) {
          lcd.print(border, border + 20 + 13,
                    svInfo.names[0] + "\n\n" + printV(svInfo.volt[0]),
                    LCD_WHITE);
          if (svInfo.movingDir[0] == 2)
            lcd.image(25, 35, "image_arrowUp");
          if (svInfo.movingDir[0] == 1)
            lcd.image(25, 35, "image_arrowDown");
        }
        if (svInfo.present[1]) {
          lcd.printR(lcd.width - border, border + 20 + 13,
                     svInfo.names[1] + "\n\n" + printV(svInfo.volt[1]),
                     LCD_WHITE);
          if (svInfo.movingDir[1] == 2)
            lcd.image(lcd.width - 36, 35, "image_arrowUp");
          if (svInfo.movingDir[1] == 1)
            lcd.image(lcd.width - 36, 35, "image_arrowDown");
        }
        if (svInfo.present[2]) {
          lcd.print(border, border + 20 + lcd.height / 2,
                    svInfo.names[2] + "\n\n" + printV(svInfo.volt[2]),
                    LCD_WHITE);
          if (svInfo.movingDir[2] == 2)
            lcd.image(25, lcd.height - 42, "image_arrowUp");
          if (svInfo.movingDir[2] == 1)
            lcd.image(25, lcd.height - 42, "image_arrowDown");
        }
        if (svInfo.present[3]) {
          lcd.printR(lcd.width - border, border + 20 + lcd.height / 2,
                     svInfo.names[3] + "\n\n" + printV(svInfo.volt[3]),
                     LCD_WHITE);
          if (svInfo.movingDir[3] == 2)
            lcd.image(lcd.width - 35, lcd.height - 42, "image_arrowUp");
          if (svInfo.movingDir[3] == 1)
            lcd.image(lcd.width - 35, lcd.height - 42, "image_arrowDown");
        }
        lcd.setFont(0, &fontData);
        if (data.receiverPresent) {
          uint16_t compColor;
          if (svInfo.svComp->isCompressorRunning()) {
            compColor = LCD_YELLOW;
          } else {
            if (svInfo.svComp->isCompressorLocked()) {
              compColor = LCD_RED;
            } else {
              compColor = LCD_GREEN;
            }
          }
          lcd.printC(0, lcd.width, 76,
                     printfString("%u psi", svInfo.svComp->getPressure()),
                     compColor);
        }
        lcd.image(64, BW + 20 + 62, "image_bigarrdown");
        lcd.refresh();
        resetTimer(lcdTimer);
      }
      DELAY(10);
      if (!exitTimer && !svInfo.moving)
        exitTimer = TICKS();
      if (exitTimer) {
        if (TICKS() - exitTimer > 1000)
          exit = true;
      }
    }
    if (!calibStep) {
      frontRange = axle(MF_FRONT_AXLE)
                       ? min(abs(svInfo.wheel[0]->getLastMoveRange()),
                             abs(svInfo.wheel[1]->getLastMoveRange()))
                       : 100;
      rearRange = axle(MF_REAR_AXLE)
                      ? min(abs(svInfo.wheel[2]->getLastMoveRange()),
                            abs(svInfo.wheel[3]->getLastMoveRange()))
                      : 100;
      debug.printf("Ranges: %.2f - %.2f\n", frontRange, rearRange);
    }
    calibStep++;
    for (auto dl = 0; dl < 1000; dl++) {
      locked = false;
      for (uint8_t i = 0; i < 4; i++) {
        if (svInfo.locked[i])
          locked = true;
      }
      if (!locked)
        break;
      DELAY(1);
    }
    DELAY(1000);
  }
  restorePresent();
  lcd.noRefresh();
  lcd.boxwh(64, BW + 20 + 62, 31, 16, LCD_BLACK);
  lcd.refresh();
  DELAY(2000);
  for (uint8_t i = 0; i < 4; i++) {
    if (svInfo.present[i]) {
      bottom[i] = svInfo.raw[i];
      bottomV[i] = svInfo.volt[i];
    }
  }
  /* moving up */
  if (buttonMenu == 22) {
    ledDOWN.setColor(MF_WHITE);
    ledOK.setColor(MF_WHITE);
    ledUP.setColor(MF_WHITE);
    ledUP.blink(~0UL, 1000, 500);
    ledOK.blink(~0UL, 1000, 0);
    ledDOWN.off();
  }
  calibStep = 0;
  while ((calibStep < maxStep) && !breakFlag) {
    exit = false;
    exitTimer = 0;
    startTimer = TICKS();
    for (uint8_t i = 0; i < 4; i++) {
      svInfo.doNotStop[i] = true;
    }
    switch (calibStep) {
    case 2:
      restorePresent();
      if (frontRange >= rearRange) {
        svInfo.wheel[2]->present = false;
        svInfo.wheel[3]->present = false;
      } else {
        svInfo.wheel[0]->present = false;
        svInfo.wheel[1]->present = false;
      }
      debug.printf("Step2: %d %d\n", svInfo.wheel[0]->present,
                   svInfo.wheel[2]->present);
      break;
    case 1:
      restorePresent();
      if (frontRange < rearRange) {
        svInfo.wheel[2]->present = false;
        svInfo.wheel[3]->present = false;
      } else {
        svInfo.wheel[0]->present = false;
        svInfo.wheel[1]->present = false;
      }
      debug.printf("Step1: %d %d\n", svInfo.wheel[0]->present,
                   svInfo.wheel[2]->present);
      break;
    default:
    case 0:
      restorePresent();
      debug.printf("Step0\n");
      break;
    }
    setLevel(up, 4, false);
    for (auto dl = 0; dl < 1000; dl++) {
      if (svInfo.moving)
        break;
      DELAY(1);
    }
    while (!exit) {
      checkACC();
      if (buttonMenu == 22)
        for (uint8_t b = 0; b < buttons.size(); b++) {
          mfButtonFlags state = buttons[b]->getState();
          if (state != MFBUTTON_NOSIGNAL) {
            if ((state == MFBUTTON_PRESSED_LONG) && (b == B_OK))
              breakFlag = true;
          }
        }
      for (uint8_t b = 0; b < lcdButtons.size(); b++) {
        mfButtonFlags state = lcdButtons[b]->getState();
        if (state != MFBUTTON_NOSIGNAL) {
          if ((state == MFBUTTON_PRESSED_LONG) && (b == B_OK))
            breakFlag = true;
        }
      }
      if (breakFlag) {
        valid = false;
        exit = true;
      }
      for (uint8_t i = 0; i < 4; i++) {
        bool dns = svInfo.doNotStop[i];
        if (svInfo.wheel[i]->isStartedMoving())
          if (dns) {
            svInfo.doNotStop[i] = false;
            debug.printf("release [%d]\n", i);
          }
        if (!svInfo.forceStop[i])
          if (svInfo.wheel[i]->isStoppedMoving()) {
            svInfo.forceStop[i] = true;
          }
      }
      uint32_t doNotStop = (calibStep > 0) ? up2ndDelay : up1stDelay;
      if (TICKS() - startTimer > doNotStop) {
        for (uint8_t i = 0; i < 4; i++)
          svInfo.doNotStop[i] = false;
        // debug.println("Release noStop");
      }
      if (TICKS() - lcdTimer > 70) {
        lcd.noRefresh();
        lcdClear();
        lcd.setFont(1, &fontData);
        if (svInfo.present[0]) {
          lcd.print(border, border + 20 + 13,
                    svInfo.names[0] + "\n" + printV(svInfo.volt[0]) + "\n" +
                        printV(bottomV[0]),
                    LCD_WHITE);
          if (svInfo.movingDir[0] == 2)
            lcd.image(25, 35, "image_arrowUp");
          if (svInfo.movingDir[0] == 1)
            lcd.image(25, 35, "image_arrowDown");
        }
        if (svInfo.present[1]) {
          lcd.printR(lcd.width - border, border + 20 + 13,
                     svInfo.names[1] + "\n" + printV(svInfo.volt[1]) + "\n" +
                         printV(bottomV[1]),
                     LCD_WHITE);
          if (svInfo.movingDir[1] == 2)
            lcd.image(lcd.width - 36, 35, "image_arrowUp");
          if (svInfo.movingDir[1] == 1)
            lcd.image(lcd.width - 36, 35, "image_arrowDown");
        }
        if (svInfo.present[2]) {
          lcd.print(border, border + 20 + lcd.height / 2,
                    svInfo.names[2] + "\n" + printV(svInfo.volt[2]) + "\n" +
                        printV(bottomV[2]),
                    LCD_WHITE);
          if (svInfo.movingDir[2] == 2)
            lcd.image(25, lcd.height - 42, "image_arrowUp");
          if (svInfo.movingDir[2] == 1)
            lcd.image(25, lcd.height - 42, "image_arrowDown");
        }
        if (svInfo.present[3]) {
          lcd.printR(lcd.width - border, border + 20 + lcd.height / 2,
                     svInfo.names[3] + "\n" + printV(svInfo.volt[3]) + "\n" +
                         printV(bottomV[3]),
                     LCD_WHITE);
          if (svInfo.movingDir[3] == 2)
            lcd.image(lcd.width - 35, lcd.height - 42, "image_arrowUp");
          if (svInfo.movingDir[3] == 1)
            lcd.image(lcd.width - 35, lcd.height - 42, "image_arrowDown");
        }
        lcd.setFont(0, &fontData);
        if (data.receiverPresent) {
          uint16_t compColor;
          if (svInfo.svComp->isCompressorRunning()) {
            compColor = LCD_YELLOW;
          } else {
            if (svInfo.svComp->isCompressorLocked()) {
              compColor = LCD_RED;
            } else {
              compColor = LCD_GREEN;
            }
          }
          lcd.printC(0, lcd.width, 76,
                     printfString("%u psi", svInfo.svComp->getPressure()),
                     compColor);
        }
        lcd.image(64, BW + 20 + 17, "image_bigarrup");
        lcd.refresh();
        resetTimer(lcdTimer);
      }
      DELAY(10);
      if (!exitTimer && !svInfo.moving)
        exitTimer = TICKS();
      if (exitTimer) {
        if (TICKS() - exitTimer > 1000)
          exit = true;
      }
    }
    if (!calibStep) {
      frontRange = axle(MF_FRONT_AXLE)
                       ? min(abs(svInfo.wheel[0]->getLastMoveRange()),
                             abs(svInfo.wheel[1]->getLastMoveRange()))
                       : 100;
      rearRange = axle(MF_REAR_AXLE)
                      ? min(abs(svInfo.wheel[2]->getLastMoveRange()),
                            abs(svInfo.wheel[3]->getLastMoveRange()))
                      : 100;
      debug.printf("Ranges: %.2f - %.2f\n", frontRange, rearRange);
    }
    calibStep++;
    for (auto dl = 0; dl < 1000; dl++) {
      locked = false;
      for (uint8_t i = 0; i < 4; i++) {
        if (svInfo.locked[i])
          locked = true;
      }
      if (!locked)
        break;
      DELAY(1);
    }
    DELAY(1000);
  }
  restorePresent();
  lcd.noRefresh();
  lcd.boxwh(64, BW + 20 + 17, 31, 16, LCD_BLACK);
  lcd.refresh();
  std::vector<String> invalidChannels = {};
  uint8_t invalidAxles = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (svInfo.present[i]) {
      top[i] = svInfo.raw[i];
      topV[i] = svInfo.volt[i];
      range[i] = top[i] - bottom[i];
      if (::fabsf(range[i]) * vMult < 1.0) {
        valid = false;
        invalidChannels.push_back(svInfo.names[i]);
        if (i < 2)
          invalidAxles |= 1;
        else
          invalidAxles |= 2;
      }
    }
  }
  if (buttonMenu != 22) {
    lcd.noRefresh();
    lcd.boxwh(20, 44, lcd.width - 40, 60, LCD_BLACK);
    lcd.rect(20, 44, lcd.width - 20, 104, LCD_WHITE);
    if (valid) {
      lcd.printC(BW, lcd.width - BW * 2, 70,
                 getTranslation(autoCalibrationSuccess, LANG), LCD_WHITE);
    } else {
      String channels;
      for (uint8_t i = 0; i < invalidChannels.size(); i++)
        channels += invalidChannels[i] + " ";
      if (!breakFlag)
        lcd.printC(BW, lcd.width - BW * 2, 68,
                   getTranslation(autoCalibrationLowRange, LANG) + channels,
                   LCD_WHITE);
    }
    lcd.refresh();
    exit = false;
    if (!breakFlag)
      while (!exit) {
        checkACC();
        DELAY(10);
        for (uint8_t b = 0; b < lcdButtons.size(); b++) {
          mfButtonFlags state = lcdButtons[b]->getState();
          if (state != MFBUTTON_NOSIGNAL) {
            switch (state) {
            case MFBUTTON_PRESSED:
              break;
            case MFBUTTON_PRESSED_LONG:
              if (b == B_OK) {
                valid = false;
                exit = true;
              }
              break;
            case MFBUTTON_RELEASED:
              if (b == B_OK) {
                exit = true;
                if (valid)
                  quitOnExit = true;
              }
              break;
            case MFBUTTON_RELEASED_LONG:
              break;
            default:;
            }
          }
        }
      }
  }
  if (valid) {
    if (buttonMenu == 22) {
      quitOnExit = true;
      ledDOWN.setColor(MF_GREEN);
      ledOK.setColor(MF_GREEN);
      ledUP.setColor(MF_GREEN);
      ledUP.blink(~0UL, 500, 0);
      ledOK.blink(~0UL, 500, 0);
      ledDOWN.blink(~0UL, 500, 0);
      DELAY(2000);
    }
    debug.printf("Calibs: ");
    for (uint8_t i = 0; i < 4; i++) {
      if (svInfo.present[i]) {
        float tenPercents = range[i] / 9;
        data.sensorRawMin[i] = bottom[i];
        data.sensorAutoMin[i] = data.sensorRawMin[i] + tenPercents;
        data.sensorRawMax[i] = top[i];
        data.sensorAutoMax[i] = data.sensorRawMax[i] - tenPercents;
        svInfo.curPreset.value[i] = oldPreset[i];
        debug.printf("%d:[%i(%i)]-[%i(%i)]//(%i/%i) ", i,
                     int(data.sensorRawMin[i]), int(data.sensorAutoMin[i]),
                     int(data.sensorRawMax[i]), int(data.sensorAutoMax[i]),
                     int(tenPercents), int(range[i]));
      }
    }
    debug.println();
    data.calibration = true;
    restartSV();
    DELAY(500);
    svInfo.autoCalibration = false;
    int16_t preset[4] = {data.presets[2][0], data.presets[2][1],
                         data.presets[2][2], data.presets[2][3]};
    // svInfo.svComp->disable();
    setLevel(preset, 4, false);
    data.lastPreset = 2;
  } else {
    if (buttonMenu == 22) {
      debug.printf("Invalid axles: %d\n", invalidAxles);
      quitOnExit = false;
      ledOK.off();
      if (invalidAxles & 1) {
        ledUP.setColor(MF_RED);
        ledUP.blink(~0UL, 500);
      } else {
        ledUP.off();
      }
      if (invalidAxles & 2) {
        ledDOWN.setColor(MF_RED);
        ledDOWN.blink(~0UL, 500);
      } else {
        ledDOWN.off();
      }
      DELAY(2000);
    }
    for (uint8_t i = 0; i < 4; i++) { // restore calibrations
      data.sensorAutoMin[i] = oldAutoMin[i];
      data.sensorAutoMax[i] = oldAutoMax[i];
      data.sensorRawMin[i] = oldRawMin[i];
      data.sensorRawMax[i] = oldRawMax[i];
      svInfo.curPreset.value[i] = oldPreset[i];
    }
    data.calibration = oldCalibration;
    svInfo.enableSupervisor = false;
    // svInfo.svComp->disable();
    svInfo.autoCalibration = false;
  }
  if (svInfo.svComp) {
    svInfo.svComp->pause();
  }

  // settings.save();
}
/**********************************************
 *
 * SPEED CALIBRATION
 *
 **********************************************/

bool calibrateSpeed() {
  uint32_t maxFreq = 0, lastFreq = 0;
  uint32_t speedTimer = TICKS();
  MF_TRANSLATION TR_stopCar = {"Your vehicle is moving!\nPlease stop.",
                               "Автомобиль двигается!\nОстановитесь."};
  MF_TRANSLATION TR_accelerateCar = {"Accelerate to 50km/h\nand then stop.",
                                     "Разгонитесь до 50км/ч\nи остановитесь."};
  constexpr uint8_t border = 7;
  MFLCD_FONTDATA fontData = {0};

  lcd.noRefresh();
  lcd.clear(LCD_BLACK);
  lcd.boxwh(BW + 0, BW + 0, lcd.width - BW, 20, HEADER_COLOR);
  lcd.setFont(1, &fontData);
  lcd.printC(BW + 0, lcd.width - BW * 2, BW + 13,
             getTranslation(TR_speedCalibration, LANG), LCD_WHITE);

  lcd.setFont(0, &fontData);
  lcd.refresh();
  bool oldCalibrated = data.speedSensCalibrated;
  double oldCalibration = data.speedSensCalibration;
  data.speedSensCalibration = 0;
  data.speedSensCalibrated = false;
  settings.save();
  uint32_t ticks = getMeasured();
  if (ticks) {
    lcdClear();
    lcd.printC(0, lcd.width, lcd.height / 2 - 10,
               getTranslation(TR_stopCar, LANG), LCD_WHITE);
    lcd.refresh();
    if (buttonMenu == 4) {
      ledOK.setColor(MF_RED);
    }
    while (getMeasured()) {
      checkACCbool();
      DELAY(10);
      if (buttonMenu == 4)
        for (uint8_t b = 0; b < buttons.size(); b++) {
          mfButtonFlags state = buttons[b]->getState();
          if ((state == MFBUTTON_PRESSED_LONG) && (b == B_OK)) {
            data.speedSensCalibrated = oldCalibrated;
            data.speedSensCalibration = oldCalibration;
            settings.save();
            return false;
          }
        }
      for (uint8_t b = 0; b < lcdButtons.size(); b++) {
        mfButtonFlags state = lcdButtons[b]->getState();
        if (state != MFBUTTON_NOSIGNAL) {
          switch (state) {
          case MFBUTTON_PRESSED:
            break;
          case MFBUTTON_PRESSED_LONG:
            if (b == B_OK) {
              data.speedSensCalibrated = oldCalibrated;
              data.speedSensCalibration = oldCalibration;
              settings.save();
              return false;
            }
            break;
          case MFBUTTON_RELEASED:
            break;
          case MFBUTTON_RELEASED_LONG:
            break;
          default:;
          }
        }
      }
      uint32_t curFreq = getMeasured();
      if (buttonMenu == 4)
        if (TICKS() - speedTimer > 1000) {
          speedTimer = TICKS();
          if (curFreq > lastFreq)
            ledUP.on(100);
          if (curFreq < lastFreq)
            ledDOWN.on(100);
          lastFreq = curFreq;
        }
    }
  }
  lcdClear();
  if (buttonMenu == 4) {
    ledOK.setColor(MF_ORANGE);
  }
  lcd.printC(0, lcd.width, lcd.height / 2 - 10,
             getTranslation(TR_accelerateCar, LANG), LCD_WHITE);
  lcd.refresh();
  debug.println("Wait for start.");

  while (getMeasured() < 4) {
    checkACCbool();
    DELAY(10);
    if (buttonMenu == 4)
      for (uint8_t b = 0; b < buttons.size(); b++) {
        mfButtonFlags state = buttons[b]->getState();
        if ((state == MFBUTTON_PRESSED_LONG) && (b == B_OK)) {
          data.speedSensCalibrated = oldCalibrated;
          data.speedSensCalibration = oldCalibration;
          settings.save();
          return false;
        }
      }
    for (uint8_t b = 0; b < lcdButtons.size(); b++) {
      mfButtonFlags state = lcdButtons[b]->getState();
      if (state != MFBUTTON_NOSIGNAL) {
        switch (state) {
        case MFBUTTON_PRESSED:
          break;
        case MFBUTTON_PRESSED_LONG:
          if (b == B_OK) {
            data.speedSensCalibrated = oldCalibrated;
            data.speedSensCalibration = oldCalibration;
            settings.save();
            return false;
          }
          break;
        case MFBUTTON_RELEASED:
          break;
        case MFBUTTON_RELEASED_LONG:
          break;
        default:;
        }
      }
    }
  }
  debug.println("Calibrating...");
  if (buttonMenu == 4) {
    ledOK.setColor(MF_WHITE);
  }
  while (getMeasured()) {
    checkACCbool();
    DELAY(10);
    if (buttonMenu == 4)
      for (uint8_t b = 0; b < buttons.size(); b++) {
        mfButtonFlags state = buttons[b]->getState();
        if ((state == MFBUTTON_PRESSED_LONG) && (b == B_OK)) {
          data.speedSensCalibrated = oldCalibrated;
          data.speedSensCalibration = oldCalibration;
          settings.save();
          return false;
        }
      }
    for (uint8_t b = 0; b < lcdButtons.size(); b++) {
      mfButtonFlags state = lcdButtons[b]->getState();
      if (state != MFBUTTON_NOSIGNAL) {
        switch (state) {
        case MFBUTTON_PRESSED:
          break;
        case MFBUTTON_PRESSED_LONG:
          if (b == B_OK) {
            data.speedSensCalibrated = oldCalibrated;
            data.speedSensCalibration = oldCalibration;
            settings.save();
            return false;
          }
          break;
        case MFBUTTON_RELEASED:
          break;
        case MFBUTTON_RELEASED_LONG:
          break;
        default:;
        }
      }
    }
    uint32_t curFreq = getMeasured();
    if (curFreq > maxFreq) {
      maxFreq = curFreq;
    }
    if (buttonMenu == 4)
      if (TICKS() - speedTimer > 1000) {
        speedTimer = TICKS();
        if (curFreq > lastFreq)
          ledUP.on(100);
        if (curFreq < lastFreq)
          ledDOWN.on(100);
        lastFreq = curFreq;
      }
  }
  data.speedSensCalibrated = true;
  data.speedSensCalibration = 50.0 / maxFreq;
  debug.printf("Speed calibration: state: %d, maxFreq: %u, mult: %.2f\n",
               data.speedSensCalibrated, maxFreq, data.speedSensCalibration);
  settings.save();
  if (buttonMenu == 4) {
    ledOK.setColor(MF_GREEN);
    ledOK.blink(~0UL, 200);
  }
  DELAY(1000);
  return true;
}
