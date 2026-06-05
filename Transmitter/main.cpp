#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "system_init.h"
#include "lcd.h"
#include "shift_registers.h"
#include "config.h"
#include "uart.h"

#define PATH1_FIRST_TABLE 1
#define PATH1_LAST_TABLE 4
#define PATH2_FIRST_TABLE 5
#define PATH2_LAST_TABLE 9

uint8_t shiftRegisterState[NUM_OF_74HC595] = {0};
int8_t selectedPath1Table = -1;
int8_t selectedPath2Table = -1;
uint8_t path2TableMask = 0;
uint8_t path1DirectionMask = 0;
uint8_t path2DirectionMask = 0;
uint8_t currentCommand = CMD_STOP;
uint8_t path2Command = CMD_STOP;
uint8_t is_moving = 0;
uint8_t path2_moving = 0;
char path1Status[10] = "WAIT";
char path2Status[10] = "WAIT";

static inline bool is_button_pressed(uint8_t pinState, uint8_t buttonPin) {
	return !(pinState & (1 << buttonPin));
}

void update_shift_register_for_tables(void);

uint16_t ADC_Read_Button(uint8_t channel) {
	ADMUX = (1 << REFS0) | (channel & 0x07);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

bool is_adc_button_pressed(uint8_t channel) {
	return ADC_Read_Button(channel) < ADC_BUTTON_THRESHOLD;
}

void write_shift_register_state(void) {
	shiftOutMultiple(shiftRegisterState, NUM_OF_74HC595);
}

void run_output_register_startup_test(void) {
#if OUTPUT_TEST_HOLD_UNTIL_STOP_ENABLED
	for (uint8_t i = 0; i < NUM_OF_74HC595; i++) {
		shiftRegisterState[i] = 0xFF;
	}
	write_shift_register_state();
	LCD_Clear();
	LCD_PrintTwoLines("OUTPUT TEST", "Press STOP", 0);

	while (!is_button_pressed(PINC, BUTTON_STOP_PIN)) {
		_delay_ms(20);
	}

	for (uint8_t i = 0; i < NUM_OF_74HC595; i++) {
		shiftRegisterState[i] = 0x00;
	}
	write_shift_register_state();
	_delay_ms(300);
#else
	for (uint8_t reg = 0; reg < NUM_OF_74HC595; reg++) {
		for (uint8_t bit = 0; bit < NUM_BITS_74HC595; bit++) {
			for (uint8_t i = 0; i < NUM_OF_74HC595; i++) {
				shiftRegisterState[i] = 0x00;
			}

			shiftRegisterState[reg] = (1 << bit);
			write_shift_register_state();
			_delay_ms(1000);
		}
	}

	for (uint8_t i = 0; i < NUM_OF_74HC595; i++) {
		shiftRegisterState[i] = 0x00;
	}
	write_shift_register_state();
#endif
}

void activate_ext_logic(void) {
	PORTB |= (1 << POWER_INDICATION_ENABLE);
}

bool is_table9_pressed(void) {
	if (!TABLE9_BUTTON_ENABLED) return false;
	return !(PINB & (1 << TABLE9_BUTTON_PIN));
}

int8_t get_detected_path1_table(uint8_t states) {
	for (uint8_t i = 0; i < 4; i++) {
		if (states & (1 << i)) return i + 1;
	}
	return -1;
}

int8_t get_detected_path2_table(uint8_t states) {
	if (is_table9_pressed()) return 9;

	for (uint8_t i = 4; i < 8; i++) {
		if (states & (1 << i)) return i + 1;
	}
	return -1;
}

uint8_t get_path2_table_mask(uint8_t states) {
	return states & (TABLE_5 | TABLE_6 | TABLE_7 | TABLE_8);
}

void set_path1_direction_indicator(uint8_t cmd) {
	path1DirectionMask = 0;

	if (cmd == CMD_FORWARD) {
		path1DirectionMask = PATH1_FORWARD_INDICATOR;
	} else if (cmd == CMD_BACKWARD) {
		path1DirectionMask = PATH1_BACKWARD_INDICATOR;
	}

	update_shift_register_for_tables();
}

void set_path2_direction_indicator(uint8_t cmd) {
	path2DirectionMask = 0;

	if (cmd == CMD_FORWARD) {
		path2DirectionMask = PATH2_FORWARD_INDICATOR;
	} else if (cmd == CMD_BACKWARD) {
		path2DirectionMask = PATH2_BACKWARD_INDICATOR;
	}

	update_shift_register_for_tables();
}

void update_shift_register_for_tables(void) {
	uint8_t tableBits = 0;
	uint8_t auxBits = path1DirectionMask | path2DirectionMask;

	if (TABLE9_BUTTON_ENABLED) {
		auxBits &= ~TABLE_9_INDICATOR;
	}

	if (selectedPath1Table >= PATH1_FIRST_TABLE && selectedPath1Table <= PATH1_LAST_TABLE) {
		tableBits |= (1 << (selectedPath1Table - 1));
	}

	tableBits |= path2TableMask;

	if (TABLE9_BUTTON_ENABLED && selectedPath2Table == 9) {
		auxBits |= TABLE_9_INDICATOR;
	}

	shiftRegisterState[TABLE_INDICATOR_REGISTER] = tableBits;
	shiftRegisterState[AUX_INDICATOR_REGISTER] = auxBits;
	write_shift_register_state();
}

void format_table_status_line(char *buffer, uint8_t bufferSize, const char *pathName, int8_t table, const char *status) {
	if (table > 0) {
		snprintf(buffer, bufferSize, "%s T%-2d %s", pathName, table, status);
	} else {
		snprintf(buffer, bufferSize, "%s T-  %s", pathName, status);
	}
}

void update_lcd_for_tables(void) {
	char line1[17];
	char line2[17];

	format_table_status_line(line1, sizeof(line1), "P1", selectedPath1Table, path1Status);
	format_table_status_line(line2, sizeof(line2), "P2", selectedPath2Table, path2Status);
	LCD_Clear();
	LCD_PrintTwoLines(line1, line2, 0);
}

void handle_path1_table_change(int8_t newTable) {
	if (is_moving) return;	
	if (newTable <= 0) return;
	if (newTable == selectedPath1Table) return;

	selectedPath1Table = newTable;
	snprintf(path1Status, sizeof(path1Status), "SELECT");
	update_shift_register_for_tables();
	update_lcd_for_tables();
}

void handle_path2_table_change(int8_t newTable) {
	if (path2_moving) return;
	if (newTable <= 0) return;
	if (newTable == selectedPath2Table) return;

	selectedPath2Table = newTable;
	snprintf(path2Status, sizeof(path2Status), "SELECT");
	if (selectedPath2Table == 9) {
		path2TableMask = 0;
	}
	update_shift_register_for_tables();
	update_lcd_for_tables();
}

void handle_table_inputs(uint8_t states) {
	int8_t path1Table = get_detected_path1_table(states);
	int8_t path2Table = get_detected_path2_table(states);
	uint8_t newPath2TableMask = get_path2_table_mask(states);

	handle_path1_table_change(path1Table);

	if (!path2_moving && newPath2TableMask) {
		path2TableMask = newPath2TableMask;
	}

	handle_path2_table_change(path2Table);
}

void clear_motion_state(void) {
	currentCommand = CMD_STOP;
	is_moving = 0;
	set_path1_direction_indicator(CMD_STOP);
}

void clear_path2_motion_state(void) {
	path2Command = CMD_STOP;
	path2_moving = 0;
	set_path2_direction_indicator(CMD_STOP);
}

void set_path1_command_status(const char *cmdText, uint8_t ack) {
	snprintf(path1Status, sizeof(path1Status), "%s %s", cmdText, ack ? "OK" : "F");
	update_lcd_for_tables();
}

void set_path2_command_status(const char *cmdText, uint8_t ack) {
	snprintf(path2Status, sizeof(path2Status), "%s %s", cmdText, ack ? "OK" : "F");
	update_lcd_for_tables();
}

void handle_control_buttons(void) {
	uint8_t pinState = PINC;
	uint8_t ack = 0;

	bool forwardPressed  = is_button_pressed(pinState, BUTTON_FORWARD_PIN);
	bool backwardPressed = is_button_pressed(pinState, BUTTON_BACKWARD_PIN);
	bool stopPressed     = is_button_pressed(pinState, BUTTON_STOP_PIN);
	bool path2ForwardPressed = is_button_pressed(pinState, PATH2_BUTTON_FORWARD_PIN);
	bool path2BackwardPressed = PATH2_CONTROL_BUTTONS_ENABLED && is_adc_button_pressed(PATH2_BUTTON_BACKWARD_ADC);
	bool path2StopPressed = PATH2_CONTROL_BUTTONS_ENABLED && is_adc_button_pressed(PATH2_BUTTON_STOP_ADC);

	if (stopPressed) {
		ack = send_command_with_ack(CMD_STOP_PATH1, selectedPath1Table > 0 ? selectedPath1Table : 0x00, 0x00);
		clear_motion_state();
		set_path1_command_status("STOP", ack);
		return;
	}

	if (path2StopPressed) {
		ack = send_command_with_ack(CMD_STOP_PATH2, selectedPath2Table > 0 ? selectedPath2Table : 0x00, 0x00);
		clear_path2_motion_state();
		set_path2_command_status("STOP", ack);
		return;
	}

	if (selectedPath1Table > 0 && forwardPressed && currentCommand != CMD_FORWARD) {
		currentCommand = CMD_FORWARD;
		is_moving = 1;
		set_path1_direction_indicator(CMD_FORWARD);
		ack = send_command_with_ack(CMD_FORWARD, selectedPath1Table, 0x00);
		set_path1_command_status("FWD", ack);
	} else if (selectedPath1Table > 0 && backwardPressed && currentCommand != CMD_BACKWARD) {
		currentCommand = CMD_BACKWARD;
		is_moving = 1;
		set_path1_direction_indicator(CMD_BACKWARD);
		ack = send_command_with_ack(CMD_BACKWARD, selectedPath1Table, 0x00);
		set_path1_command_status("BACK", ack);
	}

	if (!PATH2_CONTROL_BUTTONS_ENABLED) return;
	if (selectedPath2Table <= 0) return;

	if (path2ForwardPressed && path2Command != CMD_FORWARD) {
		path2Command = CMD_FORWARD;
		path2_moving = 1;
		set_path2_direction_indicator(CMD_FORWARD);
		ack = send_command_with_ack(CMD_FORWARD, selectedPath2Table, 0x00);
		set_path2_command_status("FWD", ack);
	} else if (path2BackwardPressed && path2Command != CMD_BACKWARD) {
		path2Command = CMD_BACKWARD;
		path2_moving = 1;
		set_path2_direction_indicator(CMD_BACKWARD);
		ack = send_command_with_ack(CMD_BACKWARD, selectedPath2Table, 0x00);
		set_path2_command_status("BACK", ack);
	}
}

void display_overload_error(void) {
	snprintf(path1Status, sizeof(path1Status), "ERR");
	snprintf(path2Status, sizeof(path2Status), "ERR");
	update_lcd_for_tables();
}

void handle_incoming_uart_packets(void) {
	uint8_t packet_buffer[PACKET_SIZE];

	if (UART_receive_packet(packet_buffer)) {
		uint8_t cmd = packet_buffer[1];
		uint8_t table_id = packet_buffer[2];
		uint8_t seq = packet_buffer[3];

		switch (cmd) {
			case CMD_OVER_LOAD_STOP:
				send_command(ACK_CMD, cmd, seq);
				display_overload_error();
				clear_motion_state();
				clear_path2_motion_state();
			break;
			
			case CMD_CLEAR_EMERGENCY:
				send_command(ACK_CMD, cmd, seq);
				update_lcd_for_tables();
			break;
			
			case CMD_ARRIVED: {
				uint8_t arrivedCommand = currentCommand;
				uint8_t path2ArrivedCommand = path2Command;

				if (table_id == selectedPath1Table) {
					if (arrivedCommand == CMD_BACKWARD) {
						snprintf(path1Status, sizeof(path1Status), "KITCHEN");
					} else {
						snprintf(path1Status, sizeof(path1Status), "AT TABLE");
					}
					clear_motion_state();
				} else if (table_id == selectedPath2Table) {
					if (path2ArrivedCommand == CMD_BACKWARD) {
						snprintf(path2Status, sizeof(path2Status), "KITCHEN");
					} else {
						snprintf(path2Status, sizeof(path2Status), "AT TABLE");
					}
					clear_path2_motion_state();
				}
				update_lcd_for_tables();
			break;
			}

			default:
			break;
		}
	}
}

void checkLocoMovementTimeout(void) {
	if (!is_moving && !path2_moving) return;
}

int main(void) {
	system_init();
	LCD_Clear();
	activate_ext_logic();
	run_output_register_startup_test();
	update_lcd_for_tables();
	
	send_command_with_ack(CMD_STOP, 0x00, 0x00);
		
	while (1) {
		checkLocoMovementTimeout();
		
		uint8_t rawBits = read_74HC165();
		uint8_t invertedBits = ~rawBits;

		handle_table_inputs(invertedBits);
		handle_control_buttons();
		handle_incoming_uart_packets();
	}

	return 0;
}
