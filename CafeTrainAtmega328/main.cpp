#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "lcd.h"
#include "railroad_switch.h"
#include "timer0.h"
#include "config.h"
#include "routes.h"
#include "PWM.h"
#include "uart.h"
#include "shift_registers.h"

volatile uint16_t tick_counter = 0;

static uint8_t sensorStates = 0;

uint8_t LOCO_CTRL = 0;
uint8_t SWITCH_A_CTRL = 0;
uint8_t SWITCH_B_CTRL = 0;

void initAllComponents() {
	I2C_Init();
	LCD_Init();
	LCD_Clear();

	UART_init();
	timer0_init();

	init_74HC165_ports();
	init_74HC595_ports();

	// Очистка регистров при старте
	uint8_t initData[MUM_OF_74HC595] = {0};
	shiftOutMultiple(initData, MUM_OF_74HC595);

	sei();

	LCD_PrintTwoLines("Waiting for Cmd", "Cmd: ?", 0);
}


void move_forward(uint8_t table_id, uint8_t cmd) {
	uint8_t enb_bit = (table_id - 1) * 2;
	uint16_t mask = 0;

	for (uint8_t i = 1; i < enb_bit; i += 2) {
		mask |= (1 << i);
	}
	mask |= (1 << enb_bit);

	SWITCH_A_CTRL = (uint8_t)(mask & 0xFF);
	SWITCH_B_CTRL = (uint8_t)((mask >> 8) & 0xFF);
	LOCO_CTRL = LOCO_FORWARD;

	send_ack(cmd);
}

void process_packet(UART_Packet packet) {
	if (!packet.valid) {
		return;
	}

	SWITCH_A_CTRL = 0;
	SWITCH_B_CTRL = 0;

	switch (packet.cmd) {
		case 0x30: // STOP
		LOCO_CTRL = LOCO_STOP;
		send_ack(packet.cmd);
		break;

		case 0x20: // MOVE_FORWARD
		move_forward(packet.table_id, packet.cmd);
		break;

		case 0x21: // MOVE_BACKWARD
		LOCO_CTRL = LOCO_BACKWARD;
		send_ack(packet.cmd);
		break;

		default:
		send_nack(packet.cmd);
		break;
	}

		update_shift_registers();
		update_lcd(packet.cmd);
}

int main(void) {
	initAllComponents();

	while (1) {
		sensorStates = read_74HC165();
		UART_Packet packet = UART_receive_full_packet();
		process_packet(packet);
				


	}

	return 0;
}

