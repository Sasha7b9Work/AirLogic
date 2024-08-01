#include "backup.h"
#include "bootloader.h"
#include "stm32yyxx_ll_utils.h"
#include "timer_init.h"
#include <cPins.h>
#include <debug.h>
#include <mfbus.h>
#include <mfcrc.h>
#include <mfproto.h>
#include <mfprotophy.h>
#include <mfprotophyserial.h>
#include <wiring.h>

#ifdef AES_DECRYPT
#include <AES.h>
#include <CBC.h>
#include <Crypto.h>
CBC<AES128> cbc;
#include "aeskey.h"
#endif

/* programming led, high level api */
CPIN(PRG, BL_LED, CPIN_LED, BL_LEDLVL);

#if defined(BL_F051)
#define BAUD (MFPROTO_SERIAL_MAXBAUD / 20)
//#define BAUD 115200
MFPROTOPHYSERIAL(SlavePhy, Serial1, PA12, BAUD); // Serial physical layer
static const char deviceName[] = "alpro_v10_button";
static const uint16_t *flashtable = NULL;
static const uint8_t deviceId = MF_ALPRO_V10_BUTTON;
#define FLASH_SECTOR_TOTAL ((LL_GetFlashSize() << 10) / FLASH_PAGE_SIZE)
#define FLASH_TYPEERASE_SECTORS FLASH_TYPEERASE_PAGES
#endif

#if defined(BL_F401)
#define PROXY_MODE
#include "f401_init.h"
//#define BAUD 115200
#define BAUD (MFPROTO_SERIAL_MAXBAUD / 10)
MFPROTOPHYSERIAL(SlavePhy, Serial1, MFPROTO_PHY_SERIAL_NC,
                 BAUD); // Serial physical layer
static const char deviceName[] = "alpro_v10_lcd";
static const uint8_t deviceId = MF_ALPRO_V10_LCD;
static const uint16_t flashtable[] = {16, 16, 16, 16, 64, 128, 128, 128};
/* display backlight, defined here to turn it off by default */
CPIN(LED, PB1, CPIN_LED, CPIN_HIGH);

#include "mfprotophypipe.h"
#include "mfprotopipe.h"
MFPROTOPHYPIPE(pipemasphy);
MFPROTOPHYPIPE(spiphy);
MFPROTOPIPE(pipe, {&pipemasphy});
MFPROTO(pipemasproto, pipemasphy);
MFPROTO(spiproto, spiphy);
MFBUS(pipeMaster, pipemasproto, MFBUS_MASTER);
MFBUS(alpro_v10_lcd_spi, spiproto, MF_ALPRO_V10_LCD_SPI,
      MFBUS_CAP_DEBUG | MFBUS_CAP_BOOTLOADER);

#define _INT_FLASH alpro_v10_lcd_spi
#define _W25QXX_SPI hspi2
#include "w25qxx.h"
extern w25qxx_t w25qxx;
#endif

#if defined(BL_MASTER_F405)
#define PROXY_MODE
#include "f405_init.h"
#include <mfprotophyacm.h>
#define BAUD (MFPROTO_SERIAL_MAXBAUD / 10)
MFPROTOPHYACM(SlavePhy, BAUD); // CDC physical layer
static const char deviceName[] = "alpro_v10";
static const uint8_t deviceId = MF_ALPRO_V10;
static const uint16_t flashtable[] = {16,  16,  16,  16,  64,  128,
                                      128, 128, 128, 128, 128, 128};
/* buses power enable pins */
CPIN(EN5V, PE5, CPIN_PIN, CPIN_HIGH);  // EN 5V
CPIN(EN12V, PE4, CPIN_PIN, CPIN_HIGH); // EN 12V
CPIN(LOK, PB7, CPIN_LED, CPIN_OD);
/* bus masters */
MFPROTOPHYSERIAL(COM1, Serial1, PA1, BAUD / 2);
MFPROTO(wifiproto, COM1);
mfBus wifiBus(deviceName, wifiproto, deviceId,
              MFBUS_CAP_DEBUG | MFBUS_CAP_BOOTLOADER);

MFPROTOPHYSERIAL(COM2, Serial2, PA1, BAUD / 2);
MFPROTO(Bus1proto, COM2);
MFBUS(Bus1, Bus1proto, MFBUS_MASTER);

MFPROTOPHYSERIAL(COM3, Serial3, PD12, BAUD / 2);
MFPROTO(Bus2Proto, COM3);
MFBUS(Bus2, Bus2Proto, MFBUS_MASTER);

MFPROTOPHYSERIAL(COM4, Serial4, MFPROTO_PHY_SERIAL_NC, BAUD);
MFPROTO(Bus3Proto, COM4);
MFBUS(Bus3, Bus3Proto, MFBUS_MASTER);

#include "mfprotophypipe.h"
#include "mfprotopipe.h"
MFPROTOPHYPIPE(pipemasphy);
MFPROTOPHYPIPE(spiphy);
MFPROTOPIPE(pipe, {&pipemasphy});
MFPROTO(pipemasproto, pipemasphy);
MFPROTO(spiproto, spiphy);
MFBUS(pipeMaster, pipemasproto, MFBUS_MASTER);
MFBUS(alpro_v10_spi, spiproto, MF_ALPRO_V10_SPI,
      MFBUS_CAP_DEBUG | MFBUS_CAP_BOOTLOADER);

#define _INT_FLASH alpro_v10_spi
#define _W25QXX_SPI hspi1
#include "w25qxx.h"
extern w25qxx_t w25qxx;
#endif

MFPROTO(SlaveProto, SlavePhy); // protocol instance
/* bus instance */
mfBus SlaveBus(deviceName, SlaveProto, deviceId,
               MFBUS_CAP_DEBUG | MFBUS_CAP_BOOTLOADER);

DebugSerial debug(SlaveBus); // debug container

inline uint32_t disable_irq(void) {
  uint32_t irq_state = __get_PRIMASK();
  __disable_irq();
  return irq_state;
}
inline void enable_irq(uint32_t state) { __set_PRIMASK(state); }

/* provide universal and simple way to poll main flashing interface, just poll
 * it every systick handler call */
void (*serialPolling)(void) = NULL;
void HAL_SYSTICK_Callback() {
  if (serialPolling != NULL) {
    serialPolling();
  }
}

#ifdef PROXY_MODE
void sortSlaves(std::vector<MFBUS_SLAVE_T> &arr) {
  uint32_t len = arr.size();
  for (uint32_t i = 0; i < len - 1; i++)
    for (uint32_t j = 0; j < len - i - 1; j++)
      if (arr[j].device > arr[j + 1].device) {
        MFBUS_SLAVE_T temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
}

mfBus *getBusById(uint32_t device) {
  for (uint32_t i = 0; i < mfBus::instances.size(); i++)
    if (mfBus::instances[i]->deviceId == MFBUS_MASTER)
      for (uint32_t j = 0; j < mfBus::instances[i]->getSlavesCount(); j++)
        if (mfBus::instances[i]->getSlave(j).device == device)
          return mfBus::instances[i];
  return nullptr;
}
#endif

#ifdef _INT_FLASH
/* reserve two blocks (2*64K) for internal needs */
#define SPI_FLASH_OFFSET (w25qxx.BlockSize * 2)
/* spi nor flash virtual slave messages processing */
uint8_t spiAnswerCallback(mfBus *instance, MF_MESSAGE *msg,
                          MF_MESSAGE *newmsg) {
  // debug.printf("Got message type: %u\n", msg->head.type);
  uint8_t ret = 1;
  static const uint16_t *flashtable = NULL;
  switch (msg->head.type) {
  /* check bootloader vars */
  case MFBUS_BL_GETVARS: {
    MFBUS_BL_VARS blData = {0};
    blData.offset = SPI_FLASH_OFFSET;
    blData.start = 0;
    blData.end = w25qxx.BlockCount * w25qxx.BlockSize;
    blData.pagesize = w25qxx.BlockSize;
    blData.UID[2] = *(uint32_t *)&w25qxx.UniqID[0];
    blData.UID[1] = *(uint32_t *)&w25qxx.UniqID[4];
    /* */
    blData.maxPacket = MFPROTO_MAXDATA;
    blData.flashmap = (flashtable != NULL) ? 1 : 0;
    instance->sendMessage(newmsg, (uint8_t *)&blData, sizeof(MFBUS_BL_VARS));
  } break;

  /* get flash map */
  case MFBUS_BL_GETFLASHMAP: {
    instance->sendMessage(newmsg, (uint8_t *)&flashtable, sizeof(flashtable));
  } break;
  case MFBUS_BL_ERASE: {
    uint8_t ret = 0xFF;
    if (msg->head.size == sizeof(uint32_t)) {
      uint32_t *erasePage = (uint32_t *)msg->data;
      ret = 0x1F;
      if ((*erasePage < w25qxx.BlockCount) &&
          (*erasePage >= (SPI_FLASH_OFFSET / w25qxx.BlockSize))) {
        ret = 0xF;
        W25qxxEraseBlock(*erasePage);
        ret = 0;
      }
    }
    instance->sendMessage(newmsg, (uint8_t *)&ret, sizeof(ret));
  } break;
  case MFBUS_BL_WRITE: {
    uint8_t ret1 = 0xFF;
    /* check for the proper size */
    if (msg->head.size >= sizeof(MFBUS_FLASH_PACKET_HEADER)) {
      MFBUS_FLASH_PACKET_HEADER *head = (MFBUS_FLASH_PACKET_HEADER *)msg->data;
      ret1 = 0x7F;
      /*check for the proper size again cuz now we got the header*/
      if (msg->head.size ==
          sizeof(MFBUS_FLASH_PACKET_HEADER) + head->data.data_size) {
        ret1 = 0x3f;
        if ((head->addr >= SPI_FLASH_OFFSET) &&
            (head->addr + head->data.data_size <
             w25qxx.BlockCount * w25qxx.BlockSize)) {
          ret1 = 0x1f;
          /* check the crc32 */
          uint32_t crc = mfCRC::crc32(
              (uint32_t *)&msg->data[sizeof(MFBUS_FLASH_PACKET_HEADER)],
              head->data.data_size);
          if (crc == head->data.crc) {
            ret1 = 0x0f;
#ifdef AES_DECRYPT
            /* normally we assume encrypted firmware transfer but
             * for debug reasons it can be disabled */
            cbc.decrypt(
                (uint8_t *)&msg->data[sizeof(MFBUS_FLASH_PACKET_HEADER)],
                (uint8_t *)&msg->data[sizeof(MFBUS_FLASH_PACKET_HEADER)],
                head->data.data_size);
            /* recalc crc32 after decrypt */
            crc = mfCRC::crc32(
                (uint32_t *)&msg->data[sizeof(MFBUS_FLASH_PACKET_HEADER)],
                head->data.data_size);
#endif
            /* let's flash it */
            /* but first create a copy */
            uint8_t tempBuf[head->data.data_size];
            memcpy(tempBuf, &msg->data[sizeof(MFBUS_FLASH_PACKET_HEADER)],
                   head->data.data_size);
            /* and write a copy, as it will be overwritten by data
               transfer */
            W25qxxWriteBytes(tempBuf, head->addr, head->data.data_size);
            ret1 = 0x07;
            /* check the crc32 of written data, in case of
             * encryption, we comparing it with internally
             * recalculated checksum here */
            /* read back data to temporary buffer */
            W25qxxReadBytes(tempBuf, head->addr, head->data.data_size);
            uint32_t crcWritten =
                mfCRC::crc32((uint32_t *)tempBuf, head->data.data_size);
            if (crcWritten == crc) {
              ret1 = 0x0; // woohoo
            }
          }
        }
      }
    }
    instance->sendMessage(newmsg, &ret1, sizeof(ret1));
  } break;
  case MFBUS_REBOOT:
  case MFBUS_REBOOT_BL:
  case MFBUS_REBOOT_SYSBL: {
    instance->sendMessage(newmsg, NULL, 0);
    return 1;
  }
  default:
    ret = 0;
  }
  return ret;
}
#endif

uint8_t answerCallback(mfBus *instance, MF_MESSAGE *msg, MF_MESSAGE *newmsg) {
  if ((msg->head.type == MFBUS_BROADCAST) || (msg->head.type == MFBUS_DISCOVER))
    return 0;
  uint8_t ret = 1;
  if (msg->head.to == instance->deviceId) {
    switch (msg->head.type) {
    /* check bootloader vars */
    case MFBUS_BL_GETVARS: {
      MFBUS_BL_VARS blData;
      blData.offset = BL_OFFSET;
      blData.start = FLASH_BASE;
      blData.end = FLASH_BASE + (LL_GetFlashSize() << 10);
      blData.pagesize = FLASH_PAGE_SIZE;
      blData.UID[0] = LL_GetUID_Word0();
      blData.UID[1] = LL_GetUID_Word1();
      blData.UID[2] = LL_GetUID_Word2();
      blData.maxPacket = MFPROTO_MAXDATA;
      blData.flashmap = (flashtable != NULL) ? 1 : 0;
      instance->sendMessage(newmsg, (uint8_t *)&blData, sizeof(MFBUS_BL_VARS));
    } break;

    /* get flash map */
    case MFBUS_BL_GETFLASHMAP: {
      instance->sendMessage(newmsg, (uint8_t *)&flashtable, sizeof(flashtable));
    } break;

    /* erase one flash page/sector */
    case MFBUS_BL_ERASE: {
      uint8_t ret = 0xFF;
      if (msg->head.size == sizeof(uint32_t)) {
        if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != RESET) {
          ret = 0xFE; // need to wait a bit
          break;
        } else {
          uint32_t *erasePage = (uint32_t *)msg->data;
          uint32_t PAGEError = 0;
          /* check margins and protect bootloader from erasing */
          uint16_t blStartPage = 0;
          if (flashtable == NULL) {
            blStartPage = (BL_OFFSET / FLASH_PAGE_SIZE);
          } else {
            // find start page
            uint32_t offset = 0;
            for (uint32_t i = 0; i < sizeof(flashtable) / sizeof(uint16_t);
                 i++) {
              if (offset * 1024 == BL_OFFSET) {
                blStartPage = i;
                break;
              }
              offset += flashtable[i];
            }
          }
          ret = 0x1F;
          if ((*erasePage >= blStartPage) &&
              (*erasePage < FLASH_SECTOR_TOTAL)) {
            ret = 0xF;
            if (HAL_FLASH_Unlock() == HAL_OK) {
              FLASH_EraseInitTypeDef EraseInitStruct = {0};
#ifdef BL_F051
              /* f0 have different api */
              EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
              EraseInitStruct.PageAddress =
                  FLASH_BASE + (*erasePage * FLASH_PAGE_SIZE);
              EraseInitStruct.NbPages = 1;
#else
              EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
              EraseInitStruct.Sector = *erasePage;
              EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
              EraseInitStruct.NbSectors = 1;
#endif
              /* erase :) */
              // HAL_NVIC_DisableIRQ(getTimerCCIrq(_TIMER_));
              PRG.blink(~0U, 50);
              ret = 0xF;
              if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) == HAL_OK) {
                ret = 0x1;
                if (PAGEError == 0xFFFFFFFFU) {
                  ret = 0;
                }
              }
              // HAL_NVIC_EnableIRQ(getTimerCCIrq(_TIMER_));
              HAL_FLASH_Lock();
            }
          }
        }
      }
      instance->sendMessage(newmsg, (uint8_t *)&ret, sizeof(ret));
    } break;

    /* write a block to flash */
    case MFBUS_BL_WRITE: {
      // LOK.blinkfade(300, 100);
      uint8_t ret1 = 0xFF;
      /* check for the proper size */
      if (msg->head.size >= sizeof(MFBUS_FLASH_PACKET_HEADER)) {
        if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != RESET) {
          ret = 0xFE; // need to wait a bit
        } else {
          MFBUS_FLASH_PACKET_HEADER *head =
              (MFBUS_FLASH_PACKET_HEADER *)msg->data;
          ret1 = 0x7F;
          /* check for the proper size again cuz now we got the header */
          if (msg->head.size ==
              sizeof(MFBUS_FLASH_PACKET_HEADER) + head->data.data_size) {
            /* check the crc32 */
            uint32_t crc = mfCRC::crc32(
                (uint32_t *)&msg->data[sizeof(MFBUS_FLASH_PACKET_HEADER)],
                head->data.data_size);
            ret1 = 0x3f;
            if (crc == head->data.crc) {
              ret1 = 0x1f;
#ifdef AES_DECRYPT
              /* normally we assume encrypted firmware transfer but for
               * debug reasons it can be disabled */
              cbc.decrypt(
                  (uint8_t *)&msg->data[sizeof(MFBUS_FLASH_PACKET_HEADER)],
                  (uint8_t *)&msg->data[sizeof(MFBUS_FLASH_PACKET_HEADER)],
                  head->data.data_size);
              /* recalc crc32 after decrypt */
              crc = mfCRC::crc32(
                  (uint32_t *)&msg->data[sizeof(MFBUS_FLASH_PACKET_HEADER)],
                  head->data.data_size);
#endif
              if (HAL_FLASH_Unlock() == HAL_OK) {
                uint32_t writeErr = 0;
                /* let's flash it */
                for (uint32_t d = 0;
                     d < (head->data.data_size / sizeof(uint16_t)); d++) {
                  // wait till previous write
                  FLASH_WaitForLastOperation(100);
                  uint16_t *halfword =
                      (uint16_t *)(msg->data +
                                   sizeof(MFBUS_FLASH_PACKET_HEADER) +
                                   (d * sizeof(uint16_t)));
                  uint32_t state = disable_irq();
                  writeErr += HAL_FLASH_Program(
                      FLASH_TYPEPROGRAM_HALFWORD,
                      head->addr + (d * sizeof(uint16_t)), *halfword);
                  enable_irq(state);
                }
                ret1 = 0x0f;
                if (writeErr == HAL_OK) {
                  /* check the crc32 of written data, in case of encryption,
                   * we comparing it with internally recalculated checksum
                   * here */
                  uint32_t *buf = (uint32_t *)head->addr;
                  uint32_t crcWritten = mfCRC::crc32(buf, head->data.data_size);
                  ret1 = 0x07;
                  if (crcWritten == crc) {
                    ret1 = 0x0; // woohoo
                  }
                }
                if (ret1 == 0xF)
                  debug.printf("%08X\n", HAL_FLASH_GetError());
              }
              HAL_FLASH_Lock();
            }
          }
        }
      }
      instance->sendMessage(newmsg, &ret1, sizeof(ret1));
    } break;
    case MFBUS_DEBUG:
#ifdef BL_MASTER_F405
      LOK.blink(100, 100);
#endif
      ret = 0;
    default:
      ret = 0;
      break;
    }
  } else { // proxying incoming messages
#ifdef PROXY_MODE
    mfBus *MasterBus = getBusById(msg->head.to);
    if (MasterBus != nullptr)
      MasterBus->proto.sendMessage(msg);
#endif
  }
  return ret;
};

#ifdef BL_F051
#define timerFrequency 60 // pwm frequency
#else
#define timerFrequency 120 // pwm frequency
#endif
void setup() {
#ifdef AES_DECRYPT
  cbc.setKey(AES_KEY, 16);
  cbc.setIV(AES_IV, 16);
#endif
  std::vector<MFBUS_SLAVE_T> slaves;
  slaves.clear();
  slaves.push_back({deviceId, SlaveBus.getCaps()}); // push myself to list
  cPins::timerFreq = timerFrequency;
  TIM_Init(timerFrequency * 256);                   // init timer for led pwm
  TIMirqHandler = []() { cPins::timerCallback(); }; // call pwm callback
  PRG.setMode(CPIN_CONTINUE); // non blocking and phase friendly
#if defined(BL_F401) || defined(BL_F051)
  PRG.setPWM(80); // rgb leds are too bright
#endif
#ifdef BL_MASTER_F405
  EN5V.on();
  EN12V.on();
  DELAY(500);
#endif

#ifdef _INT_FLASH
  _INT_SPI_Init();
  delay(300);
  if (W25qxxInit(&_W25QXX_SPI, _W25QXX_CS_GPIO_Port, _W25QXX_CS_Pin)) {
    pipe.insertMember(&spiphy);
    /*    debug.printf("SPI Inited: %lu %lu %lu %lu\n", w25qxx.PageSize,
                     w25qxx.PageCount, w25qxx.SectorSize, w25qxx.SectorCount);*/
    _INT_FLASH.setRecvCallback([](mfBus *bus) {
      while (bus->haveMessages()) {
        MF_MESSAGE msg;
        bus->readFirstMessage(msg);
        bus->answerMessage(&msg, spiAnswerCallback);
      }
    });
  }
#endif

#ifdef PROXY_MODE
#define MAX_RETRIES 5
  // begin master buses, firstly we need to put all devices to bootloader mode
  // to fetch all bus members
  for (uint32_t i = 0; i < mfBus::instances.size(); i++) {
    if (mfBus::instances[i]->deviceId == MFBUS_MASTER) { // only masters
      mfBus::instances[i]->begin();
      for (uint8_t s = 0; s < mfBus::instances[i]->getSlavesCount(); s++) {
        MFBUS_SLAVE_T slave = mfBus::instances[i]->getSlave(s);
        uint8_t retries = MAX_RETRIES;
        while (retries--) { // make some tries
          uint32_t id = mfBus::instances[i]->sendMessageWait(
              slave.device, MFBUS_BL_CHECK, NULL, 0); // check mode
          if (id) {
            MF_MESSAGE msg;
            mfBus::instances[i]->readMessageId(msg, id);
            if (msg.head.size == 1) {
              if (!*msg.data) { // reboot if not in the bootloader mode
                id = mfBus::instances[i]->sendMessageWait(
                    slave.device, MFBUS_REBOOT_BL, NULL, 0);
                // DELAY(100);
              } else {
                retries = 0;
              }
            }
          }
        }
      }
    }
  }
  for (uint32_t i = 0; i < mfBus::instances.size(); i++) { // get all slaves
    if (mfBus::instances[i]->deviceId == MFBUS_MASTER) {
      mfBus::instances[i]->begin();
      for (uint8_t s = 0; s < mfBus::instances[i]->getSlavesCount(); s++) {
        MFBUS_SLAVE_T slave = mfBus::instances[i]->getSlave(s);
        slaves.push_back(slave);
      }
    }
  }
  sortSlaves(slaves);
  SlaveBus.initProxyMode(slaves);
#endif
  PRG.breathe(~0U, 2000);
  SlaveBus.begin(); // start the bus
/* poll buses for new messages */
#ifdef BL_MASTER_F405
  // serialPolling = []() { SlaveBus.poll(); };
#endif
  /* init irq handlers for non-polling message capture */
  for (uint32_t i = 0; i < mfProtoPhySerial::instances.size(); i++) {
    // mfProtoPhySerial::instances[i]->redefineIrqHandler();
  }
  /* blink while receiving messages */
  SlaveBus.setRecvCallback(
      [](mfBus *mf) { PRG.blink(100 + (100 * mf->haveMessages()), 50); });
}

void loop() {
  while (1) {
    for (uint32_t s = 0; s < mfBus::instances.size(); s++) { // get all slaves
#ifdef _INT_FLASH
      if (_INT_FLASH.deviceId != mfBus::instances[s]->deviceId)
#endif
        if (mfBus::instances[s]->deviceId != MFBUS_MASTER) {
          mfBus::instances[s]->poll();
          if (mfBus::instances[s]->haveMessages()) {
            MF_MESSAGE msg;
            mfBus::instances[s]->readFirstMessage(msg);
            mfBus::instances[s]->answerMessage(&msg, answerCallback);
          }
#ifdef PROXY_MODE
          for (uint32_t i = 0; i < mfBus::instances.size(); i++) {
            if (mfBus::instances[i]->deviceId == MFBUS_MASTER) {
              if (mfBus::instances[i]->haveMessages()) {
                MF_MESSAGE msg;
                mfBus::instances[i]->readFirstMessage(msg);
                SlaveBus.sendMessage(msg);
              }
            }
          }
#endif
          DELAY(1);
        }
    }
  }
}