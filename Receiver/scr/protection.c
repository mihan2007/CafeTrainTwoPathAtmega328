#include "../include/protection.h"
#include "../include/config.h"

#include "../include/adcRead.h"          // ADC_Read
#include "../include/lcd.h"              // LCD_Clear, LCD_PrintTwoLines, update_lcd
#include "../include/loco_control.h"     // LocoStop
#include "../include/uart.h"             // send_command_with_ack
#include "../include/railroad_control.h" // update_lcd (если прототип тут)
#include "../include/timer0.h"           // extern volatile uint16_t rail_switch_step_counter


void check_and_send_overload_stop(void) {
	// --- Пороговая логика с гистерезисом ---
	const uint16_t thrHigh = OVERLOAD_THRESHOLD;
	const uint16_t thrLow  = (OVERLOAD_THRESHOLD > OVERLOAD_HYSTERESIS)
	? (OVERLOAD_THRESHOLD - OVERLOAD_HYSTERESIS) : 0;

	// --- Фильтр тока (EMA 1/8) ---
	static uint16_t filt = 0;
	static uint8_t  filtInit = 0;
	uint16_t raw = ADC_Read(0);
	if (!filtInit) { filt = raw; filtInit = 1; }
	else           { filt = (uint16_t)((((uint32_t)filt) * 7 + raw) / 8); }

	// --- Простая машина состояний перегруза ---
	typedef enum { OL_ARMED, OL_ACTIVE } OL_State;
	static OL_State state = OL_ARMED;

	static uint16_t holdStartTick = 0;     // с какого тика держится > thrHigh
	static uint16_t lastResendTick = 0;    // когда последний раз слали STOP

	switch (state) {
		case OL_ARMED: {
			// ждём устойчивое превышение thrHigh
			if (filt >= thrHigh) {
				if (holdStartTick == 0) holdStartTick = rail_switch_step_counter;
				uint16_t held = (uint16_t)(rail_switch_step_counter - holdStartTick);
				if (held >= OVERLOAD_HOLD_TICKS) {
					// подтверждённая перегрузка
					LocoStop();
					isLocoMoving = 0;
					routeSetupInProgress = 0;
					emergencyStopActive = 1;

					(void)send_command_with_ack(CMD_OVER_LOAD_STOP, 0x00, 0x00);
					LCD_Clear();
					LCD_PrintTwoLines((char*)"EMERGENCY", (char*)"OVERLOAD", 0);

					lastResendTick = rail_switch_step_counter;
					state = OL_ACTIVE;
					// сбросим счётчик удержания — он больше не нужен
					holdStartTick = 0;
				}
				} else {
				// ниже порога — сбрасываем «удержание»
				holdStartTick = 0;
			}
		} break;

		case OL_ACTIVE: {
			// пока активен перегруз: периодически шлём STOP, если держится > thrHigh
			if (filt >= thrHigh) {
				if ((uint16_t)(rail_switch_step_counter - lastResendTick) >= OVERLOAD_RESEND_TICKS) {
					(void)send_command_with_ack(CMD_OVER_LOAD_STOP, 0x00, 0x00);
					lastResendTick = rail_switch_step_counter;
				}
			}
			// вышли из перегруза устойчиво ниже thrLow -> сразу очищаем аварию и рисуем стартовый экран
			if (filt <= thrLow) {
				emergencyStopActive = 0;
				(void)send_command_with_ack(CMD_CLEAR_EMERGENCY, 0x00, 0x00);

				LCD_Clear();
				update_lcd(lastCmd, SelectedTable); // возврат к стартовому экрану

				state = OL_ARMED;
			}
		} break;
	}
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