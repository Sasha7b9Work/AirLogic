#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_
#include <stdio.h>

/* Ensure DTR_TOGGLING_SEQ enabled */
#if defined(BL_LEGACY_LEAF) || defined(BL_HID) || defined(BL_LEAF_DFU)
  #ifndef DTR_TOGGLING_SEQ
    #define DTR_TOGGLING_SEQ
  #endif /* DTR_TOGGLING_SEQ || BL_HID */
#endif /* BL_LEGACY_LEAF */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Request to jump to system memory boot */
void jumpToBootloaderRequested(void);

/* Jump to system memory boot from user application */
void jumpToBootloader(void);

#ifdef DTR_TOGGLING_SEQ
/* DTR toggling sequence management */
void dtr_togglingHook(uint8_t *buf, uint32_t *len);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _BOOTLOADER_H_ */
