#include "lcd.h"
#include "gfxfont.h"
#include "stm32_def.h"
#include "stm32yyxx_ll_gpio.h"

#include <SPI.h>

#define __builtin_bswap16(a) (a)
LCD::LCD(SPIClass &SPI_instance, uint16_t _cmd, uint16_t _rst, uint8_t _dir)
    : spi(SPI_instance), direction(_dir), cmdPin(_cmd),
      cmdPort(get_GPIO_Port(STM_PORT(digitalPinToPinName(_cmd)))),
      cmdLLPin(STM_LL_GPIO_PIN(digitalPinToPinName(_cmd))), resetPin(_rst),
      resetPort(get_GPIO_Port(STM_PORT(digitalPinToPinName(_rst)))),
      resetLLPin(STM_LL_GPIO_PIN(digitalPinToPinName(_rst))) {

  /* init reset pin */
  set_GPIO_Port_Clock(STM_PORT(digitalPinToPinName(resetPin)));
  LL_GPIO_SetPinMode(resetPort, resetLLPin, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinOutputType(resetPort, resetLLPin, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(resetPort, resetLLPin, LL_GPIO_PULL_NO);
  LL_GPIO_SetPinSpeed(resetPort, resetLLPin, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_ResetOutputPin(resetPort, resetLLPin);
  /* init cmd pin */
  set_GPIO_Port_Clock(STM_PORT(digitalPinToPinName(cmdPin)));
  LL_GPIO_SetPinMode(cmdPort, cmdLLPin, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinOutputType(cmdPort, cmdLLPin, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(cmdPort, cmdLLPin, LL_GPIO_PULL_NO);
  LL_GPIO_SetPinSpeed(cmdPort, cmdLLPin, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_ResetOutputPin(cmdPort, cmdLLPin);
}
void LCD::init(void) {
  spi.begin();
  SPISettings settings(-1 /* maximum available speed */, MSBFIRST, SPI_MODE0,
                       SPI_TRANSMITONLY);
  spi.beginTransaction(settings);
  initST7735R();
}

inline uint16_t LCD::isqrt(uint32_t n) {
  uint16_t x = n;
  uint16_t y = 1;
  while (x > y) {
    x = (x + y) >> 1;
    y = n / x;
  }
  return x;
}
inline void LCD::setCommand(void) { LL_GPIO_ResetOutputPin(cmdPort, cmdLLPin); }
inline void LCD::setData(void) { LL_GPIO_SetOutputPin(cmdPort, cmdLLPin); }
void LCD::reset(void) {
  LL_GPIO_ResetOutputPin(resetPort, resetLLPin);
  delay(2);
  LL_GPIO_SetOutputPin(resetPort, resetLLPin);
  setCommand();
  uint8_t byte = 0x0;
  fastTransfer(&byte, 1);
  delay(2);
}
inline void LCD::writeReg(uint8_t data) {
  setCommand();
  fastTransfer(&data, 1);
}
inline void LCD::writeData(uint8_t data) {
  setData();
  fastTransfer(&data, 1);
}
inline void LCD::setReg(uint8_t reg, uint16_t value) {
  writeReg(reg);
  writeData(value);
}
/*
void LCD::setDirectionILI9341(uint8_t dir) {
  direction = dir;
  width = (direction & 1) ? LCD_H : LCD_W;
  height = (direction & 1) ? LCD_W : LCD_H;
  uint8_t val;
  switch (direction) {
  case 0:
    val = 0x40 | 0x08; // 0 degree
    break;
  case 1:
    val = 0x20 | 0x08; // 90 degree
    break;
  case 2:
    val = 0x80 | 0x10 | 0x08; // 180 degree
    break;
  case 3:
    val = 0x40 | 0x80 | 0x10 | 0x20 | 0x08; // 270 degree
    break;
  }
  setReg(0x36, val);
}
*/
void LCD::setDirection(uint8_t dir) {
  direction = dir;
  switch (direction) {
  default:
  case 0:
    width = LCD_W;
    height = LCD_H;
    setReg(0x36, 0xD0);
    // BGR==1,MY==0,MX==0,MV==0
    break;
  case 1:
    width = LCD_H;
    height = LCD_W;
    setReg(0x36, 0xA0);
    // BGR==1,MY==1,MX==0,MV==1
    break;
  case 2:
    width = LCD_W;
    height = LCD_H;
    setReg(0x36, 0x00);
    // BGR==1,MY==0,MX==0,MV==0
    break;
  case 3:
    width = LCD_H;
    height = LCD_W;
    setReg(0x36, 0x60);
    // BGR==1,MY==1,MX==0,MV==1
    break;
  }
}
void LCD::setWindow(uint16_t startX, uint16_t startY, uint16_t endX,
                    uint16_t endY) {
  writeReg(setXCmd);
  writeData((startX >> 8) & 0xFF);
  writeData(startX & 0xFF);
  writeData((endX >> 8) & 0xFF);
  writeData(endX & 0xFF);

  writeReg(setYCmd);
  writeData((startY >> 8) & 0xFF);
  writeData(startY & 0xFF);
  writeData((endY >> 8) & 0xFF);
  writeData(endY & 0xFF);
}

void LCD::clear(uint16_t color) {
  uint16_t block[width];
  uint16_t swappedcolor = __builtin_bswap16(color);
  for (uint16_t i = 0; i < width; i++)
    block[i] = swappedcolor;
  setWindow(0, 0, width - 1, height - 1);
  writeReg(writeRamCmd);
  setData();
  for (int z = 0; z < height; z++) {
    fastTransfer(block, width * sizeof(uint16_t));
  }
}
void LCD::point(uint16_t x, uint16_t y, uint16_t color) {
  setWindow(x, y, x, y);
  writeReg(writeRamCmd);
  setData();
  uint16_t c = __builtin_bswap16(color);
  fastTransfer(&c, sizeof(uint16_t));
}
void LCD::Hline(uint16_t x, uint16_t x2, uint16_t y, uint16_t color) {
  uint16_t *buf;
  uint16_t size = abs(x2 - x + 1) * sizeof(uint16_t);
  buf = (uint16_t *)malloc(size);
  for (uint32_t i = 0; i <= abs(x2 - x); i++)
    buf[i] = __builtin_bswap16(color);
  setWindow(x, y, x2, y);
  writeReg(writeRamCmd);
  setData();
  fastTransfer(buf, size);
  free(buf);
}
void LCD::Vline(uint16_t x, uint16_t y, uint16_t y2, uint16_t color) {
  uint16_t *buf;
  uint16_t size = abs(y2 - y + 1) * sizeof(uint16_t);
  buf = (uint16_t *)malloc(size);
  for (uint32_t i = 0; i <= abs(y2 - y); i++)
    buf[i] = __builtin_bswap16(color);
  setWindow(x, y, x, y2);
  writeReg(writeRamCmd);
  setData();
  fastTransfer(buf, size);
  free(buf);
}

void LCD::rectangle(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2,
                    uint16_t color) {
  Vline(x, y, y2, color);
  Vline(x2, y, y2, color);
  Hline(x, x2, y, color);
  Hline(x, x2, y2, color);
}
void LCD::box(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2,
              uint16_t color) {
  uint16_t *buf;
  uint16_t size = abs(x2 - x) * sizeof(uint16_t);
  buf = (uint16_t *)malloc(size);
  setWindow(x, y, x2, y2);
  writeReg(writeRamCmd);
  setData();
  for (uint32_t i = y; i < y2; i++) {
    for (uint32_t z = 0; z < abs(x2 - x); z++)
      buf[z] = __builtin_bswap16(color);
    fastTransfer(buf, size);
  }
  free(buf);
}
void LCD::boxx(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  uint16_t *buf;
  uint16_t size = w * sizeof(uint16_t);
  buf = (uint16_t *)malloc(size);
  for (uint32_t z = 0; z < w; z++)
    buf[z] = __builtin_bswap16(color);
  setWindow(x, y, x + w - 1, y + w - 1);
  writeReg(writeRamCmd);
  setData();
  for (uint32_t i = y; i < y + h; i++) {
    fastTransfer(buf, size);
  }
  free(buf);
}
__always_inline void LCD::fastTransfer(void *buffer, uint32_t size) {
  uint8_t *p = (uint8_t *)buffer;
#ifdef USE_REGS
  SPI_TypeDef *_SPI = spi._spi.handle.Instance;
  for (uint32_t i = 0; i < size; i++) {
    _SPI->DR = *p++;
    while ((_SPI->SR & SPI_SR_TXE) == RESET)
      ; // by design we should wait for TXE _before_ transfer, but for better
        // timings its moved to here
  }
#else
  spi.transfer(p, size);
#endif
}

void LCD::beforeBufferWrite(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  setWindow(x, y, x + w - 1, y + h - 1);
  writeReg(writeRamCmd);
  setData();
}

void LCD::drawBuffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                     void *buf) {
  setWindow(x, y, x + w - 1, y + h - 1);
  writeReg(writeRamCmd);
  setData();
  fastTransfer(buf, w * h * sizeof(uint16_t));
}

void LCD::drawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                     void *buf) {
  uint16_t size = w * sizeof(uint16_t);
  uint16_t *mapped = (uint16_t *)buf;
  uint16_t *buf2 = (uint16_t *)malloc(size);
  setWindow(x, y, x + w - 1, y + h - 1);
  writeReg(writeRamCmd);
  setData();
  for (uint32_t z = 0; z < h; z++) {
    for (uint32_t i = 0; i < w; i++)
      buf2[i] = __builtin_bswap16(mapped[z * w + i]);
    fastTransfer(buf2, size);
  }
  free(buf2);
}

void LCD::drawBitmapFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         void *buf) {
  uint16_t color;
  uint16_t *colors;
  uint32_t n = w * h;
  uint16_t *bl = (uint16_t *)buf;
  if (!(colors = (uint16_t *)malloc(n * 2)))
    return;
  setWindow(x, y, x + w - 1, y + h - 1);
  uint32_t c = 0;
  while (c < n) {
    color = (*bl++);
    colors[c++] = (color >> 8) | ((color & 0xff) << 8);
  }
  writeReg(writeRamCmd);
  setData();
  fastTransfer(colors, n * 2);
  free(colors);
}

void LCD::line(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2,
               uint16_t color) {
  bool yLonger = false;
  int incrementVal;

  int shortLen = y2 - y;
  int longLen = x2 - x;
  if (abs(shortLen) > abs(longLen)) {
    int swap = shortLen;
    shortLen = longLen;
    longLen = swap;
    yLonger = true;
  }

  if (longLen < 0)
    incrementVal = -1;
  else
    incrementVal = 1;

  double divDiff;
  if (shortLen == 0)
    divDiff = longLen;
  else
    divDiff = (double)longLen / (double)shortLen;
  if (yLonger) {
    for (int i = 0; i != longLen; i += incrementVal) {
      point(x + (int)((double)i / divDiff), y + i, color);
    }
  } else {
    for (int i = 0; i != longLen; i += incrementVal) {
      point(x + i, y + (int)((double)i / divDiff), color);
    }
  }
}

void LCD::circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color) {
  Hline(x - r, x + r, y, color);
  for (uint16_t h = 1; h <= r; h++) {
    uint16_t l = isqrt(r * r - h * h);
    Hline(x - l, x + l, y + h, color);
    Hline(x - l, x + l, y - h, color);
  }
}
void LCD::round(uint16_t x, uint16_t y, uint16_t r, uint16_t color) {
  uint16_t pl = r;
  point(x - r, y, color);
  point(x + r, y, color);
  for (uint16_t h = 1; h <= r; h++) {
    uint16_t l = isqrt(r * r - h * h);
    Hline(x - pl, x - l, y + h, color);
    Hline(x + l, x + pl, y + h, color);
    Hline(x - pl, x - l, y - h, color);
    Hline(x + l, x + pl, y - h, color);
    pl = l;
    // delay(300);
  }
}

void LCD::drawCharFast(uint16_t x, uint16_t y, unsigned char c, uint16_t color,
                       uint16_t bg) {

  c -= (uint8_t)gfxFont->first;
  GFXglyph *glyph = gfxFont->glyph + c;
  uint8_t *bitmap = gfxFont->bitmap;

  uint16_t bo = glyph->bitmapOffset;
  uint8_t w = glyph->width, h = glyph->height;
  int8_t xo = glyph->xOffset, yo = glyph->yOffset;
  uint8_t xx, yy, bits = 0, bit = 0;

  uint16_t *charBuf = (uint16_t *)malloc(w * h * sizeof(uint16_t));

  for (yy = 0; yy < h; yy++) {
    for (xx = 0; xx < w; xx++) {
      if (!(bit++ & 7)) {
        bits = bitmap[bo++];
      }
      charBuf[yy * w + xx] = __builtin_bswap16(bits & 0x80 ? color : bg);
      bits <<= 1;
    }
  }
  setWindow(x + xo, y + yo, x + xo + w - 1, y + yo + h - 1);
  writeReg(writeRamCmd);
  setData();
  fastTransfer(charBuf, w * h * sizeof(uint16_t));
  free(charBuf);
}

void LCD::drawChar(uint16_t x, uint16_t y, unsigned char c, uint16_t color,
                   uint16_t bg, uint8_t size_x, uint8_t size_y) {

  c -= (uint8_t)gfxFont->first;
  GFXglyph *glyph = gfxFont->glyph + c;
  uint8_t *bitmap = gfxFont->bitmap;

  uint16_t bo = glyph->bitmapOffset;
  uint8_t w = glyph->width, h = glyph->height;
  int8_t xo = glyph->xOffset, yo = glyph->yOffset;
  uint8_t xx, yy, bits = 0, bit = 0;
  int16_t xo16 = 0, yo16 = 0;

  if (size_x > 1 || size_y > 1) {
    xo16 = xo;
    yo16 = yo;
  }

  for (yy = 0; yy < h; yy++) {
    for (xx = 0; xx < w; xx++) {
      if (!(bit++ & 7)) {
        bits = bitmap[bo++];
      }
      if (bits & 0x80) {
        if (size_x == 1 && size_y == 1) {
          point(x + xo + xx, y + yo + yy, color);
        } else {
          boxx(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y, size_x,
               size_y, color);
        }
      } else {
        if (bg != LCD_TRANSPARENT) {
          if (size_x == 1 && size_y == 1) {
            point(x + xo + xx, y + yo + yy, bg);
          } else {
            boxx(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y, size_x,
                 size_y, bg);
          }
        }
      }
      bits <<= 1;
    }
  }
}
void LCD::setFont(const GFXfont *font) { gfxFont = (GFXfont *)font; }

uint16_t LCD::printWidth(String &s) {
  uint16_t width = 0;
  for (uint16_t c = 0; c < s.length(); c++) {
    char curChar = s.charAt(c);
    GFXglyph *glyph = gfxFont->glyph + curChar;
    width += glyph->xAdvance;
  }
  return width;
}
uint16_t LCD::printHeight(void) { return gfxFont->yAdvance; }

void LCD::print(uint16_t x, uint16_t y, String &s, uint16_t color,
                uint16_t bg) {
  uint16_t pos = x;
  for (uint16_t c = 0; c < s.length(); c++) {
    char curChar = s.charAt(c);
    drawChar(pos, y, curChar, color, bg, 1, 1);
    GFXglyph *glyph = gfxFont->glyph + curChar;
    pos += glyph->xAdvance;
  }
}
void LCD::printFast(uint16_t x, uint16_t y, String &s, uint16_t color,
                    uint16_t bg) {
  uint16_t pos = x;
  for (uint16_t c = 0; c < s.length(); c++) {
    char curChar = s.charAt(c);
    drawCharFast(pos, y, curChar, color, bg);
    GFXglyph *glyph = gfxFont->glyph + curChar;
    pos += glyph->xAdvance;
  }
}

#define SEQ(a, b, ...)                                                         \
  do {                                                                         \
    writeReg((a));                                                             \
    static uint8_t list[(b)] = {__VA_ARGS__};                                  \
    for (uint8_t __i = 0; __i < (b); __i++) {                                  \
      writeData(list[__i]);                                                    \
    }                                                                          \
  } while (0)

void LCD::initST7735R(void) {
  reset();
  writeReg(0x11); // Sleep exit
  delay(120);
  // *************************
  SEQ(0xB1, 3, 0x05, 0x3C, 0x3C);
  SEQ(0xB2, 3, 0x05, 0x3C, 0x3C);
  SEQ(0xB3, 6, 0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C);
  SEQ(0xB4, 1, 0x03);
  SEQ(0xC0, 3, 0x28, 0x08, 0x04);
  SEQ(0xC1, 1, 0xC0);
  SEQ(0xC2, 2, 0x0D, 0x00);
  SEQ(0xC3, 2, 0x8D, 0x2A);
  SEQ(0xC4, 2, 0x8D, 0xEE);
  SEQ(0xC5, 1, 0x1A);
  SEQ(0x17, 1, 0x05);
  SEQ(0x36, 1, 0x08);
  SEQ(0xE0, 16, 0x2, 0x1c, 0x7, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2b,
      0x39, 0x0, 0x1, 0x3, 0x10);
  SEQ(0xE1, 16, 0x3, 0x1d, 0x7, 0x6, 0x2e, 0x2c, 0x29, 0x2d, 0x2e, 0x2e, 0x37,
      0x3F, 0x0, 0x0, 0x2, 0x10);
  SEQ(0x3A, 1, 0x05);
  writeReg(0x29);
  setDirection(direction);
  clear(0x0);
}

void LCD::b_clear(uint16_t color) {
  for (uint32_t z = 0; z < width * height; z++) {
    buffer[z] = __builtin_bswap16(color);
  }
}
#define __CUT_W(x)                                                             \
  do {                                                                         \
    if ((x) >= width)                                                          \
      (x) = width - 1;                                                         \
  } while (0)
#define __CUT_H(y)                                                             \
  do {                                                                         \
    if ((y) >= height)                                                         \
      (y) = height - 1;                                                        \
  } while (0)

void LCD::b_point(uint16_t x, uint16_t y, uint16_t color) {
  if ((x < width) && (y < height))
    buffer[y * width + x] = __builtin_bswap16(color);
}

void LCD::b_Hline(uint16_t x, uint16_t x2, uint16_t y, uint16_t color) {
  __CUT_W(x);
  __CUT_W(x2);
  __CUT_H(y);
  uint16_t minx = min(x, x2);
  for (uint32_t i = 0; i <= abs(x2 - x); i++)
    buffer[y * width + (i + minx)] = __builtin_bswap16(color);
}
void LCD::b_Vline(uint16_t x, uint16_t y, uint16_t y2, uint16_t color) {
  __CUT_W(x);
  __CUT_H(y);
  __CUT_H(y2);
  uint16_t miny = min(y, y2);
  for (uint32_t i = 0; i <= abs(y2 - y); i++)
    buffer[(miny + i) * width + x] = __builtin_bswap16(color);
}

void LCD::b_rectangle(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2,
                      uint16_t color) {
  b_Vline(x, y, y2, color);
  b_Vline(x2, y, y2, color);
  b_Hline(x, x2, y, color);
  b_Hline(x, x2, y2, color);
}
void LCD::b_box(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2,
                uint16_t color) {
  uint16_t miny = min(y, y2);
  for (uint32_t i = 0; i <= abs(y2 - y); i++)
    b_Hline(x, x2, i + miny, color);
}

void LCD::b_boxx(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                 uint16_t color) {
  b_box(x, y, x + w, y + h, color);
}

void LCD::b_drawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       void *buf) {
  uint16_t *mapped = (uint16_t *)buf;
  for (uint32_t z = 0; z < h; z++) {
    for (uint32_t i = 0; i < w; i++) {
      // buffer[((y + z) * width) + (x + i)] =
      //     __builtin_bswap16(mapped[z * w + i]);
      b_point(x + i, y + z, mapped[z * w + i]);
    }
  }
}

void LCD::b_line(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2,
                 uint16_t color) {
  bool yLonger = false;
  int incrementVal;

  int shortLen = y2 - y;
  int longLen = x2 - x;
  if (abs(shortLen) > abs(longLen)) {
    int swap = shortLen;
    shortLen = longLen;
    longLen = swap;
    yLonger = true;
  }

  if (longLen < 0)
    incrementVal = -1;
  else
    incrementVal = 1;

  double divDiff;
  if (shortLen == 0)
    divDiff = longLen;
  else
    divDiff = (double)longLen / (double)shortLen;
  if (yLonger) {
    for (int i = 0; i != longLen; i += incrementVal) {
      b_point(x + (int)((double)i / divDiff), y + i, color);
    }
  } else {
    for (int i = 0; i != longLen; i += incrementVal) {
      b_point(x + i, y + (int)((double)i / divDiff), color);
    }
  }
}

void LCD::b_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color) {
  b_Hline(x - r, x + r, y, color);
  for (uint16_t h = 1; h <= r; h++) {
    uint16_t l = isqrt(r * r - h * h);
    b_Hline(x - l, x + l, y + h, color);
    b_Hline(x - l, x + l, y - h, color);
  }
}
void LCD::b_round(uint16_t x, uint16_t y, uint16_t r, uint16_t color) {
  uint16_t pl = r;
  b_point(x - r, y, color);
  b_point(x + r, y, color);
  for (uint16_t h = 1; h <= r; h++) {
    uint16_t l = isqrt(r * r - h * h);
    b_Hline(x - pl, x - l, y + h, color);
    b_Hline(x + l, x + pl, y + h, color);
    b_Hline(x - pl, x - l, y - h, color);
    b_Hline(x + l, x + pl, y - h, color);
    pl = l;
    // delay(300);
  }
}

void LCD::b_drawChar(uint16_t x, uint16_t y, unsigned char c, uint16_t color,
                     uint16_t bg, uint8_t size_x, uint8_t size_y) {

  c -= (uint8_t)gfxFont->first;
  // (((c >= 128) && (c <= 143)) ? c + 64 : c)
  GFXglyph *glyph = gfxFont->glyph + c;
  uint8_t *bitmap = gfxFont->bitmap;

  uint16_t bo = glyph->bitmapOffset;
  uint8_t w = glyph->width, h = glyph->height;
  int8_t xo = glyph->xOffset, yo = glyph->yOffset;
  uint8_t xx, yy, bits = 0, bit = 0;
  int16_t xo16 = 0, yo16 = 0;

  if (size_x > 1 || size_y > 1) {
    xo16 = xo;
    yo16 = yo;
  }

  for (yy = 0; yy < h; yy++) {
    for (xx = 0; xx < w; xx++) {
      if (!(bit++ & 7)) {
        bits = bitmap[bo++];
      }
      if (bits & 0x80) {
        if (size_x == 1 && size_y == 1) {
          b_point(x + xo + xx, y + yo + yy, color);
        } else {
          b_boxx(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y, size_x,
                 size_y, color);
        }
      } else {
        if (bg != LCD_TRANSPARENT) {
          if (size_x == 1 && size_y == 1) {
            b_point(x + xo + xx, y + yo + yy, bg);
          } else {
            b_boxx(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y, size_x,
                   size_y, bg);
          }
        }
      }
      bits <<= 1;
    }
  }
}

/*void LCD::b_print(uint16_t x, uint16_t y, String s, uint16_t color,
                  uint16_t bg) {
                      uint16_t pos = x;
  for (uint16_t c = 0; c < s.length(); c++) {
    char curChar = s.charAt(c);
    b_drawChar(pos, y, curChar, color, bg, 1, 1);
    GFXglyph *glyph = gfxFont->glyph + curChar;
    pos += glyph->xAdvance;
  }
}*/
void LCD::b_print(uint16_t x, uint16_t y, String &s, uint16_t color,
                  uint16_t bg, uint8_t scale) {
  uint16_t pos = x;
  for (uint16_t c = 0; c < s.length(); c++) {
    char curChar = s.charAt(c);
    if (curChar == '\n') {
      pos = x;
      y += gfxFont->yAdvance;
      continue;
    }
    b_drawChar(pos, y, curChar, color, bg, scale, scale);
    GFXglyph *glyph = gfxFont->glyph + curChar - gfxFont->first;
    pos += glyph->xAdvance * scale;
  }
}

void LCD::b_printRight(uint16_t x, uint16_t y, String &s, uint16_t color,
                       uint16_t bg, uint8_t scale) {
  uint16_t pos = x - LCD::printWidth(s) * scale;
  for (uint16_t c = 0; c < s.length(); c++) {
    char curChar = s.charAt(c);
    b_drawChar(pos, y, curChar, color, bg, scale, scale);
    GFXglyph *glyph = gfxFont->glyph + curChar - gfxFont->first;
    pos += glyph->xAdvance * scale;
  }
}

void LCD::b_printCenter(uint16_t x, uint16_t w, uint16_t y, String &s,
                        uint16_t color, uint16_t bg, uint8_t scale) {
  uint16_t pos = x + ((w - LCD::printWidth(s) * scale) / 2);
  for (uint16_t c = 0; c < s.length(); c++) {
    char curChar = s.charAt(c);
    b_drawChar(pos, y, curChar, color, bg, scale, scale);
    GFXglyph *glyph = gfxFont->glyph + curChar - gfxFont->first;
    pos += glyph->xAdvance * scale;
  }
}

MFLCD_FONTDATA LCD::getFontData(void) {
  MFLCD_FONTDATA data;
  data.yAdvance = gfxFont->yAdvance;
  GFXglyph *glyph = gfxFont->glyph;
  data.xAdvance = glyph->xAdvance;
  data.xOffset = glyph->xOffset;
  data.yOffset = glyph->yOffset;
  return data;
}

LCD::~LCD() {}
