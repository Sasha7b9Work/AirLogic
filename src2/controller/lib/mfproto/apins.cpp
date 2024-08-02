#include <wiring.h>

#include "HardwareTimer.h"
#include <vector>

#include "Kalman.h"
#include "apins.h"

#ifdef STM32F4xx
#include "stm32_def.h"
#include "stm32f4xx_ll_adc.h"

uint32_t f4_get_adc_channel(PinName pin);

__attribute__((optimize(3))) inline uint16_t aPins::f4_adc_read(void) {
  ADC_HandleTypeDef AdcHandle = {};
  ADC_ChannelConfTypeDef AdcChannelConf = {};
  __IO uint16_t uhADCxConvertedValue = 0;
  uint32_t samplingTime = ADC_SAMPLINGTIME;
  if (ADCInstance == ADC2)
    return 4095;
  AdcHandle.Instance = ADCInstance;

  if (AdcHandle.Instance == NP) {
    return -1;
  }

#ifdef ADC_CLOCK_DIV
  AdcHandle.Init.ClockPrescaler =
      ADC_CLOCK_SYNC_PCLK_DIV2; // ADC_CLOCK_DIV; /* (A)synchronous clock mode,
                                // input ADC clock divided */
#endif
  AdcHandle.Init.Resolution =
      ADC_RESOLUTION_12B; /* resolution for converted data */
#ifdef ADC_DATAALIGN_RIGHT
  AdcHandle.Init.DataAlign =
      ADC_DATAALIGN_RIGHT; /* Right-alignment for converted data */
#endif
  AdcHandle.Init.ScanConvMode =
      DISABLE; /* Sequencer disabled (ADC conversion on only 1 channel:
                  channel set on rank 1) */
  AdcHandle.Init.EOCSelection =
      ADC_EOC_SINGLE_CONV; /* EOC flag picked-up to indicate conversion end */

  AdcHandle.Init.ContinuousConvMode =
      DISABLE; /* Continuous mode disabled to have only 1 conversion at each
                  conversion trig */
  AdcHandle.Init.NbrOfConversion =
      1; /* Specifies the number of ranks that will be converted within the
            regular group sequencer. */
  AdcHandle.Init.DiscontinuousConvMode =
      DISABLE; /* Parameter discarded because sequencer is disabled */
  AdcHandle.Init.NbrOfDiscConversion =
      0; /* Parameter discarded because sequencer is disabled */
  AdcHandle.Init.ExternalTrigConv =
      ADC_SOFTWARE_START; /* Software start to trig the 1st conversion
                             manually, without external event */
  AdcHandle.Init.ExternalTrigConvEdge =
      ADC_EXTERNALTRIGCONVEDGE_NONE; /* Parameter discarded because software
                                        trigger chosen */
  AdcHandle.Init.DMAContinuousRequests =
      DISABLE; /* DMA one-shot mode selected (not applied to this example) */
#ifdef ADC_OVR_DATA_OVERWRITTEN
  AdcHandle.Init.Overrun =
      ADC_OVR_DATA_OVERWRITTEN; /* DR register is overwritten with the last
                                   conversion result in case of overrun */
#endif

#if defined(ADC_CFGR_DFSDMCFG) && defined(DFSDM1_Channel0)
  AdcHandle.Init.DFSDMConfig =
      ADC_DFSDM_MODE_DISABLE; /* ADC conversions are not transferred by DFSDM.
                               */
#endif

  AdcHandle.State = HAL_ADC_STATE_RESET;
  AdcHandle.DMA_Handle = NULL;
  AdcHandle.Lock = HAL_UNLOCKED;
  /* Some other ADC_HandleTypeDef fields exists but not required */

  if (HAL_ADC_Init(&AdcHandle) != HAL_OK) {
    return -2;
  }

  AdcChannelConf.Channel =
      ADCChannel; /* Specifies the channel to configure into ADC */

  if (!IS_ADC_CHANNEL(AdcChannelConf.Channel)) {
    return -3;
  }
  AdcChannelConf.Rank = 1; /* Specifies the rank in the
                                               regular group sequencer */

  AdcChannelConf.SamplingTime = samplingTime; /* Sampling time value to be set
                                                 for the selected channel */
  AdcChannelConf.Offset =
      0; /* Parameter discarded because offset correction is disabled */
  /*##-2- Configure ADC regular channel
   * ######################################*/
  if (HAL_ADC_ConfigChannel(&AdcHandle, &AdcChannelConf) != HAL_OK) {
    /* Channel Configuration Error */
    return -4;
  }

  /*##-3- Start the conversion process ####################*/
  if (HAL_ADC_Start(&AdcHandle) != HAL_OK) {
    /* Start Conversation Error */
    return -6;
  }

  if (HAL_ADC_PollForConversion(&AdcHandle, 10) != HAL_OK) {
    /* End Of Conversion flag not set on time */
    return -7;
  }

  uhADCxConvertedValue = HAL_ADC_GetValue(&AdcHandle);

  if (HAL_ADC_Stop(&AdcHandle) != HAL_OK) {
    /* Stop Conversation Error */
    return -8;
  }

  if (HAL_ADC_DeInit(&AdcHandle) != HAL_OK) {
    return -9;
  }

  if (__LL_ADC_COMMON_INSTANCE(AdcHandle.Instance) != 0U) {
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(AdcHandle.Instance),
                                   LL_ADC_PATH_INTERNAL_NONE);
  }
  return uhADCxConvertedValue;
}
#endif

uint32_t aPins::timerFreq = 0; // timer frequency in hertz
uint32_t aPins::prevms = 0;    // not used
bool aPins::timerInited = false;
HardwareTimer *aPins::T = nullptr;
// constructor
aPins::aPins(const char *cname, uint16_t _PIN, float measure_estimate, float q)
    : pin(_PIN), initMS(measure_estimate), initNoise(q) {
  enableKalman();
  sf = new KalmanFilter(1, 1, 100);
  // pinMode(_PIN, INPUT); // analog input
  pin_name = analogInputToPinName(_PIN);
  if (!(pin_name & PADC_BASE)) {
    pinmap_pinout(pin_name, PinMap_ADC);
  }
  strncpy(name, cname, 31);

#ifdef STM32F4xx
  ADCInstance = (ADC_TypeDef *)pinmap_peripheral(pin_name, PinMap_ADC);
  ADCChannel = f4_get_adc_channel(pin_name);
#endif
  instances.push_back(this);
}
void aPins::enableKalman(void) {
  disableKalman();
  kf = new KalmanFilter(initMS, initMS, initNoise);
}
void aPins::disableKalman(void) {
  if (kf != nullptr) {
    delete kf;
    kf = nullptr;
  }
}
// reads current pin value and puts it to kalman filter
void __attribute__((optimize("O3"))) aPins::read(void) {
  if (pin_name == NC)
    return;
  uint32_t readValue;
  if (dmaValue) {
    readValue = *dmaValue;
  } else
    readValue =
#if 1
#ifdef STM32F4xx
        // 4095;
        f4_adc_read();
#else
        adc_read_value(pin_name, ADC_RESOLUTION);
#endif
#else
        4095;
#endif
  if (readValue > (1UL << ADC_RESOLUTION))
    return;
  lastValue = readValue;
  if (kf != nullptr) // if we have valid kalman filter instance
    value_f = kf->updateEstimate(lastValue);
  else // just read without approximation
    value_f = (value_f + lastValue) / 2.0;
  value = lrint(value_f);
  uint32_t currentTime = HAL_GetTick();
  if (currentTime - speedLastTime > 500) {
    currentSpeed = sf->updateEstimate ((value_f - speedLastValue) /
                                           (currentTime - speedLastTime));
    speedLastTime = currentTime;
    speedLastValue = value_f;
  }
}
float aPins::getSpeed(void) { return currentSpeed; }
// read pin value <count> times
void aPins::readMany(uint32_t count) {
  while (count--) {
    read();
  }
}

// enable reading this pin
void aPins::stop() { active = false; }

// disable reading this pin
void aPins::start() { active = true; }

// returns approximated value for this pin
uint32_t aPins::get(void) {
  read();
  return (value);
}

// returns approximated value for this pin
uint32_t aPins::getA(void) {
  read();
  if (::llabs(value - lastAverage) > 10)
    lastAverage = value;
  return lastAverage;
}

// returns approximated raw (float) value for this pin
float aPins::getFloat(void) {
  read();
  return (value_f);
}

// reinit estimate values for kalman
void aPins::setEstimate(float est) {
  initMS = est;
  if (kf != nullptr) {
    kf->setEstimateError(est);
    kf->setMeasurementError(est);
  }
};

// reinit noise q for kalman
void aPins::setNoise(float q) {
  initNoise = q;
  if (kf != nullptr) {
    kf->setProcessNoise(q);
  }
}

// callback for timer
__attribute__((optimize(3))) void aPins::timerCallback(void) {
  for (uint16_t c = 0; c < instances.size(); c++) { // for all inited pins
    if (instances[c]->active) {                     // if this pin is active
      instances[c]->read();                         // read its value
    }
  }
}

// init timer for pins reading
void aPins::initTimer(TIM_TypeDef *timer, uint32_t freq) {
  timerFreq = freq;
  if (timerInited)
    freeTimer();
  T = new HardwareTimer(timer);
  T->pause();
  T->setMode(1, TIMER_OUTPUT_COMPARE);
  T->setOverflow(timerFreq, HERTZ_FORMAT);
  T->attachInterrupt(timerCallback);
  timerInited = true;
  T->resume();
}

// free timer and stop using it
void aPins::freeTimer(void) {
  T->pause();
  delete T;
  timerInited = false;
  timerFreq = 0;
}

// destructor
aPins::~aPins() {
  disableKalman();
  if (sf)
    delete sf;
  instances.erase(std::find(instances.begin(), instances.end(), this));
  if (instances.empty() && timerInited) {
    freeTimer(); // normally it should never happen cuz all pins have to be
                 // statically defined, but if you want to malloc apins
                 // instances, it will free timer after last pin deletion
  }
}
#ifdef STM32F4xx
uint32_t f4_get_adc_channel(PinName pin) {
  uint32_t function = pinmap_function(pin, PinMap_ADC);
  uint32_t channel = 0;
  switch (STM_PIN_CHANNEL(function)) {
#ifdef ADC_CHANNEL_0
  case 0:
    channel = ADC_CHANNEL_0;
    break;
#endif
  case 1:
    channel = ADC_CHANNEL_1;
    break;
  case 2:
    channel = ADC_CHANNEL_2;
    break;
  case 3:
    channel = ADC_CHANNEL_3;
    break;
  case 4:
    channel = ADC_CHANNEL_4;
    break;
  case 5:
    channel = ADC_CHANNEL_5;
    break;
  case 6:
    channel = ADC_CHANNEL_6;
    break;
  case 7:
    channel = ADC_CHANNEL_7;
    break;
  case 8:
    channel = ADC_CHANNEL_8;
    break;
  case 9:
    channel = ADC_CHANNEL_9;
    break;
  case 10:
    channel = ADC_CHANNEL_10;
    break;
  case 11:
    channel = ADC_CHANNEL_11;
    break;
  case 12:
    channel = ADC_CHANNEL_12;
    break;
  case 13:
    channel = ADC_CHANNEL_13;
    break;
  case 14:
    channel = ADC_CHANNEL_14;
    break;
  case 15:
    channel = ADC_CHANNEL_15;
    break;
#ifdef ADC_CHANNEL_16
  case 16:
    channel = ADC_CHANNEL_16;
    break;
#endif
  case 17:
    channel = ADC_CHANNEL_17;
    break;
#ifdef ADC_CHANNEL_18
  case 18:
    channel = ADC_CHANNEL_18;
    break;
#endif
#ifdef ADC_CHANNEL_19
  case 19:
    channel = ADC_CHANNEL_19;
    break;
#endif
#ifdef ADC_CHANNEL_20
  case 20:
    channel = ADC_CHANNEL_20;
    break;
  case 21:
    channel = ADC_CHANNEL_21;
    break;
  case 22:
    channel = ADC_CHANNEL_22;
    break;
  case 23:
    channel = ADC_CHANNEL_23;
    break;
  case 24:
    channel = ADC_CHANNEL_24;
    break;
  case 25:
    channel = ADC_CHANNEL_25;
    break;
  case 26:
    channel = ADC_CHANNEL_26;
    break;
#ifdef ADC_CHANNEL_27
  case 27:
    channel = ADC_CHANNEL_27;
    break;
  case 28:
    channel = ADC_CHANNEL_28;
    break;
  case 29:
    channel = ADC_CHANNEL_29;
    break;
  case 30:
    channel = ADC_CHANNEL_30;
    break;
  case 31:
    channel = ADC_CHANNEL_31;
    break;
#endif
#endif
  default:
    channel = 0;
    break;
  }
  return channel;
}
#endif