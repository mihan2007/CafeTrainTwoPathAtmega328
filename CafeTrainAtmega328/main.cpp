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

        uint8_t cmd;
        uint8_t table_id;
        uint8_t param;

	
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

    uint16_t last_tick = step_counter;
    uint8_t sensorStates = 0;
    uint8_t LOCO_CTRL = 0;
    uint8_t SWITCH_A_CTRL = 0;
    uint8_t SWITCH_B_CTRL = 0;

	while (1) {
		

		sensorStates = read_74HC165();
					
		uint8_t cmd_code = UART_receive_packet();
		
		if (UART_read_command(&cmd, &table_id, &param)) {

			switch (cmd) {
				case 0x30: // STOP

				LOCO_CTRL = 0;

				break;

				case 0x20: // MOVE_FORWARD


				LOCO_CTRL = 0b00000001;
			
				break;

				case 0x21: // MOVE_BACKWARD

				LOCO_CTRL = 0b00000010;

				break;

			}
			


			// Инвертируем данные для синхронизации с LCD
			uint8_t invertedSensorStates = ~sensorStates;

			// Создаем 24-битное слово для трех 74HC595
			uint8_t shiftData[MUM_OF_74HC595] = { LOCO_CTRL, SWITCH_A_CTRL, SWITCH_B_CTRL };

			// Отправляем данные в сдвиговые регистры
			shiftOutMultiple(shiftData, MUM_OF_74HC595);			
			
            LCD_Clear();
            char buffer[16];
            snprintf(buffer, sizeof(buffer), "Cmd: %02X", cmd);
            LCD_PrintTwoLines("Received", buffer, 0);
			
		}
	}
	
	return 0;
}
