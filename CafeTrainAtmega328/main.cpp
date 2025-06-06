#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "system_init.h"
#include "lcd.h"
#include "railroad_control.h"
#include "loco_control.h"
#include "timer0.h"
#include "config.h"
#include "PWM.h"
#include "uart.h"
#include "shift_registers.h"

static uint8_t sensorStates = 0;

uint8_t triggeredBitMask = 0;

uint8_t triggeredBitsHistory = 0;

uint8_t SelectedTable = 0;    // Индекс текущего регистра

uint8_t previousSensorStates = 0xFF;

bool isForwardDirection() {
	return !(PINB & (1 << REVERS_PIN));
}

void handleForwardSensors(uint8_t mask) {
	if (mask & TABLE_STOP_SENSOR) {
			triggeredBitsHistory = 0;
			LocoStop();
		} else if (mask & TABLE_SLOW_SENSOR) {
			SlowMode();
		} else {
		triggeredBitsHistory |= mask;
	}
	print_triggered_sensor(triggeredBitsHistory);
}

void handleReverseSensors(uint8_t mask) {
	if (mask & KITCHEN_STOP_SENSOR) {
			triggeredBitsHistory = 0;
			LocoStop(); // остановка у кухни
		} else if (mask & KITCHEN_SLOW_SENSOR) {
			SlowMode(); // замедление при приближении к кухне
		} else {
		triggeredBitsHistory |= mask;
	}
	print_triggered_sensor(triggeredBitsHistory);
}



void checkSensorsState() {
	sensorStates = ~read_74HC165(); // если активный высокий уровень с регистра

	if (sensorStates == previousSensorStates)
	return;

	uint8_t changedBits = sensorStates ^ previousSensorStates;
	triggeredBitMask = sensorStates & changedBits;
	previousSensorStates = sensorStates;

	if (triggeredBitMask) {
		if (isForwardDirection()) {
			handleForwardSensors(triggeredBitMask);
			} else {
			handleReverseSensors(triggeredBitMask);
		}
	}
}



void process_packet(UART_Packet packet) {
	if (!packet.valid) {
		return;
	}
	
	switch (packet.cmd) {
		case CMD_STOP: // STOP
		{
			LocoStop();
		}
		send_ack(packet.cmd);
		break;
		
		case CMD_FORWARD: // MOVE_FORWARD
			
			LocoStop();

			send_ack(packet.cmd);
		
		if (!routeSetupInProgress) {
			
			SelectedTable = packet.table_id;
			
			routeSetupInProgress = 1;
		}
		break;
		
		case CMD_BACKWARD: // MOVE_BACKWARD
		{
			SelectedTable = packet.table_id;
			MoveLocoBackward(SelectedTable);
		}
		send_ack(packet.cmd);
		break;
		
		default:
		break;
	}
	
	update_lcd(packet.cmd, SelectedTable);
}


int main(void) {
	
	system_init();

	while (1) {
		
	checkSensorsState();
	
	if (sensorStates != previousSensorStates) {
		previousSensorStates = sensorStates;
		print_triggered_sensor(sensorStates);
	}		
			
		UART_Packet packet = UART_receive_full_packet();
		process_packet(packet);

		processPWMUp();  // обрабатываем плавный разгон	
			
		if (routeSetupInProgress) {
			
			activate_route_non_blocking(SelectedTable);
			
		}
	}

	return 0;
}
