#ifndef MF_BOOTLOADER_H
#define MF_BOOTLOADER_H
#include <wiring.h>

/* apllication offset to write and jump, can be redefined */
#ifndef BL_OFFSET
#define BL_OFFSET 0xC000
#endif

/* simple blink */
#define _blink(a, b)                                                           \
  do {                                                                         \
    for (int _z = 0; _z < (a)*2; _z++) {                                       \
      LL_GPIO_TogglePin(LL_LED);                                               \
      HAL_Delay(((b) / 2));                                                    \
    }                                                                          \
  } while (0)

uint32_t getSysMemAddr(void);
void _enableGPIOClock(GPIO_TypeDef *_port);
void jumpToSysBootloader(void);
#endif