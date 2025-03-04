#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>

#include "lcd.h"
#include "railroadSwitch.h"
#include "config.h"
#include "routes.h"
#include "PWM.h"
#include "uart.h"

char firstLineText[16];
char secondLineText[16];

int main(void) {
	I2C_Init();
	LCD_Init();
	LCD_Clear();
	UART_init();
	initRailRoadSwitch();

	while (1) {
		uint8_t received_packet[6]; // Буфер для приема пакета
		uint8_t cmd_code = 0xFF;    // Код команды

		// Ожидание первого байта (STX)
		if (UART_receive() == 0x02) {
			received_packet[0] = 0x02; // Запоминаем STX

			// Получаем остальные 5 байтов пакета
			for (uint8_t i = 1; i < 6; i++) {
				received_packet[i] = UART_receive();
			}

			// Проверяем корректность пакета (CRC, структура)
			if (crc8(&received_packet[1], 3) == received_packet[4]) {
				process_command(received_packet);
				cmd_code = received_packet[1]; // Правильный байт команды
			}
		}

		// Выводим данные на LCD
		snprintf(firstLineText, sizeof(firstLineText), "Control Panel");
		snprintf(secondLineText, sizeof(secondLineText), "Cmd: %02X", cmd_code);
		LCD_PrintTwoLines(firstLineText, secondLineText, 0);

		_delay_ms(10);
	}

	return 0;
}
