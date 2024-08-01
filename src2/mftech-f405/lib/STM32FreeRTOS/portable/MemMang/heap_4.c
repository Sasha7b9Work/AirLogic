/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of http://www.FreeRTOS.org for more information.
 */
#include <errno.h>  // ENOMEM
#include <malloc.h> // mallinfo...
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "newlib.h"
#if ((__NEWLIB__ == 2) && (__NEWLIB_MINOR__ < 5)) ||                           \
    ((__NEWLIB__ == 3) && (__NEWLIB_MINOR__ > 1))
#warning                                                                       \
    "This wrapper was verified for newlib versions 2.5 - 3.1; please ensure newlib's external requirements for malloc-family are unchanged!"
#endif

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#if !defined(configUSE_NEWLIB_REENTRANT) || (configUSE_NEWLIB_REENTRANT != 1)
#warning                                                                       \
    "#define configUSE_NEWLIB_REENTRANT 1 // Required for thread-safety of newlib sprintf, dtoa, strtok, etc..."
// If you're *REALLY* sure you don't need FreeRTOS's newlib reentrancy support,
// comment out the above warning...
#endif
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if (configSUPPORT_DYNAMIC_ALLOCATION == 0)
#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE ((size_t)(xHeapStructSize << 1))

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE ((size_t)8)

/* Allocate the memory for the heap. */
#if (configAPPLICATION_ALLOCATED_HEAP == 1)
/* The application writer has already defined the array used for the RTOS
heap - probably so it can be placed in a special segment or address. */
extern uint8_t ucHeap[configTOTAL_HEAP_SIZE];
#else
static uint8_t ucHeap[configTOTAL_HEAP_SIZE];
#endif /* configAPPLICATION_ALLOCATED_HEAP */

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK {
  struct A_BLOCK_LINK *pxNextFreeBlock; /*<< The next free block in the list. */
  size_t xBlockSize;                    /*<< The size of the free block. */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert);

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvHeapInit(void);

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize =
    (sizeof(BlockLink_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) &
    ~((size_t)portBYTE_ALIGNMENT_MASK);

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

/*-----------------------------------------------------------*/

void *pvPortMalloc(size_t xWantedSize) {
  BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
  void *pvReturn = NULL;

  vTaskSuspendAll();
  {
    /* If this is the first call to malloc then the heap will require
    initialisation to setup the list of free blocks. */
    if (pxEnd == NULL) {
      prvHeapInit();
    } else {
      mtCOVERAGE_TEST_MARKER();
    }

    /* Check the requested block size is not so large that the top bit is
    set.  The top bit of the block size member of the BlockLink_t structure
    is used to determine who owns the block - the application or the
    kernel, so it must be free. */
    if ((xWantedSize & xBlockAllocatedBit) == 0) {
      /* The wanted size is increased so it can contain a BlockLink_t
      structure in addition to the requested amount of bytes. */
      if (xWantedSize > 0) {
        xWantedSize += xHeapStructSize;

        /* Ensure that blocks are always aligned to the required number
        of bytes. */
        if ((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00) {
          /* Byte alignment required. */
          xWantedSize +=
              (portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK));
          configASSERT((xWantedSize & portBYTE_ALIGNMENT_MASK) == 0);
        } else {
          mtCOVERAGE_TEST_MARKER();
        }
      } else {
        mtCOVERAGE_TEST_MARKER();
      }

      if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining)) {
        /* Traverse the list from the start	(lowest address) block until
        one	of adequate size is found. */
        pxPreviousBlock = &xStart;
        pxBlock = xStart.pxNextFreeBlock;
        while ((pxBlock->xBlockSize < xWantedSize) &&
               (pxBlock->pxNextFreeBlock != NULL)) {
          pxPreviousBlock = pxBlock;
          pxBlock = pxBlock->pxNextFreeBlock;
        }

        /* If the end marker was reached then a block of adequate size
        was	not found. */
        if (pxBlock != pxEnd) {
          /* Return the memory space pointed to - jumping over the
          BlockLink_t structure at its start. */
          pvReturn = (void *)(((uint8_t *)pxPreviousBlock->pxNextFreeBlock) +
                              xHeapStructSize);

          /* This block is being returned for use so must be taken out
          of the list of free blocks. */
          pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

          /* If the block is larger than required it can be split into
          two. */
          if ((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE) {
            /* This block is to be split into two.  Create a new
            block following the number of bytes requested. The void
            cast is used to prevent byte alignment warnings from the
            compiler. */
            pxNewBlockLink = (void *)(((uint8_t *)pxBlock) + xWantedSize);
            configASSERT((((size_t)pxNewBlockLink) & portBYTE_ALIGNMENT_MASK) ==
                         0);

            /* Calculate the sizes of two blocks split from the
            single block. */
            pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
            pxBlock->xBlockSize = xWantedSize;

            /* Insert the new block into the list of free blocks. */
            prvInsertBlockIntoFreeList(pxNewBlockLink);
          } else {
            mtCOVERAGE_TEST_MARKER();
          }

          xFreeBytesRemaining -= pxBlock->xBlockSize;

          if (xFreeBytesRemaining < xMinimumEverFreeBytesRemaining) {
            xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
          } else {
            mtCOVERAGE_TEST_MARKER();
          }

          /* The block is being returned - it is allocated and owned
          by the application and has no "next" block. */
          pxBlock->xBlockSize |= xBlockAllocatedBit;
          pxBlock->pxNextFreeBlock = NULL;
        } else {
          mtCOVERAGE_TEST_MARKER();
        }
      } else {
        mtCOVERAGE_TEST_MARKER();
      }
    } else {
      mtCOVERAGE_TEST_MARKER();
    }

    traceMALLOC(pvReturn, xWantedSize);
  }
  (void)xTaskResumeAll();

#if (configUSE_MALLOC_FAILED_HOOK == 1)
  {
    if (pvReturn == NULL) {
      extern void vApplicationMallocFailedHook(void);
      vApplicationMallocFailedHook();
    } else {
      mtCOVERAGE_TEST_MARKER();
    }
  }
#endif

  configASSERT((((size_t)pvReturn) & (size_t)portBYTE_ALIGNMENT_MASK) == 0);
  return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree(void *pv) {
  uint8_t *puc = (uint8_t *)pv;
  BlockLink_t *pxLink;

  if (pv != NULL) {
    /* The memory being freed will have an BlockLink_t structure immediately
    before it. */
    puc -= xHeapStructSize;

    /* This casting is to keep the compiler from issuing warnings. */
    pxLink = (void *)puc;

    /* Check the block is actually allocated. */
    configASSERT((pxLink->xBlockSize & xBlockAllocatedBit) != 0);
    configASSERT(pxLink->pxNextFreeBlock == NULL);

    if ((pxLink->xBlockSize & xBlockAllocatedBit) != 0) {
      if (pxLink->pxNextFreeBlock == NULL) {
        /* The block is being returned to the heap - it is no longer
        allocated. */
        pxLink->xBlockSize &= ~xBlockAllocatedBit;

        vTaskSuspendAll();
        {
          /* Add this block to the list of free blocks. */
          xFreeBytesRemaining += pxLink->xBlockSize;
          traceFREE(pv, pxLink->xBlockSize);
          prvInsertBlockIntoFreeList(((BlockLink_t *)pxLink));
        }
        (void)xTaskResumeAll();
      } else {
        mtCOVERAGE_TEST_MARKER();
      }
    } else {
      mtCOVERAGE_TEST_MARKER();
    }
  }
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize(void) { return xFreeBytesRemaining; }
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize(void) {
  return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks(void) {
  /* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

static void prvHeapInit(void) {
  BlockLink_t *pxFirstFreeBlock;
  uint8_t *pucAlignedHeap;
  size_t uxAddress;
  size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;

  /* Ensure the heap starts on a correctly aligned boundary. */
  uxAddress = (size_t)ucHeap;

  if ((uxAddress & portBYTE_ALIGNMENT_MASK) != 0) {
    uxAddress += (portBYTE_ALIGNMENT - 1);
    uxAddress &= ~((size_t)portBYTE_ALIGNMENT_MASK);
    xTotalHeapSize -= uxAddress - (size_t)ucHeap;
  }

  pucAlignedHeap = (uint8_t *)uxAddress;

  /* xStart is used to hold a pointer to the first item in the list of free
  blocks.  The void cast is used to prevent compiler warnings. */
  xStart.pxNextFreeBlock = (void *)pucAlignedHeap;
  xStart.xBlockSize = (size_t)0;

  /* pxEnd is used to mark the end of the list of free blocks and is inserted
  at the end of the heap space. */
  uxAddress = ((size_t)pucAlignedHeap) + xTotalHeapSize;
  uxAddress -= xHeapStructSize;
  uxAddress &= ~((size_t)portBYTE_ALIGNMENT_MASK);
  pxEnd = (void *)uxAddress;
  pxEnd->xBlockSize = 0;
  pxEnd->pxNextFreeBlock = NULL;

  /* To start with there is a single free block that is sized to take up the
  entire heap space, minus the space taken by pxEnd. */
  pxFirstFreeBlock = (void *)pucAlignedHeap;
  pxFirstFreeBlock->xBlockSize = uxAddress - (size_t)pxFirstFreeBlock;
  pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

  /* Only one block exists - and it covers the entire usable heap space. */
  xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
  xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

  /* Work out the position of the top bit in a size_t variable. */
  xBlockAllocatedBit = ((size_t)1)
                       << ((sizeof(size_t) * heapBITS_PER_BYTE) - 1);
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert) {
  BlockLink_t *pxIterator;
  uint8_t *puc;

  /* Iterate through the list until a block is found that has a higher address
  than the block being inserted. */
  for (pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert;
       pxIterator = pxIterator->pxNextFreeBlock) {
    /* Nothing to do here, just iterate to the right position. */
  }

  /* Do the block being inserted, and the block it is being inserted after
  make a contiguous block of memory? */
  puc = (uint8_t *)pxIterator;
  if ((puc + pxIterator->xBlockSize) == (uint8_t *)pxBlockToInsert) {
    pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
    pxBlockToInsert = pxIterator;
  } else {
    mtCOVERAGE_TEST_MARKER();
  }

  /* Do the block being inserted, and the block it is being inserted before
  make a contiguous block of memory? */
  puc = (uint8_t *)pxBlockToInsert;
  if ((puc + pxBlockToInsert->xBlockSize) ==
      (uint8_t *)pxIterator->pxNextFreeBlock) {
    if (pxIterator->pxNextFreeBlock != pxEnd) {
      /* Form one big block from the two blocks. */
      pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
      pxBlockToInsert->pxNextFreeBlock =
          pxIterator->pxNextFreeBlock->pxNextFreeBlock;
    } else {
      pxBlockToInsert->pxNextFreeBlock = pxEnd;
    }
  } else {
    pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
  }

  /* If the block being inserted plugged a gab, so was merged with the block
  before and the block after, then it's pxNextFreeBlock pointer will have
  already been set, and should not be set here as that would make it point
  to itself. */
  if (pxIterator != pxBlockToInsert) {
    pxIterator->pxNextFreeBlock = pxBlockToInsert;
  } else {
    mtCOVERAGE_TEST_MARKER();
  }
}

#define STM_VERSION         // use STM standard exported LD symbols
#define MALLOCS_INSIDE_ISRs // #define iff you have this crap code (ie
                            // unrepaired STM USB CDC)
#define ISR_STACK_MONITOR   // #define to enable ISR (MSP) stack diagnostics
#define ISR_STACK_LENGTH_BYTES                                                 \
  512 // #define bytes to reserve for ISR (MSP) stack

register char *stack_ptr asm("sp");

#ifdef STM_VERSION // Use STM CubeMX LD symbols for heap+stack area
// To avoid modifying STM LD file (and then having CubeMX trash it), use
// available STM symbols Unfortunately STM does not provide standardized markers
// for RAM suitable for heap! STM CubeMX-generated LD files provide the
// following symbols:
//    end     /* aligned first word beyond BSS */
//    _estack /* one word beyond end of "RAM" Ram type memory, for STM32F429
//    0x20030000 */
// Kludge below uses CubeMX-generated symbols instead of sane LD definitions
#define __HeapBase end
#define __HeapLimit                                                            \
  _estack // In K64F LD this is already adjusted for ISR stack space...
static int heapBytesRemaining;
// no DRN HEAP_SIZE symbol from LD... // that's (&__HeapLimit)-(&__HeapBase)
uint32_t TotalHeapSize; // publish for diagnostic routines; filled in first
                        // _sbrk call.
#else
// Note: DRN's K64F LD provided: __StackTop (byte beyond end of memory),
// __StackLimit, HEAP_SIZE, STACK_SIZE
// __HeapLimit was already adjusted to be below reserved stack area.
extern char
    HEAP_SIZE; // make sure to define this symbol in linker LD command file
static int heapBytesRemaining =
    (int)&HEAP_SIZE; // that's (&__HeapLimit)-(&__HeapBase)
#endif

#ifdef MALLOCS_INSIDE_ISRs // STM code to avoid malloc within ISR (USB CDC
                           // stack)
// We can't use vTaskSuspendAll() within an ISR.
// STM's stunningly bad coding malpractice calls malloc within ISRs (for
// example, on USB connect function USBD_CDC_Init) So, we must just
// suspend/resume interrupts, lengthening max interrupt response time,
// aarrggg...
#define DRN_ENTER_CRITICAL_SECTION(_usis)                                      \
  {                                                                            \
    _usis = taskENTER_CRITICAL_FROM_ISR();                                     \
  } // Disables interrupts (after saving prior state)
#define DRN_EXIT_CRITICAL_SECTION(_usis)                                       \
  {                                                                            \
    taskEXIT_CRITICAL_FROM_ISR(_usis);                                         \
  } // Re-enables interrupts (unless already disabled prior taskENTER_CRITICAL)
#else
#define DRN_ENTER_CRITICAL_SECTION(_usis)                                      \
  vTaskSuspendAll(); // Note: safe to use before FreeRTOS scheduler started, but
                     // not in ISR
#define DRN_EXIT_CRITICAL_SECTION(_usis)                                       \
  xTaskResumeAll(); // Note: safe to use before FreeRTOS scheduler started, but
                    // not in ISR
#endif

#ifndef NDEBUG
static int totalBytesProvidedBySBRK = 0;
#endif
extern char __HeapBase, __HeapLimit; // symbols from linker LD command file

// Use of vTaskSuspendAll() in _sbrk_r() is normally redundant, as newlib malloc
// family routines call
// __malloc_lock before calling _sbrk_r(). Note vTaskSuspendAll/xTaskResumeAll
// support nesting.

//! _sbrk_r version supporting reentrant newlib (depends upon above symbols
//! defined by linker control file).
void *_sbrk_r(struct _reent *pReent, int incr) {
#ifdef MALLOCS_INSIDE_ISRs // block interrupts during free-storage use
  UBaseType_t usis;        // saved interrupt status
#endif
  static char *currentHeapEnd = &__HeapBase;
#ifdef STM_VERSION // Use STM CubeMX LD symbols for heap
  if (TotalHeapSize == 0) {
    TotalHeapSize = heapBytesRemaining =
        (int)((&__HeapLimit) - (&__HeapBase)) - ISR_STACK_LENGTH_BYTES;
  };
#endif
  char *limit =
      (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
          ? stack_ptr
          : // Before scheduler is started, limit is stack pointer (risky!)
          &__HeapLimit -
              ISR_STACK_LENGTH_BYTES; // Once running, OK to reuse all remaining
                                      // RAM except ISR stack (MSP) stack
  DRN_ENTER_CRITICAL_SECTION(usis);
  if (currentHeapEnd + incr > limit) {
// Ooops, no more memory available...
#if (configUSE_MALLOC_FAILED_HOOK == 1)
    {
      extern void vApplicationMallocFailedHook(void);
      DRN_EXIT_CRITICAL_SECTION(usis);
      vApplicationMallocFailedHook();
    }
#elif defined(configHARD_STOP_ON_MALLOC_FAILURE)
    // If you want to alert debugger or halt...
    // WARNING: brkpt instruction may prevent watchdog operation...
    while (1) {
      __asm("bkpt #0");
    }; // Stop in GUI as if at a breakpoint (if debugging, otherwise loop
       // forever)
#else
    // Default, if you prefer to believe your application will gracefully trap
    // out-of-memory...
    pReent->_errno = ENOMEM; // newlib's thread-specific errno
    DRN_EXIT_CRITICAL_SECTION(usis);
#endif
    return (
        (char *)-1); // the malloc-family routine that called sbrk will return 0
  }
  // 'incr' of memory is available: update accounting and return it.
  char *previousHeapEnd = currentHeapEnd;
  currentHeapEnd += incr;
  heapBytesRemaining -= incr;
#ifndef NDEBUG
  totalBytesProvidedBySBRK += incr;
#endif
  DRN_EXIT_CRITICAL_SECTION(usis);
  return (char *)previousHeapEnd;
}
//! non-reentrant sbrk uses is actually reentrant by using current context
// ... because the current _reent structure is pointed to by global _impure_ptr
char *sbrk(int incr) { return _sbrk_r(_impure_ptr, incr); }
//! _sbrk is a synonym for sbrk.
char *_sbrk(int incr) { return sbrk(incr); };

#ifdef MALLOCS_INSIDE_ISRs // block interrupts during free-storage use
static UBaseType_t malLock_uxSavedInterruptStatus;
#endif
void __malloc_lock(struct _reent *r) {
#if defined(MALLOCS_INSIDE_ISRs)
  DRN_ENTER_CRITICAL_SECTION(malLock_uxSavedInterruptStatus);
#else
  bool insideAnISR = xPortIsInsideInterrupt();
  configASSERT(!insideAnISR); // Make damn sure no more mallocs inside ISRs!!
  vTaskSuspendAll();
#endif
};
void __malloc_unlock(struct _reent *r) {
#if defined(MALLOCS_INSIDE_ISRs)
  DRN_EXIT_CRITICAL_SECTION(malLock_uxSavedInterruptStatus);
#else
  (void)xTaskResumeAll();
#endif
};

// newlib also requires implementing locks for the application's environment
// memory space, accessed by newlib's setenv() and getenv() functions. As these
// are trivial functions, momentarily suspend task switching (rather than
// semaphore). Not required (and trimmed by linker) in applications not using
// environment variables. ToDo: Move __env_lock/unlock to a separate newlib
// helper file.
void __env_lock() { vTaskSuspendAll(); };
void __env_unlock() { (void)xTaskResumeAll(); };
