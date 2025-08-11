#ifndef CONFIG_H_
#define CONFIG_H_

#include <avr/io.h>

#define F_CPU 16000000UL

// Защита по времени от работы на холостом ходу
#define TICKS_PER_UNIT 10000
#define MAX_TIME_UNITS 30

// Защита по времени от работы на холостом ходу
#define TICKS_PER_UNIT 10000
#define MAX_TIME_UNITS 30

//  Control Buttons
#define BUTTON_FORWARD_PIN   PC3  // A3
#define BUTTON_STOP_PIN      PC2  // A2
#define BUTTON_BACKWARD_PIN  PC1  // A1

//	Indication
#define INDICATOR_FORWARD_PIN PB2   // D0 — выход, индикатор "Вперёд"
#define INDICATOR_BACKWARD_PIN PB3 // D11 — выход, индикатор "Назад"
#define POWER_INDICATION_ENABLE PB0// Power supply PIN (D8)

//  UART
#define BAUD 9600
#define UBRR_VALUE ((unsigned long)(F_CPU / 16 / BAUD) - 1)

#define NUM_BITS_74HC595 8  // Количество бит в байте
#define NUM_OF_74HC595 1    // Количество включенных в каскад 74HC595


// Shift register PINs 74HC165
#define LATCH_74HC165 PD6   // LATCH для 74HC165
#define CLOCK_74HC165 PD5   // CLOCK для 74HC165
#define DATA_74HC165  PD7   // DATA для 74HC165

// Shift register PINs 74HC595
#define DATA_74HC595    PD2
#define CLOCK_74HC595   PD3
#define LATCH_74HC595   PD4

// Uart Comands
#define CMD_FORWARD		0x20
#define CMD_BACKWARD	0x21
#define CMD_SLOW_MODE	0x22
#define CMD_ARRIVED		0x23
#define CMD_STOP		0x30
#define CMD_IDEL_STOP	0x31
#define CMD_OVER_LOAD_STOP	0x32
#define CMD_CLEAR_EMERGENCY 0x33

// Sensors bytes
#define TABLE_1 (1 << 0)
#define TABLE_2 (1 << 1)
#define TABLE_3 (1 << 2)
#define TABLE_4 (1 << 3)
#define TABLE_5 (1 << 4)
#define TABLE_6 (1 << 5)
#define TABLE_7 (1 << 6)
#define TABLE_8 (1 << 7)

#endif /* CONFIG_H_ */
