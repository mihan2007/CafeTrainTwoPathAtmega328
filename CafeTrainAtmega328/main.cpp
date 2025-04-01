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

static uint8_t sensorStates = 0;
uint8_t switchMask[NUM_SWITCH_REGS];


void process_packet(UART_Packet packet) {
    if (!packet.valid) {
        return;
    }
    
    switch (packet.cmd) {
        case 0x30: // STOP

            {
                uint8_t shiftData[NUM_OF_74HC595];
                shiftData[0] = LOCO_STOP;
                // Сброс переключателей
                for (uint8_t i = 1; i < NUM_OF_74HC595; i++) {
                    shiftData[i] = 0;
                }
                shiftOutMultiple(shiftData, NUM_OF_74HC595);
            }
            send_ack(packet.cmd);
            break;
            
        case 0x20: // MOVE_FORWARD
			send_ack(packet.cmd);
			calculate_mask(packet.table_id, switchMask);
			//step_counter = 0;
			move_forward(packet.table_id);

            break;
            
        case 0x21: // MOVE_BACKWARD
            
            {
                uint8_t shiftData[NUM_OF_74HC595];
                shiftData[0] = LOCO_BACKWARD;
                for (uint8_t i = 1; i < NUM_OF_74HC595; i++) {
                    shiftData[i] = 0;
                }
                shiftOutMultiple(shiftData, NUM_OF_74HC595);
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
		    
		
		if (rail_switch_step_counter <= 500)
		{
			rail_switch_step_counter = 0;
		}
		
    }

    return 0;
}
