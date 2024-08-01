#ifndef BPINS_H
#define BPINS_H

#include <HardwareTimer.h>
#include <sketch.h>

enum bpinmode { BPIN_NORMAL = 0, BPIN_ADDITIVE, BPIN_MODEEND };
enum bpinstate { BPIN_LOW = 0, BPIN_HIGH = 1, BPIN_OD = 2 };

class bPins {
private:
  GPIO_TypeDef *port;           // lowlevel GPIO port
  uint16_t pin;                 // lowlevel GPIO pin
  uint32_t pinHW;               // arduino pin num
  uint8_t channel;              // timer channel
  uint16_t Duty = 100;          // pwm duty cycle before led correction
  volatile uint32_t onTime = 0; // time of blink/breath etc
  uint32_t onInitTime = 0;      // container for time initial time value
  uint8_t mode = BPIN_NORMAL;   // initial mode, no addition or phase shift
  bool noInterruptable = false; // thread safe mutex
  static uint32_t prevms;       // previous millisecond value
  void setTime(uint32_t time);  // set pin time value depending on current mode

public:
  static HardwareTimer *T;      // hardware timer object
  static bool timerInited;      // is timer inited or not
  static uint64_t timerCounter; // pin instances counter
  static uint32_t timerFreq;    // timer frequency
  uint8_t highState, lowState;  // pin states gpio level
  inline static std::vector<bPins *> instances = {};
  char name[32] = ""; // pin name, filled by BPIN define, can be used
                      // for debug purposes

  bPins(const char *cname, uint16_t _PIN, uint8_t ch, uint8_t hs = BPIN_HIGH);
  // BPIN_PIN will init pin as usual gpio, use BPIN_LED to make brightness
  // change better for leds hs = 0 is for active low pins

  /* direct pin access routines, don't use it without special needs */
  __always_inline void write(uint8_t value);
  __always_inline void reset(void);
  __always_inline void set(void);
  __always_inline void toggle(void);
  __always_inline uint8_t read(void);

  bool isActive(void);         // return false when its off
  uint32_t getRemaining(void); // return 0 when its off and ms value when on
  uint32_t getTime(void);      // return working time given at start

  void setPWM(uint16_t duty);     // set PWM duty for pin
  void updateChannel(uint8_t ch); // select another channel for pin (1..4)
  uint16_t getDuty(void);         // for internal use

  void on(uint32_t time = ~0UL); // just switch it on for a specific time

  void off(); // switch off

  void setMode(
      uint8_t newMode); // normally any new call will reset previous state
                        // BPIN_CONTINUE allows to set new time without changing
                        // period BPIN_ADDITIVE will add time with each new call

  void noInterrupt(void); // make pin noninterruptable, any new calls will be
                          // ignored till previously set job will expire

  static void
  initTimer(TIM_TypeDef *timer = TIM1,
            uint32_t freq = 1000); // by default it will use TIM1 at 60Hz, dont
                                   // forget to init it at startup
  static void setFrequency(uint32_t freq);
  void updateFrequency();
  static void freeTimer(void);
  std::function<void(bPins *pin)> finishCallback = [](bPins *) {
  }; // callback called after time expiring or off command
  std::function<void(bPins *pin)> tickCallback = [](bPins *) {
  }; // callback called every ms
  ~bPins();
};

#define BPIN(a, ...) bPins(a)(#a, __VA_ARGS__)

#endif
