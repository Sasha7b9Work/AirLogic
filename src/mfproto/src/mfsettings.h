#ifndef MF_SETTINGS_H
#define MF_SETTINGS_H

#include "SPI.h"
#include "mfproto.h"
#include "mfsettings_data.h"
#include "w25qxx.h"

class mfSettings {
private:
  const uint16_t sectors = 2;
  static void initGPIOpin(GPIO_TypeDef *port, uint16_t pin);

public:
  MF_SETTINGS data;
  mfSettings();
  ~mfSettings();
  bool begin(SPIClass &SPI, uint32_t CSPin);
  void save(std::function<void()> saveCallback = []() {});
  void load(std::function<void()> loadCallback = []() {});
  void reset(std::function<void()> resetCallback = []() {});
};
#endif
