#include "sketch.h"

#include "init.h"
#include "mfproto.h"
#include "mfprotophy.h"
#include "mfprotophyacm.h"
#include "mfprotophypipe.h"
#include "mfprotophyserial.h"
#include "mfprotopipe.h"
#include "mfwheel.h"
#include <STM32FreeRTOS.h>
#include <cPins.h>

#include "mfbus.h"
#include "mfbus_button.h"
#include "mfbusrgb.h"
#include "mfbutton.h"
#include "mflcd.h"
#include "mflcd_data.h"
#include "mfmenu.h"

#include "SPI.h"
#define SPI_CS PA15
#include "w25qxx.h"
extern w25qxx_t w25qxx;

#define TEST_PACKET 144

CPIN(EN5V, PE5, CPIN_PIN, CPIN_HIGH);  // EN 5V
CPIN(EN12V, PE4, CPIN_PIN, CPIN_HIGH); // EN 12V

CPIN(L, PE7, CPIN_LED, CPIN_OD); // PROG_LED
CPIN(LUP, PB6, CPIN_LED, CPIN_OD);
CPIN(LOK, PB7, CPIN_LED, CPIN_OD);
CPIN(LDOWN, PB8, CPIN_LED, CPIN_OD);

#define BAUD (MFPROTO_SERIAL_MAXBAUD / 10)

MFPROTOPHYACM(_ACM, BAUD);
// MFPROTOPHYSERIAL(COM2, Serial2, PA1, BAUD);
MFPROTOPHYSERIAL(COM3, Serial3, PD12, BAUD / 2);

MFPROTO(USB, _ACM);
// MFPROTO(Master, COM2);
MFPROTO(Slave, COM3);

MFBUS(ACM, USB, 2, MFBUS_CAP_DEBUG);
MFBUS(MAS, Slave, MFBUS_MASTER);
// MFBUS(SLA, Master, 2, MFBUS_CAP_KEYS);

MFPROTOPHYSERIAL(COM4, Serial4, MFPROTO_PHY_SERIAL_NC, BAUD);
MFPROTO(Bus3Proto, COM4);
MFBUS(Bus3, Bus3Proto, MFBUS_MASTER);

//#define debug Serial
#include "debug.h"
DebugSerial debug(ACM);
//#define debug Serial

MFPROTOPHYPIPE(pipe1phy);
MFPROTOPHYPIPE(pipe2phy);
MFPROTOPIPE(pipe, {&pipe1phy, &pipe2phy});
MFPROTO(pipe1proto, pipe1phy);
MFPROTO(pipe2proto, pipe2phy);
MFBUS(pipe1bus, pipe1proto, MFBUS_MASTER);
MFBUS(pipe2bus, pipe2proto, 20, MFBUS_CAP_DEBUG);

/* lcd and lcd led */
MFLCD(lcd, Bus3, MF_ALPRO_V10_LCD);
MFBUSRGB(ledOK, Bus3, MF_ALPRO_V10_LCD);

MFBUTTON(UP, PB9, INPUT_PULLUP);
MFBUTTON(OK, PB10, INPUT_PULLUP);
MFBUTTON(DOWN, PB11, INPUT_PULLUP);

uint32_t lostpipe = 0, sentpipe = 0, recvdpipe = 0;
uint32_t lastlost = 0, lastsent = 0;
void dump_messages(mfProto *dev, bool data = false);

#ifdef EXTERNAL_TIMER
volatile uint64_t ticksCounter = 0;
HardwareTimer ticksTimer(TIM9);
uint32_t TICKS(void) { return ticksCounter & 0xFFFFFFFFUL; }
void incrementTicks(void) { ticksCounter++; }
#endif

static void pipe1Thread(void *arg) {
  UNUSED(arg);
  // pipe2bus.monitorMode(true);
  pipe1bus.begin();
  pipe1bus.setRecvCallback(
      [](mfBus *bus) { // dump_messages(&bus->proto, false);
      });

  while (1) {
    for (uint8_t i = 0; i < pipe1bus.getSlavesCount(); i++) {
      static const uint32_t packetSize = random(100);
      // MFPROTO_MAXDATA;
      uint8_t packet[packetSize] = {0};
      uint32_t curid = pipe1bus.getSent();
      // dump_messages(&pipe1proto);
      uint32_t id = pipe1bus.sendMessageWait(
          pipe1bus.getSlave(i).device, TEST_PACKET, packet, packetSize, 3, 500);
      if (id) {
        sentpipe++;
        pipe1proto.deleteMessageId(id);
      } else {
        lostpipe++;
        debug.printf("%s: Transfer error (%lu).\n", pipe1bus.name, curid);
      }
      // DELAY(1);
    }
    YIELD();
  }
}
static void pipe2Thread(void *arg) {
  UNUSED(arg);
  uint32_t sz = 0, time = TICKS();
  // pipe2bus.monitorMode(true);
  pipe2bus.begin();
  while (1) {
    if (pipe2bus.haveMessages()) {
      // dump_messages(&pipe2proto);
      MF_MESSAGE msg;
      pipe2bus.readFirstMessage(msg);
      pipe2bus.answerMessage(&msg,
                             [&sz](mfBus *instance, MF_MESSAGE *msg,
                                   MF_MESSAGE *newmsg) -> uint8_t {
                               recvdpipe++;
                               if (msg->head.type == TEST_PACKET) {
                                 instance->sendMessage(newmsg, NULL, 0);
                                 sz += msg->head.size + sizeof(MF_HEADER) +
                                       sizeof(MF_FOOTER);
                                 return 1;
                               }
                               return 0;
                             });
    }
    YIELD();
    uint32_t curtime = TICKS();
    if (curtime - time > 3000) {
      uint32_t bandwidth = (1000 * sz) / (curtime - time);
      sz = 0;
      debug.printf("PIPE: Bandwidth: %lu nc:%lu, dc:%lu\n", bandwidth,
                   mfProto::mallocCaptured, mfProto::deleteCaptured);
      debug.printf("PIPE: sent %lu(+%lu), lost:%lu(+%lu), recvd:%lu\n",
                   sentpipe, sentpipe - lastsent, lostpipe, lostpipe - lastlost,
                   recvdpipe);
      lastsent = sentpipe;
      lastlost = lostpipe;
      debug.println("---------------");
      time = TICKS();
    }
    YIELD();
  }
}

void dump_messages(mfProto *dev, bool data) {
  debug.printf("==========%s===========\n", dev->name);
  for (uint8_t i = 0; i < MF_QUEUE_LEN; i++) {
    MF_MESSAGE m;
    dev->_readMessageIndex(m, i);
    char *_M =
        (char *)(&m.head.magic); // get the signature first letter (should be M)
    char *_F = _M + 1; // get the signature second letter (should be F)
    if (m.head.id > 0) {
      debug.print(String(i) + ": ");
      debug.printf("%c%c %3u>%-3u [%-3u] #%-3u (%02X) Timeout: %u, =%-4u\n",
                   *_M, *_F, m.head.from, m.head.to, m.head.type, m.head.id,
                   m.foot.crc, uwTick - m.time, m.head.size);
      if (data) {
        debug.print("[");
        for (uint8_t c = 0; c < m.head.size; c++)
          debug.printf("%02X", m.data[c]);
        debug.println("]");
      }
    } else {
      // debug.println("EMPTY");
    }
  }
}

uint8_t wrongc = 0;

uint8_t cnt = 1;
__IO uint32_t lost = 0;
volatile uint32_t ticks = 0, ticks2 = 0;
uint32_t mlost = 0, msent = 0, mrecvd = 0;
uint32_t lastmlost = 0, lastmsent = 0;
#if 0
static void masterThread(void *arg) {
  uint32_t tick = TICKS();
  do {
    MAS.beginStart();
    // dump_messages(&Bus3.proto);
    MAS.beginEnd();
    debug.printf("MAS got %u slaves\n", MAS.getSlavesCount());
    YIELD();
  } while (!MAS.getSlavesCount() && (TICKS() - tick < 5000));
  // MAS.setRecvCallback([](mfBus *) { LOK.blink(100, 50); });
  // MAS.setRecvCallback([](mfBus *B) { dump_messages(&B->proto, true); });
  COM3.redefineIrqHandler();
  mf
  uint8_t slaves = MAS.getSlavesCount();
  while (1) {
    for (uint8_t sl = 0; sl < slaves; sl++) {
      uint8_t did = MAS.getSlave(sl).device;
      uint32_t num = MAS.getSent();
      uint32_t dt = sizeof(num) * 1;
      uint8_t data[dt] = {0};
      *((uint32_t *)data) = num;
      uint32_t id =
          MAS.sendMessageWait(did, MFBUS_BL_CHECK, (uint8_t *)data, dt, 3, 300);
      msent++;
      if (!id) {
        debug.printf("%s: (%u) Transfer Error\n", MAS.name, MAS.getSent() - 1);
        mlost++;
      } else {
        MF_MESSAGE msg;
        MAS.readMessageId(msg, id);
        if (msg.head.size == sizeof(uint8_t)) {
          if (msg.data) {
            uint8_t data = msg.data[0];
            if (data != (num & 0xFF)) {
              debug.printf("Got wrong num, %u!=%u\n", (num & 0xff), data);
              mlost++;
            } else {
              mrecvd++;
            }
          } else {
            debug.printf("%s: data pointer invalid\n", MAS.name);
          }
        }
      }
      uint32_t curtime = TICKS();
      if (curtime - tick > 3000) {
        uint32_t bandwidth = (1000 * dt) / (curtime - tick);
        dt = 0;
        debug.printf("MAS: Bandwidth: %lu, nc:%lu, dc:%lu\n", bandwidth,
                     mfProto::mallocCaptured, mfProto::deleteCaptured);

        debug.printf("MAS: sent %lu(+%lu), lost:%lu(+%lu), recvd:%lu\n", msent,
                     msent - lastmsent, mlost, mlost - lastmlost, mrecvd);
        lastmsent = msent;
        lastmlost = mlost;
        debug.println("---------------");
        tick = TICKS();
      }
    }
    DELAY(50);
  }
}
#endif
#if 0
static void slaveThread(void *arg) {
  // SLA.monitorMode(true);
  SLA.begin();
  // SLA.denyDefaultAnswer();
  uint32_t sz = 0, time = TICKS();
  while (1) {
    // SLA.poll();
    // SLA.deleteUsefulMessages();
    if (SLA.haveMessages()) {
      // dump_messages(&SLA.proto);
      MF_MESSAGE msg;
      SLA.readFirstMessage(msg);
      SLA.answerMessage(
          msg,
          [&sz](mfBus *instance, MF_MESSAGE &msg,
                MF_MESSAGE &newmsg) -> uint8_t {
            // debug.println("Message recieved " + String(msg->head.id));
            if (msg.head.type == TEST_PACKET) {
              instance->prepareMessage(newmsg, instance->deviceId,
                                       msg.head.from, msg.head.type,
                                       msg.head.id, NULL, 0);
              sz += msg.head.size + sizeof(MF_HEADER) + sizeof(MF_FOOTER);
              return 1;
            }
            return 0;
          });
      uint32_t curtime = TICKS();
      if (curtime - time > 3000) {
        uint32_t bandwidth = (1000 * sz) / (curtime - time);
        sz = 0;
        debug.printf("Bandwidth: %lu\n", bandwidth);
        debug.printf("Free: %u, lost:%ul\n", FreeStack(), lost);
        debug.println("---------------");
        time = TICKS();
      }
      YIELD();
    }
    // OK.read();
  }
}
#endif
static void acmThread(void *arg) {
  UNUSED(arg);
  // ACM.monitorMode(true);
  ACM.begin();
  ACM.setRecvCallback([](mfBus *bus) { LOK.blink(200, 50); });
  while (1) {
    // ACM.poll();
    // SLA.deleteUsefulMessages();
    // dump_messages(&USB);
    if (ACM.haveMessages()) {
      // dump_messages(&USB);
      MF_MESSAGE msg;
      ACM.readFirstMessage(msg);
      ACM.answerMessage(&msg,
                        [](mfBus *instance, MF_MESSAGE *msg,
                           MF_MESSAGE *newmsg) -> uint8_t { return 0; });
    }
    YIELD();
    // DELAY(1000);
    // OK.read();
  }
}
void dumpDebug(mfBus *bus, uint8_t device) {
  uint32_t bytesRead = 0;
  do {
    uint32_t id = bus->sendMessageWait(device, MFBUS_DEBUG, NULL, 0, 1, 50);
    if (id) {
      bytesRead = 0;
      MF_MESSAGE msg;
      bus->readMessageId(msg, id);
      if (msg.head.type == MFBUS_DEBUG) {
        bytesRead = msg.head.size;
        if (bytesRead > 0) {
          // debug.printf("[%u]", device);
          if (msg.data)
            debug.write(msg.data, (uint32_t)bytesRead);
          else
            debug.write("dumpDebug: data pointer invalid\n");
        }
      }
      // bus->deleteMessageId(id);
    } else {
      debug.printf("DEBUG: transfer Error!\n");
    }
  } while (bytesRead);
}
void lcdThread(void *arg) {
  UNUSED(arg);
  uint32_t tick = TICKS();
  do {
    Bus3.begin();
    YIELD();
  } while (!Bus3.getSlavesCount() && (TICKS() - tick < 5000));
  debug.printf("Timed: %lu\n", TICKS() - tick);
  COM4.redefineIrqHandler();
  Bus3.setRecvCallback([](mfBus *) { LDOWN.blink(100, 50); });
  lcd.begin();
  debug.printf("LCDs: %u\n", mfLCD::instances.size());
  // DELAY(5000);
  MFLCD_FONTDATA fontData = {0};
  lcd.setFont(0, &fontData);
  debug.printf("xO: %d, yO: %d, xA: %u, yA: %u\n", fontData.xOffset,
               fontData.yOffset, fontData.xAdvance, fontData.yAdvance);

  YIELD();

  while (1) {
#define DLY 30
    if (TICKS() - tick > DLY) {
      tick += DLY;
//#define REFR
#ifdef REFR
      lcd.noRefresh();
#endif
      dumpDebug(&Bus3, MF_ALPRO_V10_LCD);
      YIELD();
      lcd.clear(0);
#if 0
      lcd.bitmap(random(32), random(102), 128, 26, (uint8_t *)mftech);
#else
      lcd.image(random(32), random(102), "image_logomf2");
#endif
      String s = "xO:" + String(fontData.xOffset) +
                 " yO:" + String(fontData.yOffset) +
                 " xA:" + String(fontData.xAdvance) +
                 " yA:" + String(fontData.yAdvance) +
                 "\nтестовая строка\nвторая строка\nтретья "
                 "строка\nчетвертая строка\nпятая строка\nшестая строка";
      lcd.print(random(160), random(128), s, random(65536));
#ifdef REFR
      lcd.refresh();
      YIELD();
#endif
    }
  }
}
void buttonThread(void *arg) {
  UNUSED(arg);
  cPins *l[3] = {&LUP, &LOK, &LDOWN};
  DELAY(1000);
  debug.printf("Init buttons (%u):\n", mfButton::instances.size());

  for (uint8_t i = 0; i < mfButton::instances.size(); i++) {
    mfButton::instances[i]->begin();
    debug.printf("%u: Button [%s]\n", i, mfButton::instances[i]->name);
  }
  while (1) {
    for (uint8_t i = 0; i < mfButton::instances.size(); i++) {
      mfButtonFlags state = mfButton::instances[i]->getState();
      if (state != MFBUTTON_NOSIGNAL) {
        debug.printf("%s: ", mfButton::instances[i]->name);
        switch (state) {
        case MFBUTTON_PRESSED:
          debug.printf("pressed");
          l[i]->on(~0UL);
          break;
        case MFBUTTON_PRESSED_LONG:
          debug.printf("pressed long");
          l[i]->setPWM(127);
          break;
        case MFBUTTON_RELEASED:
          debug.printf("released");
          l[i]->off();
          break;
        case MFBUTTON_RELEASED_LONG:
          debug.printf("released long");
          l[i]->setPWM(255);
          l[i]->off();
          break;
        default:;
        }
        debug.println();
      }
    }
    DELAY(10);
    YIELD();
  }
}

void menuThread(void *arg) {
  UNUSED(arg);
  uint32_t tick = TICKS();
  // list_nodes(rootlist);
  list_nodes(settingsList);
  debug.printf("Sizeof settings = %u\n", sizeof(dSet));
  do {
    Bus3.begin();
    YIELD();
  } while (!Bus3.getSlavesCount() && (TICKS() - tick < 5000));
  /*do {
    MAS.begin();
    YIELD();
  } while (!MAS.getSlavesCount() && (TICKS() - tick < 5000));*/
  debug.printf("Timed: %lu\n", TICKS() - tick);
  COM4.redefineIrqHandler();
  Bus3.setRecvCallback([](mfBus *) { LDOWN.blink(100, 50); });
  // COM3.redefineIrqHandler();
  // MAS.setRecvCallback([](mfBus *) { LUP.blink(100, 50); });
  lcd.begin();
  lcd.setBrightness(255);
  YIELD();
  mfBusButton *BUSUP = new mfBusButton("UP", Bus3, MF_ALPRO_V10_LCD);
  mfBusButton *BUSOK = new mfBusButton("OK", Bus3, MF_ALPRO_V10_LCD);
  mfBusButton *BUSDOWN = new mfBusButton("DOWN", Bus3, MF_ALPRO_V10_LCD);
  /*
  mfBusButton *BUSUP = new mfBusButton("UP", MAS, MF_ALPRO_V10_BUTTON);
  mfBusButton *BUSOK = new mfBusButton("OK", MAS, MF_ALPRO_V10_BUTTON);
  mfBusButton *BUSDOWN = new mfBusButton("DOWN", MAS, MF_ALPRO_V10_BUTTON);
  */
  lcdButtons = {BUSUP, BUSOK, BUSDOWN};
  /*item7->disabledCheck = []() { return dSet.b; };
  list2->disabledCheck = []() { return dSet.b; };
  item7->onSet = [](mfMenuItemGen *item) {
    ledOK.setColor(255, 0, 0);
    ledOK.on(100);
  };*/
  set.load([]() { applyItemRules(); }); // load data and apply rules
  while (1) {
    menuWalkthrough(lcd, settingsList);
    debug.printf("Exited menu\n");
  }
}
void setup() {
  // SCnSCB->ACTLR |= SCnSCB_ACTLR_DISDEFWBUF_Msk; //  Disable write buffer to
  // make BusFaults precise.
  Serial.begin(115200);
  ACM.setName("alpro_v10");
  cPins::initTimer(TIM2, 90);
  cPins::T->setInterruptPriority(10, 0);
#ifdef EXTERNAL_TIMER
  ticksTimer.pause();
  ticksTimer.setOverflow(1000, HERTZ_FORMAT);
  ticksTimer.attachInterrupt(incrementTicks);
  ticksTimer.setInterruptPriority(0, 0);
  ticksTimer.resume();
#endif
  LUP.setMode(CPIN_CONTINUE);
  LOK.setMode(CPIN_CONTINUE);
  LDOWN.setMode(CPIN_CONTINUE);
  L.setMode(CPIN_CONTINUE);
  LOK.setPWM(63);
  L.setPWM(63);
  debug.println("Starting threads...");
  EN5V.on();
  EN12V.on();

  SPI.setMISO(PB4);
  SPI.setMOSI(PB5);
  SPI.setSCLK(PB3);
  if (set.begin(SPI, SPI_CS))
    debug.println("Settings SPI Init finished...");
  // portBASE_TYPE s1, s2, s3, s4, s5, s6, s7;
  // xTaskCreate(masterThread, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  // xTaskCreate(slaveThread, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  // xTaskCreate(buttonThread, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(acmThread, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  // xTaskCreate(lcdThread, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(menuThread, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(pipe1Thread, NULL, configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
  xTaskCreate(pipe2Thread, NULL, configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);

  // check for creation errors
  /*if ((s1 != pdPASS) || (s2 != pdPASS)) {
    debug.println(F("Creation problem"));
    while (1) {
      ;
    }
  }*/
  // start scheduler
  vTaskStartScheduler();
  debug.println("Insufficient RAM");
  while (1) {
    ;
  }
}
void loop() {}

#if 0
extern "C" {
void delayMS(uint32_t millis);
void errorBlink(int n);
__attribute__((naked)) void HardFault_Handler(void) {
  __asm volatile("tst lr, #4                                    \n"
                 "ite eq                                        \n"
                 "mrseq r0, msp                                 \n"
                 "mrsne r0, psp                                 \n"
                 "ldr r1, debugHardfault_address                \n"
                 "bx r1                                         \n"
                 "debugHardfault_address: .word hard_fault_handler_c  \n");
}

void hard_fault_handler_c(uint32_t *sp) {
  uint32_t cfsr = SCB->CFSR;
  uint32_t hfsr = SCB->HFSR;
  uint32_t mmfar = SCB->MMFAR;
  uint32_t bfar = SCB->BFAR;

  uint32_t r0 = sp[0];
  uint32_t r1 = sp[1];
  uint32_t r2 = sp[2];
  uint32_t r3 = sp[3];
  uint32_t r12 = sp[4];
  uint32_t lr = sp[5];
  uint32_t pc = sp[6];
  uint32_t psr = sp[7];

  /*printf("HardFault:\n");
  printf("SCB->CFSR   0x%08lx\n", cfsr);
  printf("SCB->HFSR   0x%08lx\n", hfsr);
  printf("SCB->MMFAR  0x%08lx\n", mmfar);
  printf("SCB->BFAR   0x%08lx\n", bfar);
  printf("\n");

  printf("SP          0x%08lx\n", (uint32_t)sp);
  printf("R0          0x%08lx\n", r0);
  printf("R1          0x%08lx\n", r1);
  printf("R2          0x%08lx\n", r2);
  printf("R3          0x%08lx\n", r3);
  printf("R12         0x%08lx\n", r12);
  printf("LR          0x%08lx\n", lr);
  printf("PC          0x%08lx\n", pc);
  printf("PSR         0x%08lx\n", psr);*/
  //__BKPT(0);
  errorBlink(10);
}
}
#else
extern "C" {
void errorBlink(int n);
void HardFault_Handler(void) {
  while (1)
    errorBlink(10);
}
}
#endif
