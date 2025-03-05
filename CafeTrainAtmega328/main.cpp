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

        uint8_t cmd;
        uint8_t table_id;
        uint8_t param;

int main(void) {
	I2C_Init();
	LCD_Init();
	LCD_Clear();
	UART_init();
	timer0_init();
	initRailRoadSwitch();
	sei();
	
	LCD_Clear();
	LCD_PrintTwoLines("Waiting for Cmd", "Cmd: ?", 0);

	while (1) {
			
		uint8_t cmd_code = UART_receive_packet();
		
		if (UART_read_command(&cmd, &table_id, &param)) {

			switch (cmd) {
				case 0x30: // STOP
				stopLocomotive();
				break;

				case 0x20: // MOVE_FORWARD
				routSetup(table_id - 1);
				break;

				case 0x21: // MOVE_BACKWARD
				moveLocomotive(0);
				break;

			}

            LCD_Clear();
            char buffer[16];
            snprintf(buffer, sizeof(buffer), "Cmd: %02X", cmd);
            LCD_PrintTwoLines("Received", buffer, 0);
		}
	}
	
	return 0;
}
