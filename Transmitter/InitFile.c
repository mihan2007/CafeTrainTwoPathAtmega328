#include "system_init.h"

#include "lcd.h"
#include "uart.h"
#include "shift_registers.h"
#include "config.h"

#include <avr/interrupt.h>

uint8_t routeSetupInProgress = 0;

void system_init(void) {
	I2C_Init();
	LCD_Init();
	LCD_Clear();

	UART_init();


	init_74HC165_ports();
	init_74HC595_ports();
	
	DDRB |= (1 << RAIL_POWER_ENABLE);
	
	// Очистка регистров при старте
	uint8_t initData[NUM_OF_74HC595] = {0};
	shiftOutMultiple(initData, NUM_OF_74HC595);

	LCD_PrintTwoLines("Waiting for Cmd", "", 0);
}
