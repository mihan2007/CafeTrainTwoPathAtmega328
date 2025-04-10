#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "system_init.h"
#include "lcd.h"
#include "railroad_control.h"
#include "timer0.h"
#include "config.h"
#include "PWM.h"
#include "uart.h"
#include "shift_registers.h"
#include "loco_control.h"

static uint8_t sensorStates = 0;
uint8_t currentRegister = 0;    // Индекс текущего регистра
uint8_t routeSetupInProgress = 0;  // Флаг, что команда MOVE_FORWARD активна и процесс ещё не завершён
uint8_t previousSensorStates = 0xFF;

void LocoStop() {
	uint8_t shiftData[NUM_OF_74HC595] = {0};
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
	reset_route_state();
	routeSetupInProgress = 0;
}

void MoveLocoBackward() {
	uint8_t shiftData[NUM_OF_74HC595] = {0};
	shiftData[NUM_OF_74HC595 - 1] = LOCO_BACKWARD;
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
	routeSetupInProgress = 0;
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
			currentRegister = packet.table_id - 1;
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
		
		sensorStates = ~read_74HC165();
		
	if (sensorStates != previousSensorStates) {
		previousSensorStates = sensorStates;
		print_triggered_sensor(sensorStates);
	}		
			
		UART_Packet packet = UART_receive_full_packet();
		process_packet(packet);
		
		if (routeSetupInProgress) {
			activate_route_non_blocking(currentRegister);
		}
	}

	return 0;
}
