#include <sketch.h>

#include "gfxfont.h"
#include <stm32yyxx_ll_dma.h>
#include <stm32yyxx_ll_spi.h>

#include "font_DejaVuSans10.h"
#include "font_NotoSansBold10.h"
#include "font_NotoSansBold9.h"
#include "font_RobotoBold12.h"
#include "font_fixed6.h"
#include "font_fixedsys8.h"
#include "font_terminusbold12.h"

const GFXfont *MFLCD_availableFonts[]{&fixed66pt8b,        &fixedsys8pt8b,
                                      &NotoSans_Bold9pt8b, &NotoSans_Bold10pt8b,
                                      &DejaVuSans10pt8b,   &Roboto_Bold12pt8b,
                                      &terminus_bold12pt8b};

#include "images.h"
#include "mfbus.h"
#include "mfbusrgb.h"
#include "mfbutton.h"
#include "mflcd_data.h"
#include "mfproto.h"
#include "mfprotophy.h"
#include "mfprotophyserial.h"
#include "mfrgb.h"

MFBUTTON(UP, PB3, INPUT_PULLUP);
MFBUTTON(OK, PB4, INPUT_PULLUP);
MFBUTTON(DOWN, PB5, INPUT_PULLUP);

#define BAUD (MFPROTO_SERIAL_MAXBAUD / 10)
MFPROTOPHYSERIAL(SlavePhy, Serial1, NC, BAUD); // Serial physical layer
MFPROTO(SlaveProto, SlavePhy);                 // protocol instance
mfBus SlaveBus = mfBus("alpro_v10_lcd", SlaveProto, MF_ALPRO_V10_LCD,
                       MFBUS_CAP_DEBUG); // bus instance
#include "debug.h"
DebugSerial debug(SlaveBus);

// Prepare suitable timer

#define TIMx TIM1

#include <cPins.h>

#include <SPI.h>

#include "lcd.h"
LCD disp(SPI, PB0, PB10, 1);

#include <SPIFlash.h>
SPIClass SPI_2(PB15, PB14, PB13, PB12);
#define FLASH_SS PB12
SPIFlash flash(FLASH_SS, 0xEF40);

#define _CS PA4
#define CD PB0
#define RST PB10

#define _MOSI PA7
#define _MISO PA6
#define _SCLK PA5

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE RGB(31, 62, 31)
HardwareTimer refreshTimer(TIM2);
//#define debug Serial2
//#define bus Serial1

#if defined(__arm__)
extern "C" char *sbrk(int incr);
uint32_t FreeStack(void) {
  char top = 't';
  return &top - reinterpret_cast<char *>(sbrk(0));
}
#endif

CPIN(BL, PB8, CPIN_HWLED, CPIN_HIGH);
CPIN(GR, PB7, CPIN_HWLED, CPIN_HIGH);
CPIN(RD, PB6, CPIN_HWLED, CPIN_HIGH);
MFRGB(ledOK, &RD, &GR, &BL);
CPIN(LED, PB1, CPIN_HWLED, CPIN_HIGH);

#pragma pack(push, 1)
typedef struct {
  uint8_t b : 5;
  uint8_t g : 6;
  uint8_t r : 5;
} RGB_T;
#pragma pack(pop)

typedef struct {
  uint32_t id = 0;
  mfButtonFlags state;
} buttonState_t;

volatile std::atomic<bool> refreshMutex;
volatile bool messagesArrived = false;
buttonState_t *states;

void refreshCallback(void);
/*#define LCD_PAUSE() \
  do {                                                                         \
    refreshTimer.pause();                                                      \
    while (refreshMutex.load())                                                \
      ;                                                                        \
  } while (0)
#define LCD_RESUME()                                                           \
  do {                                                                         \
    refreshTimer.resume();                                                     \
  } while (0)

*/
#define LCD_PAUSE()                                                            \
  do {                                                                         \
    while (refreshMutex.load())                                                \
      ;                                                                        \
  } while (0)
#define LCD_RESUME()                                                           \
  do {                                                                         \
    refreshCallback();                                                         \
  } while (0)
/*void dump_messages(mfProto *dev) {
  debug.printf("==========%s===========\n", dev->name);
  for (uint8_t i = 0; i < MF_QUEUE_LEN; i++) {
    MF_MESSAGE m;
    dev->_readMessageIndex(m, i);
    char *_M =
        (char *)(&m.head.magic); // get the signature first letter (should be
M) char *_F = _M + 1; // get the signature second letter (should be F) if
(m.head.id > 0) { debug.print(String(i) + ": "); debug.printf("%c%c %3u>%-3u
[%-3u] #%-3u (%02X) Timeout: %u, =%-4u\n",
                   *_M, *_F, m.head.from, m.head.to, m.head.type, m.head.id,
                   m.foot.crc, uwTick - m.time, m.head.size);
      debug.print("[");
      for (uint8_t c = 0; c < m.head.size; c++)
        debug.printf("%02X", m.data[c]);
      debug.println("]");
    } else {
      // debug.println("EMPTY");
    }
  }
}*/
void refreshCallback(void) {
  // if (messagesArrived)
  // return;
  if (!refreshMutex.exchange(true)) {
    /* lcd commands require 8bit bus */
    LL_SPI_SetDataWidth(SPI._spi.handle.Instance, LL_SPI_DATAWIDTH_8BIT);
    /* define area to write, whole schreen here */
    disp.beforeBufferWrite(0, 0, disp.width, disp.height);
    /* prepare for DMA trasfer, set 16bit data width */
    LL_SPI_SetDataWidth(SPI._spi.handle.Instance, LL_SPI_DATAWIDTH_16BIT);
    /* we don't need half transfer interrupt */
    //    LL_DMA_DisableIT_HT(DMA2, LL_DMA_STREAM_3);
    /* but need transfer complete interrupt, to free mutex */
    LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_3);
    /* start framebuffer transferring with DMA */
    HAL_SPI_Transmit_DMA(&SPI._spi.handle, (uint8_t *)disp.buffer,
                         disp.width * disp.height);
  }
}

#include "init.h"

inline String RUS(String source) {
  uint32_t length = source.length();
  uint32_t i = 0;
  while (i < length) {
    switch (source[i]) {
    case 208:
      source.remove(i, 1);
      length--;
      break;
    case 209:
      source.remove(i, 1);
      length--;
      source[i] += 64;
      break;
    default:
      break;
    }
    i++;
  }
  return source;
}

void flash_init() {
  bool flashInit = flash.initialize();
  if (!flashInit)
    debug.println("Flash init Failed!");
  // debug.print("SPI NOR DeviceID: ");
  // debug.println(flash.readDeviceId(), HEX);
}
uint8_t answerCallback(mfBus *instance, MF_MESSAGE *msg, MF_MESSAGE *newmsg) {
  uint8_t ret = 1;
  switch (msg->head.type) {
  case MFLCD_SIZECMD: {
    MFLCD_SIZE size = {.w = disp.width, .h = disp.height};
    instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                          msg->head.id, (uint8_t *)&size, sizeof(MFLCD_SIZE));
  } break;
  case MFLCD_CLEARCMD: {
    if (msg->head.size == sizeof(MFLCD_CLEAR)) {
      disp.b_clear(((MFLCD_CLEAR *)(msg->data))->color);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_POINTCMD: {
    if (msg->head.size == sizeof(MFLCD_POINT)) {
      MFLCD_POINT *cmd = (MFLCD_POINT *)msg->data;
      disp.b_point(cmd->x, cmd->y, cmd->color);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_LINECMD: {
    if (msg->head.size == sizeof(MFLCD_POINT)) {
      MFLCD_LINE *cmd = (MFLCD_LINE *)msg->data;
      disp.b_line(cmd->x, cmd->y, cmd->x2, cmd->y2, cmd->color);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_HLINECMD: {
    if (msg->head.size == sizeof(MFLCD_HLINE)) {
      MFLCD_HLINE *cmd = (MFLCD_HLINE *)msg->data;
      disp.b_Hline(cmd->x, cmd->x2, cmd->y, cmd->color);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_VLINECMD: {
    if (msg->head.size == sizeof(MFLCD_VLINE)) {
      MFLCD_VLINE *cmd = (MFLCD_VLINE *)msg->data;
      disp.b_Vline(cmd->x, cmd->y, cmd->y2, cmd->color);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_BOXCMD: {
    if (msg->head.size == sizeof(MFLCD_BOX)) {
      MFLCD_BOX *cmd = (MFLCD_BOX *)msg->data;
      disp.b_box(cmd->x, cmd->y, cmd->x2, cmd->y2, cmd->color);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_BOXWHCMD: {
    if (msg->head.size == sizeof(MFLCD_BOXWH)) {
      MFLCD_BOXWH *cmd = (MFLCD_BOXWH *)msg->data;
      disp.b_boxx(cmd->x, cmd->y, cmd->w, cmd->h, cmd->color);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_RECTCMD: {
    if (msg->head.size == sizeof(MFLCD_RECT)) {
      MFLCD_RECT *cmd = (MFLCD_RECT *)msg->data;
      disp.b_rectangle(cmd->x, cmd->y, cmd->x2, cmd->y2, cmd->color);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_CIRCLECMD: {
    if (msg->head.size == sizeof(MFLCD_CIRCLE)) {
      MFLCD_CIRCLE *cmd = (MFLCD_CIRCLE *)msg->data;
      disp.b_circle(cmd->x, cmd->y, cmd->r, cmd->color);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_ROUNDCMD: {
    if (msg->head.size == sizeof(MFLCD_ROUND)) {
      MFLCD_ROUND *cmd = (MFLCD_ROUND *)msg->data;
      disp.b_round(cmd->x, cmd->y, cmd->r, cmd->color);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_BITMAPCMD: {
    if (msg->head.size >= sizeof(MFLCD_BITMAP)) {
      MFLCD_BITMAP *cmd = (MFLCD_BITMAP *)msg->data;
      if (msg->head.size ==
          sizeof(*cmd) + (cmd->w * cmd->h * sizeof(uint16_t))) {
        uint8_t *pix = (uint8_t *)(msg->data + sizeof(*cmd));
        disp.b_drawBitmap(cmd->x, cmd->y, cmd->w, cmd->h, pix);
        instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                              msg->head.id, NULL, 0);
      }
    }
  } break;
  case MFLCD_PICCMD: {
    if (msg->head.size >= sizeof(MFLCD_PIC)) {
      MFLCD_PIC *cmd = (MFLCD_PIC *)msg->data;
      MFLCD_IMAGE *image = findImageByName(cmd->name);
      if (image) {
        disp.b_drawBitmap(cmd->x, cmd->y, image->size->w, image->size->h,
                          image->image);
      }
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_PRINTCMD: {
    if (msg->head.size >= sizeof(MFLCD_PRINT)) {
      MFLCD_PRINT *cmd = (MFLCD_PRINT *)msg->data;
      char *text = (char *)(msg->data + sizeof(*cmd));
      String txt = RUS(text);
      disp.b_print(cmd->x, cmd->y, txt, cmd->color, cmd->bg, cmd->scale);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_PRINTRIGHTCMD: {
    if (msg->head.size >= sizeof(MFLCD_PRINT)) {
      MFLCD_PRINT *cmd = (MFLCD_PRINT *)msg->data;
      char *text = (char *)(msg->data + sizeof(*cmd));
      String txt = RUS(text);
      disp.b_printRight(cmd->x, cmd->y, txt, cmd->color, cmd->bg, cmd->scale);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_PRINTCENTERCMD: {
    if (msg->head.size >= sizeof(MFLCD_PRINTCENTER)) {
      MFLCD_PRINTCENTER *cmd = (MFLCD_PRINTCENTER *)msg->data;
      char *text = (char *)(msg->data + sizeof(*cmd));
      String txt = RUS(text);
      disp.b_printCenter(cmd->x, cmd->w, cmd->y, txt, cmd->color, cmd->bg,
                         cmd->scale);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  case MFLCD_DRAWBARCMD: {
    if (msg->head.size == sizeof(MFLCD_DRAWBAR)) {
      MFLCD_DRAWBAR *cmd = (MFLCD_DRAWBAR *)msg->data;
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
      constexpr uint16_t colors[11] = {
          RGB(31, 0, 0),  RGB(31, 63, 0), RGB(31, 63, 0), RGB(0, 63, 0),
          RGB(0, 63, 0),  RGB(0, 63, 0),  RGB(0, 63, 0),  RGB(0, 63, 0),
          RGB(31, 63, 0), RGB(31, 63, 0), RGB(31, 0, 0)};
      constexpr uint16_t gray = RGB(7, 14, 7);
      uint16_t color;
      for (uint8_t i = 0; i < 11; i++) {
        if (cmd->value >= i * 10)
          color = colors[i];
        else
          color = gray;
        disp.b_boxx(cmd->x, cmd->y + ((cmd->h + cmd->s + 1) * (10 - i)), cmd->w,
                    cmd->h, color);
      }
    }
  } break;
  case MFLCD_REFRESHPAUSE: {
    LCD_PAUSE();
    instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                          msg->head.id, NULL, 0);
  } break;
  case MFLCD_REFRESHRESUME: {
    LCD_RESUME();
    instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                          msg->head.id, NULL, 0);
  } break;
  case MFLCD_SETFONTCMD: {
    if (msg->head.size >= sizeof(MFLCD_FONT)) {
      MFLCD_FONT *cmd = (MFLCD_FONT *)msg->data;
      disp.setFont(MFLCD_availableFonts[cmd->num]);
      MFLCD_FONTDATA fontData = disp.getFontData();
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, (uint8_t *)&fontData,
                            sizeof(MFLCD_FONTDATA));
    }
  } break;
  case MFLCD_BRIGHTNESSCMD: {
    if (msg->head.size == sizeof(MFLCD_BRIGHTNESS)) {
      MFLCD_BRIGHTNESS *cmd = (MFLCD_BRIGHTNESS *)msg->data;
      LED.setPWM(cmd->brightness);
      instance->sendMessage(newmsg->head.from, msg->head.from, msg->head.type,
                            msg->head.id, NULL, 0);
    }
  } break;
  /********************************
   ************ buttons ***********
   ********************************/
  case MFBUS_BUTTON_GETSTATECMD: {
    if (msg->head.size) {
      if (msg->data[msg->head.size - 1] == '\0') {

        for (uint32_t i = 0; i < mfButton::instances.size(); i++) {
          if (!strcmp((const char *)msg->data, mfButton::instances[i]->name)) {
            mfButtonFlags state;
            if (states[i].id) {
              state = states[i].state;
            } else {
              state = mfButton::instances[i]->getState();
              if (state != MFBUTTON_NOSIGNAL) {
                states[i] = {msg->head.id, state};
              }
            }
            instance->sendMessage(newmsg, (uint8_t *)&state,
                                  sizeof(mfButtonFlags));
          }
        }
      }
    }
  } break;
  case MFBUS_BUTTON_CONFIRMSTATECMD: {
    if (msg->head.size) {
      if (msg->data[msg->head.size - 1] == '\0') {
        for (uint32_t i = 0; i < mfButton::instances.size(); i++) {
          if (!strcmp((const char *)msg->data, mfButton::instances[i]->name)) {
            states[i].id = 0;
            instance->sendMessage(newmsg, NULL, 0);
          }
        }
      }
    }
  } break;
  /********************************
   *********** rgb leds ***********
   ********************************/
  case MFBUS_RGB_ONCMD: {
    if (msg->head.size > sizeof(MFBUS_RGB_ON)) {
      if (msg->data[msg->head.size - 1] == '\0') {
        MFBUS_RGB_ON *cmd = (MFBUS_RGB_ON *)msg->data;
        char *_name = (char *)&msg->data[sizeof(*cmd)];
        mfRGB *rgb = mfRGB::findByName(_name);
        if (rgb)
          rgb->on(cmd->time);
        instance->sendMessage(newmsg, NULL, 0);
      }
    }
  } break;
  case MFBUS_RGB_OFFCMD: {
    if (msg->head.size) {
      if (msg->data[msg->head.size - 1] == '\0') {
        char *_name = (char *)msg->data;
        mfRGB *rgb = mfRGB::findByName(_name);
        if (rgb)
          rgb->off();
        instance->sendMessage(newmsg, NULL, 0);
      }
    }
  } break;
  case MFBUS_RGB_BLINKCMD: {
    if (msg->head.size > sizeof(MFBUS_RGB_BLINK)) {
      if (msg->data[msg->head.size - 1] == '\0') {
        MFBUS_RGB_BLINK *cmd = (MFBUS_RGB_BLINK *)msg->data;
        char *_name = (char *)&msg->data[sizeof(*cmd)];
        mfRGB *rgb = mfRGB::findByName(_name);
        if (rgb)
          rgb->blink(cmd->time, cmd->period);
        instance->sendMessage(newmsg, NULL, 0);
      }
    }
  } break;
  case MFBUS_RGB_BREATHECMD: {
    if (msg->head.size > sizeof(MFBUS_RGB_BLINK)) {
      if (msg->data[msg->head.size - 1] == '\0') {
        MFBUS_RGB_BLINK *cmd = (MFBUS_RGB_BLINK *)msg->data;
        char *_name = (char *)&msg->data[sizeof(*cmd)];
        mfRGB *rgb = mfRGB::findByName(_name);
        if (rgb)
          rgb->breathe(cmd->time, cmd->period);
        instance->sendMessage(newmsg, NULL, 0);
      }
    }
  } break;
  case MFBUS_RGB_BLINKFADECMD: {
    if (msg->head.size > sizeof(MFBUS_RGB_BLINK)) {
      if (msg->data[msg->head.size - 1] == '\0') {
        MFBUS_RGB_BLINK *cmd = (MFBUS_RGB_BLINK *)msg->data;
        char *_name = (char *)&msg->data[sizeof(*cmd)];
        mfRGB *rgb = mfRGB::findByName(_name);
        if (rgb)
          rgb->blinkfade(cmd->time, cmd->period);
        instance->sendMessage(newmsg, NULL, 0);
      }
    }
  } break;
  case MFBUS_RGB_SETCOLORCMD: {
    if (msg->head.size > sizeof(MFBUS_RGB_SETCOLOR)) {
      if (msg->data[msg->head.size - 1] == '\0') {
        MFBUS_RGB_SETCOLOR *cmd = (MFBUS_RGB_SETCOLOR *)msg->data;
        char *_name = (char *)&msg->data[sizeof(*cmd)];
        mfRGB *rgb = mfRGB::findByName(_name);
        if (rgb)
          rgb->setColor(cmd->r, cmd->g, cmd->b);
        instance->sendMessage(newmsg, NULL, 0);
      }
    }
  } break;
  case MFBUS_RGB_SETBRIGHTNESSCMD: {
    if (msg->head.size > sizeof(MFBUS_RGB_BRIGHTNESS)) {
      if (msg->data[msg->head.size - 1] == '\0') {
        MFBUS_RGB_BRIGHTNESS *cmd = (MFBUS_RGB_BRIGHTNESS *)msg->data;
        char *_name = (char *)&msg->data[sizeof(*cmd)];
        mfRGB *rgb = mfRGB::findByName(_name);
        if (rgb)
          rgb->setBrightness(cmd->brightness);
        instance->sendMessage(newmsg, NULL, 0);
      }
    }
  } break;
  default:
    ret = 0;
  }
  return ret;
}
void protect() {
#define FLASH_TIMEOUT_VALUE 50000U /* 50 s */
  FLASH_OBProgramInitTypeDef config;
  HAL_FLASHEx_OBGetConfig(&config);
  if (config.RDPLevel != OB_RDP_LEVEL_2) {
    uint32_t irq_state = __get_PRIMASK();
    __disable_irq();
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    config.OptionType = OPTIONBYTE_RDP;
    config.RDPLevel = OB_RDP_LEVEL_2;
    HAL_FLASHEx_OBProgram(&config);
    FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);
    HAL_FLASH_OB_Launch();
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
    __set_PRIMASK(irq_state);
  }
}

void setup() {
  states = new buttonState_t[mfButton::instances.size()];
  protect();
  // SlaveBus.setName("alpro_v10_lcd");
  SlaveBus.begin();
  Serial2.begin(115200);
  SPI.setMOSI(_MOSI);
  SPI.setMISO(_MISO);
  SPI.setSCLK(_SCLK);
  SPI.setSSEL(_CS); /*
   SPI_2.setMOSI(PB15);
   SPI_2.setMISO(PB14);
   SPI_2.setSCLK(PB12);
   SPI_2.setSSEL(PB13);*/
  SPI_2.begin();
  // pinMode(_CS, OUTPUT);
  // digitalWrite(_CS, LOW);
  /*  debug.setRx(PA3);
    debug.setTx(PA2);
    debug.begin(115200);*/
  disp.init();
  LL_SPI_SetBaudRatePrescaler(SPI._spi.handle.Instance,
                              LL_SPI_BAUDRATEPRESCALER_DIV2);
  initDMA();

  flash_init();
  LED.setPWM(0);
  LED.on();
  disp.clear(BLACK);
  cPins::initTimer(TIM9, 60);
  /*  refreshTimer.pause();
    // refreshTimer->setMode(1, TIMER_OUTPUT_COMPARE);
    refreshTimer.setOverflow(20, HERTZ_FORMAT);
    refreshTimer.attachInterrupt(refreshCallback);
    refreshTimer.setInterruptPriority(0, 6);
    refreshTimer.resume();*/
  disp.b_clear(0);
  disp.setFont(MFLCD_availableFonts[0]);
  SlaveBus.setRecvCallback([](mfBus *) { messagesArrived = true; });
  // PUSHIMAGE(image_logomf15132);
  PUSHIMAGE(image_logomf2);
  // PUSHIMAGE(image_compressor);
  // PUSHIMAGE(image_battery);
  PUSHIMAGE(image_arrowDown);
  PUSHIMAGE(image_arrowUp);
  PUSHIMAGE(image_compG);
  PUSHIMAGE(image_compR);
  PUSHIMAGE(image_compY);
  PUSHIMAGE(image_bat);
  PUSHIMAGE(image_bigarrdown);
  PUSHIMAGE(image_bigarrup);
  PUSHIMAGE(image_service);
  PUSHIMAGE(image_noengine);
  PUSHIMAGE(image_frame);
  SlavePhy.redefineIrqHandler();
  for (uint8_t i = 0; i < mfButton::instances.size(); i++) {
    mfButton::instances[i]->begin();
  }
}
void loop() {
  uint32_t ticks = TICKS();
  while (1) {
    if (SlaveBus.haveMessages()) {
      MF_MESSAGE msg;
      SlaveBus.readFirstMessage(msg);
      SlaveBus.answerMessage(&msg, answerCallback);
    } else {
      messagesArrived = false;
    }
    if (TICKS() - ticks > 5000) {
      ticks = TICKS();
    }
#if (MFBUTTON_USE_EXTI != 1)
    for (uint8_t i = 0; i < mfButton::instances.size(); i++)
      mfButton::instances[i]->read();
#endif
  }
}
