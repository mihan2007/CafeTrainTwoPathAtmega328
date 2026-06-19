#ifndef CONFIG_H_
#define CONFIG_H_

#include <avr/io.h>

#define F_CPU 16000000UL

// Timeout protection
#define TICKS_PER_UNIT 10000
#define MAX_TIME_UNITS 30

// Control buttons - path 1
#define BUTTON_FORWARD_PIN   PC3  // A3
#define BUTTON_STOP_PIN      PC2  // A2
#define BUTTON_BACKWARD_PIN  PC1  // A1

// Control buttons - path 2
#define PATH2_BUTTON_FORWARD_PIN   PC0  // A0
#define PATH2_BUTTON_BACKWARD_ADC  6    // A6, ADC-only on ATmega328P
#define PATH2_BUTTON_STOP_ADC      7    // A7, ADC-only on ATmega328P
#define ADC_BUTTON_THRESHOLD       512
#define PATH2_CONTROL_BUTTONS_ENABLED 1

// Table 9 selector
#define TABLE9_BUTTON_PIN PB2      // D10
#define TABLE9_BUTTON_ENABLED 1

// Indication / external logic
#define POWER_INDICATION_ENABLE PB0 // Power supply PIN (D8)

// UART
#define BAUD 9600
#define UBRR_VALUE ((unsigned long)(F_CPU / 16 / BAUD) - 1)

#define NUM_BITS_74HC595 8
#define NUM_OF_74HC595 2
#define OUTPUT_TEST_HOLD_UNTIL_STOP_ENABLED 0

#define TABLE_INDICATOR_REGISTER 0
#define AUX_INDICATOR_REGISTER   1

#define TABLE_9_INDICATOR        (1 << 0)
#define PATH1_FORWARD_INDICATOR  (1 << 4)
#define PATH1_BACKWARD_INDICATOR (1 << 5)
#define PATH2_FORWARD_INDICATOR  (1 << 6)
#define PATH2_BACKWARD_INDICATOR (1 << 7)

// Shift register PINs 74HC165
#define LATCH_74HC165 PD6
#define CLOCK_74HC165 PD5
#define DATA_74HC165  PD7

// Shift register PINs 74HC595
#define DATA_74HC595    PD2
#define CLOCK_74HC595   PD3
#define LATCH_74HC595   PD4

// Uart commands
#define CMD_FORWARD         0x20
#define CMD_BACKWARD        0x21
#define CMD_SLOW_MODE       0x22
#define CMD_ARRIVED         0x23
#define CMD_STOP            0x30
#define CMD_IDEL_STOP       0x31
#define CMD_OVER_LOAD_STOP  0x32
#define CMD_CLEAR_EMERGENCY 0x33
#define CMD_STOP_PATH1 0x34
#define CMD_STOP_PATH2 0x35
#define CMD_DIAG_RESULT 0x36
#define CMD_MENU_REQUEST 0x37
#define CMD_MENU_DATA 0x38
#define CMD_MENU_ENTER 0x39
#define CMD_MENU_EXIT 0x3A
#define CMD_MENU_SET  0x3B
#define DIAG_RESULT_OK 0
#define DIAG_RESULT_RAIL 10

#define MENU_ITEM_SENSORS 1
#define MENU_ITEM_OVERLOAD_THRESHOLD 2
#define MENU_ITEM_PWM_SLOW_PATH1 3
#define MENU_ITEM_PWM_SLOW_PATH2 4
#define MENU_ITEM_LAST MENU_ITEM_PWM_SLOW_PATH2
#define PWM_SLOW_DUTY 120

// Sensors bytes
#define TABLE_1 (1 << 0)
#define TABLE_2 (1 << 1)
#define TABLE_3 (1 << 2)
#define TABLE_4 (1 << 3)
#define TABLE_5 (1 << 4)
#define TABLE_6 (1 << 5)
#define TABLE_7 (1 << 6