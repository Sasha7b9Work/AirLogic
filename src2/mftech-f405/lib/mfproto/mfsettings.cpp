#include "mfsettings.h"
#include "mfcrc.h"
#include <debug.h>
#include <functional>
#include <w25qxx.h>

//#define SETTINGS_DEBUG
#ifdef SETTINGS_DEBUG
extern DebugSerial debug;
#define DBG(a) a
#else
#define DBG(a)
#endif

DBG(void debugDump(void *data, uint32_t size) {
  const uint8_t perLine = 16;
  uint8_t *_data = (uint8_t *)data;
  uint32_t tempsize = 0;
  while (tempsize < size) {
    uint8_t _perLine = perLine;
    if (tempsize + perLine > size) {
      _perLine = size - tempsize;
    }
    for (uint8_t i = 0; i < _perLine; i++)
      debug.printf("%02X", _data[tempsize + i]);
    debug.printf("\n");
    tempsize += _perLine;
  }
})

mfSettings::mfSettings() {}
mfSettings::~mfSettings() {}
void mfSettings::initGPIOpin(GPIO_TypeDef *port, uint16_t pin) {
  /* init gpio, gona use HAL here cuz im lazy to rewrite W25q library as it
   * works with HAL calls for CS pin */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(port, &GPIO_InitStruct);
}
bool mfSettings::begin(SPIClass &SPI, uint32_t CSPin) {
  /* we need to init chip select pin first */
  GPIO_TypeDef *_spi_port = digitalPinToPort(CSPin);
  uint16_t _spi_pin = digitalPinToBitMask(CSPin);
  /* enable clock */
  set_GPIO_Port_Clock(STM_PORT(digitalPinToPinName(CSPin)));
  initGPIOpin(_spi_port, _spi_pin);
  /* start SPI */
  SPI.begin();
  SPI.beginTransaction(SPISettings(SPI_CLOCK_DIV16, MSBFIRST, SPI_MODE0));
  /* init spi flash */
  return W25qxxInit(&SPI._spi.handle, _spi_port, _spi_pin);
}

void mfSettings::load(std::function<void()> loadCallback) {
  DBG(debug.printf("LOAD: started...\n");)
  uint8_t *_data = (uint8_t *)&data;
  uint32_t crc32 = ~1U;
  // ~0UL equal to erased flash, 0UL is initial at start, ~1UL unique enuff
  // here
  for (uint16_t sec = 0; sec < sectors; sec++) { // check every copy
    DBG(debug.printf("LOAD: processing copy %u...\n", sec);)
    W25qxxReadBytes(_data, w25qxx.SectorSize * sec, sizeof(data));
    // DBG(debugDump(&data, sizeof(data));)
    crc32 = mfCRC::crc32((uint32_t *)(&_data[sizeof(data.crc32)]),
                         (uint16_t)(sizeof(data) - sizeof(data.crc32)));
    DBG(debug.printf("LOAD: copy %u crc32 = %08X, recalculated %08X...\n", sec,
                     data.crc32, crc32);)
    if (crc32 == data.crc32) // if crc is ok, exiting
      break;
  }
  if (crc32 != data.crc32) // reset in case of all copies broken
    reset();
  DBG(debug.printf("LOAD: loaded...\n");)
  loadCallback();
}

void mfSettings::save(std::function<void()> saveCallback) {
  uint8_t *_data = (uint8_t *)&data;
  DBG(debug.printf("SAVE: started...\n");)
  data.crc32 = mfCRC::crc32((uint32_t *)(&_data[sizeof(data.crc32)]),
                            (uint16_t)(sizeof(data) - sizeof(data.crc32)));
  DBG(debug.printf("SAVE: calculated crc32 %08X\n", data.crc32);)
  /* make few copies, one sector (4K) for each */
  for (uint16_t sec = 0; sec < sectors; sec++) {
    W25qxxEraseSector(sec); // we need to erase every page
    DBG(debug.printf("SAVE: erasing sec %u\n", sec);)
    MF_SETTINGS _data = data; // use copy as it will be overwritten
    // DBG(debugDump(&_data, sizeof(data));)
    W25qxxWriteBytes((uint8_t *)&_data, w25qxx.SectorSize * sec,
                     sizeof(data)); // write whole structure to flash
    DBG(debug.printf("Saving %u bytes to sec %u (at %08x)\n", sizeof(data), sec,
                     w25qxx.SectorSize * sec);)
  }
  DBG(debug.printf("SAVE: data written\n");)
  load(); // load will read written data back and check crc
  saveCallback();
}
void mfSettings::reset(std::function<void()> resetCallback) {
  DBG(debug.printf("RESET: started...\n");)
  MF_SETTINGS temp; // creating temporary structure with default values
  data = temp;      // copy it to main settings holder
  save();           // and save it
  resetCallback();
}
#ifdef DBG
#undef DBG
#endif
