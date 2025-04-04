#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "system_init.h"
#include "lcd.h"
#include "railroad_control.h"
#include "timer0.h"
#include "config.h"
#include "routes.h"
#include "PWM.h"
#include "uart.h"
#include "shift_registers.h"

#define	ROUTE_TO_TABLE_1 0b0000000000000001
#define	ROUTE_TO_TABLE_2 0b0000000000000110
#define	ROUTE_TO_TABLE_3 0b0000000000011010
#define	ROUTE_TO_TABLE_4 0b0000000001101010
#define	ROUTE_TO_TABLE_5 0b0000000110101010
#define	ROUTE_TO_TABLE_6 0b0000011010101010
#define	ROUTE_TO_TABLE_7 0b0001101010101010
#define	ROUTE_TO_TABLE_8 0b0110101010101010
#define	ROUTE_TO_TABLE_9 0b1010101010101010

static uint8_t sensorStates = 0;
uint8_t currentRegister = 0;    // Индекс текущего регистра
uint32_t enableBit = 0;         // Конечный бит, который зажигается последним
uint8_t moveForwardActive = 0;  // Флаг, что команда MOVE_FORWARD активна и процесс ещё не завершён

void activate_route() {
	static const uint16_t routeMasks[] = {
		ROUTE_TO_TABLE_1, ROUTE_TO_TABLE_2, ROUTE_TO_TABLE_3, ROUTE_TO_TABLE_4,
		ROUTE_TO_TABLE_5, ROUTE_TO_TABLE_6, ROUTE_TO_TABLE_7, ROUTE_TO_TABLE_8,
		ROUTE_TO_TABLE_9
	};

	uint16_t mask = routeMasks[currentRegister];
	uint8_t shiftData[NUM_OF_74HC595] = {0};  // Сброс всех битов

	for (uint8_t i = 0; i < 16; i++) {  // Проходим по всем битам от 0 до 15
		// Сбрасываем все биты перед зажиганием нового
		shiftData[0] = 0;
		shiftData[1] = 0;

		if (mask & (1 << i)) {   // Проверяем, установлен ли бит (i)
			// Устанавливаем нужный бит в shiftData
			if (i < 8) {
				shiftData[0] = (1 << i);  // Устанавливаем бит в первом байте
				} else {
				shiftData[1] = (1 << (i - 8));  // Устанавливаем бит во втором байте
			}

			// Отправляем текущий бит (массив shiftData) в регистры сдвига
			shiftOutMultiple(shiftData, NUM_OF_74HC595);
			
			// Задержка, чтобы увидеть последовательное загорание
			_delay_ms(100);  // Задержка применяется только при установке бита в 1
		}
	}

	shiftData[0] = 0;
	shiftData[1] = 0;
	shiftData[2] = LOCO_FORWARD;
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
	moveForwardActive = 0;  // Завершаем процесс зажигания битов
}


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
			moveForwardActive = 0;
		}
		send_ack(packet.cmd);
		break;
		
		case 0x20: // MOVE_FORWARD
		if (!moveForwardActive) {
			send_ack(packet.cmd);			
			currentRegister = packet.table_id - 1;
			moveForwardActive = 1;
		}
		break;
		
		case 0x21: // MOVE_BACKWARD
		{
			uint8_t shiftData[NUM_OF_74HC595] = {0};
			shiftData[NUM_OF_74HC595-1] = LOCO_BACKWARD;
			shiftOutMultiple(shiftData, NUM_OF_74HC595);
			moveForwardActive = 0;
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
		
		if (moveForwardActive)
		{
			activate_route();
		}
	}

	return 0;
}
