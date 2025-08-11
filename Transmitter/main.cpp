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
uint8_t is_moving = 0;

static inline bool is_button_pressed(uint8_t pinState, uint8_t buttonPin) {
	return !(pinState & (1 << buttonPin));
}

int8_t get_triggered_sensor(uint8_t states) {
	for (uint8_t i = 0; i < 8; i++) {
		if (states & (1 << i)) {
			return i + SENSOR_INDEX_OFFSET;
		}
	}
	return -1;
}

void turn_off_directions_ligts(void) {
	PORTB &= ~(1 << INDICATOR_FORWARD_PIN);
	PORTB &= ~(1 << INDICATOR_BACKWARD_PIN);
}

void forward_light_on(){
	PORTB |= (1 << INDICATOR_FORWARD_PIN);
	PORTB &= ~(1 << INDICATOR_BACKWARD_PIN);
}

void bacward_light_on(){
	PORTB |= (1 << INDICATOR_BACKWARD_PIN);
	PORTB &= ~(1 << INDICATOR_FORWARD_PIN);
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

void handle_detected_table(int8_t detectedTable) {
	if (is_moving) return;
	if (detectedTable <= 0) return;
	if (detectedTable == currentTable) return;

	handle_table_change(detectedTable);

	char line2[17];
	snprintf(line2, sizeof(line2), "TABLE: %-2d        ", currentTable);
	LCD_Clear();
	LCD_PrintTwoLines("Select command", line2, 0);
}

void handle_control_buttons(void) {
	if (currentTable <= 0) return;

	uint8_t pinState = PINC;
	uint8_t ack = 0;
	char cmdLine[17];

	bool forwardPressed  = is_button_pressed(pinState, BUTTON_FORWARD_PIN);
	bool backwardPressed = is_button_pressed(pinState, BUTTON_BACKWARD_PIN);
	bool stopPressed     = is_button_pressed(pinState, BUTTON_STOP_PIN);

	bool isForward = (currentCommand == CMD_FORWARD);
	bool isBackward = (currentCommand == CMD_BACKWARD);
	bool isStop = (currentCommand == CMD_STOP);

	if (forwardPressed && !isForward) {
		currentCommand = CMD_FORWARD;
		is_moving = 1;
		forward_light_on();
		ack = send_command_with_ack(CMD_FORWARD, currentTable, 0x00);
		snprintf(cmdLine, sizeof(cmdLine), "FORWARD: %s", ack ? "OK" : "FAIL");

		} else if (backwardPressed && !isBackward) {
		currentCommand = CMD_BACKWARD;
		is_moving = 1;
		bacward_light_on();
		ack = send_command_with_ack(CMD_BACKWARD, currentTable, 0x00);
		snprintf(cmdLine, sizeof(cmdLine), "BACK: %s", ack ? "OK" : "FAIL");

		} else if (stopPressed && !isStop) {
		currentCommand = CMD_STOP;
		is_moving = 0;
		turn_off_directions_ligts();
		ack = send_command_with_ack(CMD_STOP, 0x00, 0x00);
		snprintf(cmdLine, sizeof(cmdLine), "STOP: %s", ack ? "OK" : "FAIL");

		} else {
		return;
	}

	LCD_UpdateLine1(cmdLine);
}

void checkLocoMovementTimeout () {
	if (!is_moving) return;
}

void display_overload_error(void) {
	LCD_Clear();
	LCD_PrintTwoLines("ERROR!", "short circuit", 0);
}

void handle_incoming_uart_packets(void) {
	uint8_t packet_buffer[PACKET_SIZE];

	if (UART_receive_packet(packet_buffer)) {
		uint8_t cmd = packet_buffer[1];
		uint8_t table_id = packet_buffer[2];

		switch (cmd) {
			case CMD_OVER_LOAD_STOP:
				display_overload_error();
				currentCommand = CMD_STOP;
				is_moving = 0;
				turn_off_directions_ligts();
			break;
			
			case CMD_CLEAR_EMERGENCY: {
					char line2[17];
					snprintf(line2, sizeof(line2), "TABLE: %-2d        ", currentTable);
					LCD_Clear();
					LCD_PrintTwoLines("Select command", line2, 0);
			}
			break;
			
			case CMD_ARRIVED: {
				uint8_t prevCmd = currentCommand;   // запоминаем направление

				is_moving = 0;
				turn_off_directions_ligts();

				char line2[17];
				snprintf(line2, sizeof(line2), "TABLE: %-2d        ", table_id);

				LCD_Clear();
				if (prevCmd == CMD_BACKWARD) {
					// Верхняя строка с указанием кухни
					LCD_PrintTwoLines("ARRIVED KITCHEN", line2, 0);
					} else {
					LCD_PrintTwoLines("ARRIVED", line2, 0);
				}

				currentCommand = CMD_STOP;
				break;
			}


			default:
			// другие команды по мере необходимости
			break;
		}
	}
}



int main(void) {
	system_init();       // Инициализация портов, UART и 74HC165
	I2C_Init();          // Для LCD
	LCD_Init();
	LCD_Clear();
	LCD_PrintTwoLines("Welcome", "Select table", 0);
	activate_ext_logic();
	
	send_command_with_ack(CMD_STOP, 0x00, 0x00);
		
	
	while (1) {
		uint8_t packet_buffer[PACKET_SIZE];	
			
		checkLocoMovementTimeout();
		
		uint8_t rawBits = read_74HC165();
		uint8_t invertedBits = ~rawBits;
		int8_t detectedTable = get_triggered_sensor(invertedBits);

		handle_detected_table(detectedTable);
		handle_control_buttons();
		handle_incoming_uart_packets();
	}

	return 0;
}
