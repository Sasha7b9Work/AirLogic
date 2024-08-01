typedef struct {
  float value[4];
} t_preset;
struct T_wheels {
  mfWheel &w;
  mfSensor &s;
  bool present;
  // float lastValue =
  // this->s.getPercentD();
  T_wheels *linked = nullptr;
  uint32_t linkedTimer = TICKS();
  uint32_t upTimer = TICKS();
  uint32_t downTimer = TICKS();
};
struct T_SVINFO {
  volatile bool enableSupervisor = false;
  mfBusCompControl *svComp = nullptr;
  t_preset curPreset = {{50, 50, 50, 50}};
  volatile uint8_t movingDir[4] = {0, 0, 0, 0};
  volatile bool movingDown = false;
  volatile bool movingUp = false;
  volatile bool moving = false;
  volatile bool resetUpDownCounters = false;
  int32_t level[4] = {0, 0, 0, 0};
  float volt[4] = {0, 0, 0, 0};
  float raw[4] = {0, 0, 0, 0};
  String names[4] = {"", "", "", ""};
  int32_t levelDiff[4] = {data.sensitivityRange, data.sensitivityRange,
                          data.sensitivityRange, data.sensitivityRange};
  std::function<bool(void)> moveFeedback = []() { return true; };
  bool calibrate = false;
  bool present[4] = {false, false, false, false};
  float speed[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  bool locked[4] = {false, false, false, false};
  uint8_t retry[4] = {0, 0, 0, 0};
  bool restartSV = false;
  bool restartCCM = false;
  bool autoCalibration = false;
  bool needAdapt = true;
  mfWheel *wheel[4];
  bool doNotStop[4] = {false, false, false, false};
  bool forceStop[4] = {false, false, false, false};
} svInfo;
typedef struct {
  uint32_t valve;
  uint32_t speedSense;
} t_sensitivity;
t_sensitivity sensitivity[3] = {{1000000, 500}, {500000, 1000}, {100000, 2000}};
TaskHandle_t svTask = NULL;

float recalcAdaptive(mfWheel *wheel, float from, float to, float now,
                     float oldAdaptive) {
  if (!wheel->present)
    return oldAdaptive;
  float diff;
  float adaptive = oldAdaptive, expect;
  String dir = "";

  if (::fabsf(from - now) < data.sensitivityRange * 2) {
    debug.printf("[%s] movement is to low\n", wheel->name);
    return oldAdaptive; // if movement is too low, skip
  }
  if (::fabsf(to - now) > data.sensitivityRange * 2) {
    debug.printf("[%s] error is too high\n", wheel->name);
    return oldAdaptive; // if error is too high, skip
  }
  if (from < now) {
    expect = to - oldAdaptive;
    diff = now - expect;
    dir = "U";
  } else {
    expect = to + oldAdaptive;
    diff = expect - now;
    dir = "D";
  }
  float newAdaptive = (diff * 0.2 + oldAdaptive * 0.8); // 20% new value weight
  if (diff >= 0)
    adaptive = newAdaptive;
  debug.printf("[%s] %s fr=%.2f to=%.2f exp=%.2f act=%.2f diff=%.2f "
               "was=%.2f now=%.2f\n",
               wheel->name, dir.c_str(), from, to, expect, now, diff,
               oldAdaptive, adaptive);
  return adaptive;
}

void supervisorTask(void *arg) {
  UNUSED(arg);
  { // create a scope, we need to free anything before task restart
    bool upPriority = (data.valves & 1U) != 0 ? false : true;
    bool lockDir = (data.valves & 1U) != 0 ? false : true;
    uint32_t svTimer = TICKS();
    uint32_t speedTimer[4] = {TICKS(), TICKS(), TICKS(), TICKS()};

    S1.setSensitivity(sensitivity[data.sensitivity].valve);
    S1.setRange(data.sensorAutoMin[0], data.sensorAutoMax[0]);

    S2.setSensitivity(sensitivity[data.sensitivity].valve);
    S2.setRange(data.sensorAutoMin[1], data.sensorAutoMax[1]);

    S3.setSensitivity(sensitivity[data.sensitivity].valve);
    S3.setRange(data.sensorAutoMin[2], data.sensorAutoMax[2]);

    S4.setSensitivity(sensitivity[data.sensitivity].valve);
    S4.setRange(data.sensorAutoMin[3], data.sensorAutoMax[3]);

    mfWheel *FL, *FR, *RL, *RR;
    mfValve M1 = mfValve("M1", &MOS1);
    mfValve M2 = mfValve("M2", &MOS2);
    mfValve M3 = mfValve("M3", &MOS3);
    mfValve M4 = mfValve("M4", &MOS4);
    mfValve M5 = mfValve("M5", &MOS5);
    mfValve M6 = mfValve("M6", &MOS6);
    mfValve M7 = mfValve("M7", &MOS7);
    mfValve M8 = mfValve("M8", &MOS8);
    mfValve *temp[8] = {
        &M1, &M2, &M3, &M4, data.receiverPresent ? &M5 : nullptr,
        &M6, &M7, &M8};
    for (uint8_t i = 0; i < 8; i++)
      temp[i]->setDuty(data.valvesDuty);
    std::function<bool(uint8_t)> axle = [](uint8_t flag) {
      return (data.axles & flag);
    };
    if ((data.valves & 1U) != 0) {
      bool wabco = (data.dryer == MF_DRYER_COMP);
      debug.printf("wabco = %d, no comp = %d\n", wabco,
                   (data.receiverPresent && !wabco));
      FL = new mfWheel("FL", {
        up : &M1,
        down : &M2,
        receiverPresent : true,
        receiver : nullptr,
        commonExhaust : wabco,
        comp : data.receiverPresent && !wabco ? nullptr : svInfo.svComp,
        wabco8ch : wabco,
        sensor : &S1,
        present : axle(MF_FRONT_AXLE)
      });
      FR = new mfWheel("FR", {
        up : &M3,
        down : &M4,
        receiverPresent : true,
        receiver : nullptr,
        commonExhaust : wabco,
        comp : data.receiverPresent && !wabco ? nullptr : svInfo.svComp,
        wabco8ch : wabco,
        sensor : &S2,
        present : axle(MF_FRONT_AXLE)
      });

      RL = new mfWheel("RL", {
        up : &M5,
        down : &M6,
        receiverPresent : true,
        receiver : nullptr,
        commonExhaust : wabco,
        comp : data.receiverPresent && !wabco ? nullptr : svInfo.svComp,
        wabco8ch : wabco,
        sensor : &S3,
        present : axle(MF_REAR_AXLE)
      });
      RR = new mfWheel("RR", {
        up : &M7,
        down : &M8,
        receiverPresent : true,
        receiver : nullptr,
        commonExhaust : wabco,
        comp : data.receiverPresent && !wabco ? nullptr : svInfo.svComp,
        wabco8ch : wabco,
        sensor : &S4,
        present : axle(MF_REAR_AXLE)
      });
    } else {
      //      lockDir = true;
      FL = new mfWheel("FL", {
        up : &M1,
        down : &M1,
        receiverPresent : data.receiverPresent,
        receiver : data.receiverPresent ? &M5 : nullptr,
        commonExhaust : true,
        comp : svInfo.svComp,
        sensor : &S1,
        present : axle(MF_FRONT_AXLE)
      });
      FR = new mfWheel("FR", {
        up : &M2,
        down : &M2,
        receiverPresent : data.receiverPresent,
        receiver : data.receiverPresent ? &M5 : nullptr,
        commonExhaust : true,
        comp : svInfo.svComp,
        sensor : &S2,
        present : axle(MF_FRONT_AXLE)
      });

      RL = new mfWheel("RL", {
        up : &M3,
        down : &M3,
        receiverPresent : data.receiverPresent,
        receiver : data.receiverPresent ? &M5 : nullptr,
        commonExhaust : true,
        comp : svInfo.svComp,
        sensor : &S3,
        present : axle(MF_REAR_AXLE)
      });
      RR = new mfWheel("RR", {
        up : &M4,
        down : &M4,
        receiverPresent : data.receiverPresent,
        receiver : data.receiverPresent ? &M5 : nullptr,
        commonExhaust : true,
        comp : svInfo.svComp,
        sensor : &S4,
        present : axle(MF_REAR_AXLE)
      });
      if (data.receiverPresent) {
        M5.valve->updateChannel(2);
        M5.valve->setPWM(100);
      }
    }

    svInfo.wheel[0] = FL;
    svInfo.wheel[1] = FR;
    svInfo.wheel[2] = RL;
    svInfo.wheel[3] = RR;
    uint8_t wheels = 4;

    for (uint8_t w = 0; w < wheels; w++) {
      svInfo.present[w] = svInfo.wheel[w]->present;
      if (svInfo.present[w]) {
        svInfo.names[w] = svInfo.wheel[w]->name;
        svInfo.levelDiff[w] = data.sensitivityRange;
      }
    }

    std::function<bool(void)> isUpActive = [&wheels]() {
      bool ret = false;
      for (auto wheel = 0; wheel < wheels; wheel++)
        if (svInfo.wheel[wheel]->isUpOn()) {
          ret = true;
          break;
        }
      return ret;
    };
    std::function<bool(void)> isDownActive = [&wheels]() {
      bool ret = false;
      for (auto wheel = 0; wheel < wheels; wheel++)
        if (svInfo.wheel[wheel]->isDownOn()) {
          ret = true;
          break;
        }
      return ret;
    };
    std::function<void(void)> fillPercentAndVoltage = [&wheels]() {
      for (auto i = 0; i < wheels; i++) {
        if (svInfo.wheel[i]->present) {
          svInfo.level[i] = svInfo.wheel[i]->config.sensor->getPercentI();
          svInfo.volt[i] = svInfo.wheel[i]->config.sensor->getVoltageA();
          svInfo.raw[i] = svInfo.wheel[i]->config.sensor->getValueD();
          bool upLock = (svInfo.wheel[i]->config.up)
                            ? svInfo.wheel[i]->config.up->isLocked()
                            : false;
          bool downLock = (svInfo.wheel[i]->config.down)
                              ? svInfo.wheel[i]->config.down->isLocked()
                              : false;
          svInfo.locked[i] = upLock || downLock;
        }
      }
    };

    std::function<void(float[4])> calcAdaptives = [&wheels](float moveFrom[4]) {
      for (uint8_t z = 0; z < wheels; z++) {
        float from = moveFrom[z];
        float to = svInfo.curPreset.value[z];
        float now = svInfo.wheel[z]->config.sensor->getPercentD();
        float *adapt =
            (from < now) ? &data.adaptivesUp[z] : &data.adaptivesDown[z];
#if 1
        float newAdapt =
            svInfo.needAdapt
                ? recalcAdaptive(svInfo.wheel[z], from, to, now, *adapt)
                : *adapt;
        *adapt = newAdapt;
#endif
      }
    };
    std::function<void(void)> resetUpDownCounters = [&wheels] {
      for (auto i = 0; i < wheels; i++) {
        if (!svInfo.wheel[i]->present)
          continue;
        svInfo.wheel[i]->upTimer = 0;
        svInfo.wheel[i]->downTimer = 0;
      }
    };
    std::function<void(void)> disableUpDownCounters = [&wheels] {
      for (auto i = 0; i < wheels; i++) {
        if (!svInfo.wheel[i]->present)
          continue;
        svInfo.wheel[i]->upTimer = TICKS();
        svInfo.wheel[i]->downTimer = TICKS();
      }
    };
    std::function<bool(mfWheel *, int16_t, int32_t, bool, bool)>
        checkWeNeedToMoveUp =
            [isDownActive](mfWheel *wheel, int16_t preset, int32_t delta,
                           bool directionLocked, bool upPriority) {
              if (wheel->isUpOn() || (wheel->isDownOn() && !upPriority)) {
                resetTimer(wheel->upTimer);
                resetTimer(wheel->downTimer);
                return false;
              }

              if (wheel->config.sensor->getPercentD() < preset - delta) {
                if (TICKS() - wheel->upTimer > data.delay * 1000) {
                  if (!(directionLocked && (isDownActive())))
                    wheel->upOn();
                  return true;
                }
                return false;
              } else {
                resetTimer(wheel->upTimer);
                return false;
              }
              return false;
            };
    std::function<void(mfWheel *, int16_t, int32_t, bool, bool)>
        checkWeNeedToMoveDown =
            [isUpActive](mfWheel *wheel, int16_t preset, int32_t delta,
                         bool directionLocked, bool upPriority) {
              if (wheel->isUpOn() || wheel->isDownOn()) {
                resetTimer(wheel->upTimer);
                resetTimer(wheel->downTimer);
                return;
              }

              if (wheel->config.sensor->getPercentD() > preset + delta) {
                if (TICKS() - wheel->downTimer >
                    data.delay * (1000 + (upPriority ? 100U : 0U)))
                  if (!(directionLocked && (isUpActive())))
                    wheel->downOn();
              } else {
                resetTimer(wheel->downTimer);
              }
            };

    std::function<bool(mfWheel *, uint32_t &, uint32_t &, float &, bool &)>
        isSpeedTooSlow = [](mfWheel *wheel, uint32_t &timer, uint32_t &delay,
                            float &prevSpeed, bool &flag) {
          return wheel->isStucked();
          ///////////////////////////////////////
          const uint32_t speedCheckDelay = svInfo.autoCalibration ? 2000 : 2000;
          float speed = wheel->config.sensor->getSpeed();
          bool ret = false;

          if ((TICKS() - delay > speedCheckDelay) && svInfo.moving) {
            if ((wheel->upOnTime() > sensitivity[data.sensitivity].speedSense >>
                 1) ||
                (wheel->downOnTime() >
                 sensitivity[data.sensitivity].speedSense >> 1)) {
              ret = (speed > -0.007) && (speed < 0.007);
              if (!flag) {
                if (ret) {
                  resetTimer(timer);
                  flag = true;
                  ret = false;
                }
              } else {
                if (ret) {
                  if ((TICKS() - timer <
                       sensitivity[data.sensitivity].speedSense) ||
                      ((speed / prevSpeed) > 1))
                    ret = false;
                } else {
                  flag = false;
                }
              }
            } else {
              flag = false;
            }
          }
          prevSpeed = speed;
          return ret;
        };
    std::function<void(mfWheel *, float &, float, float &, bool, uint8_t &)>
        checkWeNeedToStopDown = [](mfWheel *wheel, float &preset, float from,
                                   float &adaptive, bool tooSlow,
                                   uint8_t &retry) {
          if (!wheel->isDownOn())
            return;
          float level = wheel->config.sensor->getPercentD();
          bool overloaded = wheel->isDownOverloaded();
          if ((level <= preset + adaptive) || overloaded || tooSlow) {
            wheel->downOff();
            if (overloaded || tooSlow) {
              if (wheel->config.down)
                wheel->config.down->lock(svInfo.autoCalibration ? 100 : 5000);
              if ((retry >= 3) || svInfo.autoCalibration) {
                preset = level; // set stop value as valid level
              } else {
                retry++;
              }
            } else {
              retry = 0;
            }
            resetTimer(wheel->downTimer);
            resetTimer(wheel->upTimer);
            float newAdapt =
                svInfo.needAdapt
                    ? recalcAdaptive(wheel, from, preset,
                                     wheel->config.sensor->getPercentD(),
                                     adaptive)
                    : adaptive;
            debug.printf("[%s] stop down, v=%.2f, l=%.2f, trg=%.2f\n",
                         wheel->name, wheel->config.sensor->getPercentD(),
                         level, preset + adaptive);
            adaptive = newAdapt;
          }
        };
    std::function<void(mfWheel *, float &, float, float &, bool, uint8_t &)>
        checkWeNeedToStopUp = [](mfWheel *wheel, float &preset, float from,
                                 float &adaptive, bool tooSlow,
                                 uint8_t &retry) {
          if (!wheel->isUpOn())
            return;
          float level = wheel->config.sensor->getPercentD();
          bool overloaded = wheel->isUpOverloaded();
          if ((level >= preset - adaptive) || overloaded || tooSlow) {
            wheel->upOff();
            if (overloaded || tooSlow) {
              if (wheel->config.up)
                wheel->config.up->lock(svInfo.autoCalibration ? 100 : 5000);
              if ((retry >= 3) || svInfo.autoCalibration) {
                preset = level; // set stop value as valid level
              } else {
                retry++;
              }
            } else {
              retry = 0;
            }
            resetTimer(wheel->downTimer);
            resetTimer(wheel->upTimer);
            float newAdapt =
                svInfo.needAdapt
                    ? recalcAdaptive(wheel, from, preset,
                                     wheel->config.sensor->getPercentD(),
                                     adaptive)
                    : adaptive;
            debug.printf("[%s] stop up, v=%.2f, l=%.2f, trg=%.2f\n",
                         wheel->name, wheel->config.sensor->getPercentD(),
                         level, preset - adaptive);
            adaptive = newAdapt;
          }
        };

    /* main loop */
    DELAY(100);
    if (!svInfo.autoCalibration)
      if (!data.upOnStart)
        for (auto i = 0; i < wheels; i++)
          svInfo.curPreset.value[i] =
              lrint(svInfo.wheel[i]->config.sensor->getPercentD());

    bool tooSlow[4] = {false, false, false, false};
    uint32_t lastSpeedTimer[4] = {TICKS(), TICKS(), TICKS(), TICKS()};
    float startPos[4] = {0, 0, 0, 0};
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (!svInfo.restartSV) { // main loop
      vTaskDelayUntil(&xLastWakeTime, 1);
      if (TICKS() - svTimer > 500) {
        freeStack.sv = uxTaskGetStackHighWaterMark(NULL);
        debug.printf("Speed "
                     "%.4f %.4f %.4f %.4f // ext: %d (%d) // "
                     "%.4f %.4f %.4f %.4f\n",
                     svInfo.speed[0], svInfo.speed[1], svInfo.speed[2],
                     svInfo.speed[3],
                     lrint(getMeasured() * data.speedSensCalibration),
                     getMeasured(), svInfo.wheel[0]->getCurrentSpeed(),
                     svInfo.wheel[1]->getCurrentSpeed(),
                     svInfo.wheel[2]->getCurrentSpeed(),
                     svInfo.wheel[3]->getCurrentSpeed());
        // uint32_t interval = TICKS() - svTimer;
        // debug.printf("ADC readings: %lu per sec\n",
        //             (1000 * dmaReadingsCounter) / interval);
        // dmaReadingsCounter = 0;
        resetTimer(svTimer);
      }
      fillPercentAndVoltage();

      if (svInfo.svComp->settings.externalValve) {
        bool requested = svInfo.svComp->isValveRequested();
        bool extLock = svInfo.svComp->isExtLocked();
        if (M5.isLocked()) {
          if (!extLock) {
            svInfo.svComp->denyExtValve();
            if (requested)
              svInfo.svComp->stopCompressor();
          }
        } else {
          if (requested) {
            if (extLock) {
              // if (!M5.isActive())
              M5.on();
              svInfo.svComp->allowExtValve();
              debug.println("extValve requested, unlocking...");
            }
          } else {
            if (!extLock) {
              svInfo.svComp->denyExtValve();
              // if (M5.isActive())
              M5.off();
              debug.println("extValve released, locking...");
            }
          }
        }
        //
      }

      if (svInfo.enableSupervisor &&
          (data.calibration || svInfo.autoCalibration) && engineOn()) {
        if (svInfo.moving && (!svInfo.movingDown && !svInfo.movingUp)) {
          svInfo.moving = false;
          svInfo.needAdapt = svInfo.moveFeedback();
          svInfo.moveFeedback = []() { return true; };
          /*if (needAdapt &&
              !svInfo.autoCalibration) // got cmd to recalc adaptives
            calcAdaptives(startPos);*/
          if (!svInfo.autoCalibration)
            for (uint8_t x = 0; x < 4; x++)
              data.lastPosition[x] = svInfo.curPreset.value[x];
          settings.save(); // save new position and adaptives
        }
        if ((!svInfo.moving) && (svInfo.movingDown || svInfo.movingUp)) {
          svInfo.moving = true;
          for (uint8_t x = 0; x < wheels; x++) {
            resetTimer(speedTimer[x]);
            if (svInfo.wheel[x]->present)
              startPos[x] = svInfo.wheel[x]->config.sensor->getPercentD();
          }
        }
        if (svInfo.autoCalibration)
          disableUpDownCounters(); // prevent correction in the calibration
        if (svInfo.resetUpDownCounters)
          resetUpDownCounters(); // make timers huge enuff

        svInfo.movingDown = isDownActive();
        svInfo.movingUp = isUpActive();

        for (auto i = 0; i < wheels; i++) {
          if (!svInfo.wheel[i]->present)
            continue;
          if (svInfo.wheel[i]->isUpOn())
            svInfo.movingDir[i] |= 2U;
          else
            svInfo.movingDir[i] &= ~2U;

          if (svInfo.wheel[i]->isDownOn())
            svInfo.movingDir[i] |= 1U;
          else
            svInfo.movingDir[i] &= ~1U;

          bool upTriggered =
              checkWeNeedToMoveUp(svInfo.wheel[i], svInfo.curPreset.value[i],
                                  svInfo.levelDiff[i], lockDir, upPriority);
          if (upTriggered && isDownActive() && upPriority) {
            for (auto wh = 0; wh < 4; wh++) {
              if (svInfo.wheel[wh]->isDownOn())
                svInfo.wheel[wh]->downOff();
            }
          } else
            checkWeNeedToMoveDown(svInfo.wheel[i], svInfo.curPreset.value[i],
                                  svInfo.levelDiff[i], lockDir, upPriority);

          if (svInfo.doNotStop[i])
            speedTimer[i] = TICKS();
          bool speedTooSlow =
              svInfo.doNotStop[i]
                  ? false
                  : isSpeedTooSlow(svInfo.wheel[i], lastSpeedTimer[i],
                                   speedTimer[i], svInfo.speed[i], tooSlow[i]);
          if (svInfo.forceStop[i])
            speedTooSlow = true;
          checkWeNeedToStopDown(svInfo.wheel[i], svInfo.curPreset.value[i],
                                startPos[i], data.adaptivesDown[i],
                                speedTooSlow, svInfo.retry[i]);
          checkWeNeedToStopUp(svInfo.wheel[i], svInfo.curPreset.value[i],
                              startPos[i], data.adaptivesUp[i], speedTooSlow,
                              svInfo.retry[i]);
          if (svInfo.forceStop[i]) {
            svInfo.forceStop[i] = false;
            debug.printf("%d: forced stop\n", i);
          }

          // fill svInfo struct
        }
      } else // switch off all channels
        for (auto i = 0; i < wheels; i++) {
          if (!svInfo.wheel[i]->present)
            continue;
          resetTimer(svInfo.wheel[i]->downTimer);
          resetTimer(svInfo.wheel[i]->upTimer);
          if (svInfo.wheel[i]->isUpOn())
            svInfo.wheel[i]->upOff();
          if (svInfo.wheel[i]->isDownOn())
            svInfo.wheel[i]->downOff();
        }
      // YIELD();
      DELAY(5);
    }
    for (auto i = 0; i < wheels; i++) {
      if (!svInfo.wheel[i]->present)
        continue;
      if (svInfo.wheel[i]->isUpOn())
        svInfo.wheel[i]->upOff();
      if (svInfo.wheel[i]->isDownOn())
        svInfo.wheel[i]->downOff();
      if (svInfo.wheel[i]->config.comp) {
        if (svInfo.wheel[i]->config.comp->isCompressorRunning())
          svInfo.wheel[i]->config.comp->stopCompressor();
        if (svInfo.wheel[i]->config.comp->isDryerRunning())
          svInfo.wheel[i]->config.comp->stopDryer();
      }
    }
    for (auto i = 0; i < 4; i++) {
      svInfo.wheel[i]->~mfWheel();
    }
  }
  svInfo.restartSV = false;
  while (1)
    ;
}

void restartSV() {
  if (svTask != NULL) {
    svInfo.restartSV = true;
    while (svInfo.restartSV)
      DELAY(1);
    vTaskDelete(svTask);
  }
  if (xTaskCreate(supervisorTask, NULL, SV_STACK_SIZE, NULL, 1, &svTask) !=
      pdPASS) {
    debug.printf("Cant create supervisor task, HALTED!");
    DELAY(10000);
    HAL_NVIC_SystemReset();
  }
};

void setLevel(int16_t *num, uint8_t _len, bool adapt, uint8_t tolerance = 2) {
  svInfo.enableSupervisor = false;
  DELAY(10);
  for (auto i = 0; i < _len; i++) {
    svInfo.curPreset.value[i] = num[i];
    svInfo.levelDiff[i] = tolerance;
  }
  bool oldNeedAdapt = svInfo.needAdapt;
  svInfo.needAdapt = adapt;
  svInfo.resetUpDownCounters = true;
  svInfo.moveFeedback = [oldNeedAdapt]() mutable {
    svInfo.enableSupervisor = data.autoHeightCorrection;
    return oldNeedAdapt;
  };
  svInfo.enableSupervisor = true;
  DELAY(100);
  while (_len--)
    svInfo.levelDiff[_len] = data.sensitivityRange;
  svInfo.resetUpDownCounters = false;
};
void stopUp(bool setMax = true) {
  int16_t max = -100;
  int32_t cur[4] = {svInfo.level[0], svInfo.level[1], svInfo.level[2],
                    svInfo.level[3]};
  if (setMax) {
    for (uint8_t i = 0; i < 4; i++)
      if (svInfo.present[i])
        if (max <= cur[i])
          max = cur[i];
    int16_t lev[4] = {(int16_t)(max + 1), (int16_t)(max + 1),
                      (int16_t)(max + 1), (int16_t)(max + 1)};
    debug.printf("Stop UP at %d [%d, %d, %d, %d]\n", max + 1, cur[0], cur[1],
                 cur[2], cur[3]);
    setLevel(lev, 4, false /*, data.sensitivityRange*/);
  } else {
    int16_t lev[4] = {(int16_t)cur[0], (int16_t)cur[1], (int16_t)cur[2],
                      (int16_t)cur[3]};
    debug.printf("Stop UP at [%d, %d, %d, %d]\n", cur[0], cur[1], cur[2],
                 cur[3]);
    setLevel(lev, 4, false, data.sensitivityRange);
  }
}
void stopDown(bool setMin = true) {
  int16_t min = 200;
  int32_t cur[4] = {svInfo.level[0], svInfo.level[1], svInfo.level[2],
                    svInfo.level[3]};
  if (setMin) {
    for (uint8_t i = 0; i < 4; i++)
      if (svInfo.present[i])
        if (min >= cur[i])
          min = cur[i];
    int16_t lev[4] = {(int16_t)(min - 1), (int16_t)(min - 1),
                      (int16_t)(min - 1), (int16_t)(min - 1)};
    debug.printf("Stop DOWN at %d [%d, %d, %d, %d]\n", min - 1, cur[0], cur[1],
                 cur[2], cur[3]);
    setLevel(lev, 4, false /*, data.sensitivityRange*/);
  } else {
    int16_t lev[4] = {(int16_t)cur[0], (int16_t)cur[1], (int16_t)cur[2],
                      (int16_t)cur[3]};
    debug.printf("Stop DOWN at [%d, %d, %d, %d]\n", cur[0], cur[1], cur[2],
                 cur[3]);
    setLevel(lev, 4, false, data.sensitivityRange);
  }
}
