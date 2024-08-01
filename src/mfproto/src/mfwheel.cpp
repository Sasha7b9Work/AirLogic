#include "mfwheel.h"

mfWheel::mfWheel(const char *cname, mfSensor *_SENSOR, mfValve *_VALVE)
    : valve(_VALVE), sensor(_SENSOR){
  name = (char *)malloc(strlen(cname) + 1);
  strcpy(name, cname);
  instances.push_back(this);
  _me = instances.end();
}
mfWheel::~mfWheel() {
  if (name)
    free(name);
  instances.erase(_me); 
}