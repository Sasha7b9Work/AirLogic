
#ifdef STM32_CORE
#include <wiring.h>
#else
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#include "mfbus.h"
#include "mfproto.h"

// constructor
mfProto::mfProto(const char *cname, mfProtoPhy &_phy) : PHY(_phy) {
  name = (char *)malloc(strlen(cname) + 1);
  if (name != __NULL__) {
    strcpy(name, cname);
  }
  instances.push_back(this);
  _me = instances.end();
#ifdef MF_RTOS
  xSemaphoreCreateMutex(); // strange workaround
#endif
  /*  idMutex.store(false);
    bufferMutex.store(false);
    sendMutex.store(false);
    messagesMutex.store(false);*/
#ifdef USE_VECTOR_BUFFER
  buf.reserve(MFPROTO_BUFSIZE);
#endif
  buf.clear();
  // messages.reserve(MF_QUEUE_LEN);
  clear();
  PHY.begin();
  PHY.putChar = std::bind(&mfProto::putChar, this, std::placeholders::_1);
}
void mfProto::begin(void) {}
uint8_t mfProto::addMessage(MF_MESSAGE &message, mfProtoAdd push) {
  return addMessage(message.head, message.foot, message.data, message.head.size,
                    push);
}
uint8_t mfProto::addMessage(MF_HEADER h, MF_FOOTER f, uint8_t *data,
                            uint16_t len, mfProtoAdd push) {
  if (getMutex(messagesMutex, (irqSet ? 1 : MFPROTO_MUTEX_WAIT))) {
    if (messages.size() >= MF_QUEUE_LEN)
      return MF_FULL;
    if (len > MFPROTO_MAXDATA)
      return MF_ERR;
    /* messages.insert(push ? messages.end() : messages.begin(), MF_MESSAGE(h,
     * f, data, len)); */
    /* contruct inside the container instead of copying it from temporary one */
    messages.emplace(push ? messages.end() : messages.begin(), h, f, data, len);
    freeMutex(messagesMutex);
  } else
    return MF_FULL; // we are not full, but mutex locked is not an error
#ifndef STM32_CORE
// printf("Really pushed, become %lu\n", messages.size());
#endif
  return MF_OK;
}

uint8_t mfProto::pushMessage(MF_MESSAGE &message) {
  return addMessage(message, MFPROTO_PUSH);
}

uint8_t mfProto::insertMessage(MF_MESSAGE &message) {
  return addMessage(message, MFPROTO_INSERT);
}

uint8_t mfProto::indexById(uint32_t id) {
  if (messages.size() > 0) {
    for (uint8_t m = 0; m < messages.size(); m++) {
      if (messages[m].head.id == id) {
        return m;
      }
    }
  }
  return MF_QUEUE_LEN; // error
}
void mfProto::readFirstMessage(MF_MESSAGE &mess) { readMessageIndex(mess, 0); }
void mfProto::readLastMessage(MF_MESSAGE &mess) {
  readMessageIndex(mess, messages.size() - 1);
}

/*inline*/ void mfProto::_readMessageIndex(MF_MESSAGE &mess, uint8_t index) {
  /*#ifdef MF_RTOS
    if (irqSet) {
      _primask.store(taskENTER_CRITICAL_FROM_ISR());
    } else {
      taskENTER_CRITICAL();
    }
  #endif*/
  /* remove expired messages */
  uint32_t ticks = TICKS();
  while ((messages.size() > 0) &&
         (ticks - messages.begin()->time > MFPROTO_EXPIRE)) {
    _deleteMessageIndex(0);
  }
  /* and read finally */

  if (index < (uint8_t)messages.size()) {
    mess = *(messages.begin() + index);
  } else {
    mess.head.id = 0;
  }

  /*#ifdef MF_RTOS
    if (irqSet) {
      taskEXIT_CRITICAL_FROM_ISR(_primask.load());
    } else {
      taskEXIT_CRITICAL();
    }
  #endif*/
}

void mfProto::readMessageIndex(MF_MESSAGE &mess, uint8_t index) {
  if (getMutex(messagesMutex)) {
    _readMessageIndex(mess, index);
    if (mess.head.id)
      _deleteMessageIndex(index);
    freeMutex(messagesMutex);
  }
}

void mfProto::readMessageId(MF_MESSAGE &mess, uint32_t id) {
  if (getMutex(messagesMutex)) {
    uint8_t index = indexById(id);
    _readMessageIndex(mess, index);
    if (mess.head.id)
      _deleteMessageIndex(index);
    freeMutex(messagesMutex);
  }
}

void mfProto::deleteLastMessage(void) {
  if (getMutex(messagesMutex)) {
    _deleteMessageIndex(messages.size() - 1);
    freeMutex(messagesMutex);
  }
}
void mfProto::deleteFirstMessage(void) {
  if (getMutex(messagesMutex)) {
    _deleteMessageIndex(0);
    freeMutex(messagesMutex);
  }
}
void mfProto::clear(void) {
  if (getMutex(messagesMutex)) {
    messages.clear();
    freeMutex(messagesMutex);
  }
}
/*inline*/ void mfProto::_deleteMessageIndex(uint8_t index) {
  if (index < messages.size())
    messages.erase(messages.begin() + index);
}

void mfProto::deleteMessageIndex(uint8_t index) {
  if (getMutex(messagesMutex)) {
    _deleteMessageIndex(index);
    freeMutex(messagesMutex);
  }
}

void mfProto::deleteMessageId(uint32_t id) {
  if (getMutex(messagesMutex)) {
#if 1
    _deleteMessageIndex(indexById(id));
#else
    for (auto msgIt = messages.begin(); msgIt != messages.end(); ++msgIt) {
      if (msgIt->head.id == id) {
        messages.erase(msgIt);
        break;
      }
    }
#endif
    freeMutex(messagesMutex);
  }
}

void mfProto::putChar(uint8_t c) {
#ifdef STM32_CORE
// Serial.printf("%02X", c);
#else
  // printf("%02X", c);
#endif
  if (buf.size() < MFPROTO_BUFSIZE) {
    if (getMutex(bufferMutex,
                 (irqSet) ? MFPROTO_MUTEX_WAIT
                          : MFPROTO_MUTEX_WAIT)) { // lock buffer processing
      if (messages.size() < MF_QUEUE_LEN) {
#ifndef USE_VECTOR_BUFFER
        buf.push(c);
#else
        buf.push_back(c);
#endif
        processBuffer();
      } else { // queue is full, drop all data
        if (buf.size())
          buf.clear();
      }
      freeMutex(bufferMutex);
    }
  }
}
void mfProto::processBuffer(void) {
  while (buf.size()) {
    if (buf.size() >= sizeof(MF_HEADER) + sizeof(MF_FOOTER)) {
      if ((uint16_t)MF_MAGIC ==
          (uint16_t)(buf[1] << 8) + buf[0]) { // found magic word
        // buffer have enuff size to fit minimal packet size
        // copy/map buffer to the temporary header to analyze
        uint16_t dataSize = 0;
#ifndef USE_VECTOR_BUFFER
        MF_HEADER tempHead;
        for (uint16_t i = 0; i < sizeof(MF_HEADER); i++)
          ((uint8_t *)&tempHead)[i] = buf[i];
        dataSize = tempHead.size;
#else
        MF_HEADER *_tempHead = (MF_HEADER *)&buf[0];
        dataSize = _tempHead->size;
#endif
        if (dataSize > MFPROTO_MAXDATA) { // packet is too big or broken
#ifndef USE_VECTOR_BUFFER
          for (uint16_t i = 0; i < sizeof(MF_MAGIC); i++)
            buf.shift();
#else
          buf.erase(buf.begin(),
                    buf.begin() + sizeof(MF_MAGIC)); // drop current magic
#endif
          continue;
        }
        uint16_t headAndDataLen = sizeof(MF_HEADER) + dataSize;
        uint16_t fullLen = headAndDataLen + sizeof(MF_FOOTER);
        if (buf.size() >= fullLen) {
          /* seems like we have a full packet here */
          uint8_t packet[fullLen]; // temprorary packet storage
#ifndef USE_VECTOR_BUFFER
          /* move whole packet to temporary storage */
          for (uint16_t i = 0; i < fullLen; i++)
            packet[i] = buf.shift();
#else
          /* copy whole packet to temporary storage */
          for (uint16_t i = 0; i < fullLen; i++)
            packet[i] = buf[i];
          /* erase copied packet from buffer */
          buf.erase(buf.begin(), buf.begin() + fullLen);
#endif
          /* map temporary header and footer */
          MF_HEADER *h = (MF_HEADER *)&packet[0];
          MF_FOOTER *f = (MF_FOOTER *)&packet[headAndDataLen];
          /* check crc here */
#ifdef TEST_IGNORECRC
          crc_type crc = 0;
          mess.foot.crc = crc;
#else
          crc_type crc = _CRC((crc_type *)packet, headAndDataLen);
#endif
          /* still cant believe */
          if (crc == f->crc) { // bingo :)
            uint8_t counter = MFPROTO_MESSAGE_ADD_RETRIES;
            while (counter--) {
              uint8_t ret = addMessage(*h, *f, &packet[sizeof(MF_HEADER)],
                                       dataSize, MFPROTO_PUSH);
              if (ret == MF_OK) {
                if (recvCallback != NULL) {
                  recvCallback(this);
                }
                break;
              }
              if (ret == MF_ERR) // don't try again in case of error
                break;
              uint32_t delay = 1 + (MFPROTO_MUTEX_WAIT >> (irqSet ? 10 : 1));
              while (delay--) {
                __asm volatile("nop");
              }
            }
          }
        } else {
          break;
        }
      } else {
        // not a magic, drop this junk
#ifndef USE_VECTOR_BUFFER
        buf.shift();
#else
        buf.erase(buf.begin());
#endif
      }
    } else {
      break;
    }
  }
}
void mfProto::send(uint8_t *packet, uint16_t len, uint8_t flags) {
  // Serial.println(String(name) + " sending");
#ifdef MF_RTOS
//  taskENTER_CRITICAL();
#endif
  PHY.send(packet, len, flags);
#ifdef MF_RTOS
//  taskEXIT_CRITICAL();
#endif
}
void mfProto::poll(void) { PHY.read(); }
// void mfProto::sendMessage(MF_MESSAGE &msg) { sendMessage(&msg); }
void mfProto::sendMessage(MF_MESSAGE *msg) {
  sendMessage(msg, msg->data, msg->head.size);
}
void mfProto::sendMessage(MF_MESSAGE *msg, uint8_t *data, uint16_t size) {
  uint32_t len = msg->head.size;
  msg->head.size = size;
  if (!msg->head.id)
    return;
  if (size > MFPROTO_MAXDATA)
    return;
  // msg->head.magic = MF_MAGIC;
  if (getMutex(sendMutex)) {
#if 0
    uint8_t packet[sizeof(MF_HEADER) + sizeof(MF_FOOTER) + size];
    for (uint32_t i = 0; i < sizeof(MF_HEADER); i++) {
      packet[i] = ((uint8_t *)&msg->head)[i];
    }
    if (size) {
      for (uint32_t i = 0; i < size; i++)
        packet[sizeof(MF_HEADER) + i] = data[i];
    }
    MF_FOOTER *foot = (MF_FOOTER *)(packet + sizeof(MF_HEADER) + size);
    foot->crc = _CRC((crc_type *)packet, sizeof(MF_HEADER) + size);
    send(packet, sizeof(MF_HEADER) + sizeof(MF_FOOTER) + size, MFPHY_STARTEND);
#else
    crc_type crc = _CRC((crc_type *)&msg->head, sizeof(MF_HEADER));
    send((uint8_t *)&msg->head, sizeof(MF_HEADER), MFPHY_START);
    if (size > 0) {
      crc = _CRC(crc, (crc_type *)data, size);
      send(data, size, MFPHY_CONTINUE);
    }
    msg->foot.crc = crc;
    send((uint8_t *)&msg->foot, sizeof(MF_FOOTER), MFPHY_END);
#endif
    freeMutex(sendMutex);
  }
  msg->head.size = len;
}
void mfProto::sendMessage(uint8_t from, uint8_t to, uint16_t type, uint32_t id,
                          uint8_t *data, uint16_t size) {
  MF_MESSAGE mess;
  mess.head.from = from;
  mess.head.to = to;
  mess.head.type = type;
  mess.head.id = id;
  sendMessage(&mess, data, size);
}

uint32_t mfProto::getSpeed(void) { return PHY.getSpeed(); }

uint8_t mfProto::size(void) { return messages.size(); }
bool mfProto::isFull(void) { return (messages.size() >= MF_QUEUE_LEN); }
// destructor
mfProto::~mfProto() {
  instances.erase(_me);
  if (name != __NULL__) {
    free(name);
  }
}

void mfProto::setRecvCallback(recvCallback_f callback) {
  recvCallback = callback;
}
void mfProto::clearRecvCallback(void) { recvCallback = nullptr; }

/*bool test(atomic_bool &mutex) {
  atomic_bool tmp;
  tmp.store(mutex);
  mutex.store(true);
  return tmp;
}*/
bool mfProto::getMutex(atomic_bool &_mutex, uint32_t timeout) {
#if 1
  uint32_t end = timeout;
  if (!end)
    return true;
  while (_mutex.test_and_set()) {
    if (!end)
      return false;
    end--;
    __asm volatile("nop");
  }
#endif
  return true;
}
void mfProto::freeMutex(atomic_bool &_mutex) { _mutex.clear(); }

uint32_t mfProto::mallocCaptured = 0;
uint32_t mfProto::deleteCaptured = 0;