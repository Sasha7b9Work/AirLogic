#include "mfcomp.h"
#include "debug.h"
extern DebugSerial debug;

mfCompControl::mfCompControl(aPins *_TEMP, aPins *_PRESSURE, cPins *_PUMP,
                             cPins *_DRY)
    : temp(_TEMP), pressure(_PRESSURE), pump(_PUMP), dry(_DRY) {
  if (temp) {
    temp->disableKalman();
    /*temp->setEstimate(4096);
    temp->setNoise(4.096);
    temp->enableKalman();*/
  }
  if (pressure) {
    pressure->disableKalman();
    pressure->setEstimate(4096);
    pressure->setNoise(40.96);
    pressure->enableKalman();
  }
  if (pump) {
    pump->setMode(CPIN_ADDITIVE);
    pump->finishCallback =
        std::bind(&mfCompControl::pumpCallback, this, std::placeholders::_1);
    pump->tickCallback = std::bind(&mfCompControl::tickCompCallback, this,
                                   std::placeholders::_1);
  }
  if (dry) {
    dry->setMode(CPIN_CONTINUE);
    dry->finishCallback =
        std::bind(&mfCompControl::dryCallback, this, std::placeholders::_1);
    dry->tickCallback =
        std::bind(&mfCompControl::tickDryCallback, this, std::placeholders::_1);
  }
}
mfCompControl::~mfCompControl() {}

#define dumpFlags()                                                            \
  do {                                                                         \
    debug.printf("FLAG%lu LIM%lu HEAT%lu PUMP%lu DRY%lu DRY_REQ%lu, RAW%lu\n", \
                 flag, checkFlag(MF_CC_OVER_LIMIT), checkFlag(MF_CC_OVERHEAT), \
                 checkFlag(MF_CC_PUMP_ON), checkFlag(MF_CC_DRY_ON),            \
                 checkFlag(MF_CC_DRYER_REQUIRED), state.load());               \
  } while (0)

void mfCompControl::setFlag(MF_CC_STATE flag) {
  std::atomic<uint32_t> oldstate = state;
  state |= (1UL << flag);
  /*if (oldstate.load() != state.load()) {
    debug.printf("SET ");
    dumpFlags();
  }*/
}
void mfCompControl::clearFlag(MF_CC_STATE flag) {
  std::atomic<uint32_t> oldstate = state;
  state &= ~(1UL << flag);
  /*if (oldstate.load() != state.load()) {
    debug.printf("CLEAR ");
    dumpFlags();
  }*/
}
bool mfCompControl::checkFlag(MF_CC_STATE flag) {
  return ((state & (1UL << flag)) > 0);
}

void mfCompControl::pumpCallback(cPins *pin) {
  while (pumpMutex.test_and_set())
    ;
  if (pin->getTime() >= settings.maxCompTime + pin->getRemaining()) {
    lockPump(LOCK_LIMIT, settings.compressorLockTime,
             []() { clearFlag(MF_CC_OVER_LIMIT); });
    setFlag(MF_CC_OVER_LIMIT);
  }
  if ((settings.dryerEnable) && (pin->getTime() > 0)) {
    setFlag(MF_CC_DRYER_REQUIRED);
  }
  clearFlag(MF_CC_PUMP_ON);
  pumpMutex.clear();
}
void mfCompControl::dryCallback(cPins *pin) {
  while (dryMutex.test_and_set())
    ;
  if (isPumpLocked() && checkFlag(MF_CC_DRY_LOCK)) {
    unlockPump(LOCK_DRY);
    clearFlag(MF_CC_DRY_LOCK);
  }
  clearFlag(MF_CC_DRY_ON);
  clearFlag(MF_CC_DRYER_REQUIRED);
  dryMutex.clear();
}
void mfCompControl::tickCompCallback(cPins *pin) {
  isPumpLocked();
} // make unlock independed
void mfCompControl::tickDryCallback(cPins *pin) {
  isDryerLocked();
} // make unlock independed

float mfCompControl::getVolt(aPins &sensor) {
  float value = sensor.getFloat();
  if (value > (1UL << ADC_RESOLUTION))
    return 5.0;
  else
    return value *
           (SENS_VOLTAGE / ((1U << ADC_RESOLUTION) /* 12 bit ADC*/ - 1.0));
}
uint16_t mfCompControl::getPSI(float voltage) {
  if (voltage < 0)
    voltage = 0;
  if (voltage > MAX_SENSOR_VOLTAGE)
    voltage = MAX_SENSOR_VOLTAGE;
  float newPSI = (settings.maxPSI / MAX_SENSOR_VOLTAGE) * voltage;
  if (::fabsf(newPSI - lastPSI) > 0.5)
    lastPSI = newPSI;
  return lrint(lastPSI);
}
uint16_t mfCompControl::getPSI(aPins &sensor) {
  return getPSI(getVolt(sensor));
}
void mfCompControl::lockPump(MF_CC_LOCK_TYPE type, uint32_t time,
                             std::function<void()> callback) {
  pumpLocks[type].moment = HAL_GetTick();
  pumpLocks[type].time = time;
  pumpLocks[type].lock = true;
  pumpLocks[type].callback = callback;
}
void mfCompControl::unlockPump(MF_CC_LOCK_TYPE type) {
  if (pumpLocks[type].lock) {
    pumpLocks[type].callback();
    pumpLocks[type].callback = []() {};
  }
  pumpLocks[type].lock = false;
}
void mfCompControl::unlockPump() {
  for (int types = 0; types < LOCK_END; types++) {
    unlockPump((MF_CC_LOCK_TYPE)types);
  }
}

bool mfCompControl::isPumpLocked() {
  bool locked = false;
  for (int types = 0; types < LOCK_END; types++) {
    if ((pumpLocks[types].lock) &&
        ((HAL_GetTick() - pumpLocks[types].moment) >= pumpLocks[types].time))
      unlockPump((MF_CC_LOCK_TYPE)types);
    if (pumpLocks[types].lock)
      locked = true;
  }
  return locked;
}
void mfCompControl::lockDryer(uint32_t time, std::function<void()> callback) {
  dryLock.moment = HAL_GetTick();
  dryLock.time = time;
  dryLock.lock = true;
  dryLock.callback = callback;
}
void mfCompControl::unlockDryer() {
  if (dryLock.lock) {
    dryLock.callback();
    dryLock.callback = []() {};
  }
  dryLock.lock = false;
}
bool mfCompControl::isDryerLocked() {
  bool locked = false;
  if ((dryLock.lock) && ((HAL_GetTick() - dryLock.moment) >= dryLock.time))
    unlockPump();
  if (dryLock.lock)
    locked = true;
  return locked;
}

void mfCompControl::startCompressor(uint32_t time) {
  if (pump && !isPumpLocked()) {
    uint32_t pumpCurrentTime = pump->getTime();
    // if adding time will cause too much working time
    if (pumpCurrentTime + time > settings.maxCompTime)
      time = settings.maxCompTime -
             pumpCurrentTime; // decrease time to not overheat
    while (pumpMutex.test_and_set())
      ;
    setFlag(MF_CC_PUMP_ON);
    pumpMutex.clear();
    pump->on(time);
  }
}
void mfCompControl::startCompressor() { startCompressor(settings.maxCompTime); }
void mfCompControl::stopCompressor() {
  if (pump && !isPumpLocked()) {
    while (pumpMutex.test_and_set())
      ;
    clearFlag(MF_CC_PUMP_ON);
    pumpMutex.clear();
    pump->off();
  }
}
void mfCompControl::startDryer(uint32_t time) {
  if (dry && !isDryerLocked()) {
    lockPump(LOCK_DRY, settings.dryerTime);
    while (dryMutex.test_and_set())
      ;
    setFlag(MF_CC_DRY_LOCK);
    setFlag(MF_CC_DRY_ON);
    dryMutex.clear();
    dry->on(time);
  }
}
void mfCompControl::stopDryer() {
  if (dry) {
    while (dryMutex.test_and_set())
      ;
    clearFlag(MF_CC_DRY_ON);
    dryMutex.clear();
    dry->off();
  }
}
uint16_t mfCompControl::getPressure(void) { return getPSI(*pressure); }

/* get overheat sensor temperature */
float mfCompControl::getTempF(void) {
  if (!settings.tempControl)
    return 25; // return 25C if we don't control temp
  /* thermistor parameters */
  constexpr float beta = 3950.0;      // NTC beta coef
  constexpr float nominalC = 25.0;    // nominal temp
  constexpr float nominalR = 10000.0; // nominal resistance
  /* some consts */
  constexpr float kelvin = 273.15;              // kelvin - celsius diff
  constexpr float nominalK = kelvin + nominalC; // nominal temp in K
  constexpr float vOut = 5.0;                   // voltage output to the NTC

  if (temp) {
    float value = temp->getFloat(); // read ADC value

    if ((value < 0.01) && (value > (1UL << ADC_RESOLUTION)))
      return nominalC; // sensor error

    float vAdc = (3.3 / ((1U << ADC_RESOLUTION) /* 12 bit ADC*/ - 1.0)) *
                  value; // sensor voltage
    float tempR =
        PARALLEL_R * (vOut / vAdc - 1) - SERIAL_R; // calc NTC resistance
    if (tempR < 1)
      tempR = 1;
    float tempK =
        (beta * nominalK) /
        (beta + (nominalK * log(tempR / nominalR))); // calc measured temp in K
    // float tempC = tempKalman.updateEstimate((tempK - kelvin) / 1) * 1;
    float tempC = tempK - kelvin;
    if ((tempC > 1000.0) || (tempC < -100.0))
      return nominalC; // calc error, too hot, also very low temperature can be
                       // caused by disconnected sensor
    if (settings.tempControl) {
      if (tempC >= settings.overheatWarnTemp) {
        setFlag(MF_CC_WARN_TEMP);
      } else {
        clearFlag(MF_CC_WARN_TEMP);
      }
      if (tempC >= (checkFlag(MF_CC_OVERHEAT) ? settings.overheatOnTemp
                                              : settings.overheatOffTemp)) {
        /*if (!checkFlag(MF_CC_OVERHEAT))
          debug.printf("TEMP %.4f/ LOCK\n", tempC);*/
        if ((pump->isActive()) || (checkFlag(MF_CC_PUMP_ON)))
          stopCompressor();
        lockPump(LOCK_HEAT, settings.compressorHeatLockTime,
                 []() { clearFlag(MF_CC_OVERHEAT); });
        setFlag(MF_CC_OVERHEAT);
      }
    }
    return tempC; // return in celsius
  } else {
    // in case of no sensor exists
    return nominalC; // wattafa
  }
}
int32_t mfCompControl::getTemp(void) { return lrint(getTempF()); }

uint8_t mfCompControl::busAnswerCallback(mfBus *instance, MF_MESSAGE *msg,
                                         MF_MESSAGE *newmsg) {
  uint8_t ret = 1;
  switch (msg->head.type) {
  case MFBUS_CC_ENABLE: {
    if (msg->head.size == sizeof(uint8_t)) {
      uint8_t param = *((uint8_t *)msg->data);
      if (!param)
        clearFlag(MF_CC_ENABLED);
      else
        setFlag(MF_CC_ENABLED);
    }
    uint8_t answer = checkFlag(MF_CC_ENABLED) ? 1 : 0;
    instance->sendMessage(newmsg, &answer, sizeof(answer));
  } break;
  case MFBUS_CC_SETTINGS: {
    if (msg->head.size == sizeof(MF_CC_SETTINGS)) {
      MF_CC_SETTINGS *gotSettings = (MF_CC_SETTINGS *)msg->data;
      settings = *gotSettings;
    }
    instance->sendMessage(newmsg, NULL, 0);
  } break;
  case MFBUS_CC_GET_SETTINGS: {
    instance->sendMessage(newmsg, (uint8_t *)&settings, sizeof(MF_CC_SETTINGS));
  } break;
  case MFBUS_CC_START_COMPRESSOR: {
    if (msg->head.size == sizeof(uint32_t)) {
      uint32_t time = *((uint32_t *)msg->data);
      startCompressor(time);
    }
    uint8_t flag = (uint8_t)checkFlag(MF_CC_PUMP_ON);
    instance->sendMessage(newmsg, &flag, sizeof(flag));
  } break;
  case MFBUS_CC_STOP_COMPRESSOR: {
    stopCompressor();
    uint8_t flag = (uint8_t)checkFlag(MF_CC_PUMP_ON);
    instance->sendMessage(newmsg, &flag, sizeof(flag));
  } break;
  case MFBUS_CC_IS_COMP_RUNNING: {
    uint8_t flag = (uint8_t)checkFlag(MF_CC_PUMP_ON);
    instance->sendMessage(newmsg, &flag, sizeof(flag));
  } break;
  case MFBUS_CC_START_DRYER: {
    if (msg->head.size == sizeof(uint32_t)) {
      uint32_t time = *((uint32_t *)msg->data);
      startDryer(time);
    }
    uint8_t flag = (uint8_t)checkFlag(MF_CC_DRY_ON);
    instance->sendMessage(newmsg, &flag, sizeof(flag));
  } break;
  case MFBUS_CC_STOP_DRYER: {
    stopDryer();
    uint8_t flag = (uint8_t)checkFlag(MF_CC_DRY_ON);
    instance->sendMessage(newmsg, &flag, sizeof(flag));
  } break;
  case MFBUS_CC_IS_DRYER_RUNNING: {
    uint8_t flag = (uint8_t)checkFlag(MF_CC_DRY_ON);
    instance->sendMessage(newmsg, &flag, sizeof(flag));
  } break;
  case MFBUS_CC_GET_PRESSURE: {
    uint32_t psi = getPressure();
    instance->sendMessage(newmsg, (uint8_t *)&psi, sizeof(psi));
  } break;
  case MFBUS_CC_GET_TEMP: {
    int32_t temp = getTemp();
    instance->sendMessage(newmsg, (uint8_t *)&temp, sizeof(temp));
  } break;
  case MFBUS_CC_GET_STATE: {
    uint32_t temp = state.load();
    instance->sendMessage(newmsg, (uint8_t *)&temp, sizeof(temp));
  } break;
  case MFBUS_CC_LOCK_COMP: {
    lockPump(LOCK_EXT, ~1UL); // infinite atm
    instance->sendMessage(newmsg, NULL, 0);
  } break;
  case MFBUS_CC_LOCK_DRY: { // infinite atm
    lockDryer(~1UL);
    instance->sendMessage(newmsg, NULL, 0);
  } break;
  case MFBUS_CC_UNLOCK_COMP: {
    unlockPump(LOCK_EXT);
    instance->sendMessage(newmsg, NULL, 0);
  } break;
  case MFBUS_CC_UNLOCK_DRY: {
    unlockDryer();
    instance->sendMessage(newmsg, NULL, 0);
  } break;
  case MFBUS_CC_IS_COMP_LOCKED: {
    uint8_t flag = (uint8_t)isPumpLocked();
    instance->sendMessage(newmsg, &flag, sizeof(flag));
  } break;
  case MFBUS_CC_IS_DRY_LOCKED: {
    uint8_t flag = (uint8_t)isDryerLocked();
    instance->sendMessage(newmsg, &flag, sizeof(flag));
  } break;
  default:
    ret = 0;
  }
  return ret;
}
void mfCompControl::processBus(mfBus &BUS) {
  if (BUS.haveMessages()) {
    MF_MESSAGE msg;
    BUS.readFirstMessage(msg);
    BUS.answerMessage(&msg,
                      std::bind(&mfCompControl::busAnswerCallback, this,
                                std::placeholders::_1, std::placeholders::_2,
                                std::placeholders::_3));
  }
}

mfBusCompControl::mfBusCompControl(const char *_name, mfBus &_bus,
                                   uint8_t _device)
    : bus(_bus), device(_device) {
  instances.push_back(this);
  me = instances.end();
  name = (char *)malloc(strlen(_name) + 1);
  if (name)
    strcpy(name, _name);
}

mfBusCompControl::~mfBusCompControl(void) {
  instances.erase(me);
  if (name)
    free(name);
}

bool mfBusCompControl::enable(bool status) {
  uint8_t *answer;
  bool ret = false;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_ENABLE, (uint8_t *)&status,
                                    sizeof(status));
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(uint8_t))) {
      answer = (uint8_t *)msg.data;
      ret = status && (*answer);
    }
  }
  return ret;
}
void mfBusCompControl::setup() {
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_SETTINGS,
                                    (uint8_t *)&settings, sizeof(settings));
  if (id)
    bus.deleteMessageId(id);
}
void mfBusCompControl::setup(MF_CC_SETTINGS _settings) {
  settings = _settings;
  setup();
}
void mfBusCompControl::readSettings(void) {
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_GET_SETTINGS, NULL, 0);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(MF_CC_SETTINGS))) {
      settings = *((MF_CC_SETTINGS *)msg.data);
    }
  }
}
bool mfBusCompControl::startCompressor(uint32_t time) {
  uint8_t *answer;
  bool ret = false;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_START_COMPRESSOR,
                                    (uint8_t *)&time, sizeof(time));
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(uint8_t))) {
      answer = (uint8_t *)msg.data;
      ret = (*answer != 0);
    }
  }
  return ret;
}
bool mfBusCompControl::stopCompressor() {
  uint8_t *answer;
  bool ret = false;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_STOP_COMPRESSOR, NULL, 0);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(uint8_t))) {
      answer = (uint8_t *)msg.data;
      ret = (*answer == 0);
    }
  }
  return ret;
}
bool mfBusCompControl::isCompressorRunning() {
  uint8_t *answer;
  bool ret = false;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_IS_COMP_RUNNING, NULL, 0);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(uint8_t))) {
      answer = (uint8_t *)msg.data;
      ret = (*answer != 0);
    }
  }
  return ret;
}
bool mfBusCompControl::startDryer(uint32_t time) {
  uint8_t *answer;
  bool ret = false;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_START_DRYER,
                                    (uint8_t *)&time, sizeof(time));
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(uint8_t))) {
      answer = (uint8_t *)msg.data;
      ret = (*answer != 0);
    }
  }
  return ret;
}
bool mfBusCompControl::stopDryer() {
  uint8_t *answer;
  bool ret = false;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_STOP_DRYER, NULL, 0);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(uint8_t))) {
      answer = (uint8_t *)msg.data;
      ret = (*answer == 0);
    }
  }
  return ret;
}
bool mfBusCompControl::isDryerRunning() {
  uint8_t *answer;
  bool ret = false;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_IS_DRYER_RUNNING, NULL, 0);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(uint8_t))) {
      answer = (uint8_t *)msg.data;
      ret = (*answer != 0);
    }
  }
  return ret;
}
uint32_t mfBusCompControl::getPressure() {
  uint32_t *answer;
  uint32_t ret = -1;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_GET_PRESSURE, NULL, 0);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(uint32_t))) {
      answer = (uint32_t *)msg.data;
      ret = *answer;
    }
  }
  return ret;
}
int32_t mfBusCompControl::getTemp() {
  int32_t *answer;
  int32_t ret = 10000;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_GET_TEMP, NULL, 0);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(int32_t))) {
      answer = (int32_t *)msg.data;
      ret = *answer;
    }
  }
  return ret;
}
uint32_t mfBusCompControl::getState() {
  uint32_t *answer;
  uint32_t ret = 0;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_GET_STATE, NULL, 0);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(uint32_t))) {
      answer = (uint32_t *)msg.data;
      ret = *answer;
      lastState = ret;
    }
  }
  return ret;
}
bool mfBusCompControl::checkFlag(MF_CC_STATE flag) {
  return ((lastState & (1UL << flag)) > 0);
}

void mfBusCompControl::lockCompressor() {
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_LOCK_COMP, NULL, 0);
  if (id)
    bus.deleteMessageId(id);
}
void mfBusCompControl::lockDryer() {
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_LOCK_DRY, NULL, 0);
  if (id)
    bus.deleteMessageId(id);
}
void mfBusCompControl::unlockCompressor() {
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_UNLOCK_COMP, NULL, 0);
  if (id)
    bus.deleteMessageId(id);
}
void mfBusCompControl::unlockDryer() {
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_UNLOCK_DRY, NULL, 0);
  if (id)
    bus.deleteMessageId(id);
}
bool mfBusCompControl::isCompressorLocked() {
  uint8_t *answer;
  bool ret = false;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_IS_COMP_LOCKED, NULL, 0);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(uint8_t))) {
      answer = (uint8_t *)msg.data;
      ret = (*answer != 0);
    }
  }
  return ret;
}
bool mfBusCompControl::isDryerLocked() {
  uint8_t *answer;
  bool ret = false;
  uint32_t id = bus.sendMessageWait(device, MFBUS_CC_IS_DRY_LOCKED, NULL, 0);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(uint8_t))) {
      answer = (uint8_t *)msg.data;
      ret = (*answer != 0);
    }
  }
  return ret;
}
