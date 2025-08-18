#include "../include/adcRead.h"
#include <avr/io.h>

#include <stdlib.h>      // abs
#include <stdio.h>       // snprintf
#include "../include/lcd.h"              // LCD_*
#include "../include/config.h"           // HOLD_TICKS_BUTTON, ADC_MODE_BUTTON, REVERS_PIN и др.
#include "../include/timer0.h"           // extern volatile uint16_t rail_switch_step_counter
#include "../include/loco_control.h"     // LocoStop


static void lcd_clear_line(uint8_t row) {
	LCD_SetCursor(0, row);
	LCD_Print("                "); // 16 пробелов для 16x2 LCD
	LCD_SetCursor(0, row);
}

void ADC_Init(void) {
	// Установка Vref на AVcc (внешний источник питания 5В)
	ADMUX = (1 << REFS0);

	// Включение АЦП, установка предделителя на 128
	// Частота АЦП = F_CPU / 128 (должна быть в пределах 50-200 кГц)
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// Функция для выполнения преобразования АЦП на заданном канале
uint16_t ADC_Read(uint8_t channel) {
	// Убедиться, что канал находится в пределах [0-7]
	channel &= 0x07;

	// Выбрать канал, сбросить старые настройки канала
	ADMUX = (ADMUX & 0xF8) | channel;

	// Начать преобразование
	ADCSRA |= (1 << ADSC);

	// Ожидание завершения преобразования
	while (ADCSRA & (1 << ADSC));

	// Возврат результата
	return ADC;
}

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
				lcd_clear_line(1);
				} else {
				// Выход из режима
				adcModeActive = 0;
				lcd_clear_line(1);
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

