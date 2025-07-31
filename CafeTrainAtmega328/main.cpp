#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "system_init.h"
#include "lcd.h"
#include "railroad_control.h"
#include "loco_control.h"
#include "timer0.h"
#include "config.h"
#include "PWM.h"
#include "uart.h"
#include "shift_registers.h"

uint8_t lastCmd = 0xFF;  //  оманда, выполненна€ последней

uint8_t isLocoMoving = 0;
uint16_t tickCounter = 0;
uint8_t timeCounter = 0;

static uint8_t sensorStates = 0;

uint8_t triggeredBitMask = 0;

uint8_t triggeredBitsHistory = 0;

uint8_t SelectedTable = 0;    // »ндекс текущего регистра

uint8_t previousSensorStates = 0xFF;


bool isForwardDirection() {
	return !(PINB & (1 << REVERS_PIN));
}

void handleForwardSensors(uint8_t mask) {
	if (mask & TABLE_STOP_SENSOR) {
		triggeredBitsHistory = 0;
		LocoStop();
		} else if (mask & TABLE_SLOW_SENSOR) {
		SlowMode();
		} else {
		triggeredBitsHistory |= mask;
	}
	print_triggered_sensor(triggeredBitsHistory);
}

void handleReverseSensors(uint8_t mask) {
	if (mask & KITCHEN_STOP_SENSOR) {
		triggeredBitsHistory = 0;
		LocoStop(); // остановка у кухни
		} else if (mask & KITCHEN_SLOW_SENSOR) {
		SlowMode(); // замедление при приближении к кухне
		} else {
		triggeredBitsHistory |= mask;
	}
	print_triggered_sensor(triggeredBitsHistory);
}

void checkSensorsState() {
	sensorStates = ~read_74HC165(); // если активный высокий уровень с регистра

	if (sensorStates == previousSensorStates)
	return;

	uint8_t changedBits = sensorStates ^ previousSensorStates;
	triggeredBitMask = sensorStates & changedBits;
	previousSensorStates = sensorStates;

	if (triggeredBitMask) {
		if (isForwardDirection()) {
			handleForwardSensors(triggeredBitMask);
			} else {
			handleReverseSensors(triggeredBitMask);
		}
	}
}

void resetLocoTimer() {
	tickCounter = 0;
	timeCounter = 0;
}

void stopLocoDueToTimeout() {
	LocoStop();
	isLocoMoving = 0;
	resetLocoTimer();
	routeSetupInProgress = 0;
}

void checkLocoMovementTimeout() {
	if (!isLocoMoving)
	return;

	tickCounter++;

	if (tickCounter >= TICKS_PER_UNIT) {
		tickCounter = 0;
		timeCounter++;

		if (timeCounter >= MAX_TIME_UNITS) {
			stopLocoDueToTimeout();
		}
	}
}

void process_packet(UART_Packet packet) {
	if (!packet.valid)
	return;

	if (packet.cmd == lastCmd && packet.cmd != CMD_STOP && routeSetupInProgress)
	return;  // »гнорируем повтор той же команды, кроме STOP

	lastCmd = packet.cmd;

	switch (packet.cmd) {
		case CMD_STOP:
		LocoStop();
		send_ack(packet.cmd);
		resetLocoTimer();
		break;

		case CMD_FORWARD:
		LocoStop();  // сбрасываем на вс€кий случай
		
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

void check_and_send_overload_stop(void) {
	// 1. ќтправл€ем команду OVER_LOAD_STOP через UART с ожиданием ACK
	uint8_t ack = send_command_with_ack(OVER_LOAD_STOP, 0x00, 0x00);

	// 2. √отовим текст дл€ вывода на LCD
	char msg[17];
	if (ack) {
		snprintf(msg, sizeof(msg), "OVERLOAD: SENT");
		} else {
		snprintf(msg, sizeof(msg), "OVERLOAD: FAIL");
	}

	// 3. ќчищаем и выводим сообщение на экран
	LCD_Clear();
	LCD_PrintTwoLines("ERROR", msg, 0);
}

int main(void) {
	
	system_init();

	//check_and_send_overload_stop();


	while (1) {
		
		checkSensorsState();
		//checkLocoMovementTimeout();
		
		if (sensorStates != previousSensorStates) {
			previousSensorStates = sensorStates;
			print_triggered_sensor(sensorStates);
		}
		
		UART_Packet packet = UART_receive_full_packet();
		process_packet(packet);

		processPWMUp();  // обрабатываем плавный разгон
		
		if (routeSetupInProgress) {
			
			activate_route_non_blocking(SelectedTable);
			
		}
	}

	return 0;
}
