#ifndef _LCD_H
#define _LCD_H

#include "gfxfont.h"
#include "mflcd_data.h"
#include <SPI.h>

#define LCD_W 128
#define LCD_H 160
#define LCD_TRANSPARENT 0xFFFF
#define USE_REGS // use direct registry access instead of builtin transfer
// routine, but it requires a hack of SPI library to access
// internal _spi structure

#define RGB(r, g, b)                                                           \
  (uint16_t)((((b)) & 0x1F) | ((((g)) & 0x3F) << 5) |                          \
             ((((r)) & 0x1F) << 11)) &                                         \
      0xFFFF
#define RGB8(r, g, b)                                                          \
  (uint16_t)(((((b)) & 0xF8) >> 2) | ((((g)) & 0xFC) << 3) |                   \
             ((((r)) & 0xF8) << 8)) &                                          \
      0xFFFF

class LCD {
private:
  SPIClass &spi;
  const uint16_t writeRamCmd = 0x2C;
  const uint16_t setXCmd = 0x2A;
  const uint16_t setYCmd = 0x2B;
  uint8_t direction = 0;

  uint16_t cmdPin;
  GPIO_TypeDef *cmdPort;
  uint32_t cmdLLPin;

  uint16_t resetPin;
  GPIO_TypeDef *resetPort;
  uint32_t resetLLPin;

  GFXfont *gfxFont;
  void reset();
  static uint16_t isqrt(uint32_t n);
  void initST7735R(void);
  void fastTransfer(void *buffer, uint32_t size);

public:
  uint16_t width = LCD_W;
  uint16_t height = LCD_H;
  // 40960 bytes for 160*128*16bit, f401cc can not fit
  // f401ce with 96K sram preffered
  volatile uint16_t buffer[LCD_W * LCD_H] = {0};

  LCD(SPIClass &SPI_instance, uint16_t _cmd, uint16_t _rst, uint8_t _dir = 0);

  void writeReg(uint8_t data);
  void writeData(uint8_t data);
  void setReg(uint8_t reg, uint16_t value);
  void setCommand(void);
  void setData(void);
  void setDirection(uint8_t dir);
  void setWindow(uint16_t startX, uint16_t startY, uint16_t endX,
                 uint16_t endY);
  void clear(uint16_t color);
  void point(uint16_t x, uint16_t y, uint16_t color);
  void line(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color);
  void Hline(uint16_t x, uint16_t x2, uint16_t y, uint16_t color);
  void Vline(uint16_t x, uint16_t y, uint16_t y2, uint16_t color);
  void rectangle(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2,
                 uint16_t color);
  void box(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color);
  void boxx(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
  void drawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, void *buf);
  void drawBitmapFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                      void *buf);
  void drawBuffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h, void *buf);
  void circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
  void round(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
  void drawChar(uint16_t x, uint16_t y, unsigned char c, uint16_t color,
                uint16_t bg, uint8_t size_x, uint8_t size_y);
  void drawCharFast(uint16_t x, uint16_t y, unsigned char c, uint16_t color,
                    uint16_t bg);
  void setFont(const GFXfont *font);
  void print(uint16_t x, uint16_t y, String &s, uint16_t color, uint16_t bg);
  void printFast(uint16_t x, uint16_t y, String &s, uint16_t color,
                 uint16_t bg);
  uint16_t printHeight(void);
  uint16_t printWidth(String &s);

  void b_clear(uint16_t color);
  void b_point(uint16_t x, uint16_t y, uint16_t color);
  void b_line(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color);
  void b_Hline(uint16_t x, uint16_t x2, uint16_t y, uint16_t color);
  void b_Vline(uint16_t x, uint16_t y, uint16_t y2, uint16_t color);
  void b_rectangle(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2,
                   uint16_t color);
  void b_box(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t color);
  void b_boxx(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
  void b_drawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, void *buf);
  void b_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
  void b_round(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
  void b_drawChar(uint16_t x, uint16_t y, unsigned char c, uint16_t color,
                  uint16_t bg, uint8_t size_x, uint8_t size_y);
  void b_print(uint16_t x, uint16_t y, String &s, uint16_t color, uint16_t bg,
               uint8_t scale = 1);
  void b_printRight(uint16_t x, uint16_t y, String &s, uint16_t color,
                    uint16_t bg, uint8_t scale = 1);
  void b_printCenter(uint16_t x, uint16_t w, uint16_t y, String &s,
                     uint16_t color, uint16_t bg, uint8_t scale = 1);
  void beforeBufferWrite(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

  void init(void);
  MFLCD_FONTDATA getFontData(void);
  ~LCD();
};

#endif
