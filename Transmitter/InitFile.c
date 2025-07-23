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
	
	init_button_inputs();
	init_indication_pins();
	
	init_74HC165_ports();
	init_74HC595_ports();
	
	// Очистка регистров при старте
	uint8_t initData[NUM_OF_74HC595] = {0};
	shiftOutMultiple(initData, NUM_OF_74HC595);

	LCD_PrintTwoLines("Waiting for Cmd", "", 0);
}

void init_button_inputs(void) {
	DDRC &= ~((1 << BUTTON_FORWARD_PIN) | (1 << BUTTON_STOP_PIN) | (1 << BUTTON_BACKWARD_PIN));
}

void init_indication_pins(void) {
	DDRB |= (1 << INDICATOR_FORWARD_PIN);    // D10 как выход (PB2)
	DDRB |= (1 << INDICATOR_BACKWARD_PIN);   // D11 как выход (PB3)
	DDRB |= (1 << POWER_INDICATION_ENABLE);  // D8 как выход (PB0)
}
