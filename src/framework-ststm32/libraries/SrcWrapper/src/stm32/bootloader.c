#include "bootloader.h"

#include "stm32_def.h"
#include "backup.h"
#include "stm32yyxx_ll_system.h"
#include "usbd_if.h"

/*
 * STM32 built-in bootloader in system memory support
 */
/* Private definitions to manage system memory address */
#define SYSMEM_ADDR_COMMON 0xFFF

typedef struct {
  uint32_t devID;
  uint32_t sysMemAddr;
} devSysMemAddr_str;

devSysMemAddr_str devSysMemAddr[] = {
#ifdef STM32F0xx
  {0x440, 0x1FFFEC00},
  {0x444, 0x1FFFEC00},
  {0x442, 0x1FFFD800},
  {0x445, 0x1FFFC400},
  {0x448, 0x1FFFC800},
  {0x442, 0x1FFFD800},
#elif STM32F1xx
  {0x412, 0x1FFFF000},
  {0x410, 0x1FFFF000},
  {0x414, 0x1FFFF000},
  {0x420, 0x1FFFF000},
  {0x428, 0x1FFFF000},
  {0x418, 0x1FFFB000},
  {0x430, 0x1FFFE000},
#elif STM32F2xx
  {0x411, 0x1FFF0000},
#elif STM32F3xx
  {SYSMEM_ADDR_COMMON, 0x1FFFD800},
#elif STM32F4xx
  {SYSMEM_ADDR_COMMON, 0x1FFF0000},
#elif STM32F7xx
  {SYSMEM_ADDR_COMMON, 0x1FF00000},
#elif STM32G0xx
  {SYSMEM_ADDR_COMMON, 0x1FFF0000},
#elif STM32G4xx
  {SYSMEM_ADDR_COMMON, 0x1FFF0000},
#elif STM32H7xx
  {0x450, 0x1FF00000},
#elif STM32L0xx
  {SYSMEM_ADDR_COMMON, 0x1FF00000},
#elif STM32L1xx
  {SYSMEM_ADDR_COMMON, 0x1FF00000},
#elif STM32L4xx
  {SYSMEM_ADDR_COMMON, 0x1FFF0000},
#elif STM32WBxx
  {SYSMEM_ADDR_COMMON, 0x1FFF0000},
#else
#warning "No system memory address for this serie!"
#endif
  {0x0000, 0x00000000}
};

uint32_t getSysMemAddr(void)
{
  uint32_t sysMemAddr = 0;
  if (devSysMemAddr[0].devID == SYSMEM_ADDR_COMMON) {
    sysMemAddr = devSysMemAddr[0].sysMemAddr;
  } else {
    uint32_t devId = LL_DBGMCU_GetDeviceID();
    for (uint32_t id = 0; devSysMemAddr[id].devID != 0; id++) {
      if (devSysMemAddr[id].devID == devId) {
        sysMemAddr = devSysMemAddr[id].sysMemAddr;
        break;
      }
    }
  }
  return sysMemAddr;
}

/* Request to jump to system memory boot */
WEAK void jumpToBootloaderRequested(void)
{
  enableBackupDomain();
  setBackupRegister(SYSBL_MAGIC_NUMBER_BKP_INDEX, SYSBL_MAGIC_NUMBER_BKP_VALUE);
  NVIC_SystemReset();
}

/* Jump to system memory boot from user application */
WEAK void jumpToBootloader(void)
{
  enableBackupDomain();
  if (getBackupRegister(SYSBL_MAGIC_NUMBER_BKP_INDEX) == SYSBL_MAGIC_NUMBER_BKP_VALUE) {
    setBackupRegister(SYSBL_MAGIC_NUMBER_BKP_INDEX, 0);

#ifdef USBCON
    USBD_reenumerate();
#endif
    void (*sysMemBootJump)(void);

/*    __HAL_RCC_USART1_FORCE_RESET();
    HAL_Delay(5);
    __HAL_RCC_USART1_RELEASE_RESET();
    HAL_Delay(5);*/

    HAL_RCC_DeInit();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    /**
     * Step: Disable all interrupts
     */
//    __disable_irq();
    __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_HSI);
    __HAL_RCC_SYSCFG_CLK_ENABLE();
//    __DSB();
    /**
     * Get system memory address
     *
     * Available in AN2606 document:
     * Table xxx Bootloader device-dependent parameters
     */
    volatile uint32_t sysMem_addr = getSysMemAddr();
    if (sysMem_addr != 0) {
#ifdef __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH
      /* Remap system Flash memory at address 0x00000000 */
      __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
#endif

      /**
       * Set jump memory location for system memory
       * Use address with 4 bytes offset which specifies jump location
       * where program starts
       */

//	__DSB();
//	__ISB();

      sysMemBootJump = (void (*)(void))(*((uint32_t *)(sysMem_addr + 4)));

      /**
       * Set main stack pointer.
       * This step must be done last otherwise local variables in this function
       * don't have proper value since stack pointer is located on different position
       *
       * Set direct address location which specifies stack pointer in SRAM location
       */
      __set_MSP(*(uint32_t *)sysMem_addr);

      /**
       * Jump to set location
       * This will start system memory execution
       */
      sysMemBootJump();

      while (1);
    }
  }
}

/*
 * Legacy maple bootloader support
 */
#if defined(BL_LEGACY_LEAF) || defined(BL_LEAF_DFU)
void dtr_togglingHook(uint8_t *buf, uint32_t *len)
{
  /**
   * Four byte is the magic pack "1EAF" that puts the MCU into bootloader.
   * Check if the incoming contains the string "1EAF".
   * If yes, put the MCU into the bootloader mode.
   */
  if ((*len >= 4) && (buf[0] == '1') && (buf[1] == 'E') && (buf[2] == 'A') && (buf[3] == 'F')) {
#ifndef BL_LEAF_DFU
    NVIC_SystemReset();
#else
    jumpToBootloaderRequested();
#endif
  }
}
#endif /* BL_LEGACY_LEAF */

/*
 * HID bootloader support
 */
#ifdef BL_HID
void dtr_togglingHook(uint8_t *buf, uint32_t *len)
{
  /**
   * Four byte is the magic pack "1EAF" that puts the MCU into bootloader.
   * Check if the incoming contains the string "1EAF".
   * If yes, put the MCU into the bootloader mode.
   */
  if ((*len >= 4) && (buf[0] == '1') && (buf[1] == 'E') && (buf[2] == 'A') && (buf[3] == 'F')) {
    enableBackupDomain();
    /* New HID Bootloader (ver 2.2+) */
    setBackupRegister(HID_MAGIC_NUMBER_BKP_INDEX, HID_MAGIC_NUMBER_BKP_VALUE);
#ifdef HID_OLD_MAGIC_NUMBER_BKP_INDEX
    /* Compatibility to the old HID Bootloader (ver <= 2.1) */
    setBackupRegister(HID_OLD_MAGIC_NUMBER_BKP_INDEX, HID_MAGIC_NUMBER_BKP_VALUE);
#endif
    NVIC_SystemReset();
  }
}
#endif /* BL_HID */
