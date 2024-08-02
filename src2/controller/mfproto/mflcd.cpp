#include "mflcd.h"

mfLCD::mfLCD(const char *_name, mfBus &_bus, uint8_t _device)
    : bus(_bus), device(_device) {
  instances.push_back(this);
  me = instances.end();
  name = (char *)malloc(strlen(_name) + 1);
  if (name)
    strcpy(name, _name);
}

mfLCD::~mfLCD() {
  instances.erase(me);
  if (name)
    free(name);
}

void mfLCD::begin(void) { getLCDsize(); }
void mfLCD::getLCDsize(void) {
  MFLCD_SIZE *answer;
  uint32_t id = bus.sendMessageWait(device, MFLCD_SIZECMD, NULL, 0);
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if (msg.head.id && (msg.head.size == sizeof(MFLCD_SIZE))) {
      answer = (MFLCD_SIZE *)msg.data;
      height = answer->h;
      width = answer->w;
    }
  }
}

void mfLCD::clear(uint16_t color) {
  MFLCD_CLEAR cmd = {.color = color};
  uint32_t id =
      bus.sendMessageWait(device, MFLCD_CLEARCMD, (uint8_t *)&cmd, sizeof(cmd));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::point(uint16_t x, uint16_t y, uint16_t color) {
  MFLCD_POINT cmd = {.x = x, .y = y, .color = color};
  uint32_t id = bus.sendMessageWait(device, MFLCD_POINTCMD, (uint8_t *)&cmd,
                                    sizeof(MFLCD_POINT));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::line(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2,
                 uint16_t color) {
  MFLCD_LINE cmd = {.x = x, .y = y, .x2 = x2, .y2 = y2, .color = color};
  uint32_t id = bus.sendMessageWait(device, MFLCD_LINECMD, (uint8_t *)&cmd,
                                    sizeof(MFLCD_LINE));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::Hline(uint16_t x, uint16_t x2, uint16_t y, uint16_t color) {
  MFLCD_HLINE cmd = {.x = x, .x2 = x2, .y = y, .color = color};
  uint32_t id = bus.sendMessageWait(device, MFLCD_HLINECMD, (uint8_t *)&cmd,
                                    sizeof(MFLCD_HLINE));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::Vline(uint16_t x, uint16_t y, uint16_t y2, uint16_t color) {
  MFLCD_VLINE cmd = {.x = x, .y = y, .y2 = y2, .color = color};
  uint32_t id = bus.sendMessageWait(device, MFLCD_VLINECMD, (uint8_t *)&cmd,
                                    sizeof(MFLCD_VLINE));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::rect(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2,
                 uint16_t color) {
  MFLCD_RECT cmd = {.x = x, .y = y, .x2 = x2, .y2 = y2, .color = color};
  uint32_t id = bus.sendMessageWait(device, MFLCD_RECTCMD, (uint8_t *)&cmd,
                                    sizeof(MFLCD_RECT));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::box(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2,
                uint16_t color) {
  MFLCD_BOX cmd = {.x = x, .y = y, .x2 = x2, .y2 = y2, .color = color};
  uint32_t id = bus.sendMessageWait(device, MFLCD_BOXCMD, (uint8_t *)&cmd,
                                    sizeof(MFLCD_BOX));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::boxwh(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  uint16_t color) {
  MFLCD_BOXWH cmd = {.x = x, .y = y, .w = w, .h = h, .color = color};
  uint32_t id = bus.sendMessageWait(device, MFLCD_BOXWHCMD, (uint8_t *)&cmd,
                                    sizeof(MFLCD_BOXWH));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::drawBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t s,
                    int32_t value) {
  MFLCD_DRAWBAR cmd = {
      .x = x, .y = y, .w = w, .h = h, .s = s, .value = value};
  uint32_t id = bus.sendMessageWait(device, MFLCD_DRAWBARCMD, (uint8_t *)&cmd,
                                    sizeof(MFLCD_DRAWBAR));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::print(uint16_t x, uint16_t y, String text, uint16_t color,
                  uint16_t back, uint8_t scale) {
  MFLCD_PRINT cmdp;
  cmdp.x = x;
  cmdp.y = y;
  cmdp.scale = scale;
  cmdp.bg = back;
  cmdp.color = color;

  uint32_t buflen = sizeof(MFLCD_PRINT) + text.length() + 1;
  uint8_t tempBuf[buflen];
  memcpy(tempBuf, &cmdp, sizeof(MFLCD_PRINT));
  memcpy(tempBuf + sizeof(MFLCD_PRINT), text.c_str(), text.length() + 1);

  uint32_t id = bus.sendMessageWait(device, MFLCD_PRINTCMD, tempBuf, buflen);
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::printR(uint16_t x, uint16_t y, String text, uint16_t color,
                   uint16_t back, uint8_t scale) {
  MFLCD_PRINT cmdp;
  cmdp.x = x;
  cmdp.y = y;
  cmdp.scale = scale;
  cmdp.bg = back;
  cmdp.color = color;

  uint32_t buflen = sizeof(MFLCD_PRINT) + text.length() + 1;
  uint8_t tempBuf[buflen];
  memcpy(tempBuf, &cmdp, sizeof(MFLCD_PRINT));
  memcpy(tempBuf + sizeof(MFLCD_PRINT), text.c_str(), text.length() + 1);

  uint32_t id =
      bus.sendMessageWait(device, MFLCD_PRINTRIGHTCMD, tempBuf, buflen);
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::printC(uint16_t x, uint16_t w, uint16_t y, String text,
                   uint16_t color, uint16_t back, uint8_t scale) {
  MFLCD_PRINTCENTER cmdp;
  cmdp.x = x;
  cmdp.w = w;
  cmdp.y = y;
  cmdp.scale = scale;
  cmdp.bg = back;
  cmdp.color = color;

  uint32_t buflen = sizeof(MFLCD_PRINTCENTER) + text.length() + 1;
  uint8_t tempBuf[buflen];
  memcpy(tempBuf, &cmdp, sizeof(MFLCD_PRINTCENTER));
  memcpy(tempBuf + sizeof(MFLCD_PRINTCENTER), text.c_str(), text.length() + 1);

  uint32_t id =
      bus.sendMessageWait(device, MFLCD_PRINTCENTERCMD, tempBuf, buflen);
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                   uint8_t *bitmap) {
  MFLCD_BITMAP cmd;
  uint8_t *pic = bitmap;
  uint16_t linesPerPacket = ((MFPROTO_MAXDATA - sizeof(MFLCD_BITMAP)) / 2) / w;
  for (uint16_t i = 0; i < h; i += linesPerPacket) {
    cmd.x = x;
    cmd.y = y + i;
    cmd.w = w;
    cmd.h = (i + linesPerPacket > h) ? (h - i) : linesPerPacket;
    uint16_t packetSize = cmd.w * cmd.h * sizeof(uint16_t);
    uint8_t packet[sizeof(cmd) + packetSize];
    memcpy(packet, &cmd, sizeof(cmd));
    memcpy(packet + sizeof(cmd), pic, packetSize);
    pic += packetSize;
    uint32_t id = bus.sendMessageWait(
        device, MFLCD_BITMAPCMD, (uint8_t *)packet, sizeof(cmd) + packetSize);
    if (id)
      bus.deleteMessageId(id);
    YIELD();
  }
}
void mfLCD::image(uint16_t x, uint16_t y, String image) {
  MFLCD_PIC cmd;
  cmd.x = x;
  cmd.y = y;
  strncpy(cmd.name, image.c_str(), MF_LCD_IMAGE_NAME_LEN);
  uint32_t id = bus.sendMessageWait(device, MFLCD_PICCMD, (uint8_t *)&cmd,
                                    sizeof(MFLCD_PIC));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color) {
  MFLCD_CIRCLE cmd = {.x = x, .y = y, .r = r, .color = color};
  uint32_t id = bus.sendMessageWait(device, MFLCD_CIRCLECMD, (uint8_t *)&cmd,
                                    sizeof(MFLCD_CIRCLE));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::round(uint16_t x, uint16_t y, uint16_t r, uint16_t color) {
  MFLCD_ROUND cmd = {.x = x, .y = y, .r = r, .color = color};
  uint32_t id = bus.sendMessageWait(device, MFLCD_ROUNDCMD, (uint8_t *)&cmd,
                                    sizeof(MFLCD_ROUND));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::noRefresh(void) {
  uint32_t id = bus.sendMessageWait(device, MFLCD_REFRESHPAUSE, NULL, 0);
  if (id) {
    bus.deleteMessageId(id);
  }
}
void mfLCD::refresh(void) {
  uint32_t id = bus.sendMessageWait(device, MFLCD_REFRESHRESUME, NULL, 0);
  if (id) {
    bus.deleteMessageId(id);
  }
}
void mfLCD::setBrightness(uint8_t value) {
  MFLCD_BRIGHTNESS cmd = {.brightness = value};
  uint32_t id = bus.sendMessageWait(device, MFLCD_BRIGHTNESSCMD,
                                    (uint8_t *)&cmd, sizeof(MFLCD_BRIGHTNESS));
  if (id)
    bus.deleteMessageId(id);
}
void mfLCD::setFont(uint8_t font, MFLCD_FONTDATA *font_data) {
  MFLCD_FONT fontNum = {.num = font};
  uint32_t id = bus.sendMessageWait(device, MFLCD_SETFONTCMD,
                                    (uint8_t *)&fontNum, sizeof(MFLCD_FONT));
  if (id) {
    MF_MESSAGE msg;
    bus.readMessageId(msg, id);
    if ((msg.head.id) && (msg.head.size == sizeof(MFLCD_FONTDATA))) {
      memcpy(font_data, msg.data, msg.head.size);
    }
  }
}