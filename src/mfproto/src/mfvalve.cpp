#include "mfvalve.h"
/**/
mfValve::mfValve(const char *cname, cPins *_PIN) : valve(_PIN) {
  name = (char *)malloc(strlen(cname) + 1);
  strcpy(name, cname);
  instances.push_back(this);
  _me = instances.end();
  if (valve) {
    valve->setMode(CPIN_CONTINUE);
    valve->off();
  }
}
mfValve::~mfValve() {
  if (name)
    free(name);
  instances.erase(_me);
}

void mfValve::setDuty(uint16_t duty) {
  if (valve)
    valve->setPWM(duty);
}
void mfValve::on(uint32_t time) {
  if (valve)
    valve->on(time);
}
void mfValve::off(void) {
  if (valve)
    valve->off();
}
bool mfValve::isActive(void) {
  if (valve)
    return valve->isActive();
  return false;
}