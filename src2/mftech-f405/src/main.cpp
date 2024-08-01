#include "mfbus.h"
#include "mfproto.h"

#include "mfprotophy.h"
#include "mfprotophyacm.h"
#include "mfprotophypipe.h"
#include "mfprotophyserial.h"
#include "mfprotopipe.h"

#include "mfcomp.h"
#include "mfsensor.h"
#include "mfvalve.h"
#include "mfwheel.h"

#include "mftranslation.h"

#include "mflcd.h"
#include "mflcd_data.h"

#include "pins_stm.h"
#include "wiring.h"

#include <STM32FreeRTOS.h>

#include "SPI.h"
#define SPI_CS PA15
#include "w25qxx.h"

#include "mfmeasure.h"

#define SV_STACK_SIZE (0x800)
#define LCD_STACK_SIZE (0x800)
#define CCM_STACK_SIZE (0x300)
#define ACM_STACK_SIZE (0x300)
#define SETUP_STACK_SIZE (0x200)
#define SENS_STACK_SIZE (0x80)
#define ALL_STACK_SIZE                                                         \
  (SV_STACK_SIZE + LCD_STACK_SIZE + CCM_STACK_SIZE + ACM_STACK_SIZE +          \
   SETUP_STACK_SIZE + SENS_STACK_SIZE)
extern w25qxx_t w25qxx;

#include "init.h"

#include "PinManage.h"
#include "apins.h"
#include "mfbus_button.h"
#include "mfbusrgb.h"
#include "mfbustone.h"
#include "mfbutton.h"

#include "mfsettings.h"
#include "mfsettings_data.h"
uint32_t curValvesFreq = 100;
#include "mfmenu.h"
#include <cPins.h>

#include <STM32LowPower.h>

volatile uint32_t TICKS() { return uwTick; }

#define BAUD (MFPROTO_SERIAL_MAXBAUD / 10)

MFSENSOR(S1, &SENS_1);
MFSENSOR(S2, &SENS_2);
MFSENSOR(S3, &SENS_3);
MFSENSOR(S4, &SENS_4);
MFSENSOR(S5V, &SENS_5V);

MFPROTOPHYSERIAL(lcdPhy, Serial4, MFPROTO_PHY_SERIAL_NC, BAUD);
MFPROTO(lcdProto, lcdPhy);
MFBUS(lcdBus, lcdProto, MFBUS_MASTER);
MFLCD(lcd, lcdBus, MF_ALPRO_V10_LCD);
mfBusRGB(lcdOK)("ledOK", lcdBus, MF_ALPRO_V10_LCD);

MFPROTOPHYSERIAL(buttonPhy, Serial3, PD12, BAUD / 2);
MFPROTO(buttonProto, buttonPhy);
MFBUS(buttonBus, buttonProto, MFBUS_MASTER);
uint8_t buttonID = MF_ALPRO_V10_BUTTON;
mfBusRGB(ledUP)("ledUP", buttonBus, buttonID);
mfBusRGB(ledOK)("ledOK", buttonBus, buttonID);
mfBusRGB(ledDOWN)("ledDOWN", buttonBus, buttonID);
mfBusBeep(buttonBeep)("buttonBeep", buttonBus, buttonID);

mfBusButton *butUP = nullptr;
mfBusButton *butOK = nullptr;
mfBusButton *butDOWN = nullptr;

MFPROTOPHYACM(_ACM, BAUD);
MFPROTO(USB, _ACM);
MFBUS(ACM, USB, 2, MFBUS_CAP_DEBUG);

MFPROTOPHYPIPE(ccmpipephy);
MFPROTOPHYPIPE(mainpipephy);
MFPROTOPIPE(pipe, {&ccmpipephy, &mainpipephy});
MFPROTO(ccmproto, ccmpipephy);
MFPROTO(mainproto, mainpipephy);
MFBUS(mainpipe, mainproto, MFBUS_MASTER);
MFBUS(CCM, ccmproto, MF_ALPRO_V10_CCM, MFBUS_CAP_DEBUG);

mfCompControl comp = mfCompControl(&SENS_HEAT, &SENS_COMP, &COMP, &DRY);

#include "debug.h"
DebugSerial debug(ACM);

struct {
  uint32_t ACM = 0, setup = 0, sv = 0, monitor = 0, sens = 0, CCM = 0;
} freeStack;

#define dly(a) vTaskDelay((a))

#include "notes.h"
#include <HardwareTimer.h>

inline void resetTimer(uint32_t &timer) { timer = TICKS(); }

constexpr float SENS12_VOLTAGE = (((20000.0 + 5100.0) * INPUT_LIMIT) / 5100.0);
constexpr float accOnVoltage = 9.0f;
constexpr float accOffVoltage = 8.0f;
volatile bool ACCstate = false;
volatile uint32_t accTimer = TICKS();
volatile bool engineState = false;
volatile uint32_t engineTimer = TICKS();
volatile bool wifibtPresent = false;
bool ACCon(void) { // get ACC state
  float voltage =
      SENS_ACC.getA() * SENS12_VOLTAGE / 4095 + data.voltageCorrection;
  if (ACCstate) {
    if (voltage < accOffVoltage) {
      if (TICKS() - accTimer > 500) {
        ACCstate = false;
        accTimer = TICKS();
        debug.println("ACC: off!");
      }
    } else {
      accTimer = TICKS();
    }
  } else {
    if (voltage >= accOnVoltage) {
      if (TICKS() - accTimer > 500) {
        ACCstate = true;
        accTimer = TICKS();
        debug.println("ACC: on!");
      }
    } else {
      accTimer = TICKS();
    }
  }
  return ACCstate;
}
#define checkACC()                                                             \
  do {                                                                         \
    if (!ACCon())                                                              \
      return;                                                                  \
  } while (0)
#define checkACCbool()                                                         \
  do {                                                                         \
    if (!ACCon())                                                              \
      return false;                                                            \
  } while (0)

bool engineOn(void) { // get IGN state
  if (data.serviceMode)
    return false;
  if (!data.engineSensEnabled)
    return true;
  float voltage =
      SENS_12V.getA() * SENS12_VOLTAGE / 4095 + data.voltageCorrection;
  if (engineState) {
    if (voltage < data.engineSensVoltage - 0.5) {
      if (TICKS() - engineTimer > 2000) {
        debug.println("IGN: engine off!");
        engineState = false;
        engineTimer = TICKS();
      }
    } else {
      engineTimer = TICKS();
    }
  } else {
    if (voltage >= data.engineSensVoltage) {
      if (TICKS() - engineTimer > 2000) {
        debug.println("IGN: engine on!");
        engineState = true;
        engineTimer = TICKS();
      }
    } else {
      engineTimer = TICKS();
    }
  }
  return engineState;
}

//#define debug Serial
volatile uint32_t sensorReadCounter = 0;
__attribute__((optimize(3))) void read_sens(void *arg) {
  debug.println("Started read_sens");
  TickType_t xLastWakeTime = xTaskGetTickCount();
  while (1) {
    vTaskDelayUntil(&xLastWakeTime, 20);
    sensorReadCounter++;
    if (!(sensorReadCounter & 0xFF))
      freeStack.sens = uxTaskGetStackHighWaterMark(NULL);
  }
}

volatile bool lcdInited = false;
/*******************************
 * include supervisor routines *
 *******************************/
#include "supervisor.h"

void CCMTask(void *arg) {
  UNUSED(arg);
  {
    uint32_t timer = TICKS();
    /*while (sensorReadCounter < 10) {
      YIELD();
    }*/
    // comp.setFlag(MF_CC_ENABLED);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (!svInfo.restartCCM) {
      uint32_t curPSI = comp.getPressure();
      if (TICKS() - timer > 5000) {
        resetTimer(timer);
        debug.printf("CCM // COMP: %luPSI COMP T: % .4fC, LOCK % d\n ", curPSI,
                     comp.getTempF(), comp.state);
        freeStack.CCM = uxTaskGetStackHighWaterMark(NULL);
      }
      comp.processBus(CCM);
      vTaskDelayUntil(&xLastWakeTime, 10);
      if (comp.checkFlag(MF_CC_ENABLED) &&
          !comp.isPaused()) { // if automatics enabled
        if (curPSI <= comp.settings.onPressure) {
          if (/*!comp.isPumpLocked() &&*/ !comp.checkFlag(MF_CC_PUMP_ON) &&
              !comp.checkFlag(MF_CC_DRYER_REQUIRED)) {
            comp.startCompressor();
          }
        }
        YIELD();
        if (curPSI >= comp.settings.offPressure) {
          if (comp.checkFlag(MF_CC_PUMP_ON)) {
            comp.stopCompressor();
          }
        }
        YIELD();
        if (comp.checkFlag(MF_CC_DRYER_REQUIRED) && comp.settings.dryerEnable) {
          if (!comp.isDryerLocked() && !comp.checkFlag(MF_CC_DRY_ON) &&
              !comp.checkFlag(MF_CC_PUMP_ON)) {
            comp.startDryer(comp.settings.dryerTime);
          }
        }
      }
      YIELD();
      // DELAY(1);
    }
  }
  while (1)
    ;
}
void setupCCM() {
  svInfo.svComp->readSettings(); // get current CCM settings
  svInfo.svComp->settings.maxPSI = data.compMaxPSI;
  svInfo.svComp->settings.dryerEnable = data.dryer != MF_DRYER_NO; // first bit
  svInfo.svComp->settings.dryerTime = data.dryerExhaustTime * 1000;
  svInfo.svComp->settings.onPressure = data.compOnPressure;
  svInfo.svComp->settings.offPressure = data.compOffPressure;
  svInfo.svComp->settings.tempControl = (data.compTempSensor > 0);
  svInfo.svComp->settings.overheatOnTemp = data.compTempSensor - 10;
  svInfo.svComp->settings.overheatOffTemp = data.compTempSensor;
  svInfo.svComp->settings.overheatWarnTemp = data.compTempSensor - 5;
  svInfo.svComp->settings.maxCompTime =
      data.compTimeLimit ? data.compTimeLimit * 60000 : 10000000;
  svInfo.svComp->settings.externalValve =
      ((data.dryer == MF_DRYER_COMP) && data.receiverPresent &&
       !(data.valves & 1U));
  debug.printf("COMP: DRY type: %d, RECV: %d\n", data.dryer,
               data.receiverPresent);
  if (svInfo.svComp->settings.externalValve)
    debug.println("COMP: ExtValve enabled!");
  svInfo.svComp->setup();
  if (data.receiverPresent && engineOn())
    svInfo.svComp->enable();
}
void restartCCM(bool &CCMstate) {
  if (!svInfo.svComp)
    return;
  bool ccmState = CCMstate;
  svInfo.svComp->disable();
  setupCCM();
  if (ccmState) {
    CCMstate = svInfo.svComp->enable();
  }
};

/*#define lcdClear() \
  lcd.boxwh(border, border + 21, lcd.width - border, lcd.height - 22 - border, \
            LCD_BLACK)
#define printV(a) printfString("%d.%02dv", int((a)), int((a)*100) % 100)*/
/********************************
 * include calibration routines *
 ********************************/
uint32_t buttonMenu = 0;

#include "calibration.h"

void lcdTask(void *arg) {
  bool nonCalibOneShot = false;
  uint8_t serviceTrigger = 0;
  uint32_t serviceTimer = TICKS();
  uint8_t serviceRelease = 0;
  uint32_t buttonMenuTimer = TICKS();
  uint8_t buttonMenuWait = 0;
  bool menuButtonMode = false;
  bool menuOldCalibrated = false;
  double menuOldCalibration = 0;

  uint8_t axles = 0, valves = 0;

  debug.println("lcdTask started");
  debug.printf("RAM MON entry:%lu\n", freeRAM());
  {
    int16_t up[4] = {100, 100, 100, 100};
    int16_t upper[4] = {30000, 30000, 30000, 30000};
    int16_t down[4] = {0, 0, 0, 0};
    int16_t downer[4] = {-30000, -30000, -30000, -30000};

    int16_t lastPosSpeed[4];
    uint8_t lastPosPreset = 0;

    MF_TRANSLATION TR_preset = {"P", "П"};

    bool movingUp = false;
    bool movingDown = false;
    uint32_t timer = TICKS(), lcdTimer = TICKS(), buttonTimer = TICKS();
    MFLCD_FONTDATA fontData = {0};

    mfBusButton BUSUP = mfBusButton("UP", lcdBus, MF_ALPRO_V10_LCD);
    mfBusButton BUSOK = mfBusButton("OK", lcdBus, MF_ALPRO_V10_LCD);
    mfBusButton BUSDOWN = mfBusButton("DOWN", lcdBus, MF_ALPRO_V10_LCD);
    lcdButtons = {&BUSUP, &BUSOK, &BUSDOWN};

    mfBusButton _butUP = mfBusButton("UP", buttonBus, buttonID);
    mfBusButton _butOK = mfBusButton("OK", buttonBus, buttonID);
    mfBusButton _butDOWN = mfBusButton("DOWN", buttonBus, buttonID);
    ledUP.setDevice(buttonID);
    ledOK.setDevice(buttonID);
    ledDOWN.setDevice(buttonID);
    buttonBeep.setDevice(buttonID);

    butUP = &_butUP;
    butOK = &_butOK;
    butDOWN = &_butDOWN;
    buttons = {butUP, butOK, butDOWN};
    ledUP.setColor(data.buttonUpDownColor);
    ledDOWN.setColor(data.buttonUpDownColor);
    ledOK.setColor(data.buttonOkColor);
    ledOK.setBrightness(data.buttonBrightness);
    ledUP.setBrightness(data.buttonBrightness);
    ledDOWN.setBrightness(data.buttonBrightness);

    debug.println("buttons inited");

    applyItemRules();
    debug.println("settings loaded");

    do {
      mainpipe.begin();
      debug.printf("PIPE: got %u slaves\n", mainpipe.getSlavesCount());
      DELAY(100);
    } while (!mainpipe.getSlavesCount() && (TICKS() - timer < 5000));
    mfBusCompControl CCM = mfBusCompControl("CCM", mainpipe, MF_ALPRO_V10_CCM);
    svInfo.svComp = &CCM;
    debug.println("comp inited");
    debug.printf("RAM MON after CCM obj:%lu\n", freeRAM());
    bool enabledCCM = false;
    buttonBeep
        .tone(/*{{NOTE_G5, 50}, {NOTE_D5, 50}, {NOTE_PAUSE, 50},
                 {NOTE_CS5, 50}}*/
              {{NOTE_CS5, 25}, {NOTE_PAUSE, 25}, {NOTE_D5, 25}, {NOTE_G5, 25}});
    bool lastEngineState = true;

    std::function<void(void)> setLeds = [&]() {
      if (engineOn() && !buttonMenu) {
        switch (data.lastPreset) {
        case 7:
          ledUP.off();
          ledOK.off();
          ledDOWN.off();
          break;
        case 5:
          ledUP.off();
          ledOK.blink(~0UL, 500);
          ledDOWN.off();
          break;
        case 4:
          ledUP.on();
          ledOK.off();
          ledDOWN.off();
          break;
        case 3:
          ledUP.on();
          ledOK.on();
          ledDOWN.off();
          break;
        case 2:
        case 6:
          ledUP.off();
          ledOK.on();
          ledDOWN.off();
          break;
        case 1:
          ledUP.off();
          ledOK.on();
          ledDOWN.on();
          break;
        case 0:
          ledUP.off();
          ledOK.off();
          ledDOWN.on();
          break;
        }
      };
    };
    std::function<void(int16_t)> selectPreset = [&](int16_t num) {
      data.lastPreset = num;
      setLeds();
      int16_t preset[4] = {data.presets[num][0], data.presets[num][1],
                           data.presets[num][2], data.presets[num][3]};
      setLevel(preset, 4, true);
    };

    restartCCM(enabledCCM);
    debug.println("CCM restarted");
    debug.printf("RAM after CCM:%lu\n", freeRAM());

    DELAY(1000);
    restartSV();
    DELAY(100);
    debug.printf("RAM MON after SV:%lu\n", freeRAM());
    debug.println("SV restarted");
    uint32_t compPressure = CCM.getPressure();
    int32_t compTemp = CCM.getTemp();
    uint32_t compFlags = CCM.getState();
    (void)compFlags;
    debug.printf("Enabling supervisor!\n");
    if (data.calibration) {
      if (data.upOnStart) {
        if (!data.buttonManualMode)
          if (data.lastPreset < 5) {
            selectPreset(data.lastPreset);
          } else {
            data.lastPreset = 7;
            setLevel(data.lastPosition, 4, false);
          }
        else {
          int16_t lev[4] = {data.lastPosition[0], data.lastPosition[1],
                            data.lastPosition[2], data.lastPosition[3]};
          setLevel(lev, 4, false);
        }
      } else {
        data.lastPreset = 7;
        svInfo.enableSupervisor = data.autoHeightCorrection;
      }
    }
    setLeds();

    uint32_t backLightTimer = TICKS();
    uint32_t backLightFlag = true;
    debug.printf("RAM MON inited:%lu\n", freeRAM());
    /*if (!data.calibration) {
      if (!enabledCCM && data.receiverPresent && engineOn()) {
        enabledCCM = CCM.enable();
      }
      bool supervisorState = svInfo.enableSupervisor;
      CCM.pause();
      svInfo.enableSupervisor = false;
      ledOK.breathe(~0UL, 2000, 1000);
      ledUP.on();
      ledDOWN.on();
      bool answer = menuWalkthrough(lcd, &wizardListExternal);
      DELAY(100);
      resetTimer(backLightTimer); // reset backlight timer
      setupCCM();
      CCM.resume();
      if (!answer) {
        restartSV();
        svInfo.enableSupervisor = supervisorState;
      }
      setLeds();
    }*/
    while (ACCon()) {
      if (data.backLight)
        if ((TICKS() - backLightTimer > (data.backLight * 1000)) &&
            backLightFlag) {
          // darken lcd backlight if no buttons pressed too long
          backLightFlag = false;
          lcd.setBrightness(10);
        }
      /*if (TICKS() - buttonTimer > 100)*/ {
        resetTimer(buttonTimer);
        mfButtonFlags lcdstates[lcdButtons.size()];
        for (uint8_t b = 0; b < lcdButtons.size(); b++) {
          lcdstates[b] = lcdButtons[b]->getState();
        }
        if (data.calibration)
          nonCalibOneShot = true;
        if (!nonCalibOneShot) {
          lcdstates[B_OK] = MFBUTTON_PRESSED_LONG;
        }
        for (uint8_t b = 0; b < lcdButtons.size(); b++) {
          if (lcdstates[b] != MFBUTTON_NOSIGNAL) {
            resetTimer(backLightTimer); // reset backlight timer
            if (!backLightFlag) {
              // if any button pressed/released, turn on backlight
              backLightFlag = true;
              lcd.setBrightness(255);
            }
            switch (lcdstates[b]) {
            case MFBUTTON_PRESSED: {
            } break;
            case MFBUTTON_PRESSED_LONG:
              if (b == B_OK) {
                bool supervisorState = svInfo.enableSupervisor;
                float lastLevel[4] = {
                    svInfo.curPreset.value[0], svInfo.curPreset.value[1],
                    svInfo.curPreset.value[2], svInfo.curPreset.value[3]};
                uint8_t lastPreset = data.lastPreset;
                stopUp(false);
                while (svInfo.moving) {
                  DELAY(10);
                }
                CCM.pause(); // CCM.disable();
                svInfo.enableSupervisor = false;
                ledDOWN.on();
                ledUP.on();
                ledOK.breathe(~0UL, 2000, 1000);
                bool answer;
                if (!nonCalibOneShot) {
                  answer = menuWalkthrough(lcd, &wizardListExternal);
                  nonCalibOneShot = true;
                } else
                  answer = menuWalkthrough(lcd, &settingsList);
                DELAY(100);
                resetTimer(backLightTimer); // reset backlight timer
                CCM.resume();               // restartCCM(enabledCCM);
                setupCCM();
                if (!answer) {
                  data.lastPreset = lastPreset;
                  for (uint8_t z = 0; z < 4; z++)
                    svInfo.curPreset.value[z] = lastLevel[z];
                  restartSV();
                  svInfo.enableSupervisor = supervisorState;
                }
                setLeds();
              }
              break;
            case MFBUTTON_RELEASED: {
              /*
              // use lcd buttons as preset switch, not needed
              if (b == B_DOWN) {
                if (data.lastPreset > 0)
                  selectPreset(data.lastPreset - 1);
              }
              if (b == B_UP) {
                if (data.lastPreset < 4)
                  selectPreset(data.lastPreset + 1);
              }*/
            } break;
            case MFBUTTON_RELEASED_LONG:
              break;
            default:;
            }
          }
        }
        mfButtonFlags states[buttons.size()];
        for (uint8_t b = 0; b < buttons.size(); b++) {
          states[b] = buttons[b]->getState();
        }
        if (!buttonMenu) {
          if (!serviceTrigger) {
            if (states[0] == MFBUTTON_PRESSED) {
              serviceTrigger |= 1;
              serviceTimer = TICKS();
              // debug.println("No trigger, 0 pressed");
            }
            if (states[2] == MFBUTTON_PRESSED) {
              serviceTrigger |= 2;
              serviceTimer = TICKS();
              // debug.println("No trigger, 2 pressed");
            }
          } else {
            if ((states[0] == MFBUTTON_RELEASED) ||
                (states[2] == MFBUTTON_RELEASED)) {
              serviceTrigger = 0;
              states[0] = MFBUTTON_NOSIGNAL;
              states[2] = MFBUTTON_NOSIGNAL;
            }
            if (states[0] == MFBUTTON_PRESSED) {
              serviceTrigger |= 1;
              // debug.println("With trigger, 0 pressed");
            }
            if (states[2] == MFBUTTON_PRESSED) {
              serviceTrigger |= 2;
              // debug.println("With trigger, 0 pressed");
            }
            if (serviceTrigger != 3) {
              if (TICKS() - serviceTimer > 100) {
                // debug.println("Not confirmed, restore");
                if (serviceTrigger == 1) {
                  if (states[0] != MFBUTTON_RELEASED)
                    states[0] = MFBUTTON_PRESSED;
                  else
                    states[0] = MFBUTTON_NOSIGNAL;
                }
                if (serviceTrigger == 2) {
                  if (states[2] != MFBUTTON_RELEASED)
                    states[2] = MFBUTTON_PRESSED;
                  else
                    states[2] = MFBUTTON_NOSIGNAL;
                }
                serviceTrigger = 0;
                serviceRelease = 0;
              }
            } else {
              if (serviceRelease < 4) {
                serviceRelease |= 4;
                // debug.println("Switched, waiting for release");
              } else {
                if (serviceRelease < 8 && (TICKS() - serviceTimer > 2000)) {
                  if (data.serviceMode) {
                    data.serviceMode = false;
                    buttonBeep.tone(
                        {{NOTE_C5, 50}, {NOTE_G5, 50}, {NOTE_C6, 50}});
                  } else {
                    data.serviceMode = true;
                    buttonBeep.tone(
                        {{NOTE_C6, 50}, {NOTE_G5, 50}, {NOTE_C5, 50}});
                  }
                  serviceRelease |= 8;
                }
                if ((states[0] == MFBUTTON_RELEASED) ||
                    (states[0] == MFBUTTON_RELEASED_LONG))
                  serviceRelease |= 1;
                if ((states[0] == MFBUTTON_RELEASED) ||
                    (states[0] == MFBUTTON_RELEASED_LONG))
                  serviceRelease |= 2;
                if ((serviceRelease & 7) == 7) {
                  // debug.println("Released");
                  states[0] = MFBUTTON_NOSIGNAL;
                  states[2] = MFBUTTON_NOSIGNAL;
                  serviceTrigger = 0;
                  serviceRelease = 0;
                }
              }
            }
          }
        }
        if (!serviceTrigger) {
          if ((buttonMenuWait == 1) && (TICKS() - buttonMenuTimer > 9000)) {
            buttonBeep.tone({{NOTE_C5, 300},
                             {NOTE_PAUSE, 50},
                             {NOTE_G5, 50},
                             {NOTE_PAUSE, 50},
                             {NOTE_G5, 50},
                             {NOTE_PAUSE, 50},
                             {NOTE_G5, 50}});
            buttonMenuWait |= 2;
            ledUP.breathe(~0UL, 500);
            ledDOWN.breathe(~0UL, 500, 175);
            ledOK.breathe(~0UL, 500, 350);
          }
          for (uint8_t b = 0; b < buttons.size(); b++) {
            if (states[b] != MFBUTTON_NOSIGNAL) {
              resetTimer(backLightTimer); // reset backlight timer
              if (!backLightFlag) {
                // if any button pressed/released, turn on backlight
                backLightFlag = true;
                lcd.setBrightness(255);
              }
              if (data.lastPreset == 5)
                break;
              switch (states[b]) {
              case MFBUTTON_PRESSED:
                debug.printf("[%s] pressed\n", buttons[b]->name);
                if (!engineOn())
                  break;
                if (!buttonMenu) {
                  if (b == B_OK)
                    buttonMenuWait = 0;
                  if (!data.buttonManualMode) {
                  } else {
                    if (b == B_UP) {
                      bool margin = false;
                      for (uint8_t i = 0; i < 4; i++) {
                        if (svInfo.present[i])
                          if (svInfo.level[i] >= 100)
                            margin = true;
                      }
                      setLevel(margin ? upper : up, 4, false);
                      data.lastPreset = 7;
                    }
                    if (b == B_DOWN) {
                      bool margin = false;
                      for (uint8_t i = 0; i < 4; i++) {
                        if (svInfo.present[i])
                          if (svInfo.level[i] <= 0)
                            margin = true;
                      }
                      setLevel(margin ? downer : down, 4, false);
                      data.lastPreset = 7;
                    }
                  }
                } else {
                  // button menu
                }
                break;
              case MFBUTTON_PRESSED_LONG:
                debug.printf("[%s] pressed long\n", buttons[b]->name);
                if (!engineOn())
                  break;
                if (!buttonMenu) {
                  if (b == B_OK) {
                    buttonMenuTimer = TICKS();
                    buttonMenuWait |= 1;
                    if (data.buttonManualMode) {
                      buttonBeep.tone({{NOTE_C5, 100}});
                    }
                  }
                  if (!data.buttonManualMode) {
                    if (b == B_UP)
                      selectPreset(4);
                    if (b == B_DOWN)
                      selectPreset(0);
                  } else {
                    if (b == B_OK) {
                      // void
                    }
                  }
                } else {
                  switch (buttonMenu) {
                  default:
                  case 4:
                    if (b == B_OK) {
                      ledOK.setColor(MF_RED);
                      ledOK.on(1000);
                      DELAY(1000);
                      data.speedSensCalibrated = menuOldCalibrated;
                      data.speedSensCalibration = menuOldCalibration;
                      settings.save();
                      buttonMenu = 1;
                      ledUP.setColor(MF_ORANGE);
                      ledDOWN.setColor(MF_ORANGE);
                      ledOK.setColor(MF_ORANGE);
                      ledUP.blinkfade(~0UL, 2000);
                      ledDOWN.blinkfade(~0UL, 2000);
                      ledOK.blinkfade(~0UL, 2000);
                    }
                    break;
                  case 1:
                  case 2:
                  case 21:
                  case 22:
                  case 3: // button auto/manual
                    if (b == B_OK) {
                      ledOK.setColor(MF_RED);
                      ledOK.on();
                      DELAY(1000);
                      ledUP.setColor(data.buttonUpDownColor);
                      ledDOWN.setColor(data.buttonUpDownColor);
                      ledOK.setColor(data.buttonOkColor);
                      buttonMenu = 0;
                      setLeds();
                    }
                    break;
                  }
                  // button menu
                }
                break;
              case MFBUTTON_RELEASED:
                debug.printf("[%s] released\n", buttons[b]->name);
                if (!engineOn())
                  break;
                if (!buttonMenu) {
                  if (b == B_OK)
                    buttonMenuWait = 0;
                  if (!data.buttonManualMode) {
                    if (b == B_UP)
                      selectPreset(3);
                    if (b == B_OK)
                      selectPreset(2);
                    if (b == B_DOWN)
                      selectPreset(1);
                  } else {
                    if (b == B_UP) {
                      stopUp();
                    }
                    if (b == B_DOWN) {
                      stopDown();
                    }
                    if (b == B_OK) {
                      int16_t lev[4] = {data.ridePreset[0], data.ridePreset[1],
                                        data.ridePreset[2], data.ridePreset[3]};
                      setLevel(lev, 4, false);
                      data.lastPreset = 6;
                    }
                  }
                } else {
                  // button menu
                  switch (buttonMenu) {
                  case 21:
                    if (b == B_OK) {
                      ledOK.setColor(MF_GREEN);
                      ledOK.on();
                      DELAY(1000);
                      ledOK.setColor(MF_WHITE);
                      ledOK.blinkfade(~0UL, 1000);
                      ledUP.off();
                      ledDOWN.off();
                      buttonMenu = 22;
                    }
                    if (b == B_UP) {
                      axles ^= MF_FRONT_AXLE;
                      if (!axles)
                        axles |= MF_FRONT_AXLE;
                      if (axles == MF_ALL_AXLES)
                        valves |= MF_ALL_AXLES_BIT;
                      else
                        valves &= ~MF_ALL_AXLES_BIT;
                      if (axles & MF_FRONT_AXLE)
                        ledUP.on();
                      else
                        ledUP.off();
                    }
                    if (b == B_DOWN) {
                      axles ^= MF_REAR_AXLE;
                      if (!axles)
                        axles |= MF_REAR_AXLE;
                      if (axles == MF_ALL_AXLES)
                        valves |= MF_ALL_AXLES_BIT;
                      else
                        valves &= ~MF_ALL_AXLES_BIT;
                      if (axles & MF_REAR_AXLE)
                        ledDOWN.on();
                      else
                        ledDOWN.off();
                    }
                    break;
                  case 22:
                    if (b == B_OK) {
                      ledOK.setColor(MF_GREEN);
                      ledOK.on();
                      DELAY(1000);
                      data.axles = (axleType)axles;
                      data.valves = (valveType)valves;
                      bool success, supervisorState = svInfo.enableSupervisor;
                      stopUp(false);
                      CCM.pause();
                      svInfo.enableSupervisor = false;
                      backLightFlag = true;
                      lcd.setBrightness(255);

                      autoCalibration(success);

                      resetTimer(backLightTimer); // reset backlight timer
                      debug.print("Calib from button: ");
                      if (success)
                        debug.println("Success!");
                      else
                        debug.println("Failed!");

                      CCM.resume(); // restartCCM(enabledCCM);
                      setupCCM();
                      if (!success) {
                        restartSV();
                        svInfo.enableSupervisor = supervisorState;
                      }
                      if (success) {
                        ledUP.setColor(data.buttonUpDownColor);
                        ledDOWN.setColor(data.buttonUpDownColor);
                        ledOK.setColor(data.buttonOkColor);
                        buttonMenu = 0;
                        setLeds();
                      } else {
                        ledOK.setColor(MF_WHITE);
                        ledOK.blinkfade(~0UL, 1000);
                        // ledUP.off();
                        // ledDOWN.off();
                      }
                    }
                    break;
                  case 4: // speed calib
                    if (b == B_OK) {
                      ledOK.setColor(MF_GREEN);
                      ledOK.on();
                      DELAY(1000);
                      ledUP.setColor(data.buttonUpDownColor);
                      ledDOWN.setColor(data.buttonUpDownColor);
                      ledOK.setColor(data.buttonOkColor);
                      buttonMenu = 0;
                      setLeds();
                    }
                    break;

                  case 3: // button auto/manual
                    if (b == B_OK) {
                      ledOK.setColor(MF_GREEN);
                      ledOK.on();
                      DELAY(1000);
                      ledUP.setColor(data.buttonUpDownColor);
                      ledDOWN.setColor(data.buttonUpDownColor);
                      ledOK.setColor(data.buttonOkColor);
                      buttonMenu = 0;
                      setLeds();
                      data.buttonManualMode = menuButtonMode;
                    }
                    if (b == B_UP) {
                      menuButtonMode = false;
                      ledUP.on();
                      ledDOWN.off();
                    }
                    if (b == B_DOWN) {
                      menuButtonMode = true;
                      ledUP.off();
                      ledDOWN.on();
                    }

                    break;
                  case 2: // calibration
                    if (b == B_OK) {
                      ledOK.setColor(MF_GREEN);
                      ledOK.on();
                      DELAY(1000);
                      ledOK.setColor(MF_WHITE);
                      ledOK.blinkfade(~0UL, 2000);
                      if (axles & MF_REAR_AXLE)
                        ledDOWN.on();
                      else
                        ledDOWN.off();
                      if (axles & MF_FRONT_AXLE)
                        ledUP.on();
                      else
                        ledUP.off();
                      buttonMenu = 21;
                    }
                    if (b == B_UP) {
                      valves |= 1U;
                      ledUP.on();
                      ledDOWN.off();
                    }
                    if (b == B_DOWN) {
                      valves &= ~(1UL);
                      ledUP.off();
                      ledDOWN.on();
                    }
                    break;
                  case 1: // main menu
                  default:
                    if (b == B_UP) {
                      buttonMenu = 2;
                      ledOK.setColor(MF_WHITE);
                      ledOK.blinkfade(~0UL, 1000);
                      valves = data.valves;
                      axles = data.axles;
                      if (valves & 1U) {
                        ledUP.on();
                        ledDOWN.off();
                      } else {
                        ledUP.off();
                        ledDOWN.on();
                      }
                    }
                    if (b == B_OK) {
                      buttonMenu = 3;
                      ledOK.setColor(MF_WHITE);
                      ledOK.blinkfade(~0UL, 1000);
                      menuButtonMode = data.buttonManualMode;
                      if (menuButtonMode) {
                        ledUP.off();
                        ledDOWN.on();
                      } else {
                        ledUP.on();
                        ledDOWN.off();
                      }
                    }
                    if (b == B_DOWN) {
                      buttonMenu = 4;
                      menuOldCalibrated = data.speedSensCalibrated;
                      menuOldCalibration = data.speedSensCalibration;
                      ledUP.off();
                      ledDOWN.off();
                      // ledOK.setColor(MF_BLUE);
                      // ledOK.blinkfade(~0UL, 2000);
                      bool supervisorState = svInfo.enableSupervisor;

                      stopUp(false);
                      CCM.pause();
                      svInfo.enableSupervisor = false;
                      backLightFlag = true;
                      lcd.setBrightness(255);

                      bool success = calibrateSpeed();

                      resetTimer(backLightTimer); // reset backlight timer
                      CCM.resume();               // restartCCM(enabledCCM);
                      setupCCM();
                      restartSV();
                      svInfo.enableSupervisor = supervisorState;
                      if (success) {
                        ledOK.setColor(MF_GREEN);
                      } else {
                        ledOK.setColor(MF_RED);
                      }
                      ledUP.off();
                      ledDOWN.off();
                      ledOK.on(1000);
                      DELAY(1000);
                      ledOK.setColor(MF_WHITE);
                      ledOK.blinkfade(~0UL, 2000);
                    }
                    break;
                  }
                }
                break;
              case MFBUTTON_RELEASED_LONG:
                debug.printf("[%s] released long\n", buttons[b]->name);
                if (!engineOn())
                  break;
                if (buttonMenuWait & 2) {
                  buttonMenu = 1;
                  ledUP.setColor(MF_ORANGE);
                  ledDOWN.setColor(MF_ORANGE);
                  ledOK.setColor(MF_ORANGE);
                  ledUP.blinkfade(~0UL, 2000);
                  ledDOWN.blinkfade(~0UL, 2000);
                  ledOK.blinkfade(~0UL, 2000);
                  buttonMenuWait = 0;
                  break;
                }
                if (!buttonMenu) {
                  if (b == B_OK) {
                    setLeds();
                    if (buttonMenuWait & 2) {
                      ledOK.blink(2000, 200);
                      debug.printf("Call menu...\n");
                      buttonMenuWait = 0;
                      // ledOK.blink(500, 250);
                      // DELAY(500);
                      setLeds();
                    }
                    if (buttonMenuWait & 1) {
                      if (data.buttonManualMode) {
                        debug.printf("Save preset\n");
                        for (uint8_t i = 0; i < 4; i++)
                          data.ridePreset[i] = lrint(svInfo.curPreset.value[i]);
                        data.lastPreset = 6;
                        ledOK.blink(500, 250);
                        settings.save();
                        DELAY(500);
                        setLeds();
                      } else {
                        debug.printf("Ignore\n");
                      }
                    }
                    buttonMenuWait = 0;
                  }
                  if (!data.buttonManualMode) {
                  } else {
                    if (b == B_UP) {
                      stopUp();
                    }
                    if (b == B_DOWN) {
                      stopDown();
                    }
                  }
                } else {
                  // button menu
                }
                break;
              default:;
              }
            }
          }
        }
      }
      if (engineOn() && data.speedSensEnabled) {
        if (getMeasured() * data.speedSensCalibration >=
            data.speedthreshold[1]) {
          if (data.lastPreset != 5) {
            for (uint8_t s = 0; s < 4; s++) {
              lastPosSpeed[s] = data.lastPosition[s];
            }
            lastPosPreset = data.lastPreset;
            selectPreset(5);
          }
        }
        if (getMeasured() * data.speedSensCalibration <=
            data.speedthreshold[0]) {
          if (data.lastPreset == 5) {
            CCM.resume();
            setLevel(lastPosSpeed, 4, true);
            data.lastPreset = lastPosPreset;
            setLeds();
          }
        }
      }
      if ((TICKS() - lcdTimer > 100) && ACCon()) {

        compPressure = CCM.getPressure();
        compTemp = CCM.getTemp();
        compFlags = CCM.getState();

        lcd.noRefresh();
        lcd.clear(LCD_BLACK);
        lcd.setFont(2, &fontData);
        if (svInfo.present[0]) {
          lcd.print(25, 6 + fontData.yAdvance / 2,
                    printfString("%d%%", svInfo.level[0]),
                    svInfo.locked[0] ? LCD_RED : LCD_WHITE);
          lcd.drawBar(5, 5, 16, 3, 1, svInfo.level[0]);
          if (svInfo.movingDir[0] == 2)
            lcd.image(25, 35, "image_arrowUp");
          if (svInfo.movingDir[0] == 1)
            lcd.image(25, 35, "image_arrowDown");
        } else {
          lcd.drawBar(5, 5, 16, 3, 1, -1);
        }
        if (svInfo.present[1]) {
          lcd.printR(lcd.width - 25, 6 + fontData.yAdvance / 2,
                     printfString("%d%%", svInfo.level[1]),
                     svInfo.locked[1] ? LCD_RED : LCD_WHITE);
          lcd.drawBar(lcd.width - 21, 5, 16, 3, 1, svInfo.level[1]);
          if (svInfo.movingDir[1] == 2)
            lcd.image(lcd.width - 36, 35, "image_arrowUp");
          if (svInfo.movingDir[1] == 1)
            lcd.image(lcd.width - 36, 35, "image_arrowDown");
        } else {
          lcd.drawBar(lcd.width - 21, 5, 16, 3, 1, -1);
        }
        if (svInfo.present[2]) {
          lcd.print(25, lcd.height - 18, printfString("%d%%", svInfo.level[2]),
                    svInfo.locked[2] ? LCD_RED : LCD_WHITE);

          lcd.drawBar(5, lcd.height - 59, 16, 3, 1, svInfo.level[2]);
          if (svInfo.movingDir[2] == 2)
            lcd.image(25, lcd.height - 40, "image_arrowUp");
          if (svInfo.movingDir[2] == 1)
            lcd.image(25, lcd.height - 40, "image_arrowDown");
        } else {
          lcd.drawBar(5, lcd.height - 59, 16, 3, 1, -1);
        }
        if (svInfo.present[3]) {
          lcd.printR(lcd.width - 25, lcd.height - 18,
                     printfString("%d%%", svInfo.level[3]),
                     svInfo.locked[3] ? LCD_RED : LCD_WHITE);
          lcd.drawBar(lcd.width - 21, lcd.height - 59, 16, 3, 1,
                      svInfo.level[3]);
          if (svInfo.movingDir[3] == 2)
            lcd.image(lcd.width - 35, lcd.height - 40, "image_arrowUp");
          if (svInfo.movingDir[3] == 1)
            lcd.image(lcd.width - 35, lcd.height - 40, "image_arrowDown");
        } else {
          lcd.drawBar(lcd.width - 21, lcd.height - 59, 16, 3, 1, -1);
        }

        lcd.setFont(0, &fontData);
        if (CCM.isCompressorRunning()) {
          lcd.image(100, 54, "image_compY");
        } else {
          if (CCM.isCompressorLocked()) {
            lcd.image(100, 54, "image_compR");
          } else {
            lcd.image(100, 54, "image_compG");
          }
        }
        if (data.receiverPresent)
          lcd.printC(80, 60, 80, printfString("%u psi", compPressure),
                     LCD_WHITE);
        if (data.compTempSensor > 0)
          lcd.printC(80, 60, (data.receiverPresent) ? 50 : 80,
                     printfString("%d'C", compTemp),
                     (data.compTempSensor >= compTemp) ? LCD_WHITE
                                                       : RGB(31, 15, 0));
        lcd.image(40, 54, "image_bat");
        // float voltage = SENS_ACC.getA() * SENS12_VOLTAGE / 4095;
        float voltage =
            SENS_12V.getA() * SENS12_VOLTAGE / 4095 + data.voltageCorrection;
        lcd.printC(20, 60, 80,
                   printfString("%d.%dv", int(voltage), int(voltage * 10) % 10),
                   LCD_WHITE);

        if (svInfo.present[0]) {
          lcd.print(25, 26 + fontData.yAdvance / 2,
                    /*printfString("%.2fv",
                       svInfo.volt[0])*/
                    printfString("%d.%02dv", int(svInfo.volt[0]),
                                 int(svInfo.volt[0] * 100) % 100),
                    LCD_WHITE);
        }
        if (svInfo.present[1]) {
          lcd.printR(lcd.width - 25, 26 + fontData.yAdvance / 2,
                     printfString("%d.%02dv", int(svInfo.volt[1]),
                                  int(svInfo.volt[1] * 100) % 100),
                     LCD_WHITE);
        }
        if (svInfo.present[2]) {
          lcd.print(25, lcd.height - 6,
                    printfString("%d.%02dv", int(svInfo.volt[2]),
                                 int(svInfo.volt[2] * 100) % 100),
                    LCD_WHITE);
        }
        if (svInfo.present[3]) {
          lcd.printR(lcd.width - 25, lcd.height - 6,
                     printfString("%d.%02dv", int(svInfo.volt[3]),
                                  int(svInfo.volt[3] * 100) % 100),
                     LCD_WHITE);
        }
        if (data.speedSensEnabled && data.speedSensCalibrated) {
          uint32_t speed = lrint(getMeasured() * data.speedSensCalibration);
          if (speed)
            lcd.printC(0, lcd.width,
                       (lcd.height - 80) / 2 + (fontData.yAdvance / 2),

                       printfString("%lu\nkm/h",
                                    speed), // 1-5 instead of 0-4
                       LCD_WHITE);
        }
        if (engineOn()) {
          lcd.image(70, 54, "image_frame");
          lcd.setFont(1, &fontData);
          String sPreset = String(data.lastPreset + 1);
          if (data.lastPreset == 5)
            sPreset = "S";
          if (data.lastPreset == 6)
            sPreset = "R";
          if (data.lastPreset == 7)
            sPreset = "X";
          lcd.printC(1, lcd.width, lcd.height / 2 + 1,
                     sPreset, // 1-5 instead of 0-4
                     LCD_GREEN);
        } else {
          if (data.serviceMode)
            lcd.image(70, 54, "image_service");
          else
            lcd.image(70, 54, "image_noengine");
          lcd.rect(70, 51, 91, 72, RGB(16, 0, 0));
        }

        lcd.refresh();
        resetTimer(lcdTimer);
        YIELD();
      }
      if (!buttonMenu) {

        if (svInfo.movingDown && !movingDown) {
          movingDown = true;
          ledDOWN.blinkfade(~0UL, 700, data.lastPreset < 2 ? 350 : 0);
        }
        if (!svInfo.movingDown && movingDown) {
          movingDown = false;
          if (data.lastPreset < 2)
            ledDOWN.on();
          else
            ledDOWN.off();
          if (data.buttonManualMode)
            setLeds();
          if (data.lastPreset == 5)
            CCM.pause();
        }
        if (svInfo.movingUp && !movingUp) {
          movingUp = true;
          ledUP.blinkfade(~0UL, 700, data.lastPreset > 2 ? 350 : 0);
        }
        if (!svInfo.movingUp && movingUp) {
          movingUp = false;
          if (data.lastPreset > 2 && data.lastPreset < 5)
            ledUP.on();
          else
            ledUP.off();
          if (data.buttonManualMode)
            setLeds();
          if (data.lastPreset == 5)
            CCM.pause();
        }
      }
      bool curEngineState = engineOn();
      if (!curEngineState) {
        if (lastEngineState != curEngineState) {
          ledUP.off();
          ledDOWN.off();
          if (data.serviceMode)
            ledOK.breathe(~0UL, 4000);
          else {
            if (data.buttonOkColor != MF_YELLOW)
              ledOK.setColor(MF_YELLOW);
            else
              ledOK.setColor(MF_RED);
            ledOK.blink(~0UL, 4000);
          }
          CCM.pause();
          CCM.disable();
          // debug.println("Engine switched off");
        }
      } else {
        if (lastEngineState != curEngineState) {
          CCM.resume();
          ledOK.setColor(data.buttonOkColor);
          setLeds();
          // debug.println("Engine switched on");
        }
      }
      lastEngineState = curEngineState;
      if (TICKS() - timer > 500) {
        if (!enabledCCM && data.receiverPresent && engineOn()) {
          debug.printf("Sending enable to "
                       "CCM\n");
          enabledCCM = CCM.enable();
          debug.printf("ССM enabled: %d\n", enabledCCM);
        }
        resetTimer(timer);
      }
      DELAY(1);
    }
  }
  debug.printf("RAM MON deinit:%lu\n", freeRAM());
  if (svTask != NULL) {
    svInfo.restartSV = true;
    while (svInfo.restartSV)
      DELAY(1);
    vTaskDelete(svTask);
  }
  lcdButtons.clear();
  buttons.clear();
  debug.println("Stopping supervisor");
  debug.println("Stopping monitor");
  bool *gonaEnd = (bool *)arg;
  *gonaEnd = true;
  while (1)
    ;
}

#include <usbd_cdc_if.h>
static void acmThread(void *arg) {
  UNUSED(arg);
  ACM.begin();
  while (1) {
    if (ACM.haveMessages()) {
      MF_MESSAGE msg;
      ACM.readFirstMessage(msg);
      ACM.answerMessage(&msg,
                        [](mfBus *instance, MF_MESSAGE *msg,
                           MF_MESSAGE *newmsg) -> uint8_t { return 0; });
      freeStack.ACM = uxTaskGetStackHighWaterMark(NULL);
    }
    YIELD();
  }
}
void delayMS(uint32_t millis) {
  uint32_t iterations = millis * (F_CPU / 7000);
  uint32_t i;
  for (i = 0; i < iterations; ++i) {
    __NOP();
  }
}

void setupTask(void *arg) {
  UNUSED(arg);
  // PROG.setMode(CPIN_CONTINUE);
  LEDUP.setMode(CPIN_CONTINUE);
  LEDUP.setBrightness(50);
  LEDOK.setMode(CPIN_CONTINUE);
  LEDOK.setBrightness(50);
  LEDDOWN.setMode(CPIN_CONTINUE);
  LEDDOWN.setBrightness(50);
  /*MOS1.setMode(CPIN_CONTINUE);
  MOS2.setMode(CPIN_CONTINUE);
  MOS3.setMode(CPIN_CONTINUE);
  MOS4.setMode(CPIN_CONTINUE);
  MOS5.setMode(CPIN_CONTINUE);
  MOS6.setMode(CPIN_CONTINUE);
  MOS7.setMode(CPIN_CONTINUE);
  MOS8.setMode(CPIN_CONTINUE);*/
  // B_VER.readMany();
  // DELAY(2000);
  debug.println("Board revision: " + String(B_VER.get()));
  B_VER.stop();
  if (settings.begin(SPI, SPI_CS))
    debug.println("Settings SPI Init finished...");
  CCM.begin();
  comp.processBus(CCM);
  TaskHandle_t _CCMTask = NULL;
  TaskHandle_t _lcdTask = NULL;
  bool startedOnce = false;
  while (1) {
    if (ACCon()) {
      if (startedOnce) {
        NVIC_SystemReset(); // cheating here, but have not time to make it
                            // proper way
        /* fixme */
      }
      startedOnce = true;
      debug.printf("RAM start:%lu\n", freeRAM());
      LEDDOWN.breathe(-1, 1500);
      LEDOK.breathe(-1, 1500);
      LEDOK.setPhase(300);
      LEDUP.breathe(-1, 1500);
      LEDUP.setPhase(600);
      EN12V.on();
      EN5V.on();
// ,                                                                                                            vc                                              c,z      PROG.on();
      /* init slaves */
      uint32_t timer;
      resetTimer(timer);
      do {
        buttonBus.begin();
        debug.printf("Button: got %u slaves\n", buttonBus.getSlavesCount());
        DELAY(1);
      } while (!buttonBus.getSlavesCount() && (TICKS() - timer < 5000));
      for (uint8_t i = 0; i < buttonBus.getSlavesCount(); i++) {
        MFBUS_SLAVE_T slave = buttonBus.getSlave(i);
        if (slave.device == MF_ALPRO_V10_BUTTON)
          buttonID = MF_ALPRO_V10_BUTTON;
        if (slave.device == MF_ALPRO_V10_BUTTON_V2)
          buttonID = MF_ALPRO_V10_BUTTON_V2;
      }
      debug.printf("RAM after buses1:%lu\n", freeRAM());
      resetTimer(timer);
      CCM.begin();
      comp.processBus(CCM);
      resetTimer(timer);
      debug.printf("RAM after CCM:%lu\n", freeRAM());
      do {
        lcdBus.begin();
        debug.printf("LCD: got %u slaves\n", lcdBus.getSlavesCount());
        DELAY(100);
      } while (!lcdBus.getSlavesCount() && (TICKS() - timer < 5000));
      // lcd.setBrightness(70);
      settings.load();
      lcd.begin();
      lcd.noRefresh();
      MFLCD_FONTDATA fontData = {0};
      lcd.setFont(0, &fontData);
      lcd.clear(LCD_BLACK);
      lcd.image(16, 51, "image_logomf2");
      MF_TRANSLATION
      TR_loading = {"loading...", "загрузка..."};
      lcd.printC(0, lcd.width, 90, getTranslation(TR_loading, LANG), LCD_WHITE);
      lcd.setBrightness(255);
      lcd.refresh();
      debug.printf("RAM after LCD:%lu\n", freeRAM());
      // DELAY(2000);

      lcdInited = true;
      debug.println("Starting threads..."); // load data and apply item rules

      bool lcdTaskEnd = false;
      // if (!_CCMTask)
      debug.printf("CCM: %d\n", xTaskCreate(CCMTask, NULL, CCM_STACK_SIZE, NULL,
                                            1, &_CCMTask));
      debug.printf("RAM after CCM thread:%lu\n", freeRAM());
      debug.printf("lcd: %d\n", xTaskCreate(lcdTask, NULL, LCD_STACK_SIZE,
                                            &lcdTaskEnd, 1, &_lcdTask));
      debug.printf("RAM after MON thread:%lu\n", freeRAM());
      while (!lcdTaskEnd) {
        YIELD();
        freeStack.setup = uxTaskGetStackHighWaterMark(NULL);
        DELAY(10);
      }
      debug.printf("RAM after MON end:%lu\n", freeRAM());
      svInfo.restartCCM = true;
      DELAY(50);
      debug.println("Stopping power");
      vTaskDelete(_lcdTask);
      vTaskDelete(_CCMTask);
      debug.printf("RAM after thr killed:%lu\n", freeRAM());
      svInfo.restartCCM = false;
      for (size_t i = 0; i < cPins::instances.size(); i++) {
        cPins::instances[i]->off();
      }
      for (size_t i = 0; i < bPins::instances.size(); i++) {
        bPins::instances[i]->off();
      }
      DELAY(1000);
    } else {
      vTaskSuspendAll();
      LowPower.sleep(100);
      xTaskResumeAll();
    }
    YIELD();
  }
  vTaskDelete(NULL); // destroy setup thread
}
HardwareTimer *updateADCValuesTimer;

uint8_t adcReadCounter = 0;
void setup() {
  ACM.setName("alpro_v10");
  analogReadResolution(12); // 12 bit
  protect();
  bPins::initTimer(TIM3, 100);
  bPins::setFrequency(curValvesFreq);
  cPins::initTimer(TIM2, 60);
  MX_ADC2_Init();
  SENS_1.setDMAValue(&adcValues[0]);
  SENS_2.setDMAValue(&adcValues[1]);
  SENS_3.setDMAValue(&adcValues[2]);
  SENS_4.setDMAValue(&adcValues[3]);
  SENS_HEAT.setDMAValue(&adcValues[4]);
  SENS_ACC.setDMAValue(&adcValues[5]);
  SENS_COMP.setDMAValue(&adcValues[6]);
  B_VER.setDMAValue(&adcValues[7]);
  SENS_5V.setDMAValue(&adcValues[8]);
  SENS_12V.setDMAValue(&adcValues[9]);

  updateADCValuesTimer = new HardwareTimer(TIM7);
  updateADCValuesTimer->pause();
  // updateADCValuesTimer->setMode(1, TIMER_OUTPUT_COMPARE);
  updateADCValuesTimer->setOverflow(500, HERTZ_FORMAT);
  updateADCValuesTimer->attachInterrupt([]() {
    SENS_1.read();
    SENS_2.read();
    SENS_3.read();
    SENS_4.read();
    if (!(adcReadCounter & 0x3F)) {
      SENS_HEAT.read();
      SENS_ACC.read();
      SENS_COMP.read();
      SENS_12V.read();
      SENS_5V.read();
      B_VER.read();
      for (size_t i = 0; i < mfButton::instances.size(); i++)
        mfButton::instances[i]->read();
    }
    ++adcReadCounter;
  });
  updateADCValuesTimer->setInterruptPriority(1, 0);
  updateADCValuesTimer->resume();
  // aPins::initTimer(TIM7, 251);
  LowPower.begin();
  initMeasurement();
  debug.println("Starting scheduler...");
  // start scheduler
  xTaskCreate(acmThread, NULL, ACM_STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(setupTask, NULL, SETUP_STACK_SIZE, NULL, 1, NULL);
  // xTaskCreate(read_sens, NULL, SENS_STACK_SIZE, NULL, 1, NULL);
  vTaskStartScheduler();
  debug.println("Insufficient RAM");
  while (1)
    ;
}
/*****************************
 * include some menu routines
 *****************************/
#include "menuactions.h"

void loop() {}