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

volatile uint16_t tick_counter = 0; // Счётчик для 1 мс прерываний

int main(void) {
	I2C_Init();
	LCD_Init();
	LCD_Clear();
	
	UART_init();
	timer0_init();
	
	init_74HC165_ports();
	init_74HC595_ports();
	
	sei();
	
	LCD_Clear();
	LCD_PrintTwoLines("Waiting for Cmd", "Cmd: ?", 0);

	uint8_t sensorStates = 0;
	uint8_t LOCO_CTRL = 0;
	uint8_t SWITCH_A_CTRL = 0;
	uint8_t SWITCH_B_CTRL = 0;

	while (1) {
		sensorStates = read_74HC165();
		
		// Получаем пакет единовременно
		UART_Packet packet = UART_receive_full_packet();
		
		if (packet.valid) {
			switch (packet.cmd) {
				
				case 0x30: // STOP
				
				SWITCH_B_CTRL = 0;
				SWITCH_A_CTRL = 0;
				
				LOCO_CTRL = LOCO_STOP;
				break;
				
				case 0x20:
				{
					SWITCH_B_CTRL = 0;
					SWITCH_A_CTRL = 0;

					uint8_t enb_bit = (packet.table_id - 1) * 2;
					uint16_t mask = 0;

					// Устанавливаем все нечетные биты до enb_bit
					for (uint8_t i = 1; i < enb_bit; i += 2) {
						mask |= (1 << i);
					}

					// Устанавливаем целевой enb_bit
					mask |= (1 << enb_bit);

					// Разделяем mask на два байта:
					SWITCH_A_CTRL = (uint8_t)(mask & 0xFF);       // Младшие 8 бит
					SWITCH_B_CTRL = (uint8_t)((mask >> 8) & 0xFF); // Старшие 8 бит
					
					LOCO_CTRL = LOCO_FORWARD ;
		
					break;					
				} // MOVE_FORWARD
							
				case 0x21: // MOVE_BACKWARD
				
				SWITCH_B_CTRL = 0;
				SWITCH_A_CTRL = 0;
				
				LOCO_CTRL = LOCO_BACKWARD;
				
				break;
				
				// Другие команды можно добавить здесь
			}
			
			// Инвертирование для синхронизации с LCD (если требуется)
			uint8_t invertedSensorStates = ~sensorStates;

			// Формирование данных для сдвиговых регистров (74HC595)
			uint8_t shiftData[MUM_OF_74HC595] = { LOCO_CTRL, SWITCH_A_CTRL, SWITCH_B_CTRL };
			shiftOutMultiple(shiftData, MUM_OF_74HC595);
			
			LCD_Clear();
			char buffer[16];
			snprintf(buffer, sizeof(buffer), "Cmd: %02X", packet.cmd);
			LCD_PrintTwoLines("Received", buffer, 0);
		}
	}
	
	return 0;
}
