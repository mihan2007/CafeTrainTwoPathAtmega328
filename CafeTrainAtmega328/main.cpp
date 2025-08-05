#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h> 
#include "system_init.h"
#include "lcd.h"
#include "railroad_control.h"
#include "loco_control.h"
#include "timer0.h"
#include "config.h"
#include "PWM.h"
#include "uart.h"
#include "shift_registers.h"
#include "adcRead.h"

extern volatile uint16_t rail_switch_step_counter;
static uint16_t lastAdcTick = 0;

uint8_t lastCmd = 0xFF;  // Команда, выполненная последней

uint8_t isLocoMoving = 0;
uint16_t tickCounter = 0;
uint8_t timeCounter = 0;

static uint8_t sensorStates = 0;

uint8_t triggeredBitMask = 0;

uint8_t triggeredBitsHistory = 0;

uint8_t SelectedTable = 0;    // Индекс текущего регистра

uint8_t previousSensorStates = 0xFF;

void show_adc_value_on_lcd(void) {
	static uint16_t lastAdcValue = 0xFFFF;  // невозможно при 10-битном АЦП
	static uint8_t counter = 0;

	// обновляем только раз в N циклов
	counter++;
	if (counter < 10) return;
	counter = 0;

	uint16_t adcValue = ADC_Read(0);

	// обновляем экран только если значение изменилось значительно
	if (abs((int16_t)adcValue - (int16_t)lastAdcValue) >= 10) {
		lastAdcValue = adcValue;

		char adcBuffer[17];
		snprintf(adcBuffer, sizeof(adcBuffer), "ADC: %4u     ", adcValue);

		LCD_SetCursor(0, 1);  // Вторая строка дисплея
		LCD_Print(adcBuffer);
	}
}

void update_adc_display_if_due(void) {
	extern volatile uint16_t rail_switch_step_counter;
	static uint16_t lastAdcTick = 0;

	if (rail_switch_step_counter - lastAdcTick >= 10) {
		show_adc_value_on_lcd();
		lastAdcTick = rail_switch_step_counter;
	}
}

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

void check_and_send_overload_stop(void) {
	// 1. Отправляем команду OVER_LOAD_STOP через UART с ожиданием ACK
	uint8_t ack = send_command_with_ack(OVER_LOAD_STOP, 0x00, 0x00);

	// 2. Готовим текст для вывода на LCD
	char msg[17];
	if (ack) {
		snprintf(msg, sizeof(msg), "OVERLOAD: SENT");
		} else {
		snprintf(msg, sizeof(msg), "OVERLOAD: FAIL");
	}

	// 3. Очищаем и выводим сообщение на экран
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
		
		update_adc_display_if_due();
		
		if (routeSetupInProgress) {
			
			activate_route_non_blocking(SelectedTable);
			
		}
	}

	return 0;
}
