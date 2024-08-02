#ifndef PIN_MANAGE_H
#define PIN_MANAGE_H

#include "apins.h"
#include "bpins.h"
#include "mfbutton.h"
#include <cPins.h>

#define ST_MEAS 4095
#define ST_ERR (ST_MEAS / 100.0)

APIN(B_VER, PC1, ST_MEAS, ST_ERR); // board revision divider //3

APIN(SENS_12V, PA7, ST_MEAS, 10);    // 12V output voltage //1
APIN(SENS_5V, PC2, ST_MEAS, ST_ERR); // 5V output real voltage //3

APIN(SENS_1, PB1, ST_MEAS, ST_ERR); // Level Sensor 1  //1
APIN(SENS_2, PB0, ST_MEAS, ST_ERR); // Level Sensor 2 //2
APIN(SENS_3, PC5, ST_MEAS, ST_ERR); // Level Sensor 3 //2
APIN(SENS_4, PC4, ST_MEAS, ST_ERR); // Level Sensor 4 //1

APIN(SENS_COMP, PC0, ST_MEAS, ST_ERR); // compressor sensor //3
APIN(SENS_ACC, PA4, ST_MEAS, 10);      // acc signal voltage //2
APIN(SENS_HEAT, PC3, ST_MEAS, ST_ERR); // overheat sensor //1

// CPIN(PROG, PE7, CPIN_LED, CPIN_OD); // PROG_LED
BPIN(PROG, PE7, 4, CPIN_OD); // PROG_LED

BPIN(EN12V, PE4, 4, BPIN_HIGH); // EN 12V
BPIN(EN5V, PE5, 4, BPIN_HIGH);  // EN 5V

BPIN(MOS1, PE8, 1, BPIN_HIGH); // CH1
BPIN(MOS2, PE9, 1, BPIN_HIGH); // CH2

BPIN(MOS3, PE10, 1, BPIN_HIGH); // CH3
BPIN(MOS4, PE11, 1, BPIN_HIGH); // CH4

BPIN(MOS5, PE12, 1, BPIN_HIGH); // CH5
BPIN(MOS6, PE13, 1, BPIN_HIGH); // CH6

BPIN(MOS7, PE14, 1, BPIN_HIGH); // CH7
BPIN(MOS8, PE15, 1, BPIN_HIGH); // CH8

CPIN(LEDUP, PB6, CPIN_HWLED, CPIN_OD);   // LED UP
CPIN(LEDOK, PB7, CPIN_HWLED, CPIN_OD);   // LED OK
CPIN(LEDDOWN, PB8, CPIN_HWLED, CPIN_OD); // LED DOWN

CPIN(COMP, PE2, CPIN_PIN, CPIN_HIGH); // COMPRESSOR ENABLE
CPIN(DRY, PE3, CPIN_PIN, CPIN_HIGH);  // DRYER ENABLE

MFBUTTON(UP, PB9, INPUT_PULLUP);
MFBUTTON(OK, PB10, INPUT_PULLUP);
MFBUTTON(DOWN, PB11, INPUT_PULLUP);

#define CSPIN PA15 // digital pin for flash chip CS pin
#define WK_UP PA0  // wakeup signal

#define SPEED PA6

#define COM1 Serial2 // RJ-12
#define COM1_RTS PA1
#define COM2 Serial3 // RJ-9
#define COM2_RTS PD12

#endif