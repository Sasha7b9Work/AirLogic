#ifdef STM32_CORE
#include "debug.h"
#include <wiring.h>
extern DebugSerial debug;
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#include "mfbus.h"
#include "mfcrc.h"
#include "mfproto.h"
#include <functional>
#include <vector>

void mfBus::protoCallback(mfProto *proto) {
  if (recvCallback)
    recvCallback(this);
};

mfBus::mfBus(const char *cname, mfProto &_proto, uint8_t _deviceId,
             uint64_t _deviceCaps)
    : deviceCaps(_deviceCaps), proto(_proto), deviceId(_deviceId) {
  setName((char *)cname);
  if (deviceId == MFBUS_MASTER) {
    uint32_t speed = proto.PHY.getSpeed();
    if (!speed)
      speed = 1;
    perSlaveTimeout =
        1 + (proto.PHY.getMaxSpeed() / (speed << 2)); // at least 1ms delay
    deviceCaps = MFBUS_CAP_ALLMIGHTY;
  }
  instances.push_back(this);
  _me = instances.end();
  proto.setRecvCallback(
      std::bind(&mfBus::protoCallback, this, std::placeholders::_1));
  slaves.reserve(MFBUS_MAX_SLAVE_ADDRESS);
}

void mfBus::setName(const char *_name) {
  if (name != __NULL__)
    free(name);
  name = (char *)malloc(strlen(_name) + 1);
  if (name != __NULL__) {
    strcpy(name, _name);
  }
}

uint64_t mfBus::getCaps(void) { return deviceCaps; }
uint32_t mfBus::getSent(void) { return sent; }

void mfBus::incSent(void) { sent++; }
void mfBus::beginStart(void) {
  proto.begin();
  poll();
  if (deviceId == MFBUS_MASTER) {
    sendMessageInc(MFBUS_MASTER, MFBUS_DISCOVER, NULL, 0);
  }
  poll();
}
void mfBus::beginEnd(void) {
  uint32_t startTick = TICKS();
  if (deviceId == MFBUS_MASTER) {
    slaves.clear();
    uint32_t curId =
        getSent() - 1; // we need previous message id cuz it incremented
    while (TICKS() - startTick <
           (MFBUS_MAX_SLAVE_ADDRESS + 3) * perSlaveTimeout) {
      poll();
      /* wait till all slaves will answer */
      if (proto.size()) {
        MF_MESSAGE msg;
        proto.readFirstMessage(msg);
        if ((msg.head.type == MFBUS_DISCOVER) &&
            (msg.head.to == MFBUS_MASTER) &&
            (msg.head.size == sizeof(deviceCaps)) && (msg.head.id == curId)) {
          slaves.push_back({msg.head.from, *((uint64_t *)msg.data)});
        }
        YIELD();
      }
    }
  }
  poll();
  // proto.clear();
}
void mfBus::begin(void) {
  beginStart();
  beginEnd();
}
uint8_t mfBus::getSlavesCount(void) { return slaves.size(); }
MFBUS_SLAVE_T mfBus::getSlave(uint8_t num) {
  if ((num < getSlavesCount()) && (deviceId == MFBUS_MASTER)) {
    return slaves[num];
  } else {
    return {0, 0ULL};
  }
}
void mfBus::deleteUselessMessages(void) {
  if (!receiveAll) {
    if (!proto.size()) {
      return;
    }
    if (proto.getMutex(proto.messagesMutex)) {
      std::vector<uint32_t> junk;
      junk.reserve(MF_QUEUE_LEN);
      for (uint8_t m = 0; m < proto.size(); m++) {
        MF_MESSAGE msg;
        proto._readMessageIndex(msg, m);
        if ((msg.head.to != deviceId) && (msg.head.type != MFBUS_BROADCAST) &&
            (msg.head.type != MFBUS_DISCOVER)) {
          junk.push_back(msg.head.id);
        }
      }
      while (junk.size()) {
        proto._deleteMessageIndex(proto.indexById(junk.back()));
        junk.pop_back();
      }
      proto.freeMutex(proto.messagesMutex);
    }
  }
}

void mfBus::sendMessage(uint8_t from, uint8_t to, uint16_t type, uint32_t id,
                        uint8_t *data, uint16_t size) {
  proto.sendMessage(from, to, type, id, data, size);
}

void mfBus::sendMessage(MF_MESSAGE *msg, uint8_t *data, uint16_t size) {
  // MF_MESSAGE mess(msg->head, msg->foot, data, size);
  // proto.sendMessage(&mess);
  // MF_MESSAGE mess(msg->head.from, msg->head.to, msg->head.type, msg->head.id,
  //                data, size);
  // uint8_t *msg_data = msg->data;
  // uint8_t msg_head_size = msg->head.size;
  // msg->data = data;
  // msg->head.size = size;
  proto.sendMessage(msg, data, size);
  // msg->data = msg_data;
  // msg->head.size = msg_head_size;
}

void mfBus::sendMessage(MF_MESSAGE &msg) {
  proto.sendMessage(&msg, msg.data, msg.head.size);
}

uint32_t mfBus::sendMessageInc(uint8_t to, uint16_t type, uint8_t *data,
                               uint16_t size) {
  uint32_t id = getSent();
  sendMessage(deviceId, to, type, id, data, size);
  incSent();
  return id;
}
uint32_t mfBus::sendMessageWait(uint8_t to, uint16_t type, uint8_t *data,
                                uint16_t size, uint8_t retry,
                                uint16_t timeout) {
  while (retry--) {
    uint32_t id = sendMessageInc(to, type, data, size);
    mfBusStatus status = waitForAnswer(id, timeout);
    if (status == MFBUS_OK) {
#ifndef STM32_CORE
      // printf("We've got it, id=%u\n", id);
#endif
      return id;
    }
    if (status == MFBUS_TIMEOUT) {
#ifdef STRDEBUG_H
      // debug.printf("Timed out [%u]\n", id);
#endif
#ifndef STM32_CORE
      // printf("We've not got it ooops, id=%u\n", id);
#endif
    }
  }
  return 0;
}
uint32_t mfBus::answerMessage(MF_MESSAGE *msg, answerCallback_f answer) {
  if (msg->head.id) {
    MF_MESSAGE nmsg(msg->head.to, msg->head.from, msg->head.type, msg->head.id,
                    NULL, 0);

    uint8_t answerHandled = answer(this, msg, &nmsg);
    if ((!answerHandled) && (defaultAnswerAllowed) &&
        (deviceId != MFBUS_MASTER)) {
      answerHandled = defaultAnswer(this, msg, &nmsg);
    }
  }
  return msg->head.id;
}

mfBusStatus mfBus::waitForAnswer(uint32_t id, uint32_t timeout) {
  if (deviceId != MFBUS_MASTER) {
    return MFBUS_GENERROR; // slaves should not wait for answers
  }
  // debug.printf("%s]Waiting for %u\n", name, id);
  uint32_t ticks = TICKS();
  while (TICKS() - ticks < timeout) {
    DELAY(MFBUS_MINIMAL_INTERVAL);
    // YIELD();
    poll();
    uint32_t index = proto.indexById(id);
    if (index < proto.size()) {
      return MFBUS_OK;
    }
  }
  return MFBUS_TIMEOUT;
}

void mfBus::deleteMessageId(uint32_t id) { proto.deleteMessageId(id); }
void mfBus::deleteLastMessage(void) { proto.deleteLastMessage(); }
void mfBus::deleteFirstMessage(void) { proto.deleteFirstMessage(); }
void mfBus::readMessageId(MF_MESSAGE &mess, uint32_t id) {
  proto.readMessageId(mess, id);
}
void mfBus::readFirstMessage(MF_MESSAGE &mess) { proto.readFirstMessage(mess); }
void mfBus::readLastMessage(MF_MESSAGE &mess) { proto.readLastMessage(mess); }
uint8_t mfBus::haveMessages(void) {
  poll();
  return proto.size();
}

void mfBus::setRecvCallback(busRecvCallback_f callback) {
  recvCallback = callback;
}
void mfBus::clearRecvCallback(void) { recvCallback = nullptr; }
void mfBus::monitorMode(bool state) { receiveAll = state; }
void mfBus::poll(void) {
  deleteUselessMessages();
  proto.poll();
}

#ifdef SYSBL_MAGIC_NUMBER_BKP_INDEX
void setMagicAndReboot(uint16_t magic) {
  enableBackupDomain();
  setBackupRegister(SYSBL_MAGIC_NUMBER_BKP_INDEX, magic);
  HAL_NVIC_SystemReset();
}
#endif

uint8_t mfBus::defaultAnswer(mfBus *instance, MF_MESSAGE *msg,
                             MF_MESSAGE *newmsg) {
  uint8_t ret = 1;
  switch (msg->head.type) {
  case MFBUS_DISCOVER: {
    std::vector<MFBUS_SLAVE_T> tempSlaves;
    tempSlaves.clear();
    if (proxyMode) {
      for (uint8_t i = 0; i < slaves.size(); i++) {
        tempSlaves.push_back(slaves[i]);
      }
    } else {
      tempSlaves.push_back({deviceId, deviceCaps});
    }
    if (msg->head.from == MFBUS_MASTER) {
      uint32_t startTick = TICKS();
      uint8_t cnt = 0;
      do {
        startTick = TICKS();
        if (tempSlaves.front().device == cnt) {
          instance->sendMessage(tempSlaves.front().device, msg->head.from,
                                msg->head.type, msg->head.id,
                                (uint8_t *)&tempSlaves.front().caps,
                                sizeof(tempSlaves.front().caps));
          tempSlaves.erase(tempSlaves.begin());
        }
        cnt++;
        startTick += perSlaveTimeout;
        uint32_t endTick = TICKS();
        if (startTick > endTick)
          DELAY(startTick - endTick);
      } while (tempSlaves.size());
    } else {
      ret = 0;
    }
  } break;

#ifdef STRDEBUG_H
    /* Send debug block to the host */
  case MFBUS_DEBUG: {
    uint32_t dbgsize = debug.available();
    if (dbgsize > 32)
      dbgsize = 32;
    uint8_t buf[dbgsize];
    if (buf != NULL) {
      if (dbgsize) {
        uint32_t counter = 0;
        while (counter < dbgsize)
          buf[counter++] = debug.read();
      }
    } else {
      dbgsize = 0;
    }
    instance->sendMessage(newmsg, buf, dbgsize);
  } break;
#endif
#ifdef SYSBL_MAGIC_NUMBER_BKP_INDEX
  /* reboot to bootloader */
  case MFBUS_REBOOT_BL:
    instance->sendMessage(newmsg, NULL, 0);
    setMagicAndReboot(MFBL_MAGIC_NUMBER_BKP_VALUE);
    break;

  /* reboot to system bootloader */
  case MFBUS_REBOOT_SYSBL:
    instance->sendMessage(newmsg, NULL, 0);
    setMagicAndReboot(SYSBL_MAGIC_NUMBER_BKP_VALUE);
    break;

  /* reboot device */
  case MFBUS_REBOOT:
    instance->sendMessage(newmsg, NULL, 0);
    setMagicAndReboot(0x0000);
    break;
#endif
  /* check for active bootloader */
  case MFBUS_BL_CHECK: {
    /* only bl have to be with MFBUS_CAP_BOOTLOADER cap */
    uint8_t bootloader =
        (instance->getCaps() & MFBUS_CAP_BOOTLOADER) > 0 ? 1 : 0;
    instance->sendMessage(newmsg, &bootloader, 1);
  } break;
  case MFBUS_GETNAME: {
    /* return mfBus instance name */
    instance->sendMessage(newmsg, (uint8_t *)instance->name,
                          strlen(instance->name) + 1);
  } break;
  default:
    ret = 0;
  }
  return ret;
}

void mfBus::initProxyMode(std::vector<MFBUS_SLAVE_T> &members) {
  if (deviceId != MFBUS_MASTER) {
    slaves.clear();
    for (uint8_t i = 0; i < members.size(); i++)
      slaves.push_back(members[i]);
    proxyMode = true;
    receiveAll = true;
  }
}
void mfBus::stopProxyMode(bool keepMonitorMode) {
  if (deviceId != MFBUS_MASTER) {
    slaves.clear();
    proxyMode = false;
    receiveAll = keepMonitorMode;
  }
}
inline uint32_t mfBus::crc32(uint32_t *buf, uint16_t len) {
  return mfCRC::crc32(buf, len);
}
inline uint32_t mfBus::crc32(uint32_t init_crc, uint32_t *buf, uint16_t len) {
  return mfCRC::crc32(init_crc, buf, len);
}
mfBus::~mfBus() {
  instances.erase(_me);
  if (name != __NULL__) {
    free(name);
  }
}
