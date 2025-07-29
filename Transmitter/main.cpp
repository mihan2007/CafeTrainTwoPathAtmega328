#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "system_init.h"
#include "lcd.h"
#include "shift_registers.h"
#include "config.h"
#include "uart.h"

#define SENSOR_INDEX_OFFSET 1

uint8_t outputByte = 0x00;
int8_t currentTable = -1;
uint8_t currentCommand = CMD_STOP;
uint8_t is_moving = 0;  // ?? Флаг, отслеживающий движение

int8_t get_triggered_sensor(uint8_t states) {
	for (uint8_t i = 0; i < 8; i++) {
		if (states & (1 << i)) {
			return i + SENSOR_INDEX_OFFSET;
		}
	}
	return -1;
}

void activate_ext_logic(void) {
	PORTB |= (1 << POWER_INDICATION_ENABLE);
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
	if (currentTable <= 0) return;

	uint8_t pinState = PINC;
	uint8_t ack = 0;
	char cmdLine[17];
	char tableLine[17];

	snprintf(tableLine, sizeof(tableLine), "TABLE: %-2d        ", currentTable);

	if (!(pinState & (1 << BUTTON_FORWARD_PIN)) && currentCommand != CMD_FORWARD) {
		currentCommand = CMD_FORWARD;
		is_moving = 1;  // ?? Движение началось
		PORTB |= (1 << INDICATOR_FORWARD_PIN);
		PORTB &= ~(1 << INDICATOR_BACKWARD_PIN);
		ack = send_command_with_ack(CMD_FORWARD, currentTable, 0x00);
		snprintf(cmdLine, sizeof(cmdLine), "FORWARD: %s", ack ? "OK" : "FAIL");

		} else if (!(pinState & (1 << BUTTON_BACKWARD_PIN)) && currentCommand != CMD_BACKWARD) {
		currentCommand = CMD_BACKWARD;
		is_moving = 1;  // ?? Движение началось
		PORTB |= (1 << INDICATOR_BACKWARD_PIN);
		PORTB &= ~(1 << INDICATOR_FORWARD_PIN);
		ack = send_command_with_ack(CMD_BACKWARD, currentTable, 0x00);
		snprintf(cmdLine, sizeof(cmdLine), "BACK: %s", ack ? "OK" : "FAIL");

		} else if (!(pinState & (1 << BUTTON_STOP_PIN)) && currentCommand != CMD_STOP) {
		currentCommand = CMD_STOP;
		is_moving = 0;  // ? Остановка
		PORTB &= ~(1 << INDICATOR_FORWARD_PIN);
		PORTB &= ~(1 << INDICATOR_BACKWARD_PIN);
		ack = send_command_with_ack(CMD_STOP, 0x00, 0x00);
		snprintf(cmdLine, sizeof(cmdLine), "STOP: %s", ack ? "OK" : "FAIL");

		} else {
		return;
	}

	LCD_SetCursor(0, 0);
	LCD_Print("                ");  // очистка строки
	LCD_SetCursor(0, 0);
	LCD_Print(cmdLine);
}

int main(void) {
	system_init();       // Инициализация портов, UART и 74HC165
	I2C_Init();          // Для LCD
	LCD_Init();
	LCD_Clear();
	LCD_PrintTwoLines("Welcome", "Select table", 0);
	activate_ext_logic();

	while (1) {
		uint8_t rawBits = read_74HC165();
		uint8_t invertedBits = ~rawBits;
		int8_t detectedTable = get_triggered_sensor(invertedBits);

		if (!is_moving && detectedTable >= 0 && detectedTable != currentTable) {
			handle_table_change(detectedTable);
			char tableLine[17];
			snprintf(tableLine, sizeof(tableLine), "TABLE: %-2d        ", currentTable);
			LCD_Clear();
			LCD_PrintTwoLines("Select command", tableLine, 0);
		}

		handle_control_buttons();
	}
}
