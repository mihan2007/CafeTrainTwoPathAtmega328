#include "../include/protection.h"
#include "../include/config.h"

#include "../include/adcRead.h"          // ADC_Read
#include "../include/lcd.h"              // LCD_Clear, LCD_PrintTwoLines, update_lcd
#include "../include/loco_control.h"     // LocoStop
#include "../include/uart.h"             // send_command_with_ack
#include "../include/railroad_control.h" // update_lcd
#include "../include/timer0.h"           // rail_switch_step_counter
#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>

typedef enum { OL_ARMED, OL_ACTIVE, OL_DIAG_START, OL_DIAG_PULSE, OL_LATCHED } OL_State;
static OL_State   ol_state        = OL_ARMED;
static uint16_t   holdStartTick   = 0;   // c ?????? ???? ???????? ? thrHigh
static uint16_t   lastResendTick  = 0;   // ????? ????????? ??? ????? STOP
static uint16_t   overloadStartTick = 0;  // ????? ?????????? ?????????
static uint16_t   testPulseStartTick = 0; // ????? ???????? ???????? A3
static uint8_t    diagnosticPath = 1;     // 1..2 = ??????????? ????
static uint8_t    diagnosticTable = 0;    // 0 = ???? ??? ??????, ????? ????? ?????
#define DIAG_SHORT_NONE 0xFF
static uint8_t    diagnosticShortTable[3] = {DIAG_SHORT_NONE, DIAG_SHORT_NONE, DIAG_SHORT_NONE};
static uint8_t    diagnosticTxResult[3] = {DIAG_RESULT_OK, DIAG_RESULT_OK, DIAG_RESULT_OK};
static uint16_t   filt            = 0;   // EMA-?????? ????
static uint8_t    filtInit        = 0;   // ????????????? EMA
static uint8_t    runtimeThresholdStored = (uint8_t)(OVERLOAD_THRESHOLD / 10);


static inline uint16_t thr_high(void) {
	return (uint16_t)runtimeThresholdStored * 10u;
}
static inline uint16_t thr_low(void) {
	uint16_t high = thr_high();
	return (uint16_t)((high > OVERLOAD_HYSTERESIS)
	? (high - OVERLOAD_HYSTERESIS) : 0);
}

void overload_update_threshold(uint8_t storedVal) {
	runtimeThresholdStored = storedVal;
	filtInit = 0;
}


static inline uint16_t ema8_update(uint16_t prev, uint16_t raw, uint8_t *inited) {
	if (!*inited) { *inited = 1; return raw; }
	// new = (prev*7 + raw)/8
	return (uint16_t)((((uint32_t)prev) * 7 + raw) / 8);
}


static void send_event_repeated(uint8_t cmd, uint8_t table_id, uint8_t data, uint8_t count) {
	for (uint8_t i = 0; i < count; i++) {
		send_command(cmd, table_id, data);
		_delay_ms(20);
	}
}
static inline uint8_t overload_confirmed(uint16_t high_thr) {
	if (filt >= high_thr) {
		if (holdStartTick == 0) holdStartTick = rail_switch_step_counter;
		uint16_t held = (uint16_t)(rail_switch_step_counter - holdStartTick);
		return (held >= OVERLOAD_HOLD_TICKS);
		} else {
		// ???? ?????? ? ?????????? ??????????
		holdStartTick = 0;
		return 0;
	}
}


static void overload_enter_active(void) {
	send_event_repeated(CMD_OVER_LOAD_STOP, 0x00, 0x00, 3);

	LocoStop();
	isLocoMoving = 0;
	routeSetupInProgress = 0;
	emergencyStopActive = 1;

	LCD_Clear();
	LCD_PrintTwoLines((char*)"EMERGENCY", (char*)"OVERLOAD", 0);

	lastResendTick = rail_switch_step_counter;
	overloadStartTick = rail_switch_step_counter;
	holdStartTick  = 0; // ?????? ?? ????? ? ACTIVE
	ol_state = OL_ACTIVE;
}


static void overload_active_tick(uint16_t high_thr, uint16_t low_thr) {
	(void)high_thr;
	(void)low_thr;

	if ((uint16_t)(rail_switch_step_counter - overloadStartTick) < OVERLOAD_DISPLAY_TICKS) {
		return;
	}

	diagnosticPath = 1;
	diagnosticTable = 0;
	diagnosticShortTable[1] = DIAG_SHORT_NONE;
	diagnosticShortTable[2] = DIAG_SHORT_NONE;
	ol_state = OL_DIAG_START;
}

static uint8_t first_table_for_path(uint8_t path) {
	return (path == 1) ? 1 : 5;
}

static uint8_t last_table_for_path(uint8_t path) {
	return (path == 1) ? 4 : 9;
}

static uint8_t get_diagnostic_result_code(uint8_t path) {
	if (diagnosticShortTable[path] == DIAG_SHORT_NONE) {
		return DIAG_RESULT_OK;
	}
	if (diagnosticShortTable[path] == 0) {
		return DIAG_RESULT_RAIL;
	}
	return diagnosticShortTable[path];
}

static void format_diagnostic_result_code(uint8_t path, uint8_t result, char *line, uint8_t lineSize) {
	if (result == DIAG_RESULT_OK) {
		snprintf(line, lineSize, "P%u OK", path);
	} else if (result == DIAG_RESULT_RAIL) {
		snprintf(line, lineSize, "P%u SHORTED RAIL", path);
	} else {
		snprintf(line, lineSize, "P%u SHORTED T%u", path, result);
	}
}

static void transmit_diagnostic_results(void) {
	diagnosticTxResult[1] = get_diagnostic_result_code(1);
	diagnosticTxResult[2] = get_diagnostic_result_code(2);
	send_command(CMD_DIAG_RESULT, 1, diagnosticTxResult[1]);
	_delay_ms(50);
	send_command(CMD_DIAG_RESULT, 2, diagnosticTxResult[2]);
}

static void show_diagnostic_results(void) {
	char line1[17];
	char line2[17];
	transmit_diagnostic_results();
	format_diagnostic_result_code(1, diagnosticTxResult[1], line1, sizeof(line1));
	format_diagnostic_result_code(2, diagnosticTxResult[2], line2, sizeof(line2));
	LCD_Clear();
	LCD_PrintTwoLines(line1, line2, 0);
	lastResendTick = rail_switch_step_counter;
	ol_state = OL_LATCHED;
}

static void diagnostic_power_on(uint8_t path) {
	if (path == 1) {
		PORTB |= (1 << RAIL_POWER_ENABLE);
	} else {
		PORTC |= (1 << PATH2_RAIL_POWER_ENABLE);
	}
}

static void diagnostic_power_off(uint8_t path) {
	if (path == 1) {
		PORTB &= ~(1 << RAIL_POWER_ENABLE);
	} else {
		PORTC &= ~(1 << PATH2_RAIL_POWER_ENABLE);
	}
}

static void start_diagnostic_step(void) {
	char line1[17];
	snprintf(line1, sizeof(line1), "CHECK PATH %u", diagnosticPath);

	if (diagnosticPath == 1) {
		diagnostic_power_on(diagnosticPath);
	}

	if (diagnosticTable == 0) {
		clearDiagnosticPathTablePower(diagnosticPath);
		LCD_Clear();
		LCD_PrintTwoLines(line1, (char*)"NO TABLE", 0);
	} else {
		setDiagnosticPathTablePower(diagnosticPath, diagnosticTable);
		LCD_Clear();
		char line2[17];
		snprintf(line2, sizeof(line2), "TABLE %u", diagnosticTable);
		LCD_PrintTwoLines(line1, line2, 0);
	}

	if (diagnosticPath == 2) {
		diagnostic_power_on(diagnosticPath);
	}
	testPulseStartTick = rail_switch_step_counter;
	ol_state = OL_DIAG_PULSE;
}

static void move_to_next_diagnostic_path_or_finish(void) {
	if (diagnosticPath == 1) {
		diagnosticPath = 2;
		diagnosticTable = 0;
		ol_state = OL_DIAG_START;
		return;
	}

	show_diagnostic_results();
}

static void advance_diagnostic_step(void) {
	if (diagnosticTable == 0) {
		if (diagnosticShortTable[diagnosticPath] == 0) {
			move_to_next_diagnostic_path_or_finish();
			return;
		}
		diagnosticTable = first_table_for_path(diagnosticPath);
		ol_state = OL_DIAG_START;
		return;
	}

	if (diagnosticTable < last_table_for_path(diagnosticPath)) {
		diagnosticTable++;
		ol_state = OL_DIAG_START;
		return;
	}

	move_to_next_diagnostic_path_or_finish();
}

static void finish_diagnostic_step(uint16_t high_thr) {
	if ((uint16_t)(rail_switch_step_counter - testPulseStartTick) < OVERLOAD_TEST_PULSE_TICKS) {
		return;
	}

	uint16_t testValue = ADC_Read(0);
	if (diagnosticPath == 1) {
		clearDiagnosticPathTablePower(diagnosticPath);
		diagnostic_power_off(diagnosticPath);
	} else {
		diagnostic_power_off(diagnosticPath);
		clearDiagnosticPathTablePower(diagnosticPath);
	}

	if (testValue >= high_thr && diagnosticShortTable[diagnosticPath] == DIAG_SHORT_NONE) {
		diagnosticShortTable[diagnosticPath] = diagnosticTable;
	}

	advance_diagnostic_step();
}
static void overload_latched_tick(uint16_t high_thr) {
	if (filt >= high_thr) {
		if ((uint16_t)(rail_switch_step_counter - lastResendTick) >= OVERLOAD_RESEND_TICKS) {
			send_event_repeated(CMD_OVER_LOAD_STOP, 0x00, 0x00, 2);
			lastResendTick = rail_switch_step_counter;
		}
	}
}
void clear_overload_emergency(uint8_t notifyTransmitter) {
	if (!emergencyStopActive && ol_state == OL_ARMED) return;

	emergencyStopActive = 0;
	holdStartTick = 0;
	lastResendTick = rail_switch_step_counter;
	overloadStartTick = 0;
	testPulseStartTick = 0;
	diagnosticPath = 1;
	diagnosticTable = 0;
	diagnosticShortTable[1] = DIAG_SHORT_NONE;
	diagnosticShortTable[2] = DIAG_SHORT_NONE;
	diagnosticTxResult[1] = DIAG_RESULT_OK;
	diagnosticTxResult[2] = DIAG_RESULT_OK;
	filtInit = 0;
	ol_state = OL_ARMED;

	if (notifyTransmitter) {
		send_event_repeated(CMD_CLEAR_EMERGENCY, 0x00, 0x00, 2);
	}

	LCD_Clear();
	update_lcd(lastCmd, SelectedTable);
}

void check_and_send_overload_stop(void) {
	const uint16_t hi = thr_high();
	const uint16_t lo = thr_low();

	// --- ?????? ???? (EMA 1/8) ---
	uint16_t raw = ADC_Read(0);
	filt = ema8_update(filt, raw, &filtInit);

	// --- ?????? ????????? ---
	switch (ol_state) {
		case OL_ARMED:
		if (overload_confirmed(hi)) {
			overload_enter_active();
		}
		break;

		case OL_ACTIVE:
		overload_active_tick(hi, lo);
		break;

		case OL_DIAG_START:
		start_diagnostic_step();
		break;

		case OL_DIAG_PULSE:
		finish_diagnostic_step(hi);
		break;

		case OL_LATCHED:
		overload_latched_tick(hi);
		break;
	}
}

void overload_led_sync(void) {
#if ALARM_ENABLED
	if (emergencyStopActive) {
		ALARM_PORT |= (1 << ALARM_PIN);
	} else {
		ALARM_PORT &= ~(1 << ALARM_PIN);
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
