#ifndef MF_VALVE_H
#define MF_VALVE_H
#include "cPins.h"

class mfValve {
private:
  cPins *valve = nullptr;
  volatile bool locked = false;
  volatile uint32_t lockTime = 0;
  volatile uint32_t lockMoment = HAL_GetTick();
  std::function<void()> lockCallback = []() {};
  void tickCallback(cPins *pin);

  /* */
public:
#undef __inst__
#define __inst__ std::vector<mfValve *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;
  char *name = (char *)nullptr; // pin name, filled by define, can be
                                // used for debug purposes
  mfValve(const char *cname, cPins *_PIN);
  ~mfValve();
  void setDuty(uint16_t duty);
  void on(
      uint32_t time = ~0U,
      std::function<void(cPins *pin)> callback = [](cPins *) {});
  void off(void);
  bool isActive(void);
  void lock(
      uint32_t time = ~0UL, std::function<void()> callback = []() {});
  void unlock(void);
  bool isLocked(void);
  /* */
};

#define MFVALVE(a, ...) mfValve(a)(#a, __VA_ARGS__)
#endif