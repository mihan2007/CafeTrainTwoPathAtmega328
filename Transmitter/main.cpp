#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "system_init.h"
#include "lcd.h"
#include "shift_registers.h"
#include "config.h"

#define SENSOR_INDEX_OFFSET 1

uint8_t outputByte = 0x00;
int8_t currentTable = -1;
uint8_t currentCommand = CMD_STOP;
	
int8_t get_triggered_sensor(uint8_t states) {
	for (uint8_t i = 0; i < 8; i++) {
		if (states & (1 << i)) {
			return i + SENSOR_INDEX_OFFSET;
		}
	}
	return -1;
}

void activate_ext_logic(void){
	PORTB |= (1 << POWER_INDICATION_ENABLE);  // Светодиод
}

void update_lcd_for_table(int8_t tableIndex) {
	char line2[17];
	if (tableIndex > 0) {
		snprintf(line2, sizeof(line2), "TABLE: %-2d        ", tableIndex);
		} else {
		snprintf(line2, sizeof(line2), "TABLE: -           ");
	}
	LCD_PrintTwoLines("Select command", line2, 0);
}

void update_shift_register_for_table(int8_t tableIndex) {
	if (tableIndex > 0) {
		outputByte = (1 << (tableIndex - 1));
		} else {
		outputByte = 0x00;
	}
	shiftOut(outputByte);
	latchData();
}

void handle_table_change(int8_t newTable) {
	currentTable = newTable;
	
	update_lcd_for_table(currentTable);
	update_shift_register_for_table(currentTable);
}

void handle_control_buttons(void) {
	uint8_t pinState = PINC;

	if (!(pinState & (1 << BUTTON_FORWARD_PIN)) && currentCommand != CMD_FORWARD) {
		currentCommand = CMD_FORWARD;

		PORTB |= (1 << INDICATOR_FORWARD_PIN);
		PORTB &= ~(1 << INDICATOR_BACKWARD_PIN);

		LCD_UpdateLine1("FORWARD");

		} else if (!(pinState & (1 << BUTTON_BACKWARD_PIN)) && currentCommand != CMD_BACKWARD) {
		currentCommand = CMD_BACKWARD;

		PORTB |= (1 << INDICATOR_BACKWARD_PIN);
		PORTB &= ~(1 << INDICATOR_FORWARD_PIN);

		LCD_UpdateLine1("BACKWARD");

		} else if (!(pinState & (1 << BUTTON_STOP_PIN)) && currentCommand != CMD_STOP) {
		currentCommand = CMD_STOP;

		PORTB &= ~(1 << INDICATOR_FORWARD_PIN);
		PORTB &= ~(1 << INDICATOR_BACKWARD_PIN);

		LCD_UpdateLine1("Direction: STOP");
	}
}


int main(void) {
	system_init();
	
	LCD_Clear();
	LCD_PrintTwoLines("Welcome", "Select table", 0);
	activate_ext_logic();


while (1) {

		
		uint8_t rawBits = read_74HC165();
		uint8_t invertedBits = ~rawBits;

		int8_t detectedTable = get_triggered_sensor(~read_74HC165());

		if (detectedTable >= 0 && detectedTable != currentTable) {
			handle_table_change(detectedTable);
		}
		
		handle_control_buttons();

	}
}

