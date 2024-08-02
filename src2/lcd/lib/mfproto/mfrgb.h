#ifndef MF_RGB_H
#define MF_RGB_H

#include <cPins.h>
#include <vector>
class mfRGB {
private:
  const uint8_t size = 3;
  cPins *leds[3];
  uint8_t color[3] = {255, 255, 255};
  uint8_t bright = 100;

public:
  char name[32] = "";
  inline static std::vector<mfRGB *> instances = {};

  mfRGB(const char *_name, cPins *r, cPins *g, cPins *b) {
    instances.push_back(this);
    strncpy(name, _name, 31);
    leds[0] = r;
    leds[1] = g;
    leds[2] = b;
    for (uint8_t led = 0; led < size; led++) {
      leds[led]->setMode(CPIN_CONTINUE);
    }
    setBrightness(bright);
    setColor(color[0], color[1], color[2]);
    off();
  }
  void setBrightness(uint8_t brightness) {
    bright = brightness;
    if (bright > 100)
      bright = 100;
    for (uint8_t led = 0; led < size; led++)
      leds[led]->setPWM((bright * color[led]) / 100);
  }
  void setColor(uint8_t r, uint8_t g, uint8_t b) {
    color[0] = r;
    color[1] = g;
    color[2] = b;
    for (uint8_t led = 0; led < size; led++)
      leds[led]->setPWM((bright * color[led]) / 100);
  }
  void blink(uint32_t time, uint32_t period) {
    for (uint8_t led = 0; led < size; led++) {
      leds[led]->blink(time, period);
    }
  }
  void on(uint32_t time) {
    for (uint8_t led = 0; led < size; led++) {
      leds[led]->on(time);
    }
  }
  void breathe(uint32_t time, uint32_t period) {
    for (uint8_t led = 0; led < size; led++) {
      leds[led]->breathe(time, period);
    }
  }
  void blinkfade(uint32_t time, uint32_t period) {
    for (uint8_t led = 0; led < size; led++) {
      leds[led]->blinkfade(time, period);
    }
  }
  void off() {
    for (uint8_t led = 0; led < size; led++) {
      leds[led]->off();
    }
  }
  void setPhase(uint32_t phase) {
    for (uint8_t led = 0; led < size; led++) {
      leds[led]->setPhase(phase);
    }
  }

  ~mfRGB() {
    instances.erase(std::find(instances.begin(), instances.end(), this));
  }
  static mfRGB *findByName(const char *name) {
    for (uint16_t i = 0; i < instances.size(); i++)
      if (!strcmp(name, instances[i]->name))
        return instances[i];
    return nullptr;
  }
};

#define MFRGB(a, ...) mfRGB(a)(#a, __VA_ARGS__)

#endif