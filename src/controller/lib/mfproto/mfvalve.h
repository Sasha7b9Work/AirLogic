#ifndef MF_VALVE_H
#define MF_VALVE_H
#include "bpins.h"

constexpr uint32_t MF_MAX_VALVE_TIME = 300 * 1000;

class mfValve {
private:
  volatile bool locked = false;
  volatile uint32_t lockTime = 0;
  volatile uint32_t lockMoment = HAL_GetTick();
  std::function<void()> lockCallback = []() {};
  void tickCallback(bPins *pin);
  volatile bool common = false;      // common state
  volatile uint16_t entries = 0;     // used to count common entries
  volatile uint16_t lockentries = 0; // used to count common entries

  /* */
public:
  bPins *valve = nullptr;
  inline static std::vector<mfValve *> instances = {};
  char name[32] = ""; // pin name, filled by define, can be
                      // used for debug purposes
  mfValve(const char *cname, bPins *_PIN);
  ~mfValve();
  void setDuty(uint16_t duty);
  void on(
      uint32_t time = MF_MAX_VALVE_TIME,
      std::function<void(bPins *pin)> callback = [](bPins *) {});
  void off(void);
  bool isActive(void);
  void lock(
      uint32_t time = MF_MAX_VALVE_TIME,
      std::function<void()> callback = []() {});
  void unlock(void);
  bool isLocked(void);
  void setCommon(void) { common = true; }
  void clearCommon(void) { common = false; };
  bool isCommon(void) { return common; }
  /* */
};

#define MFVALVE(a, ...) mfValve(a)(#a, __VA_ARGS__)
#endif