#ifndef MFPROTO_H
#define MFPROTO_H

#include <mfcrc.h>

#ifdef STM32_CORE
#include <wiring.h>
#else
#include <ctime>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#include <atomic>
#include <vector>

#define USE_VECTOR_BUFFER
#ifndef USE_VECTOR_BUFFER
#define CIRCULAR_BUFFER_INT_SAFE
#include <CircularBuffer.h>
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
inline uint32_t htonl(uint32_t a) { return a; }
//#warning LE
#else
#if BYTE_ORDER == BIG_ENDIAN
inline uint32_t htonl(uint32_t a) { return __builtin_bswap32(a); }
//#warning BE
#else
//#warning WATTAFA
inline uint32_t htonl(uint32_t a) { return a; }
#endif
#endif
#ifdef EXTERNAL_TIMER
extern uint32_t TICKS(void);
#else
#ifdef STM32_CORE
#ifdef MF_RTOS
#define TICKS() ((1000 * xTaskGetTickCount()) / configTICK_RATE_HZ)
#else
#define TICKS() HAL_GetTick()
#endif
#else
#define TICKS() (uint32_t)(std::clock() / (CLOCKS_PER_SEC / 1000))
#endif
#endif
#include "mfprotophy.h"

#ifdef MF_RTOS
#include <STM32FreeRTOS.h>
#endif

enum mfProtoErrors { MF_OK = 0, MF_MEM, MF_FULL, MF_ERR };

enum mfProtoAdd { MFPROTO_INSERT = false, MFPROTO_PUSH = true };

#define MF_MAGIC ('M' + ('F' << 8))

#ifndef MF_QUEUE_LEN
#define MF_QUEUE_LEN 10
#endif

#ifndef MFPROTO_MESSAGE_ADD_RETRIES
#define MFPROTO_MESSAGE_ADD_RETRIES 3
#endif

#ifndef MFPROTO_MAXDATA
#define MFPROTO_MAXDATA 600U
#endif

#ifndef MFPROTO_BUFSIZE
#define MFPROTO_BUFSIZE 630U
#endif

#ifndef MFPROTO_EXPIRE
#define MFPROTO_EXPIRE 50U
#endif

#ifdef F_CPU
#define _MUTEX_MILLIS_ (F_CPU / 7000)
#else
#define _MUTEX_MILLIS_ 100000U
#endif

#ifndef MFPROTO_MUTEX_WAIT
#define MFPROTO_MUTEX_WAIT 0U * _MUTEX_MILLIS_
#endif

#define __NULL__ NULL // nullptr

#ifndef __IO
#define __IO volatile
#endif
#define atomic_bool std::atomic_flag

#pragma pack(push, 1)
typedef struct {
  uint16_t magic = MF_MAGIC; // message magic char
  uint8_t from = 0;          // sender id
  uint8_t to = 0;            // receiver id
  uint16_t type = 0;         // message type
  uint16_t size = 0;         // message size
  uint32_t id = 0;           // message id
} MF_HEADER;

//#define USE_CRC8
#if defined(USE_CRC8)
#define _CRC(...) mfCRC::crc8(__VA_ARGS__)
#define crc_type uint8_t
#else
#define _CRC(...) mfCRC::crc32(__VA_ARGS__)
#define crc_type uint32_t
#endif

typedef struct {
  crc_type crc = 0; // message crc
} MF_FOOTER;
#pragma pack(pop)

#ifndef STM32_CORE
#include <mutex>
#include <thread>
#endif

#ifndef MFPROTO_MESSAGE_OWN_STORAGE
#define MFPROTO_MESSAGE_OWN_STORAGE 8
#endif

#define MF_GOTLOCK false
#define MF_WASLOCKED true
#if 1
class MF_MESSAGE {
public:
  uint8_t intdata[MFPROTO_MESSAGE_OWN_STORAGE] = {0};
  uint8_t *data = intdata; // pointer to message data
  MF_HEADER head;          // message header
  MF_FOOTER foot;          // message footer
  uint32_t time = 0;
#ifndef STM32_CORE
#ifndef _WIN32
  std::mutex mutex;
#endif
#else
  volatile bool mutex = false;
  static bool getLock(volatile bool &Lock) {
#if (__CORTEX_M == 0)
    if (Lock == true) {
      __DMB();
      return false;
    }
    Lock = true;
    __DMB();
    return Lock;
#else
    bool status = 0;
    do {
      if (__LDREXB((volatile uint8_t *)&Lock) == true) {
        __DMB();
        return false;
      }
      status = __STREXB(true, (volatile uint8_t *)&Lock); // Try to set
    } while (status != false); // retry until lock successfully
    __DMB();                   // Do not start any other memory access
    return Lock;
#endif
  }
  static bool freeLock(volatile bool &Lock) {
    __DMB();
    Lock = false;
    return Lock;
  }
#endif
  MF_MESSAGE() : time(TICKS()) {}
  MF_MESSAGE(const MF_MESSAGE &parent) : foot(parent.foot), time(parent.time) {
    head.from = parent.head.from;
    head.to = parent.head.to;
    head.type = parent.head.type;
    head.id = parent.head.id;
    allocateCopy(parent.data, parent.head.size);
  }
  MF_MESSAGE(MF_HEADER h, MF_FOOTER f, uint8_t *_data, uint16_t len)
      : foot(f), time(TICKS()) {
    head.from = h.from;
    head.to = h.to;
    head.type = h.type;
    head.id = h.id;
    allocateCopy(_data, len);
  }
  MF_MESSAGE(MF_HEADER h, MF_FOOTER f, uint8_t *_data)
      : foot(f), time(TICKS()) {
    head.from = h.from;
    head.to = h.to;
    head.type = h.type;
    head.id = h.id;
    allocateCopy(_data, h.size);
  }
  MF_MESSAGE(uint8_t from, uint8_t to, uint16_t type, uint32_t id,
             uint8_t *_data, uint16_t size)
      : time(TICKS()) {
    head.from = from;
    head.to = to;
    head.type = type;
    head.id = id;
    allocateCopy(_data, size);
  }
  MF_MESSAGE &operator=(const MF_MESSAGE &parent) {
    if (this != &parent) {
      head.from = parent.head.from;
      head.to = parent.head.to;
      head.type = parent.head.type;
      head.id = parent.head.id;
      foot = parent.foot;
      time = parent.time;
      allocateCopy(parent.data, parent.head.size);
    }
    return *this;
  }
  ~MF_MESSAGE() { dealloc(); }
  void dealloc(void) {
#ifndef STM32_CORE
#ifndef _WIN32
    mutex.lock();
#endif
#else
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    if (getLock(mutex))
#endif
    {
      if (data != intdata) {
        delete[] data;
      }
      data = intdata;
      head.size = 0;
#ifdef STM32_CORE
      freeLock(mutex);
    }
    __set_PRIMASK(state);
#else
    }
#ifndef _WIN32
    mutex.unlock();
#endif
#endif
  }
  void allocateCopy(uint8_t *_data, uint16_t len) {
    dealloc();
#ifndef STM32_CORE
#ifndef _WIN32
    mutex.lock();
#endif
#else
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    if (getLock(mutex))
#endif
    {
      data = intdata;
      head.size = 0;
      if (len) {
        head.size = len;

        if (len > MFPROTO_MESSAGE_OWN_STORAGE)
          data = new uint8_t[len];
        for (uint16_t i = 0; i < len; i++)
          data[i] = _data[i];
      }
#ifdef STM32_CORE
      freeLock(mutex);
    }
    __set_PRIMASK(state);
#else
    }
#ifndef _WIN32
    mutex.unlock();
#endif
#endif
  }
};
#else
class MF_MESSAGE {
public:
  uint8_t intdata[MFPROTO_MESSAGE_OWN_STORAGE] = {0};
  uint8_t *data = intdata;                       // pointer to message data
  std::atomic_flag allocated = ATOMIC_FLAG_INIT; // is data memory allocated
  std::atomic_flag deleted = ATOMIC_FLAG_INIT;   // is data memory allocated
  MF_HEADER head;                                // message header
  MF_FOOTER foot;                                // message footer
  uint32_t time = 0;
  MF_MESSAGE() : time(TICKS()) { deleted.test_and_set(); }
  MF_MESSAGE(const MF_MESSAGE &parent) : foot(parent.foot), time(parent.time) {
    head.from = parent.head.from;
    head.to = parent.head.to;
    head.type = parent.head.type;
    head.id = parent.head.id;
    deleted.test_and_set();
    allocateCopy(parent.data, parent.head.size);
  }
  MF_MESSAGE(MF_HEADER h, MF_FOOTER f, uint8_t *_data, uint16_t len)
      : foot(f), time(TICKS()) {
    head.from = h.from;
    head.to = h.to;
    head.type = h.type;
    head.id = h.id;
    deleted.test_and_set();
    allocateCopy(_data, len);
  }
  MF_MESSAGE(MF_HEADER h, MF_FOOTER f, uint8_t *_data)
      : foot(f), time(TICKS()) {
    head.from = h.from;
    head.to = h.to;
    head.type = h.type;
    head.id = h.id;
    deleted.test_and_set();
    allocateCopy(_data, h.size);
  }
  MF_MESSAGE(uint8_t from, uint8_t to, uint16_t type, uint32_t id,
             uint8_t *_data, uint16_t size)
      : time(TICKS()) {
    head.from = from;
    head.to = to;
    head.type = type;
    head.id = id;
    deleted.test_and_set();
    allocateCopy(_data, size);
  }
  MF_MESSAGE &operator=(const MF_MESSAGE &parent) {
    if (this != &parent) {
      head.from = parent.head.from;
      head.to = parent.head.to;
      head.type = parent.head.type;
      head.id = parent.head.id;
      foot = parent.foot;
      time = parent.time;
      allocateCopy(parent.data, parent.head.size);
    }
    return *this;
  }
  ~MF_MESSAGE() { dealloc(); }
  void dealloc(void) {
    // DD("wanna dealloc");
#ifdef STM32_CORE
    uint32_t state = __get_PRIMASK();
    __disable_irq();
#endif
    if (data != intdata) {
      if (allocated.test_and_set() == MF_WASLOCKED) {
        if (deleted.test_and_set() == MF_GOTLOCK) {
          head.size = 0;
          if (data != intdata) {
            delete[] data;
          }
          data = intdata;
          allocated.clear();
        } else {
        }
      } else {
        deleted.test_and_set();
        allocated.clear();
        head.size = 0;
        data = intdata;
      }
    } else {
      deleted.test_and_set();
      allocated.clear();
      head.size = 0;
      data = intdata;
    }
#ifdef STM32_CORE
    __set_PRIMASK(state);
#endif
  }
  void allocateCopy(uint8_t *_data, uint16_t len) {
    dealloc();
    // DD("wanna alloc");
#ifdef STM32_CORE
    uint32_t state = __get_PRIMASK();
    __disable_irq();
#endif
    if (deleted.test_and_set() == MF_GOTLOCK) {
      deleted.clear();
    } else {
      if ((_data == NULL) || (!len) || (len > MFPROTO_MAXDATA)) {
        data = intdata;
        head.size = 0;
        deleted.test_and_set();
        allocated.clear();
      } else {
        if (allocated.test_and_set() == MF_GOTLOCK) {
          if (len > MFPROTO_MESSAGE_OWN_STORAGE) {
            data = new uint8_t[len];
          } else {
            data = intdata;
          }
          for (uint16_t i = 0; i < len; i++)
            data[i] = _data[i];
          head.size = len;
          deleted.clear();
          allocated.test_and_set();
        } else {
        }
      }
    }
#ifdef STM32_CORE
    __set_PRIMASK(state);
#endif
  }
};
#endif
#define DUMPMSG(b, a)                                                          \
  do {                                                                         \
    printf("%s: f:%u, to:%u, tp:%u, id:%u sz:%u\n", (b), (a).head.from,        \
           (a).head.to, (a).head.type, (a).head.id, (a).head.size);            \
  } while (0)

#ifdef MF_RTOS
//#define USE_RTOS_MUTEXES
#endif

class mfProto {
private:
  uint32_t msgNum = 1; // message id counter
                       /* buffer for incoming data */
#ifndef USE_VECTOR_BUFFER
  CircularBuffer<uint8_t, MFPROTO_BUFSIZE> buf;
#else
  std::vector<uint8_t> buf;
#endif
public:
  atomic_bool idMutex = ATOMIC_FLAG_INIT;
  atomic_bool bufferMutex = ATOMIC_FLAG_INIT;
  atomic_bool sendMutex = ATOMIC_FLAG_INIT;
  atomic_bool messagesMutex = ATOMIC_FLAG_INIT;
  bool irqSet = false;
  static uint32_t deleteCaptured;
  static uint32_t mallocCaptured;
  std::vector<MF_MESSAGE> messages; // messages array
  mfProtoPhy &PHY;
  typedef std::function<void(mfProto *)> recvCallback_f;
  recvCallback_f recvCallback = nullptr;
#undef __inst__
#define __inst__ std::vector<mfProto *>
  inline static __inst__ instances = {};
  __inst__::iterator _me;
  char *name = (char *)nullptr; // object name, filled by MFPROTO define, can
                                // be used for debug purposes
  mfProto(const char *cname, mfProtoPhy &_phy);
  uint8_t indexById(uint32_t id); // returns MF_QUEUE_LEN if not found
  uint8_t addMessage(MF_HEADER h, MF_FOOTER f, uint8_t *data, uint16_t len,
                     mfProtoAdd push);
  uint8_t addMessage(MF_MESSAGE &message,
                     mfProtoAdd push); // MF_PUSH or MFPROTO_INSERT
  uint8_t pushMessage(MF_MESSAGE &message);
  uint8_t insertMessage(MF_MESSAGE &message);
  void readFirstMessage(MF_MESSAGE &mess);
  void readLastMessage(MF_MESSAGE &mess);
  void readMessageId(MF_MESSAGE &mess, uint32_t id);
  void readMessageIndex(MF_MESSAGE &mess, uint8_t index);
  void _readMessageIndex(MF_MESSAGE &mess, uint8_t index); // internal
  void deleteLastMessage(void);
  void deleteFirstMessage(void);
  void deleteMessageId(uint32_t id);
  void deleteMessageIndex(uint8_t index);
  void _deleteMessageIndex(uint8_t index); // internal, not protected
  void clear(void);                        // achtung, will clear all queue
  void putChar(uint8_t c);
  void processBuffer(void);
  void send(uint8_t *packet, uint16_t len, uint8_t flags = MFPHY_STARTEND);
  void poll(void);
  void sendMessage(uint8_t from, uint8_t to, uint16_t type, uint32_t id,
                   uint8_t *data, uint16_t size);
  void sendMessage(MF_MESSAGE *msg);
  void sendMessage(MF_MESSAGE *msg, uint8_t *data, uint16_t size);
  uint8_t size(void);
  bool isFull(void);
  void redefineIrqHandler(void);
  uint32_t getSpeed(void);
  void setRecvCallback(recvCallback_f callback);
  void clearRecvCallback(void);
  static void freeMutex(atomic_bool &_mutex);
  static bool getMutex(atomic_bool &_mutex,
                       uint32_t timeout = MFPROTO_MUTEX_WAIT);
  void begin(void);
  ~mfProto();
};

#define MFPROTO(a, ...) mfProto(a)(#a, __VA_ARGS__)
#endif
