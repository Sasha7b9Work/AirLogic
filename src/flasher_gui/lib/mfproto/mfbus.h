#ifndef MFBUS_H
#define MFBUS_H

#ifdef STM32_CORE
#include <wiring.h>
#else
#include <ctime>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#ifdef LINTEST
#include <thread>
#endif
#endif

#include <vector>

#ifdef MF_RTOS
#include <STM32FreeRTOS.h>
#endif

#include "mfproto.h"
/**********
 * MF BUS *
 **********/

#define MFBL_MAGIC_NUMBER_BKP_VALUE 0xB007 // Bootloader "BOOT" signature

#ifdef MF_RTOS
#define YIELD() taskYIELD()
#else
#ifdef LINTEST
#define YIELD() std::this_thread::yield()
#else
//#warning NO RTOS
#define YIELD()
#endif
#endif

#define DELAY(a)                                                               \
  do {                                                                         \
    uint32_t startTick = TICKS();                                              \
    while (TICKS() - startTick < (uint32_t)(a))                                \
      YIELD();                                                                 \
  } while (0)

enum mfBusSignals {
  MFBUS_DISCOVER = 0,
  MFBUS_BROADCAST,
  MFBUS_DEBUG,
  MFBUS_BL_GETVARS,
  MFBUS_BL_GETFLASHMAP,
  MFBUS_BL_CHECK,
  MFBUS_WATTAFA,
  MFBUS_BL_ERASE,
  MFBUS_BL_WRITE,
  MFBUS_REBOOT,
  MFBUS_REBOOT_BL,
  MFBUS_REBOOT_SYSBL,
  MFBUS_GETNAME,
  MFBUS_LCD_RESERVED,
  MFBUS_LCD_RESERVED_END = MFBUS_LCD_RESERVED + 30,
  MFBUS_BUTTON_GETSTATECMD,
  MFBUS_RGB_RESERVED,
  MFBUS_RGB_RESERVED_END = MFBUS_RGB_RESERVED + 10,
  MFBUS_BEEP_TONECMD,
  MFBUS_BEEP_NOTONECMD,
  MFBUS_CC_RESERVED,
  MFBUS_CC_END = MFBUS_CC_RESERVED + 15,
  MFBUS_BUTTON_CONFIRMSTATECMD,
  MFBUS_CC_RESERVED2,
  MFBUS_CC_END2 = MFBUS_CC_RESERVED2 + 15,
  MFBUS_SIGNAL_END
};
enum mfBusStatus { MFBUS_OK = 0, MFBUS_TIMEOUT, MFBUS_INVALID, MFBUS_GENERROR };

typedef struct {
  uint32_t pagesize, offset;
  uint32_t UID[3];
  uint32_t start, end;
  uint16_t maxPacket;
  uint8_t flashmap;
} MFBUS_BL_VARS;

typedef struct {
  uint32_t data_size;
  uint32_t crc;
} MFBUS_DATA_PACKET_HEADER;

typedef struct {
  MFBUS_DATA_PACKET_HEADER data;
  uint32_t addr;
} MFBUS_FLASH_PACKET_HEADER;

#define MFBUS_MASTER 0U

#define BIT64(a) 1ULL << (a)
#define MFBUS_CAP_ALLMIGHTY ~0ULL
#define MFBUS_CAP_DEBUG BIT64(0)
#define MFBUS_CAP_KEYS BIT64(1)
#define MFBUS_CAP_LCD160 BIT64(16)
#define MFBUS_CAP_LCD320 BIT64(17)
#define MFBUS_CAP_TELEMETRY_STATE BIT64(32)
#define MFBUS_CAP_TELEMETRY_PERCHANNEL BIT64(33)
#define MFBUS_CAP_SPEED_STATE BIT64(48)
#define MFBUS_CAP_COMP_STATE BIT64(49)
#define MFBUS_CAP_BOOTLOADER BIT64(63)

enum mfBusKeycodes {
  MFBUS_KEYCODE_UP = 0,
  MFBUS_KEYCODE_OK,
  MFBUS_KEYCODE_DOWN,
  MFBUS_KEYCODE_END
};

#ifndef MFBUS_MAX_SLAVE_ADDRESS
#define MFBUS_MAX_SLAVE_ADDRESS 64U
#endif

#ifndef MFBUS_REPLY_TIMEOUT
#define MFBUS_REPLY_TIMEOUT 50U
#endif

#ifndef MFBUS_RETRY
#define MFBUS_RETRY 3U
#endif

#ifndef MFBUS_MINIMAL_INTERVAL
#define MFBUS_MINIMAL_INTERVAL 1
#endif

typedef struct {
  uint8_t device;
  uint64_t caps;
} MFBUS_SLAVE_T;

enum mfDeviceIds {
  MF_ALPRO_V10 = 2,
  MF_ALPRO_V10_LCD = 3,
  MF_ALPRO_V10_BUTTON = 4,
  MF_ALPRO_V10_BUTTON_V2 = 7,
  MF_ALPRO_V10_SPI = 5,
  MF_ALPRO_V10_LCD_SPI = 6,
  MF_ALPRO_V10_CCM = 11,
  MF_ALPRO_V10_LCD_EXT = 12,
  MF_ALPRO_V10_LCD_EXT_SPI = 13,
};

class mfBus {
private:
  uint32_t sent = 1;
  uint64_t deviceCaps;
  uint32_t perSlaveTimeout;
  bool receiveAll = false; // do not receive messages for all devices by default
  std::vector<MFBUS_SLAVE_T> slaves;
  bool proxyMode = false;
  bool defaultAnswerAllowed = true;
  uint8_t defaultAnswer(mfBus *instance, MF_MESSAGE *msg, MF_MESSAGE *newmsg);
  void protoCallback(mfProto *proto);

public:
  mfProto &proto;
  typedef std::function<void(mfBus *)> busRecvCallback_f;
  busRecvCallback_f recvCallback = nullptr;
  inline static std::vector<mfBus *> instances = {};
  uint8_t deviceId;
  char name[32] = ""; // object name, filled by MFBUS define, can be
                      // used for debug purposes
  mfBus(const char *cname, mfProto &_proto, uint8_t _deviceId = MFBUS_MASTER,
        uint64_t _deviceCaps = MFBUS_CAP_ALLMIGHTY);

  void incSent(void);
  uint32_t getSent(void);
  uint64_t getCaps(void);
  void beginStart(void);
  void beginEnd(void);
  void begin(void);
  MFBUS_SLAVE_T getSlave(uint8_t num);
  uint8_t getSlavesCount(void);
  bool slaveExists(uint8_t slaveId);
  mfBusStatus waitForAnswer(uint32_t id, uint32_t timeout);
  void deleteUselessMessages(void);
  void sendMessage(MF_MESSAGE *msg, uint8_t *data, uint16_t size);
  void sendMessage(MF_MESSAGE &msg);
  void sendMessage(uint8_t from, uint8_t to, uint16_t type, uint32_t id,
                   uint8_t *data, uint16_t size);

  uint32_t sendMessageInc(uint8_t to, uint16_t type, uint8_t *data,
                          uint16_t size);
  uint32_t sendMessageWait(uint8_t to, uint16_t type, uint8_t *data,
                           uint16_t size, uint8_t retry = MFBUS_RETRY,
                           uint16_t timeout = MFBUS_REPLY_TIMEOUT);
  typedef std::function<uint8_t(mfBus *, MF_MESSAGE *, MF_MESSAGE *)>
      answerCallback_f;
  uint32_t answerMessage(MF_MESSAGE *msg, answerCallback_f answer);
  void allowDefaultAnswer(void) { defaultAnswerAllowed = true; }
  void denyDefaultAnswer(void) { defaultAnswerAllowed = false; }

  void deleteMessageId(uint32_t id);
  void deleteLastMessage(void);
  void deleteFirstMessage(void);
  void readMessageId(MF_MESSAGE &mess, uint32_t id);
  void readFirstMessage(MF_MESSAGE &mess);
  void readLastMessage(MF_MESSAGE &mess);
  uint8_t haveMessages(void);
  void setRecvCallback(busRecvCallback_f callback);
  void clearRecvCallback(void);
  void monitorMode(bool state);
  void poll(void);
  void setName(const char *_name);
  void initProxyMode(std::vector<MFBUS_SLAVE_T> &members);
  void stopProxyMode(bool keepMonitorMode = false);
  static uint32_t crc32(uint32_t init_crc, uint32_t *buf, uint16_t len);
  static uint32_t crc32(uint32_t *buf, uint16_t len);
  ~mfBus();
};

#define MFBUS(_a_, ...) mfBus(_a_)(#_a_, __VA_ARGS__);

#endif
