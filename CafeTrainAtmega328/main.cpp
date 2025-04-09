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

static uint8_t sensorStates = 0;
uint8_t currentRegister = 0;    // Индекс текущего регистра
uint32_t enableBit = 0;         // Конечный бит, который зажигается последним

void process_packet(UART_Packet packet) {
	if (!packet.valid) {
		return;
	}
	
	switch (packet.cmd) {
		case 0x30: // STOP
		{
			uint8_t shiftData[NUM_OF_74HC595] = {0};
			shiftData[0] = LOCO_STOP;
			shiftOutMultiple(shiftData, NUM_OF_74HC595);
			reset_route_state();
			routeSetupInProgress = 0;
		}
		send_ack(packet.cmd);
		break;
		
		case 0x20: // MOVE_FORWARD
		send_ack(packet.cmd);
		if (!routeSetupInProgress) {
			currentRegister = packet.table_id - 1;
			routeSetupInProgress = 1;
		}
		break;
		
		case 0x21: // MOVE_BACKWARD
		{
			uint8_t shiftData[NUM_OF_74HC595] = {0};
			shiftData[NUM_OF_74HC595 - 1] = LOCO_BACKWARD;
			shiftOutMultiple(shiftData, NUM_OF_74HC595);
			routeSetupInProgress = 0;
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
		sensorStates = read_74HC165();
		UART_Packet packet = UART_receive_full_packet();
		process_packet(packet);
		
		if (routeSetupInProgress) {
			// Передаём номер стола в функцию
			activate_route_non_blocking(currentRegister);
		}
	}

	return 0;
}
