#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h> 
#include "include/system_init.h"
#include "include/lcd.h"
#include "include/railroad_control.h"
#include "include/loco_control.h"
#include "include/timer0.h"
#include "include/config.h"
#include "include/PWM.h"
#include "include/uart.h"
#include "include/shift_registers.h"
#include "include/adcRead.h"
#include "include/protection.h"

bool isForwardDirection() {
	return !(PINB & (1 << REVERS_PIN));
}

void handleSensorEvent(uint8_t mask, uint8_t stopSensor, uint8_t slowSensor) {
	if (mask & stopSensor) {
		
		isLocoMoving = 0;
		
		triggeredBitsHistory = 0;
		
		LocoStop();
		
		send_command(CMD_ARRIVED, SelectedTable, 0x00);
		
		} else if ((mask & slowSensor) && isLocoMoving) {
			
		SlowMode();
		
		} else {
			
		triggeredBitsHistory |= mask;
	}
	print_triggered_sensor(triggeredBitsHistory);
}

void checkSensorsState(void) {
	sensorStates = ~read_74HC165();

    if (updateAdcMode(sensorStates, rail_switch_step_counter)) {
	    show_adc_value_on_lcd();
	    return;
    }

	if (sensorStates == previousSensorStates) return;

	uint8_t changed = sensorStates ^ previousSensorStates;
	uint8_t mask    = sensorStates & changed;
	previousSensorStates = sensorStates;

	if (mask) {
		uint8_t stop = isForwardDirection() ? TABLE_STOP_SENSOR : KITCHEN_STOP_SENSOR;
		uint8_t slow = isForwardDirection() ? TABLE_SLOW_SENSOR : KITCHEN_SLOW_SENSOR;
		handleSensorEvent(mask, stop, slow);
	}
}

void process_packet(UART_Packet packet) {
	
	if (emergencyStopActive) return;
	
	if (!packet.valid)	return;

	if (packet.cmd == lastCmd && packet.cmd != CMD_STOP && routeSetupInProgress)
	return;  // Игнорируем повтор той же команды, кроме STOP

	lastCmd = packet.cmd;

	switch (packet.cmd) {
		case CMD_STOP:
			LocoStop();
			send_ack(packet.cmd);
			resetLocoTimer();
			break;

		case CMD_FORWARD:
			LocoStop();  // сбрасываем на всякий случай
		
			send_ack(packet.cmd);
		
			if (!routeSetupInProgress) {
				SelectedTable = packet.table_id;
				routeSetupInProgress = 1;
			}
			isLocoMoving = 1;
		break;

		case CMD_BACKWARD:
			SelectedTable = packet.table_id;
			MoveLocoBackward(SelectedTable);
			send_ack(packet.cmd);
			isLocoMoving = 1;
		break;

		default:
		break;
	}

	update_lcd(packet.cmd, SelectedTable);
}

int main(void) {
	
	system_init();

	while (1) {
		
		checkSensorsState();
		
		check_and_send_overload_stop();
		
		checkLocoMovementTimeout();
		
		if (sensorStates != previousSensorStates) {
			previousSensorStates = sensorStates;
			print_triggered_sensor(sensorStates);
		}
		
		UART_Packet packet = UART_receive_full_packet();
		process_packet(packet);

		processPWMUp();  
		
		update_adc_display_if_due();
		
		if (routeSetupInProgress) {
			
			activate_route_non_blocking(SelectedTable);
			
		}
	}

	return 0;
}
