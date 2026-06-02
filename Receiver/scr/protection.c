#include "../include/protection.h"
#include "../include/config.h"

#include "../include/adcRead.h"          // ADC_Read
#include "../include/lcd.h"              // LCD_Clear, LCD_PrintTwoLines, update_lcd
#include "../include/loco_control.h"     // LocoStop
#include "../include/uart.h"             // send_command_with_ack
#include "../include/railroad_control.h" // update_lcd
#include "../include/timer0.h"           // rail_switch_step_counter
#include <stdint.h>

typedef enum { OL_ARMED, OL_ACTIVE } OL_State;
static OL_State   ol_state        = OL_ARMED;
static uint16_t   holdStartTick   = 0;   // c какого тика держимся ? thrHigh
static uint16_t   lastResendTick  = 0;   // когда последний раз слали STOP
static uint16_t   filt            = 0;   // EMA-фильтр тока
static uint8_t    filtInit        = 0;   // инициализация EMA


static inline uint16_t thr_high(void) {
	return (uint16_t)OVERLOAD_THRESHOLD;
}
static inline uint16_t thr_low(void) {
	return (uint16_t)((OVERLOAD_THRESHOLD > OVERLOAD_HYSTERESIS)
	? (OVERLOAD_THRESHOLD - OVERLOAD_HYSTERESIS) : 0);
}


static inline uint16_t ema8_update(uint16_t prev, uint16_t raw, uint8_t *inited) {
	if (!*inited) { *inited = 1; return raw; }
	// new = (prev*7 + raw)/8
	return (uint16_t)((((uint32_t)prev) * 7 + raw) / 8);
}


static inline uint8_t overload_confirmed(uint16_t high_thr) {
	if (filt >= high_thr) {
		if (holdStartTick == 0) holdStartTick = rail_switch_step_counter;
		uint16_t held = (uint16_t)(rail_switch_step_counter - holdStartTick);
		return (held >= OVERLOAD_HOLD_TICKS);
		} else {
		// ниже порога — сбрасываем «удержание»
		holdStartTick = 0;
		return 0;
	}
}


static void overload_enter_active(void) {
	LocoStop();
	isLocoMoving = 0;
	routeSetupInProgress = 0;
	emergencyStopActive = 1;

	(void)send_command_with_ack(CMD_OVER_LOAD_STOP, 0x00, 0x00);
	LCD_Clear();
	LCD_PrintTwoLines((char*)"EMERGENCY", (char*)"OVERLOAD", 0);

	lastResendTick = rail_switch_step_counter;
	holdStartTick  = 0; // больше не нужен в ACTIVE
	ol_state = OL_ACTIVE;
}


static void overload_active_tick(uint16_t high_thr, uint16_t low_thr) {
	if (filt >= high_thr) {
		if ((uint16_t)(rail_switch_step_counter - lastResendTick) >= OVERLOAD_RESEND_TICKS) {
			(void)send_command_with_ack(CMD_OVER_LOAD_STOP, 0x00, 0x00);
			lastResendTick = rail_switch_step_counter;
		}
	}
	if (filt <= low_thr) {
		emergencyStopActive = 0;
		(void)send_command_with_ack(CMD_CLEAR_EMERGENCY, 0x00, 0x00);

		LCD_Clear();
		update_lcd(lastCmd, SelectedTable); // возврат к стартовому экрану

		ol_state = OL_ARMED;
	}
}


void check_and_send_overload_stop(void) {
	const uint16_t hi = thr_high();
	const uint16_t lo = thr_low();

	// --- Фильтр тока (EMA 1/8) ---
	uint16_t raw = ADC_Read(0);
	filt = ema8_update(filt, raw, &filtInit);

	// --- Машина состояний ---
	switch (ol_state) {
		case OL_ARMED:
		if (overload_confirmed(hi)) {
			overload_enter_active();
		}
		break;

		case OL_ACTIVE:
		overload_active_tick(hi, lo);
		break;
	}
}

void overload_led_sync(void) {
#if ALARM_ENABLED
	if (emergencyStopActive) {
		PORTB |= (1 << ALARM_PIN);
	} else {
		PORTB &= ~(1 << ALARM_PIN);
	}
#endif
}

void checkLocoMovementTimeout(void) {
	if (!isLocoMoving) return;

	tickCounter++;
	if (tickCounter >= TICKS_PER_UNIT) {
		tickCounter = 0;
		timeCounter++;
		if (timeCounter >= MAX_TIME_UNITS) {
			stopLocoDueToTimeout();
		}
	}
}

void stopLocoDueToTimeout(void) {
	LocoStop();
	isLocoMoving = 0;
	resetLocoTimer();
	routeSetupInProgress = 0;
}

void resetLocoTimer(void) {
	tickCounter = 0;
	timeCounter = 0;
}
