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

extern volatile uint16_t rail_switch_step_counter;
static uint16_t lastAdcTick = 0;

volatile uint8_t emergencyStopActive = 0;

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

uint8_t updateAdcMode(uint8_t sensorStates, uint16_t tick) {
	static uint8_t  adcModeActive     = 0;
	static uint16_t holdStartTick     = 0;
	static uint8_t  toggledThisPress  = 0;

	uint8_t pressed = (sensorStates & ADC_MODE_BUTTON) != 0;

	if (pressed) {
		if (holdStartTick == 0) {
			holdStartTick = tick;
		}
		uint16_t held = (uint16_t)(tick - holdStartTick);

		if (!toggledThisPress && held >= HOLD_TICKS_BUTTON) {
			toggledThisPress = 1;

			if (!adcModeActive) {
				// Вход в режим
				adcModeActive = 1;
				LocoStop();
				routeSetupInProgress = 0;
				lastCmd = 0xFF;
				LCD_Clear();
				} else {
				// Выход из режима
				adcModeActive = 0;
				LCD_Clear();
			}

			holdStartTick = tick; // сброс отсчёта для защиты от залипания
		}
		} else {
		holdStartTick = 0;
		toggledThisPress = 0;
	}

	return adcModeActive;
}

void update_adc_display_if_due(void) {
	extern volatile uint16_t rail_switch_step_counter;
	static uint16_t lastAdcTick = 0;

	if (rail_switch_step_counter - lastAdcTick >= 10) {
		//show_adc_value_on_lcd();
		lastAdcTick = rail_switch_step_counter;
	}
}

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

void check_and_send_overload_stop(void) {
	static uint8_t  armed = 1;
	static uint16_t holdStartTick = 0;
	static uint16_t filt = 0;
	static uint8_t  filtInit = 0;

	// читаем АЦП A0 и сглаживаем (EMA 1/8)
	uint16_t raw = ADC_Read(0);
	if (!filtInit) { filt = raw; filtInit = 1; }
	else           { filt = (uint16_t)((((uint32_t)filt) * 7 + raw) / 8); }

	uint16_t thrHigh = OVERLOAD_THRESHOLD;
	uint16_t thrLow  = (OVERLOAD_THRESHOLD > OVERLOAD_HYSTERESIS)
	? (OVERLOAD_THRESHOLD - OVERLOAD_HYSTERESIS) : 0;

	if (armed) {
		if (filt >= thrHigh) {
			if (holdStartTick == 0) holdStartTick = rail_switch_step_counter;

			if ((uint16_t)(rail_switch_step_counter - holdStartTick) >= OVERLOAD_HOLD_TICKS) {
				// подтверждённая перегрузка: стоп, защёлка, первый сигнал
				LocoStop();
				isLocoMoving = 0;
				routeSetupInProgress = 0;
				emergencyStopActive = 1;

				(void)send_command_with_ack(OVER_LOAD_STOP, 0x00, 0x00);
				LCD_Clear();
				LCD_PrintTwoLines((char*)"OVERLOAD", (char*)"STOP SENT", 0);

				// --- БЛОКИРУЮЩИЙ ЦИКЛ: пока перегрузка держится, слать периодически ---
				while (1) {
					// ждём RESEND_DELAY_TICKS тиков (ISR продолжают работать)
					uint16_t start = rail_switch_step_counter;
					while ((uint16_t)(rail_switch_step_counter - start) < OVERLOAD_RESEND_TICKS) {
						// no-op
					}

					// перечитываем АЦП и обновляем фильтр
					raw  = ADC_Read(0);
					filt = (uint16_t)((((uint32_t)filt) * 7 + raw) / 8);

					if (filt >= thrHigh) {
						(void)send_command_with_ack(OVER_LOAD_STOP, 0x00, 0x00);
						// (опционально) можно редко обновлять LCD, чтобы не «мигать»
						// LCD_PrintTwoLines((char*)"EMERGENCY", (char*)"OVERLOAD", 0);
						} else {
						// вышли из перегруза (по верхнему порогу) — прекращаем спам
						break;
					}
				}

				// выходим из события перегруза; повторно «вооружимся» ниже thrLow
				armed = 0;
				holdStartTick = 0;
			}
			} else {
			holdStartTick = 0;
		}
		} else {
		// Реарм только после устойчивого опускания ниже нижнего порога
		if (filt <= thrLow) {
			armed = 1;
			LCD_Clear();
			LCD_PrintTwoLines((char*)"OVERLOAD", (char*)"CLEARED", 0);
			// ВНИМАНИЕ: emergencyStopActive тут НЕ сбрасываем — снимайте отдельной командой/кнопкой.
		}
	}
}

int main(void) {
	
	system_init();

	//check_and_send_overload_stop();


	while (1) {
		
		checkSensorsState();
		
		check_and_send_overload_stop();
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
