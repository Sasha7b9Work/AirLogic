#ifndef MFLCD_DATA
#define MFLCD_DATA
#include <mfbus.h>

#define LCD_TRANSPARENT 0xFFFF

enum lcdSignals {
  MFLCD_CLEARCMD = MFBUS_LCD_RESERVED,
  MFLCD_POINTCMD,
  MFLCD_LINECMD,
  MFLCD_HLINECMD,
  MFLCD_VLINECMD,
  MFLCD_BOXCMD,
  MFLCD_BOXWHCMD,
  MFLCD_RECTCMD,
  MFLCD_CIRCLECMD,
  MFLCD_ROUNDCMD,
  MFLCD_PRINTCMD,
  MFLCD_BITMAPCMD,
  MFLCD_PICCMD,
  MFLCD_SETFONTCMD,
  MFLCD_REFRESHPAUSE,
  MFLCD_REFRESHRESUME,
  MFLCD_GETFONTPARAMCMD,
  MFLCD_SIZECMD,
  MFLCD_BRIGHTNESSCMD,
  MFLCD_PRINTRIGHTCMD,
  MFLCD_PRINTCENTERCMD,
  MFLCD_DRAWBARCMD,
};
typedef struct {
  uint16_t color;
} MFLCD_CLEAR;

typedef struct {
  uint16_t x, y, color;
} MFLCD_POINT;

typedef struct {
  uint16_t x, y, x2, y2, color;
} MFLCD_LINE;

typedef struct {
  uint16_t x, x2, y, color;
} MFLCD_HLINE;

typedef struct {
  uint16_t x, y, y2, color;
} MFLCD_VLINE;

#define MFLCD_BOX MFLCD_LINE
#define MFLCD_RECT MFLCD_LINE

typedef struct {
  uint16_t x, y, w, h, color;
} MFLCD_BOXWH;

typedef struct {
  uint16_t x, y, w, h, s;
  int32_t value;
} MFLCD_DRAWBAR;

typedef struct {
  uint16_t x, y, w, h;
} MFLCD_BITMAP;

#define MF_LCD_IMAGE_NAME_LEN 20
typedef struct {
  uint16_t x, y;
  char name[MF_LCD_IMAGE_NAME_LEN + 1];
} MFLCD_PIC;

typedef struct {
  uint16_t x, y, r, color;
} MFLCD_CIRCLE;
#define MFLCD_ROUND MFLCD_CIRCLE

typedef struct {
  uint16_t x, y, color, bg, scale;
} MFLCD_PRINT;

typedef struct {
  uint16_t x, w, y, color, bg, scale;
} MFLCD_PRINTCENTER;

typedef struct {
  uint16_t num;
} MFLCD_FONT;

typedef struct {
  int8_t xOffset, yOffset;
  uint8_t xAdvance, yAdvance;
} MFLCD_FONTDATA;

typedef struct {
  uint16_t w, h;
} MFLCD_SIZE;

typedef struct {
  uint8_t brightness;
} MFLCD_BRIGHTNESS;
#endif