#include "bootloader.h"
#include "stm32_def.h"
#include "stm32yyxx_ll_system.h"
#include "stm32yyxx_ll_utils.h"
#include "usbd_if.h"

/* enable GPIO clock for given port */
void _enableGPIOClock(GPIO_TypeDef *_port) {
  if (GPIOA == _port)
    __HAL_RCC_GPIOA_CLK_ENABLE();
  if (GPIOB == _port)
    __HAL_RCC_GPIOB_CLK_ENABLE();
#if defined(GPIOC)
  if (GPIOC == _port)
    __HAL_RCC_GPIOC_CLK_ENABLE();
#endif
#if defined(GPIOD)
  if (GPIOD == _port)
    __HAL_RCC_GPIOD_CLK_ENABLE();
#endif
#if defined(GPIOE)
  if (GPIOE == _port)
    __HAL_RCC_GPIOE_CLK_ENABLE();
#endif
#if defined(GPIOF)
  if (GPIOF == _port)
    __HAL_RCC_GPIOF_CLK_ENABLE();
#endif
#if defined(GPIOG)
  if (GPIOG == _port)
    __HAL_RCC_GPIOG_CLK_ENABLE();
#endif
#if defined(GPIOH)
  if (GPIOH == _port)
    __HAL_RCC_GPIOH_CLK_ENABLE();
#endif
#if defined(GPIOI)
  if (GPIOI == _port)
    __HAL_RCC_GPIOI_CLK_ENABLE();
#endif
#if defined(GPIOJ)
  if (GPIOJ == _port)
    __HAL_RCC_GPIOJ_CLK_ENABLE();
#endif
#if defined(GPIOK)
  if (GPIOK == _port)
    __HAL_RCC_GPIOK_CLK_ENABLE();
#endif
#if defined(GPIOZ)
  if (GPIOZ == _port)
    __HAL_RCC_GPIOZ_CLK_ENABLE();
#endif
}
/* we need a system bootloader to reflash our bootloader while developing, it
 * can be triggered by 1200bps rate set, porbably will remove it later */
void jumpToSysBootloader(void) {
#ifdef USBCON
  USBD_reenumerate();
#endif
  void (*sysMemBootJump)(void);
  HAL_RCC_DeInit();
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_HSI);
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  volatile uint32_t sysMem_addr = getSysMemAddr();
  if (sysMem_addr != 0) {
#ifdef __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH
    /* Remap system Flash memory at address 0x00000000 if needed */
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
#endif
    sysMemBootJump = (void (*)(void))(*((__IO uint32_t *)(sysMem_addr + 4)));
    __set_MSP(*(uint32_t *)sysMem_addr);
    sysMemBootJump();
    while (1) // this never gona happen
      ;
  }
}

/* check magic value and jump to application or stay in bootloader mode */
void jumpToBootloader(void) {
/* force bootloader button */
/* programming led to use at early start */
#define LL_LED BL_LEDPORT, BL_LEDPIN
#ifdef BL_BUTPORT1
#define LL_BUT1 BL_BUTPORT1, BL_BUTPIN1
  _enableGPIOClock(BL_BUTPORT1); // button clock
  LL_GPIO_SetPinMode(LL_BUT1, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinPull(LL_BUT1, LL_GPIO_PULL_UP);
  LL_GPIO_SetPinSpeed(LL_BUT1, LL_GPIO_SPEED_FREQ_HIGH);
#endif
#ifdef BL_BUTPORT2
#define LL_BUT2 BL_BUTPORT2, BL_BUTPIN2
  _enableGPIOClock(BL_BUTPORT2); // button clock
  LL_GPIO_SetPinMode(LL_BUT2, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinPull(LL_BUT2, LL_GPIO_PULL_UP);
  LL_GPIO_SetPinSpeed(LL_BUT2, LL_GPIO_SPEED_FREQ_HIGH);
#endif
#ifdef BL_BUTPORT3
#define LL_BUT3 BL_BUTPORT3, BL_BUTPIN3
  _enableGPIOClock(BL_BUTPORT3); // button clock
  LL_GPIO_SetPinMode(LL_BUT3, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinPull(LL_BUT3, LL_GPIO_PULL_UP);
  LL_GPIO_SetPinSpeed(LL_BUT3, LL_GPIO_SPEED_FREQ_HIGH);
#endif

  _enableGPIOClock(BL_LEDPORT); // prog led clock
  LL_GPIO_SetPinMode(LL_LED, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinOutputType(LL_LED, LL_GPIO_OUTPUT_OPENDRAIN);
  LL_GPIO_SetPinPull(LL_LED, LL_GPIO_PULL_NO);
  LL_GPIO_SetPinSpeed(LL_LED, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetOutputPin(LL_LED);

  /* get the magic number from backup reg */
  enableBackupDomain();
  if (getBackupRegister(SYSBL_MAGIC_NUMBER_BKP_INDEX) ==
      SYSBL_MAGIC_NUMBER_BKP_VALUE) {
    setBackupRegister(SYSBL_MAGIC_NUMBER_BKP_INDEX, 0);
    jumpToSysBootloader(); // jump to sys bootloader
  }

  /* get the force button state */
  bool readValue = 0
#ifdef LL_BUT1
                   || LL_GPIO_IsInputPinSet(LL_BUT1)
#endif
#ifdef LL_BUT2
                   || LL_GPIO_IsInputPinSet(LL_BUT2)
#endif
#ifdef LL_BUT3
                   || LL_GPIO_IsInputPinSet(LL_BUT3)
#endif
      ;
  uint32_t *data = (uint32_t *)(FLASH_BASE + BL_OFFSET);
  if (*data == 0xFFFFFFFF)
    readValue = false;

  /* we have no magic stored and button is not pressed, start the application */
  if ((getBackupRegister(SYSBL_MAGIC_NUMBER_BKP_INDEX) == 0) && (readValue)) {
    disableBackupDomain();
    typedef void (*pFunction)(void);
    pFunction Jump_To_Application;
    uint32_t JumpAddress;
    /* deinit HAL */
    HAL_RCC_DeInit();
    HAL_DeInit();
    /* reset SysTick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
#if defined(STM32F0xx)
    /*
     * stm32f0/cortex-m0 doesn't have VTOR register, so its assume to get vector
     * table at 0x0 addr only, no way to change. But our real table is at
     * FLASH_BASE + BL_OFFSET, so we need to copy vector table form
     * program offset to SRAM and remap 0x0 addr to SRAM. Also our main firmware
     * have to protect vector table by shifting RAM start to 0xC0 at LD script.
     * This issue exits with F0 series only, not L0/G0 with cortex-m0+
     */
    __IO uint32_t *VectorTable = (uint32_t *)0x20000000;
    for (uint8_t i = 0; i < 48; i++) {
      /* copy vector table to sram */
      VectorTable[i] = *(__IO uint32_t *)((FLASH_BASE + BL_OFFSET) + (i << 2));
    }
    __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_HSI);
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SRAM(); // remap 0x0 to SRAM instead of main flash
#else
    SCB->VTOR = FLASH_BASE + BL_OFFSET; // set new vector table in case if main
                                        // app don't set it properly
#endif
    JumpAddress = *(__IO uint32_t *)(FLASH_BASE + BL_OFFSET + 4);
    Jump_To_Application = (pFunction)JumpAddress;
    __set_MSP(*(uint32_t *)(FLASH_BASE + BL_OFFSET));
    Jump_To_Application();
    while (1) // this never gona happen
      ;
  }
  /* if we reached here, bootloader gona be started */
  _blink(10, 100);
}