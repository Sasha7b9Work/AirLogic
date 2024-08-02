#ifndef MFLCD_H
#define MFLCD_H

#include "mfbus.h"
#include "mflcd_data.h"
#include "mfproto.h"
#include <vector>
#include <wiring.h>
#define LCD_WHITE RGB(31, 62, 31)
#define LCD_BLACK RGB(0, 0, 0)
#define LCD_RED RGB(31, 0, 0)
#define LCD_BLUE RGB(0, 0, 31)
#define LCD_GREEN RGB(0, 63, 0)
#define LCD_YELLOW RGB(31, 31, 0)

#define RGB(r, g, b)                                                           \
  (uint16_t)(((b)&0x1F) | (((g)&0x3F) << 5) | (((r)&0x1F) << 11)) & 0xFFFF

class mfLCD {
private:
  mfBus &bus;
  uint8_t device;
  char name[32] = "";
  void getLCDsize(void);

public:
  uint16_t width = 160;
  uint16_t height = 128;
  inline static std::vector<mfLCD *> instances = {};

  void begin(void);
  void clear(uint16_t color);
  void point(uint16_t x, uint16_t y, uint16_t color);
  void line(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color);
  void Vline(uint16_t x, uint16_t y, uint16_t y2, uint16_t color);
  void Hline(uint16_t x, uint16_t x2, uint16_t y, uint16_t color);
  void rect(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color);
  void box(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color);
  void boxwh(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
  void print(uint16_t x, uint16_t y, String text, uint16_t color,
             uint16_t back = LCD_TRANSPARENT, uint8_t scale = 1);
  void printR(uint16_t x, uint16_t y, String text, uint16_t color,
              uint16_t back = LCD_TRANSPARENT, uint8_t scale = 1);
  void printC(uint16_t x, uint16_t x2, uint16_t y, String text, uint16_t color,
              uint16_t back = LCD_TRANSPARENT, uint8_t scale = 1);
  void bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t *bitmap);
  void image(uint16_t x, uint16_t y, String image);
  void circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
  void round(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
  void drawBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t s,
               int32_t value);
  void noRefresh(void);
  void refresh(void);
  void setFont(uint8_t font, MFLCD_FONTDATA *font_data);
  void setBrightness(uint8_t value);

  mfLCD(const char *_name, mfBus &_bus, uint8_t _device);
  ~mfLCD(void);
};

#define MFLCD(_a_, ...) mfLCD(_a_)(#_a_, __VA_ARGS__);
#endif