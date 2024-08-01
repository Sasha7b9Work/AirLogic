void editPreset(uint8_t presetNum) {
  uint8_t min[4], max[4];

  if (presetNum == 5) {
    for (uint8_t i = 0; i < 4; i++) {
      min[i] = data.presets[0][i] + 1;
      max[i] = data.presets[2][i] - 1;
    }
  } else {
    if (presetNum > 0 && presetNum < 4) {
      for (uint8_t i = 0; i < 4; i++) {
        min[i] = data.presets[presetNum - 1][i] + 1;
        max[i] = data.presets[presetNum + 1][i] - 1;
      }
    } else {
      if (presetNum == 0) {
        for (uint8_t i = 0; i < 4; i++) {
          min[i] = 0;
          max[i] = data.presets[presetNum + 1][i] - 1;
        }
      }
      if (presetNum == 4) {
        for (uint8_t i = 0; i < 4; i++) {
          min[i] = data.presets[presetNum - 1][i] + 1;
          max[i] = 100;
        }
      }
    }
  }

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
  if (presetNum != 5)
    lcd.printC(BW + 0, lcd.width - BW * 2, BW + 13,
               getTranslation(TR_preset, LANG) + " " + String(presetNum + 1),
               LCD_WHITE);
  else
    lcd.printC(BW + 0, lcd.width - BW * 2, BW + 13,
               getTranslation(TR_speedSensLimit2preset, LANG), LCD_WHITE);

  lcdClear();
  lcd.refresh();
  std::function<void(uint8_t)> incValue = [&](uint8_t num) {
    if (mod.at(modPos).mod[num])
      if (data.presets[presetNum][num] < max[num])
        data.presets[presetNum][num]++;
  };
  std::function<void(uint8_t)> decValue = [&](uint8_t num) {
    if (mod.at(modPos).mod[num])
      if (data.presets[presetNum][num] > min[num])
        data.presets[presetNum][num]--;
  };

  exit = false;
  uint32_t repeatTimer[2] = {TICKS(), TICKS()};
  bool repeat[2] = {false, false};
  while (!exit) {
    checkACC();
    DELAY(10);
    for (uint8_t b = 0; b < lcdButtons.size(); b++) {
      mfButtonFlags state = lcdButtons[b]->getState();
      if (state != MFBUTTON_NOSIGNAL) {
        switch (state) {
        case MFBUTTON_PRESSED:
          if (b == B_UP) {
            for (uint8_t p = 0; p < 4; p++)
              incValue(p);
          }
          if (b == B_DOWN) {
            for (uint8_t p = 0; p < 4; p++)
              decValue(p);
          }

          break;
        case MFBUTTON_PRESSED_LONG:
          if (b == B_OK) {
            exit = true;
          }
          if (b == B_UP) {
            repeat[0] = true;
            repeat[1] = false;
            resetTimer(repeatTimer[0]);
          }
          if (b == B_DOWN) {
            repeat[0] = false;
            repeat[1] = true;
            resetTimer(repeatTimer[1]);
          }
          break;
        case MFBUTTON_RELEASED:
          if (b == B_OK) {
            if (((size_t)modPos + 1) >= mod.size())
              modPos = 0;
            else
              modPos++;
          }
          if (b == B_UP) {
            repeat[0] = false;
            repeat[1] = false;
          }
          if (b == B_DOWN) {
            repeat[0] = false;
            repeat[1] = false;
          }
          break;
        case MFBUTTON_RELEASED_LONG:
          if (b == B_UP) {
            repeat[0] = false;
            repeat[1] = false;
          }
          if (b == B_DOWN) {
            repeat[0] = false;
            repeat[1] = false;
          }
          break;
        default:;
        }
      }
    }
    if ((repeat[0]) && ((TICKS() - repeatTimer[0]) > 50)) {
      for (uint8_t p = 0; p < 4; p++)
        incValue(p);
      resetTimer(repeatTimer[0]);
    }
    if ((repeat[1]) && ((TICKS() - repeatTimer[1]) > 50)) {
      for (uint8_t p = 0; p < 4; p++)
        decValue(p);
      resetTimer(repeatTimer[1]);
    }
    if (TICKS() - lcdTimer > 70) {
      lcd.noRefresh();
      lcdClear();
      lcd.setFont(1, &fontData);
      if (svInfo.present[0]) {
        lcd.print(border, border + 20 + 13, svInfo.names[0],
                  mod.at(modPos).mod[0] ? LCD_GREEN : LCD_WHITE);
        lcd.print(border, border + 20 + 28,
                  printfString("%d%%", data.presets[presetNum][0]), LCD_WHITE);
      }
      if (svInfo.present[1]) {
        lcd.printR(lcd.width - border, border + 20 + 13, svInfo.names[1],
                   mod.at(modPos).mod[1] ? LCD_GREEN : LCD_WHITE);
        lcd.printR(lcd.width - border, border + 20 + 28,
                   printfString("%d%%", data.presets[presetNum][1]), LCD_WHITE);
      }
      if (svInfo.present[2]) {
        lcd.print(border, border + 20 + lcd.height / 2, svInfo.names[2],
                  mod.at(modPos).mod[2] ? LCD_GREEN : LCD_WHITE);
        lcd.print(border, border + 38 + lcd.height / 2,
                  printfString("%d%%", data.presets[presetNum][2]), LCD_WHITE);
      }
      if (svInfo.present[3]) {
        lcd.printR(lcd.width - border, border + 20 + lcd.height / 2,
                   svInfo.names[3],
                   mod.at(modPos).mod[3] ? LCD_GREEN : LCD_WHITE);
        lcd.printR(lcd.width - border, border + 38 + lcd.height / 2,
                   printfString("%d%%", data.presets[presetNum][3]), LCD_WHITE);
      }
      lcd.refresh();
      resetTimer(lcdTimer);
    }
    DELAY(10);
  }
}

void doReset(MF_RESET reason) {
  MFLCD_FONTDATA fontData = {0};
  bool exit = false;
  MF_TRANSLATION TR_question = {"Are you sure\nyou want to\n",
                                "Вы уверены,\nчто хотите\n"};
  MF_TRANSLATION TR_confirmResetData = {"reset data", "сбросить данные"};
  MF_TRANSLATION TR_confirmResetSettings = {"reset settings",
                                            "сбросить настройки"};
  MF_TRANSLATION TR_confirmResetAll = {"make a factory reset",
                                       "сбросить до заводских\nнастроек"};
  MF_TRANSLATION text;
  switch (reason) {
  case MF_RESET_DATA:
    text = TR_confirmResetData;
    break;
  case MF_RESET_SETTINGS:
    text = TR_confirmResetSettings;
    break;
  case MF_RESET_ALL:
    text = TR_confirmResetAll;
    break;
  default:
    return;
  }
  lcd.noRefresh();
  lcd.clear(LCD_BLACK);
  lcd.boxwh(BW + 0, BW + 0, lcd.width - BW, 20, HEADER_COLOR);
  lcd.setFont(1, &fontData);
  lcd.printC(BW + 0, lcd.width - BW * 2, BW + 13,
             getTranslation(TR_reset, LANG), LCD_WHITE);
  lcd.setFont(0, &fontData);
  lcd.printC(BW + 0, lcd.width - BW * 2, BW + 50,
             getTranslation(TR_question, LANG) + getTranslation(text, LANG) +
                 "?",
             LCD_WHITE);
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
  MF_SETTINGS defaults;
  switch (reason) {
  case MF_RESET_DATA:
    for (uint8_t i = 0; i < 4; i++) {
      data.sensorRawMin[i] = defaults.sensorRawMin[i];
      data.sensorRawMax[i] = defaults.sensorRawMax[i];
      data.sensorAutoMin[i] = defaults.sensorAutoMin[i];
      data.sensorAutoMax[i] = defaults.sensorAutoMax[i];
      data.lastPosition[i] = defaults.lastPosition[i];
      data.adaptivesUp[i] = defaults.adaptivesUp[i];
      data.adaptivesDown[i] = defaults.adaptivesDown[i];
    }
    data.lastPreset = defaults.lastPreset;
    data.calibration = defaults.calibration;
    settings.save();

    break;
  case MF_RESET_SETTINGS:
    defaults = data;
    settings.reset();
    for (uint8_t i = 0; i < 4; i++) {
      data.sensorRawMin[i] = defaults.sensorRawMin[i];
      data.sensorRawMax[i] = defaults.sensorRawMax[i];
      data.sensorAutoMin[i] = defaults.sensorAutoMin[i];
      data.sensorAutoMax[i] = defaults.sensorAutoMax[i];
      data.lastPosition[i] = defaults.lastPosition[i];
      data.adaptivesUp[i] = defaults.adaptivesUp[i];
      data.adaptivesDown[i] = defaults.adaptivesDown[i];
    }
    data.lastPreset = defaults.lastPreset;
    data.calibration = defaults.calibration;
    settings.save();
    break;
  case MF_RESET_ALL:
    settings.reset();
    DELAY(1000);
    NVIC_SystemReset();
    break;
  default:
    break;
  }
  DELAY(1000);
}

void stressTest() {
  uint32_t stressTimer;
  MFLCD_FONTDATA fontData = {0};
  lcd.noRefresh();
  lcd.clear(LCD_BLACK);
  lcd.boxwh(BW + 0, BW + 0, lcd.width - BW, 20, HEADER_COLOR);
  lcd.setFont(1, &fontData);
  lcd.printC(BW + 0, lcd.width - BW * 2, BW + 13,
             getTranslation(TR_stressTest, LANG), LCD_WHITE);
  lcd.refresh();
  bPins *m[8] = {&MOS1, &MOS2, &MOS3, &MOS4, &MOS5, &MOS6, &MOS7, &MOS8};
  for (uint8_t i = 0; i < 8; i++) {
    m[i]->setPWM(data.valvesDuty);
  }

  while (1) {
    checkACC();
    stressTimer = TICKS();
    MOS1.off();
    MOS2.off();
    MOS3.off();
    MOS4.off();
    MOS5.off();
    MOS6.off();
    MOS7.off();
    MOS8.off();
    MOS1.on();
    MOS3.on();
    MOS5.on();
    MOS7.on();
    while (TICKS() - stressTimer < (1 * 60 * 1000)) {
      checkACC();
      for (uint8_t b = 0; b < lcdButtons.size(); b++) {
        mfButtonFlags state = lcdButtons[b]->getState();
        if (state != MFBUTTON_NOSIGNAL) {
          MOS1.off();
          MOS2.off();
          MOS3.off();
          MOS4.off();
          MOS5.off();
          MOS6.off();
          MOS7.off();
          MOS8.off();
          return;
        }
      }
      DELAY(10);
    }
    stressTimer = TICKS();
    MOS1.off();
    MOS2.off();
    MOS3.off();
    MOS4.off();
    MOS5.off();
    MOS6.off();
    MOS7.off();
    MOS8.off();
    MOS2.on();
    MOS4.on();
    MOS6.on();
    MOS8.on();
    while (TICKS() - stressTimer < (1 * 60 * 1000)) {
      checkACC();
      for (uint8_t b = 0; b < lcdButtons.size(); b++) {
        mfButtonFlags state = lcdButtons[b]->getState();
        if (state != MFBUTTON_NOSIGNAL) {
          MOS1.off();
          MOS2.off();
          MOS3.off();
          MOS4.off();
          MOS5.off();
          MOS6.off();
          MOS7.off();
          MOS8.off();
          return;
        }
      }
      DELAY(10);
    }
  }
}
void manualControl() {
  if (!engineOn())
    return;
  constexpr uint8_t border = 7;
  uint32_t lcdTimer = TICKS();
  MFLCD_FONTDATA fontData = {0};
  bool exit = false;

  lcd.noRefresh();
  lcd.clear(LCD_BLACK);
  lcd.boxwh(BW + 0, BW + 0, lcd.width - BW, 20, HEADER_COLOR);
  lcd.setFont(1, &fontData);
  lcd.printC(BW + 0, lcd.width - BW * 2, BW + 13,
             getTranslation(TR_manualControl, LANG), LCD_WHITE);

  lcdClear();
  lcd.refresh();
  exit = false;
  MF_TRANSLATION TR_locked = {"lock", "блок"};
  bool compRun = false, dryRun = false;
  while (!exit) {
    checkACC();
    if (TICKS() - lcdTimer > 70) {
      lcd.noRefresh();
      lcdClear();
      lcd.setFont(0, &fontData);
      uint16_t compColor;
      String message = getTranslation(TR_compressorEnabled, LANG) + ": ";
      if (svInfo.svComp->isCompressorRunning()) {
        compColor = RGB(31, 63, 15);
        message += getTranslation(TR_on, LANG);
        if (!compRun)
          svInfo.svComp->stopCompressor();
      } else {
        if (svInfo.svComp->isCompressorLocked()) {
          compColor = RGB(31, 31, 15);
          message += getTranslation(TR_locked, LANG);
        } else {
          compColor = RGB(15, 63, 15);
          message += getTranslation(TR_off, LANG);
          if (compRun)
            svInfo.svComp->startCompressor();
        }
      }
      if (data.dryer != MF_DRYER_NO) {
        message += "\n" + getTranslation(TR_dryer, LANG) + ": ";
        if (svInfo.svComp->isDryerRunning()) {
          message += getTranslation(TR_on, LANG);
          if (!dryRun)
            svInfo.svComp->stopDryer();
        } else {
          if (svInfo.svComp->isDryerLocked())
            message += getTranslation(TR_locked, LANG);
          else {
            message += getTranslation(TR_off, LANG);
            if (dryRun)
              svInfo.svComp->startDryer();
          }
        }
      }
      if (data.receiverPresent) {
        message += printfString("\n%u psi", svInfo.svComp->getPressure());
      }

      lcd.printC(0, lcd.width, 66, message, compColor);
      lcd.refresh();
      resetTimer(lcdTimer);
    }
    DELAY(5);
    for (uint8_t b = 0; b < lcdButtons.size(); b++) {
      mfButtonFlags state = lcdButtons[b]->getState();
      if (state != MFBUTTON_NOSIGNAL) {
        switch (state) {
        case MFBUTTON_PRESSED:
          if (b == B_UP)
            compRun = true;
          if (b == B_DOWN)
            dryRun = true;
          break;
        case MFBUTTON_PRESSED_LONG:
          if (b == B_OK)
            return;
          break;
        case MFBUTTON_RELEASED_LONG:
        case MFBUTTON_RELEASED:
          if (b == B_UP)
            compRun = false;
          if (b == B_DOWN)
            dryRun = false;
          if (b == B_OK)
            exit = true;
          break;
        default:;
        }
      }
    }
  }
  svInfo.svComp->stopCompressor();
  svInfo.svComp->stopDryer();
  DELAY(1000);
}
