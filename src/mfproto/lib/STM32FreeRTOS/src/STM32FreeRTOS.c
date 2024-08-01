/**
 * @file     STM32FreeRTOS.c
 * @brief    Based on FreeRTOS for Due and Teensy 3.0
 * @modified by Frederic Pillon <frederic.pillon@st.com> for STMicroelectronics.
 */
#include <sketch.h>
#include <STM32FreeRTOS.h>

//------------------------------------------------------------------------------
/** delay between led error flashes
 * \param[in] millis milliseconds to delay
 */
void delayMS(uint32_t millis) {
  uint32_t iterations = millis * (F_CPU / 7000);
  uint32_t i;
  for (i = 0; i < iterations; ++i) {
    __NOP();
  }
}

//------------------------------------------------------------------------------
/** Blink error pattern
 *
 * \param[in] n  number of short pulses
 */
void errorBlink(int n) {
#ifdef LED_BUILTIN
  // noInterrupts();
  // pinMode(LED_BUILTIN, OUTPUT);
  GPIO_TypeDef *port =
      get_GPIO_Port(STM_PORT(digitalPinToPinName(LED_BUILTIN)));
  uint32_t pin = STM_LL_GPIO_PIN(digitalPinToPinName(LED_BUILTIN));
  set_GPIO_Port_Clock(STM_PORT(digitalPinToPinName(LED_BUILTIN)));
  LL_GPIO_SetPinMode(port, pin, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinOutputType(port, pin, LL_GPIO_OUTPUT_OPENDRAIN);
  LL_GPIO_SetPinPull(port, pin, LL_GPIO_PULL_NO);
  LL_GPIO_SetPinSpeed(port, pin, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetOutputPin(port, pin);
  while (1) {
    int i;
    for (i = 0; i < n * 2; i++) {
      LL_GPIO_TogglePin(port, pin);
      delayMS(300);
    }
    delayMS(5000);
  }
//  HAL_NVIC_SystemReset();
#else
  while (1)
    ;
#endif // LED_BUILTIN
}

//------------------------------------------------------------------------------
/** assertBlink
 * Blink one short pulse every two seconds if configASSERT fails.
 */
void assertBlink() { errorBlink(1); }
//------------------------------------------------------------------------------
#if (configUSE_MALLOC_FAILED_HOOK == 1)
/** vApplicationMallocFailedHook()
Blink two short pulses if malloc fails.

will only be called if
configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
function that will get called if a call to pvPortMalloc() fails.
pvPortMalloc() is called internally by the kernel whenever a task, queue,
timer or semaphore is created.  It is also called by various parts of the
demo application.  If heap_1.c or heap_2.c are used, then the size of the
heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
to query the size of free heap space that remains (although it does not
provide information on how the remaining heap might be fragmented). */
void vApplicationMallocFailedHook() { errorBlink(2); }
#endif /* configUSE_MALLOC_FAILED_HOOK == 1 */

//------------------------------------------------------------------------------
#if (configUSE_IDLE_HOOK == 1)
/** vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
task.  It is essential that code added to this hook function never attempts
to block in any way (for example, call xQueueReceive() with a block time
specified, or call vTaskDelay()).  If the application makes use of the
vTaskDelete() API function (as this demo application does) then it is also
important that vApplicationIdleHook() is permitted to return to its calling
function, because it is the responsibility of the idle task to clean up
memory allocated by the kernel to any task that has since been deleted. */
void __attribute__((weak)) vApplicationIdleHook(void) {
  void loop();
  loop();
}
#endif /* configUSE_IDLE_HOOK == 1 */

/*-----------------------------------------------------------*/
#if (configCHECK_FOR_STACK_OVERFLOW >= 1)
/**  Blink three short pulses if stack overflow is detected.
Run time stack overflow checking is performed if
configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
function is called if a stack overflow is detected.
\param[in] pxTask Task handle
\param[in] pcTaskName Task name
*/
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) {
  (void)pcTaskName;
  (void)pxTask;
  errorBlink(3);
}
#endif /* configCHECK_FOR_STACK_OVERFLOW >= 1 */

//------------------------------------------------------------------------------
// catch exceptions
/** Hard fault - blink four short flash every two seconds */
void hard_fault_isr() {
  // printf("Hard fault isr\n");
  errorBlink(4);
}
/** Hard fault - blink four short flash every two seconds */
// void HardFault_Handler() { errorBlink(10); }
/* The prototype shows it is a naked function - in effect this is just an
assembly function. */

/** Bus fault - blink five short flashes every two seconds */
void bus_fault_isr() { errorBlink(5); }
/** Bus fault - blink five short flashes every two seconds */
void BusFault_Handler() { errorBlink(6); }

/** Usage fault - blink six short flashes every two seconds */
void usage_fault_isr() { errorBlink(7); }
/** Usage fault - blink six short flashes every two seconds */
void UsageFault_Handler() { errorBlink(8); }

void MemManage_Handler() { errorBlink(11); }

/*-----------------------------------------------------------*/
#if (configUSE_TICK_HOOK == 1)
/** This function will be called by each tick interrupt if
configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
added here, but the tick hook is called from an interrupt context, so
code must not attempt to block, and only the interrupt safe FreeRTOS API
functions can be used (those that end in FromISR()). */
void __attribute__((weak)) vApplicationTickHook() {}
#endif /* configUSE_TICK_HOOK == 1 */

/*-----------------------------------------------------------*/
/** Dummy time stats gathering functions need to be defined to keep the
linker happy.  Could edit FreeRTOSConfig.h to remove these.*/
void vMainConfigureTimerForRunTimeStats(void) {}
/** Dummy function
 *  \return zero
 */
unsigned long ulMainGetRunTimeCounterValue() { return 0UL; }
