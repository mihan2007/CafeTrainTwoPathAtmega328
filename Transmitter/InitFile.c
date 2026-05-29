#include "system_init.h"

#include "lcd.h"
#include "uart.h"
#include "shift_registers.h"
#include "config.h"

#include <avr/interrupt.h>

uint8_t routeSetupInProgress = 0;

void init_adc_buttons(void) {
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

void system_init(void) {
	I2C_Init();
	LCD_Init();
	LCD_Clear();

	UART_init();
	
	init_button_inputs();
	init_indication_pins();
	init_adc_buttons();
	
	init_74HC165_ports();
	init_74HC595_ports();
	
	uint8_t initData[NUM_OF_74HC595] = {0};
	shiftOutMultiple(initData, NUM_OF_74HC595);

	LCD_PrintTwoLines("Waiting for Cmd", "", 0);
}

void init_button_inputs(void) {
	DDRC &= ~((1 << BUTTON_FORWARD_PIN) |
	          (1 << BUTTON_STOP_PIN) |
	          (1 << BUTTON_BACKWARD_PIN) |
	          (1 << PATH2_BUTTON_FORWARD_PIN));

	DDRB &= ~(1 << TABLE9_BUTTON_PIN);
	PORTB |= (1 << TABLE9_BUTTON_PIN);
}

void init_indication_pins(void) {
	DDRB |= (1 << POWER_INDICATION_ENABLE);
	PORTB &= ~(1 << POWER_INDICATION_ENABLE);
}
