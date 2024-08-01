#include "mfcrc.h"
#include <stdio.h>
#include <string.h>

inline void mfCRC::crc32Init(void) {
  if (crc32Inited)
    return;
  // some kind of singleton, create first dummy instance at first access
  mfCRC dummy;
#if !defined(USE_HW_CRC)
  int i, j;
  uint32_t c;
  for (i = 0; i < 256; ++i) {
    c = i << 24;
    for (j = 8; j > 0; --j)
      c = c & 0x80000000 ? (c << 1) ^ MFCRC_CRC32_POLY : (c << 1);
    mfCRC::crc_table[i] = c;
  }
#else
  __HAL_RCC_CRC_CLK_ENABLE();
  CRC->CR = 1;
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
#endif
  crc32Inited = true;
}

#if !defined(USE_HW_CRC)
uint32_t mfCRC::crc32(uint32_t &init_crc, uint32_t *buf, uint16_t len) {
  crc32Init();
  uint32_t v = 0xFFFFFFFF;
  uint32_t crc = init_crc;
  uint32_t *p = (uint32_t *)buf;
  while (len >= 4) {
    v = (*p++);
    crc = (crc << 8) ^ crc_table[0xFF & ((crc >> 24) ^ (v >> 24))];
    crc = (crc << 8) ^ crc_table[0xFF & ((crc >> 24) ^ (v >> 16))];
    crc = (crc << 8) ^ crc_table[0xFF & ((crc >> 24) ^ (v >> 8))];
    crc = (crc << 8) ^ crc_table[0xFF & ((crc >> 24) ^ (v))];
    len -= 4;
  }
  if (len) {
    uint32_t tempVal = 0;
    memcpy(&tempVal, p, len);
    crc = (crc << 8) ^ crc_table[0xFF & ((crc >> 24) ^ (tempVal >> 24))];
    crc = (crc << 8) ^ crc_table[0xFF & ((crc >> 24) ^ (tempVal >> 16))];
    crc = (crc << 8) ^ crc_table[0xFF & ((crc >> 24) ^ (tempVal >> 8))];
    crc = (crc << 8) ^ crc_table[0xFF & ((crc >> 24) ^ (tempVal))];
  }
  return crc;
}
#else
#include <stm32yyxx_ll_crc.h>
uint32_t mfCRC::crc32(uint32_t &init_crc, uint32_t *buf, uint16_t len) {
  crc32Init();
  /*#ifdef MF_RTOS
    vTaskSuspendAll();
  #endif*/
  uint32_t *p;
  p = (uint32_t *)buf;
  if (init_crc == 0xFFFFFFFF)
    LL_CRC_ResetCRCCalculationUnit(CRC);
  while (len >= 4) {
    LL_CRC_FeedData32(CRC, (*p++));
    len -= 4;
  }
  if (len) {
    uint32_t tempVal = 0;
    memcpy(&tempVal, p, len);
    LL_CRC_FeedData32(CRC, tempVal);
  }
  return LL_CRC_ReadData32(CRC);
}
#endif

uint32_t mfCRC::crc32(uint32_t *buf, uint16_t len) {
  uint32_t reset = 0xFFFFFFFF;
  return mfCRC::crc32(reset, buf, len);
}
uint8_t mfCRC::crc8(uint8_t &start, uint8_t *data, uint16_t len) {
  uint8_t crc = start;
  while (len--) {
    uint8_t extract = *data++;
    uint8_t i = extract ^ crc;
    crc = 0x00U;
    if (i & 0x01U)
      crc ^= 0x5eU;
    if (i & 0x02U)
      crc ^= 0xbcU;
    if (i & 0x04U)
      crc ^= 0x61U;
    if (i & 0x08U)
      crc ^= 0xc2U;
    if (i & 0x10U)
      crc ^= 0x9dU;
    if (i & 0x20U)
      crc ^= 0x23U;
    if (i & 0x40U)
      crc ^= 0x46U;
    if (i & 0x80U)
      crc ^= 0x8cU;
  }
  return crc;
}
uint8_t mfCRC::crc8(uint8_t *data, uint16_t len) {
  uint8_t reset = 0U;
  return mfCRC::crc8(reset, data, len);
}
