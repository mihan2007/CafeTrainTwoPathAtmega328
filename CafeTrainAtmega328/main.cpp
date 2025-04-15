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


void checkSensorsState() {
	
	sensorStates = ~read_74HC165(); // если активный высокий уровень с регистра

	if (sensorStates != previousSensorStates) {
		uint8_t changedBits = sensorStates ^ previousSensorStates;
		triggeredBitMask = sensorStates & changedBits;

		previousSensorStates = sensorStates;

		if (triggeredBitMask) {
			// Если сработал последний датчик (например, 8-й — SENSOR_8)
			if (triggeredBitMask & TABLE_END_WAY_SENSOR) {
				triggeredBitsHistory = 0;
				LocoStop();  // обязательная остановка
				} 
				else {
				triggeredBitsHistory |= triggeredBitMask;

			}

			print_triggered_sensor(triggeredBitsHistory);
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
		send_ack(packet.cmd);
		if (!routeSetupInProgress) {
			SelectedTable = packet.table_id;
			routeSetupInProgress = 1;
		}
		break;
		
		case CMD_BACKWARD: // MOVE_BACKWARD
		{
			MoveLocoBackward();
		}
		send_ack(packet.cmd);
		break;
		
		default:
		break;
	}
	
	update_lcd(packet.cmd);
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
		
		if (routeSetupInProgress) {
			activate_route_non_blocking(SelectedTable);
		}
	}

	return 0;
}
