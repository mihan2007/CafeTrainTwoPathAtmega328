#include "system_init.h"

#include "lcd.h"
#include "uart.h"
#include "timer0.h"
#include "shift_registers.h"
#include "config.h"

#include <avr/interrupt.h>

uint8_t routeSetupInProgress = 0;

void system_init(void) {
	I2C_Init();
	LCD_Init();
	LCD_Clear();

	UART_init();
	timer0_init();

	init_74HC165_ports();
	init_74HC595_ports();
	
	initPWM();
	disablePWM(); // избавляемся от паразитного свечения
	
	DDRB |= (1 << REVERS_PIN);

	// Очистка регистров при старте
	uint8_t initData[NUM_OF_74HC595] = {0};
	shiftOutMultiple(initData, NUM_OF_74HC595);

	sei(); // Включаем глобальные прерывания

	LCD_PrintTwoLines("Waiting for Cmd", "Cmd: ?", 0);
	
}
