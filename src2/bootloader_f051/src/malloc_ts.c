#include <stm32_def.h>

volatile uint32_t __mallocIrqLockState;
volatile uint32_t __mallocMutex = 0;

// define lock/unlock mechanism for thread safe malloc
void __malloc_lock(struct _reent *r) {
  UNUSED(r);
  if (!__mallocMutex++) {
    __mallocIrqLockState = __get_PRIMASK();
    __disable_irq();
  }
};
void __malloc_unlock(struct _reent *r) {
  UNUSED(r);
  if (!(--__mallocMutex)) {
    __set_PRIMASK(__mallocIrqLockState);
  }
};
