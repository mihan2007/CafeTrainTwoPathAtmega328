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
#define MENU_HOLD_TICKS 50
#define MENU_REQUEST_TICKS 5

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
uint8_t arrivedTableByPath[3] = {0, 0, 0};
char path1Status[10] = "WAIT";
char path2Status[10] = "WAIT";
uint8_t diagnosticResultByPath[3] = {DIAG_RESULT_OK, DIAG_RESULT_OK, DIAG_RESULT_OK};
uint8_t transmitterEmergencyActive = 0;
uint8_t menuModeActive = 0;
uint8_t menuHoldCounter = 0;
uint8_t menuHoldArmed = 1;
uint8_t menuNavArmed = 1;
uint8_t menuItem = MENU_ITEM_SENSORS;
uint8_t menuData[MENU_ITEM_LAST + 1] = {0};
uint8_t menuDataValid[MENU_ITEM_LAST + 1] = {0};
uint8_t menuRequestCounter = 0;
uint8_t menuEditMode = 0;
uint8_t menuEditValue = 0;
uint8_t menuStopArmed = 1;

static inline bool is_button_pressed(uint8_t pinState, uint8_t buttonPin) {
	return !(pinState & (1 << buttonPin));
}

void update_shift_register_for_tables(void);
void display_diagnostic_results(void);
void display_menu_screen(void);
void poll_menu_data_if_due(void);
void handle_menu_stop(uint8_t stopPressed);

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
	if (transmitterEmergencyActive || menuModeActive) return;

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
	if (transmitterEmergencyActive || menuModeActive) return;

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

void request_menu_data(uint8_t item) {
	if (item < MENU_ITEM_SENSORS || item > MENU_ITEM_LAST) return;
	send_command(CMD_MENU_REQUEST, item, 0x00);
}

uint8_t menu_item_is_editable(uint8_t item) {
	return item == MENU_ITEM_OVERLOAD_THRESHOLD || item == MENU_ITEM_PWM_SLOW_PATH1 || item == MENU_ITEM_PWM_SLOW_PATH2;
}

uint8_t menu_default_value(uint8_t item) {
	return (item == MENU_ITEM_OVERLOAD_THRESHOLD) ? 90 : PWM_SLOW_DUTY;
}

uint8_t menu_edit_step(uint8_t item) {
	return (item == MENU_ITEM_OVERLOAD_THRESHOLD) ? 1 : 5;
}

uint8_t menu_edit_min(uint8_t item) {
	return (item == MENU_ITEM_OVERLOAD_THRESHOLD) ? 1 : 0;
}

uint8_t menu_edit_max(uint8_t item) {
	return (item == MENU_ITEM_OVERLOAD_THRESHOLD) ? 102 : 255;
}

uint8_t send_menu_set_with_ack(uint8_t item, uint8_t value) {
	uint8_t response[PACKET_SIZE];

	for (uint8_t retry = 0; retry < MAX_RETRIES; retry++) {
		send_command(CMD_MENU_SET, item, value);

		if (UART_receive_packet(response)) {
			if (response[1] == ACK_CMD && response[2] == CMD_MENU_SET && response[3] == value) {
				return 1;
			}
		}
	}

	return 0;
}

void update_menu_data(uint8_t item, uint8_t value) {
	if (item < MENU_ITEM_SENSORS || item > MENU_ITEM_LAST) return;
	menuData[item] = value;
	menuDataValid[item] = 1;
}

char bit_char(uint8_t value, uint8_t bit) {
	return (value & (1 << bit)) ? '1' : '0';
}

void format_menu_value_line(char *buffer, uint8_t bufferSize, uint8_t item) {
	uint8_t value = menuEditMode ? menuEditValue : menuData[item];

	if (!menuEditMode && !menuDataValid[item]) {
		snprintf(buffer, bufferSize, "WAIT DATA");
		return;
	}

	switch (item) {
		case MENU_ITEM_SENSORS:
			snprintf(buffer, bufferSize, "P1:%c%c%c%c P2:%c%c%c%c",
				bit_char(menuData[item], 0),
				bit_char(menuData[item], 1),
				bit_char(menuData[item], 2),
				bit_char(menuData[item], 3),
				bit_char(menuData[item], 4),
				bit_char(menuData[item], 5),
				bit_char(menuData[item], 6),
				bit_char(menuData[item], 7));
			break;

		case MENU_ITEM_OVERLOAD_THRESHOLD:
			snprintf(buffer, bufferSize, menuEditMode ? "ADC %u *" : "ADC %u", (uint16_t)value * 10);
			break;

		default:
			snprintf(buffer, bufferSize, menuEditMode ? "PWM %u *" : "PWM %u", value);
			break;
	}
}

void display_menu_screen(void) {
	char line1[17];
	char line2[17];

	switch (menuItem) {
		case MENU_ITEM_SENSORS:
			snprintf(line1, sizeof(line1), "SENSORS");
			break;

		case MENU_ITEM_OVERLOAD_THRESHOLD:
			snprintf(line1, sizeof(line1), "SHORT LIMIT");
			break;

		case MENU_ITEM_PWM_SLOW_PATH1:
			snprintf(line1, sizeof(line1), "P1 SLOW PWM");
			break;

		case MENU_ITEM_PWM_SLOW_PATH2:
			snprintf(line1, sizeof(line1), "P2 SLOW PWM");
			break;

		default:
			snprintf(line1, sizeof(line1), "MENU");
			break;
	}

	format_menu_value_line(line2, sizeof(line2), menuItem);
	LCD_Clear();
	LCD_PrintTwoLines(line1, line2, 0);
}

void enter_menu_mode(void) {
	if (is_moving || path2_moving) return;

	menuModeActive = 1;
	menuHoldArmed = 0;
	menuHoldCounter = 0;
	menuNavArmed = 0;
	menuStopArmed = 0;
	menuEditMode = 0;
	menuEditValue = 0;
	menuItem = MENU_ITEM_SENSORS;
	menuRequestCounter = 0;
	display_menu_screen();
	request_menu_data(menuItem);
}

void exit_menu_mode(void) {
	UART_discard_pending();
	menuModeActive = 0;
	menuHoldArmed = 0;
	menuHoldCounter = 0;
	menuEditMode = 0;
	menuEditValue = 0;
	currentCommand = CMD_STOP;
	path2Command = CMD_STOP;
	is_moving = 0;
	path2_moving = 0;
	update_lcd_for_tables();
}

uint8_t handle_menu_buttons(uint8_t stopPressed, uint8_t path2StopPressed) {
	uint8_t bothStopPressed = stopPressed && path2StopPressed;

	if (!bothStopPressed) {
		menuHoldCounter = 0;
		menuHoldArmed = 1;
		return menuModeActive;
	}

	if (!menuHoldArmed) {
		return 1;
	}

	if (menuHoldCounter < MENU_HOLD_TICKS) {
		menuHoldCounter++;
	}

	if (menuHoldCounter >= MENU_HOLD_TICKS) {
		if (menuModeActive) {
			exit_menu_mode();
		} else {
			enter_menu_mode();
		}
		menuHoldArmed = 0;
		menuHoldCounter = 0;
	}

	return 1;
}

void handle_menu_navigation(uint8_t forwardPressed, uint8_t backwardPressed) {
	if (!menuModeActive) return;

	if (!forwardPressed && !backwardPressed) {
		menuNavArmed = 1;
		return;
	}

	if (!menuNavArmed) return;

	if (menuEditMode) {
		uint8_t step = menu_edit_step(menuItem);
		uint8_t minValue = menu_edit_min(menuItem);
		uint8_t maxValue = menu_edit_max(menuItem);

		if (forwardPressed) {
			menuEditValue = (menuEditValue > maxValue - step) ? maxValue : menuEditValue + step;
		} else if (backwardPressed) {
			menuEditValue = (menuEditValue < minValue + step) ? minValue : menuEditValue - step;
		}

		menuNavArmed = 0;
		display_menu_screen();
		return;
	}

	if (forwardPressed) {
		menuItem++;
		if (menuItem > MENU_ITEM_LAST) {
			menuItem = MENU_ITEM_SENSORS;
		}
	} else if (backwardPressed) {
		if (menuItem <= MENU_ITEM_SENSORS) {
			menuItem = MENU_ITEM_LAST;
		} else {
			menuItem--;
		}
	}

	menuNavArmed = 0;
	menuRequestCounter = 0;
	display_menu_screen();
	request_menu_data(menuItem);
}

void handle_menu_stop(uint8_t stopPressed) {
	if (!menuModeActive) return;

	if (!stopPressed) {
		menuStopArmed = 1;
		return;
	}

	if (!menuStopArmed) return;
	menuStopArmed = 0;

	if (!menu_item_is_editable(menuItem)) return;

	if (!menuEditMode) {
		menuEditMode = 1;
		menuEditValue = menuDataValid[menuItem] ? menuData[menuItem] : menu_default_value(menuItem);
		menuNavArmed = 0;
		display_menu_screen();
		return;
	}

	if (send_menu_set_with_ack(menuItem, menuEditValue)) {
		menuData[menuItem] = menuEditValue;
		menuDataValid[menuItem] = 1;
		menuEditMode = 0;
		LCD_Clear();
		LCD_PrintTwoLines((char*)"SAVED", (char*)"", 0);
		_delay_ms(300);
	} else {
		LCD_Clear();
		LCD_PrintTwoLines((char*)"SAVE FAILED", (char*)"", 0);
		_delay_ms(500);
	}

	display_menu_screen();
}

void poll_menu_data_if_due(void) {
	if (!menuModeActive) {
		menuRequestCounter = 0;
		return;
	}

	if (menuEditMode || menuItem != MENU_ITEM_SENSORS) {
		menuRequestCounter = 0;
		return;
	}

	if (menuRequestCounter < MENU_REQUEST_TICKS) {
		menuRequestCounter++;
		return;
	}

	menuRequestCounter = 0;
	request_menu_data(menuItem);
}
uint8_t has_active_diagnostic_error(void) {
	return diagnosticResultByPath[1] != DIAG_RESULT_OK || diagnosticResultByPath[2] != DIAG_RESULT_OK;
}

void refresh_emergency_display_or_unlock(void) {
	if (has_active_diagnostic_error()) {
		display_diagnostic_results();
		return;
	}

	transmitterEmergencyActive = 0;
	update_lcd_for_tables();
}

uint8_t handle_emergency_stop_buttons(void) {
	uint8_t pinState = PINC;
	uint8_t handled = 0;
	uint8_t ack = 0;

	bool stopPressed = is_button_pressed(pinState, BUTTON_STOP_PIN);
	bool path2StopPressed = PATH2_CONTROL_BUTTONS_ENABLED && is_adc_button_pressed(PATH2_BUTTON_STOP_ADC);
	if (stopPressed) {
		ack = send_command_with_ack(CMD_STOP_PATH1, selectedPath1Table > 0 ? selectedPath1Table : 0x00, 0x00);
		(void)ack;
		clear_motion_state();
		diagnosticResultByPath[1] = DIAG_RESULT_OK;
		transmitterEmergencyActive = 0;
		refresh_emergency_display_or_unlock();
		handled = 1;
	}

	if (path2StopPressed) {
		ack = send_command_with_ack(CMD_STOP_PATH2, selectedPath2Table > 0 ? selectedPath2Table : 0x00, 0x00);
		(void)ack;
		clear_path2_motion_state();
		diagnosticResultByPath[2] = DIAG_RESULT_OK;
		transmitterEmergencyActive = 0;
		refresh_emergency_display_or_unlock();
		handled = 1;
	}

	return handled;
}
void handle_control_buttons(void) {
	if (transmitterEmergencyActive) {
		handle_emergency_stop_buttons();
		return;
	}

	uint8_t pinState = PINC;
	uint8_t ack = 0;

	bool forwardPressed  = is_button_pressed(pinState, BUTTON_FORWARD_PIN);
	bool backwardPressed = is_button_pressed(pinState, BUTTON_BACKWARD_PIN);
	bool stopPressed     = is_button_pressed(pinState, BUTTON_STOP_PIN);
	bool path2ForwardPressed = is_button_pressed(pinState, PATH2_BUTTON_FORWARD_PIN);
	bool path2BackwardPressed = PATH2_CONTROL_BUTTONS_ENABLED && is_adc_button_pressed(PATH2_BUTTON_BACKWARD_ADC);
	bool path2StopPressed = PATH2_CONTROL_BUTTONS_ENABLED && is_adc_button_pressed(PATH2_BUTTON_STOP_ADC);

	if (handle_menu_buttons(stopPressed, path2StopPressed)) {
		if (menuModeActive) {
			handle_menu_stop(stopPressed && !path2StopPressed);
			handle_menu_navigation(forwardPressed, backwardPressed);
		}
		return;
	}

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
		if (arrivedTableByPath[1] == selectedPath1Table) {
			return;
		}
		currentCommand = CMD_FORWARD;
		is_moving = 1;
		set_path1_direction_indicator(CMD_FORWARD);
		ack = send_command_with_ack(CMD_FORWARD, selectedPath1Table, 0x00);
		set_path1_command_status("FWD", ack);
	} else if (selectedPath1Table > 0 && backwardPressed && currentCommand != CMD_BACKWARD) {
		arrivedTableByPath[1] = 0;
		currentCommand = CMD_BACKWARD;
		is_moving = 1;
		set_path1_direction_indicator(CMD_BACKWARD);
		ack = send_command_with_ack(CMD_BACKWARD, selectedPath1Table, 0x00);
		set_path1_command_status("BACK", ack);
	}

	if (!PATH2_CONTROL_BUTTONS_ENABLED) return;
	if (selectedPath2Table <= 0) return;

	if (path2ForwardPressed && path2Command != CMD_FORWARD) {
		if (arrivedTableByPath[2] == selectedPath2Table) {
			return;
		}
		path2Command = CMD_FORWARD;
		path2_moving = 1;
		set_path2_direction_indicator(CMD_FORWARD);
		ack = send_command_with_ack(CMD_FORWARD, selectedPath2Table, 0x00);
		set_path2_command_status("FWD", ack);
	} else if (path2BackwardPressed && path2Command != CMD_BACKWARD) {
		arrivedTableByPath[2] = 0;
		path2Command = CMD_BACKWARD;
		path2_moving = 1;
		set_path2_direction_indicator(CMD_BACKWARD);
		ack = send_command_with_ack(CMD_BACKWARD, selectedPath2Table, 0x00);
		set_path2_command_status("BACK", ack);
	}
}

void display_overload_error(void) {
	transmitterEmergencyActive = 1;
	LCD_Clear();
	LCD_PrintTwoLines((char*)"EMERGENCY", (char*)"OVERLOAD", 0);
}

void format_diagnostic_result_line(char *buffer, uint8_t bufferSize, uint8_t path, uint8_t result) {
	if (result == DIAG_RESULT_OK) {
		snprintf(buffer, bufferSize, "P%u OK", path);
	} else if (result == DIAG_RESULT_RAIL) {
		snprintf(buffer, bufferSize, "P%u SHORTED RAIL", path);
	} else {
		snprintf(buffer, bufferSize, "P%u SHORTED T%u", path, result);
	}
}

void display_diagnostic_results(void) {
	transmitterEmergencyActive = 1;
	char line1[17];
	char line2[17];
	format_diagnostic_result_line(line1, sizeof(line1), 1, diagnosticResultByPath[1]);
	format_diagnostic_result_line(line2, sizeof(line2), 2, diagnosticResultByPath[2]);
	LCD_Clear();
	LCD_PrintTwoLines(line1, line2, 0);
}

void handle_incoming_uart_packets(void) {
	uint8_t packet_buffer[PACKET_SIZE];

	if (UART_get_packet(packet_buffer)) {
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
				transmitterEmergencyActive = 0;
				diagnosticResultByPath[1] = DIAG_RESULT_OK;
				diagnosticResultByPath[2] = DIAG_RESULT_OK;
				update_lcd_for_tables();
			break;

			case CMD_DIAG_RESULT:
				send_command(ACK_CMD, cmd, seq);
				if (table_id >= 1 && table_id <= 2) {
					diagnosticResultByPath[table_id] = seq;
					display_diagnostic_results();
				}
			break;

			case CMD_MENU_DATA:
				update_menu_data(table_id, seq);
				if (menuModeActive && table_id == menuItem) {
					display_menu_screen();
				}
			break;

			case CMD_ARRIVED: {
				uint8_t arrivedCommand = currentCommand;
				uint8_t path2ArrivedCommand = path2Command;

				if (table_id == selectedPath1Table) {
					if (arrivedCommand == CMD_BACKWARD) {
						arrivedTableByPath[1] = 0;
						snprintf(path1Status, sizeof(path1Status), "KITCHEN");
					} else {
						arrivedTableByPath[1] = table_id;
						snprintf(path1Status, sizeof(path1Status), "AT TABLE");
					}
					clear_motion_state();
				} else if (table_id == selectedPath2Table) {
					if (path2ArrivedCommand == CMD_BACKWARD) {
						arrivedTableByPath[2] = 0;
						snprintf(path2Status, sizeof(path2Status), "KITCHEN");
					} else {
						arrivedTableByPath[2] = table_id;
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
	//run_output_register_startup_test();
	update_lcd_for_tables();

	send_command_with_ack(CMD_STOP, 0x00, 0x00);

	while (1) {
		checkLocoMovementTimeout();

		uint8_t rawBits = read_74HC165();
		uint8_t invertedBits = ~rawBits;

		handle_table_inputs(invertedBits);
		handle_control_buttons();
		handle_incoming_uart_packets();
		poll_menu_data_if_due();
	}

	return 0;
}
