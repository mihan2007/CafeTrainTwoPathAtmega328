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

static uint8_t getTablePath(uint8_t tableId) {
	return (tableId >= 1 && tableId <= 4) ? 1 : 2;
}

static uint8_t isSamePath(uint8_t firstTable, uint8_t secondTable) {
	if (firstTable == 0 || secondTable == 0) return 0;
	return getTablePath(firstTable) == getTablePath(secondTable);
}

static uint8_t pendingRouteTable[3] = {0, 0, 0};

static void startRouteSetup(uint8_t tableId) {
	SelectedTable = tableId;
	pathSelectedTable[getTablePath(tableId)] = tableId;
	pathMode[getTablePath(tableId)] = PATH_MODE_ROUTE_SETUP;
	pathDirection[getTablePath(tableId)] = PATH_DIRECTION_FORWARD;
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

bool isForwardDirection() {
	uint8_t path = (SelectedTable >= 1 && SelectedTable <= 4) ? 1 : 2;

	if (path == 2) {
		return !(PINC & (1 << PATH2_REVERS_PIN));
	}

	return !(PINB & (1 << REVERS_PIN));
}

void handleSensorEvent(uint8_t mask, uint8_t stopSensor, uint8_t slowSensor) {
	if (mask & stopSensor) {
		uint8_t arrivedTable = SelectedTable;
		uint8_t arrivedPath = getTablePath(arrivedTable);
		
		isLocoMoving = 0;
		
		triggeredBitsHistory = 0;
		
		LocoStopTable(arrivedTable);
		pathSelectedTable[arrivedPath] = arrivedTable;
		pathMode[arrivedPath] = PATH_MODE_STOP;
		pathDirection[arrivedPath] = PATH_DIRECTION_STOP;
		
		send_command(CMD_ARRIVED, arrivedTable, 0x00);
		update_lcd(CMD_ARRIVED, arrivedTable);
		
		} else if ((mask & slowSensor) && isLocoMoving) {
			
		SlowMode();
		
		} else {
			
		triggeredBitsHistory |= mask;
	}
	print_triggered_sensor(triggeredBitsHistory);
}

void checkSensorsState(void) {
	sensorStates = ~read_74HC165();

    if (updateAdcMode(sensorStates, rail_switch_step_counter)) {
	    show_adc_value_on_lcd();
	    return;
    }

	if (sensorStates == previousSensorStates) return;

	uint8_t changed = sensorStates ^ previousSensorStates;
	uint8_t mask    = sensorStates & changed;
	previousSensorStates = sensorStates;

	if (mask) {
		uint8_t stop = isForwardDirection() ? TABLE_STOP_SENSOR : KITCHEN_STOP_SENSOR;
		uint8_t slow = isForwardDirection() ? TABLE_SLOW_SENSOR : KITCHEN_SLOW_SENSOR;
		handleSensorEvent(mask, stop, slow);
	}
}

void process_packet(UART_Packet packet) {
	
	if (emergencyStopActive) return;
	
	if (!packet.valid)	return;

	if (packet.cmd == lastCmd && packet.table_id == SelectedTable && packet.cmd != CMD_STOP && packet.cmd != CMD_STOP_PATH1 && packet.cmd != CMD_STOP_PATH2 && routeSetupInProgress)
	return;  // Игнорируем повтор той же команды, кроме STOP

	lastCmd = packet.cmd;

	switch (packet.cmd) {
		case CMD_STOP:
			pathDirection[1] = PATH_DIRECTION_STOP;
			pathDirection[2] = PATH_DIRECTION_STOP;
			LocoStop();
			send_ack(packet.cmd, packet.param);
			resetLocoTimer();
			break;

		case CMD_STOP_PATH1:
			pathDirection[1] = PATH_DIRECTION_STOP;
			LocoStopPath(1);
			if (routeSetupInProgress && getTablePath(SelectedTable) == 1) {
				routeSetupInProgress = 0;
			}
			send_ack(packet.cmd, packet.param);
			resetLocoTimer();
			break;

		case CMD_STOP_PATH2:
			pathDirection[2] = PATH_DIRECTION_STOP;
			LocoStopPath(2);
			if (routeSetupInProgress && getTablePath(SelectedTable) == 2) {
				routeSetupInProgress = 0;
			}
			send_ack(packet.cmd, packet.param);
			resetLocoTimer();
			break;

		case CMD_FORWARD: {
			uint8_t path = getTablePath(packet.table_id);

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
			SelectedTable = packet.table_id;
			pathSelectedTable[getTablePath(packet.table_id)] = packet.table_id;
			pathDirection[getTablePath(packet.table_id)] = PATH_DIRECTION_BACKWARD;
			MoveLocoBackward(SelectedTable);
			send_ack(packet.cmd, packet.param);
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
	run_output_shift_register_test();

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
