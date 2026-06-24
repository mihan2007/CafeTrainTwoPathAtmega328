#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>
#include "include/system_init.h"
#include "include/lcd.h"
#include "include/railroad_control.h"
#include "include/loco_control.h"
#include "include/timer0.h"
#include "include/config.h"
#include "include/PWM.h"
#include "include/uart.h"
#include "include/shift_registers.h"
#include "include/adcRead.h"
#include "include/protection.h"
#include "include/eeprom_settings.h"

static uint8_t getTablePath(uint8_t tableId) {
	return (tableId >= 1 && tableId <= 4) ? 1 : 2;
}

static uint8_t isSamePath(uint8_t firstTable, uint8_t secondTable) {
	if (firstTable == 0 || secondTable == 0) return 0;
	return getTablePath(firstTable) == getTablePath(secondTable);
}

static uint8_t pendingRouteTable[3] = {0, 0, 0};
static uint8_t arrivedBlockedTable[3] = {0, 0, 0};

static void send_menu_data(uint8_t menuItem) {
	uint8_t value = 0;

	switch (menuItem) {
		case MENU_ITEM_SENSORS:
			sensorStates = ~read_74HC165();
			value = sensorStates;
			break;

		case MENU_ITEM_OVERLOAD_THRESHOLD:
			value = eeprom_overload_threshold_read();
			break;

		case MENU_ITEM_PWM_SLOW_PATH1:
			value = eeprom_slow_pwm_read(1);
			break;

		case MENU_ITEM_PWM_SLOW_PATH2:
			value = eeprom_slow_pwm_read(2);
			break;

		case MENU_ITEM_ACCEL_PATH1:
			value = eeprom_accel_delay_read(1);
			break;

		case MENU_ITEM_ACCEL_PATH2:
			value = eeprom_accel_delay_read(2);
			break;

		default:
			return;
	}

	send_command(CMD_MENU_DATA, menuItem, value);
}
static void startRouteSetup(uint8_t tableId) {
	uint8_t path = getTablePath(tableId);

	SelectedTable = tableId;
	reset_route_path_state(path);
	pathSelectedTable[path] = tableId;
	arrivedBlockedTable[path] = 0;
	pathMode[path] = PATH_MODE_ROUTE_SETUP;
	pathDirection[path] = PATH_DIRECTION_FORWARD;
	if (path == 2) {
		if (pathMode[1] != PATH_MODE_MOVING) {
			PORTB |= (1 << PWM_PATH1_SWITCH_PIN);
		}
		PORTB |= (1 << PWM_PATH2_SWITCH_PIN);
		PORTB |= (1 << RAIL_POWER_ENABLE);
		PORTC &= ~(1 << PATH2_RAIL_POWER_ENABLE);
	}
	routeSetupInProgress = 1;
}

static void queueRouteSetup(uint8_t tableId) {
	pendingRouteTable[getTablePath(tableId)] = tableId;
}

static void startPendingRouteSetupIfReady(void) {
	if (routeSetupInProgress) return;

	for (uint8_t path = 1; path <= 2; path++) {
		if (pendingRouteTable[path] > 0) {
			uint8_t tableId = pendingRouteTable[path];
			pendingRouteTable[path] = 0;
			LocoStopTable(tableId);
			startRouteSetup(tableId);
			return;
		}
	}
}

void updatePathModesAfterPwm(void) {
	for (uint8_t path = 1; path <= 2; path++) {
		if (pathMode[path] == PATH_MODE_ACCELERATION && !isPWMUpRunningForPath(path)) {
			if (path == 2) {
				PORTC |= (1 << PATH2_RAIL_POWER_ENABLE);
			}
			pathMode[path] = PATH_MODE_MOVING;
			update_lcd(lastCmd, pathSelectedTable[path]);
		}
	}
}

bool isForwardDirectionPath(uint8_t path) {
	if (path == 2) {
		return !(PINC & (1 << PATH2_REVERS_PIN));
	}

	return !(PINB & (1 << REVERS_PIN));
}

void SlowModePath(uint8_t path) {
	PORTB |= (1 << RAIL_POWER_ENABLE);

	if (path == 2) {
		if (pathMode[1] == PATH_MODE_STOP) {
			PORTB |= (1 << PWM_PATH1_SWITCH_PIN);
		}
		PORTB |= (1 << PWM_PATH2_SWITCH_PIN);
		PORTC &= ~(1 << PATH2_RAIL_POWER_ENABLE);
		enablePWMPath(2);
		OCR1B = eeprom_slow_pwm_read(2);
	} else {
		PORTB |= (1 << PWM_PATH1_SWITCH_PIN);
		enablePWMPath(1);
		OCR1A = eeprom_slow_pwm_read(1);
	}
}
void handleSensorEvent(uint8_t path, uint8_t mask, uint8_t stopSensor, uint8_t slowSensor) {
	if (mask & stopSensor) {
		uint8_t arrivedTable = pathSelectedTable[path];
		if (arrivedTable == 0 || getTablePath(arrivedTable) != path) return;

		triggeredBitsHistory = 0;

		arrivedBlockedTable[path] = arrivedTable;
		pendingRouteTable[path] = 0;
		if (routeSetupInProgress && getTablePath(SelectedTable) == path) {
			routeSetupInProgress = 0;
		}

		send_command(CMD_ARRIVED, arrivedTable, 0x00);
		_delay_ms(20);
		LocoStopTable(arrivedTable);
		pathSelectedTable[path] = arrivedTable;
		pathMode[path] = PATH_MODE_STOP;
		pathDirection[path] = PATH_DIRECTION_STOP;
		isLocoMoving = (pathMode[1] != PATH_MODE_STOP || pathMode[2] != PATH_MODE_STOP);
		update_lcd(CMD_ARRIVED, arrivedTable);

	} else if ((mask & slowSensor) && pathMode[path] != PATH_MODE_STOP) {
		SlowModePath(path);

	} else {
		triggeredBitsHistory |= mask;
	}
	print_triggered_sensor(triggeredBitsHistory);
}

void checkSensorsState(void) {
	sensorStates = ~read_74HC165();

	if (sensorStates == previousSensorStates) return;

	uint8_t changed = sensorStates ^ previousSensorStates;
	uint8_t mask    = sensorStates & changed;
	previousSensorStates = sensorStates;

	if (mask) {
		uint8_t path1Stop = isForwardDirectionPath(1) ? PATH1_TABLE_STOP_SENSOR : PATH1_KITCHEN_STOP_SENSOR;
		uint8_t path1Slow = isForwardDirectionPath(1) ? PATH1_TABLE_SLOW_SENSOR : PATH1_KITCHEN_SLOW_SENSOR;
		uint8_t path2Stop = isForwardDirectionPath(2) ? PATH2_TABLE_STOP_SENSOR : PATH2_KITCHEN_STOP_SENSOR;
		uint8_t path2Slow = isForwardDirectionPath(2) ? PATH2_TABLE_SLOW_SENSOR : PATH2_KITCHEN_SLOW_SENSOR;

		handleSensorEvent(1, mask, path1Stop, path1Slow);
		handleSensorEvent(2, mask, path2Stop, path2Slow);
	}
}
void process_packet(UART_Packet packet) {

	if (!packet.valid)	return;

	if (emergencyStopActive && packet.cmd != CMD_STOP && packet.cmd != CMD_STOP_PATH1 && packet.cmd != CMD_STOP_PATH2 && packet.cmd != CMD_CLEAR_EMERGENCY && packet.cmd != CMD_MENU_REQUEST && packet.cmd != CMD_MENU_ENTER && packet.cmd != CMD_MENU_EXIT && packet.cmd != CMD_MENU_SET) return;

	if (packet.cmd == lastCmd && packet.table_id == SelectedTable && packet.cmd != CMD_STOP && packet.cmd != CMD_STOP_PATH1 && packet.cmd != CMD_STOP_PATH2 && packet.cmd != CMD_MENU_REQUEST && packet.cmd != CMD_MENU_ENTER && packet.cmd != CMD_MENU_EXIT && packet.cmd != CMD_MENU_SET && routeSetupInProgress)
	return;  // ?????????? ?????? ??? ?? ???????, ????? STOP

	lastCmd = packet.cmd;

	switch (packet.cmd) {
		case CMD_STOP:
			send_ack(packet.cmd, packet.param);
			_delay_ms(20);
			arrivedBlockedTable[1] = 0;
			arrivedBlockedTable[2] = 0;
			pathDirection[1] = PATH_DIRECTION_STOP;
			pathDirection[2] = PATH_DIRECTION_STOP;
			LocoStop();
			clear_overload_emergency(0);
			resetLocoTimer();
			break;

		case CMD_STOP_PATH1:
			send_ack(packet.cmd, packet.param);
			_delay_ms(20);
			arrivedBlockedTable[1] = 0;
			pathDirection[1] = PATH_DIRECTION_STOP;
			LocoStopPath(1);
			if (routeSetupInProgress && getTablePath(SelectedTable) == 1) {
				routeSetupInProgress = 0;
			}

			clear_overload_emergency(0);
			resetLocoTimer();
			break;

		case CMD_STOP_PATH2:
			send_ack(packet.cmd, packet.param);
			_delay_ms(20);
			arrivedBlockedTable[2] = 0;
			pathDirection[2] = PATH_DIRECTION_STOP;
			LocoStopPath(2);
			if (routeSetupInProgress && getTablePath(SelectedTable) == 2) {
				routeSetupInProgress = 0;
			}

			clear_overload_emergency(0);
			resetLocoTimer();
			break;

		case CMD_CLEAR_EMERGENCY:
			send_ack(packet.cmd, packet.param);
			clear_overload_emergency(0);
			break;

		case CMD_MENU_ENTER:
		case CMD_MENU_EXIT:
			break;

		case CMD_MENU_REQUEST:
			send_menu_data(packet.table_id);
			break;

		case CMD_MENU_SET:
			switch (packet.table_id) {
				case MENU_ITEM_OVERLOAD_THRESHOLD:
					eeprom_overload_threshold_write(packet.param);
					overload_update_threshold(eeprom_overload_threshold_read());
					break;

				case MENU_ITEM_PWM_SLOW_PATH1:
					eeprom_slow_pwm_write(1, packet.param);
					break;

				case MENU_ITEM_PWM_SLOW_PATH2:
					eeprom_slow_pwm_write(2, packet.param);
					break;

				case MENU_ITEM_ACCEL_PATH1:
					eeprom_accel_delay_write(1, packet.param);
					break;

				case MENU_ITEM_ACCEL_PATH2:
					eeprom_accel_delay_write(2, packet.param);
					break;

				default:
					break;
			}
			send_ack(packet.cmd, packet.param);
			break;

		case CMD_FORWARD: {
			uint8_t path = getTablePath(packet.table_id);

			if (arrivedBlockedTable[path] == packet.table_id && pathMode[path] == PATH_MODE_STOP) {
				send_ack(packet.cmd, packet.param);
				break;
			}

			arrivedBlockedTable[path] = 0;
			send_ack(packet.cmd, packet.param);
			pathDirection[path] = PATH_DIRECTION_FORWARD;

			if (!routeSetupInProgress) {
				LocoStopTable(packet.table_id);
				startRouteSetup(packet.table_id);
			} else if (!isSamePath(packet.table_id, SelectedTable)) {
				queueRouteSetup(packet.table_id);
				pathSelectedTable[path] = packet.table_id;
			}

			isLocoMoving = 1;
		break;
		}
		case CMD_BACKWARD:
			send_ack(packet.cmd, packet.param);
			_delay_ms(20);
			arrivedBlockedTable[getTablePath(packet.table_id)] = 0;
			SelectedTable = packet.table_id;
			pathSelectedTable[getTablePath(packet.table_id)] = packet.table_id;
			pathDirection[getTablePath(packet.table_id)] = PATH_DIRECTION_BACKWARD;
			MoveLocoBackward(SelectedTable);
			isLocoMoving = 1;
		break;

		default:
		break;
	}

	update_lcd(packet.cmd, SelectedTable);
}

void run_output_shift_register_test(void) {
	uint8_t shiftData[NUM_OF_74HC595] = {0};

	LCD_Clear();
	LCD_PrintTwoLines("OUTPUT TEST", "Running", 0);

	for (uint8_t reg = 0; reg < NUM_OF_74HC595; reg++) {
		for (uint8_t bit = 0; bit < NUM_BITS_74HC595; bit++) {
			for (uint8_t i = 0; i < NUM_OF_74HC595; i++) {
				shiftData[i] = 0x00;
			}

			shiftData[reg] = (1 << bit);
			shiftOutMultiple(shiftData, NUM_OF_74HC595);
			_delay_ms(700);
		}
	}

	for (uint8_t i = 0; i < NUM_OF_74HC595; i++) {
		shiftData[i] = 0x00;
	}
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
	LCD_Clear();
	LCD_PrintTwoLines("Waiting for Cmd", "", 0);
}
int main(void) {

	system_init();
	overload_update_threshold(eeprom_overload_threshold_read());
	//run_output_shift_register_test();

	while (1) {

		checkSensorsState();
		check_and_send_overload_stop();
		overload_led_sync();
		checkLocoMovementTimeout();

		if (sensorStates != previousSensorStates) {
			previousSensorStates = sensorStates;
			print_triggered_sensor(sensorStates);
		}

		UART_Packet packet = UART_receive_full_packet();
		process_packet(packet);

		processPWMUp();
		updatePathModesAfterPwm();

		update_adc_display_if_due();

		if (routeSetupInProgress) {
			uint8_t wasRouteSetupInProgress = routeSetupInProgress;

			activate_route_non_blocking(SelectedTable);
			if (wasRouteSetupInProgress && !routeSetupInProgress) {
				update_lcd(lastCmd, SelectedTable);
			}
			startPendingRouteSetupIfReady();

		}
	}

	return 0;
}
