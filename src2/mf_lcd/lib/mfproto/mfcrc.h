#ifndef MF_CRC_H
#define MF_CRC_H

#include <inttypes.h>

#ifdef STM32_CORE
#ifndef NOHW_CRC32
#define USE_HW_CRC
#endif
#endif

class mfCRC {
private:
#if !defined(USE_HW_CRC)
#define MFCRC_CRC32_POLY 0x04C11DB7
  inline static uint32_t crc_table[256] = {0};
#endif
  inline static bool crc32Inited = false;
  static void crc32Init(void);

public:
  mfCRC() {}
  ~mfCRC() {}
  static uint32_t crc32(uint32_t &init_crc, uint32_t *buf, uint16_t len);
  static uint32_t crc32(uint32_t *buf, uint16_t len);
  static uint8_t crc8(uint8_t *data, uint16_t len);
  static uint8_t crc8(uint8_t &start, uint8_t *data, uint16_t len);
};

#endif