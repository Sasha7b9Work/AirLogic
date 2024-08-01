#ifndef MF_VALVE_H
#define MF_VALVE_H
#include "cPins.h"

class mfValve {
private:
  cPins *valve = nullptr;
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
  void on(uint32_t time = ~0U);
  void off(void);
  bool isActive(void);
  /* */
};

#endif