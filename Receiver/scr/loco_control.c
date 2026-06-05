#include "../include/config.h"
#include "../include/loco_control.h"
#include "../include/shift_registers.h"
#include "../include/railroad_control.h"
#include "../include/PWM.h"

static uint8_t powerShiftState[NUM_OF_74HC595] = {0};

const uint8_t powerRoutesSuplly[] = {
	DC_SUPLLY_TABLE_1,
	DC_SUPLLY_TABLE_2,
	DC_SUPLLY_TABLE_3,
	DC_SUPLLY_TABLE_4,
	DC_SUPLLY_TABLE_5,
	DC_SUPLLY_TABLE_6,
	DC_SUPLLY_TABLE_7,
	DC_SUPLLY_TABLE_8,
	DC_SUPLLY_TABLE_9
};

const uint8_t powerRoutesSupplyRegister[] = {
	POWER_SUPPLY_REGISTER_A,
	POWER_SUPPLY_REGISTER_A,
	POWER_SUPPLY_REGISTER_A,
	POWER_SUPPLY_REGISTER_A,
	POWER_SUPPLY_REGISTER_A,
	POWER_SUPPLY_REGISTER_A,
	POWER_SUPPLY_REGISTER_B,
	POWER_SUPPLY_REGISTER_B,
	POWER_SUPPLY_REGISTER_B
};

static uint8_t get_table_path(uint8_t tableIndex) {
	return (tableIndex < 4) ? 1 : 2;
}

static void clearPowerPath(uint8_t tableIndex) {
	if (get_table_path(tableIndex) == 1) {
		powerShiftState[POWER_SUPPLY_REGISTER_A] &= ~(DC_SUPLLY_TABLE_1 | DC_SUPLLY_TABLE_2 | DC_SUPLLY_TABLE_3 | DC_SUPLLY_TABLE_4);
	} else {
		powerShiftState[POWER_SUPPLY_REGISTER_A] &= ~(DC_SUPLLY_TABLE_5 | DC_SUPLLY_TABLE_6);
		powerShiftState[POWER_SUPPLY_REGISTER_B] &= ~(DC_SUPLLY_TABLE_7 | DC_SUPLLY_TABLE_8 | DC_SUPLLY_TABLE_9);
	}
}

static void clearPowerByPath(uint8_t path) {
	if (path == 1) {
		powerShiftState[POWER_SUPPLY_REGISTER_A] &= ~(DC_SUPLLY_TABLE_1 | DC_SUPLLY_TABLE_2 | DC_SUPLLY_TABLE_3 | DC_SUPLLY_TABLE_4);
	} else if (path == 2) {
		powerShiftState[POWER_SUPPLY_REGISTER_A] &= ~(DC_SUPLLY_TABLE_5 | DC_SUPLLY_TABLE_6);
		powerShiftState[POWER_SUPPLY_REGISTER_B] &= ~(DC_SUPLLY_TABLE_7 | DC_SUPLLY_TABLE_8 | DC_SUPLLY_TABLE_9);
	}
}

static uint8_t hasPath1PowerRoute(void) {
	return powerShiftState[POWER_SUPPLY_REGISTER_A] & (DC_SUPLLY_TABLE_1 | DC_SUPLLY_TABLE_2 | DC_SUPLLY_TABLE_3 | DC_SUPLLY_TABLE_4);
}

static uint8_t isPathActive(uint8_t path) {
	return pathMode[path] != PATH_MODE_STOP;
}

static uint8_t shouldKeepSharedPathPowerForPath2(void) {
	return isPathActive(2);
}

static void applyPowerRoute(uint8_t tableIndex) {
	if (tableIndex >= 9) return;

	clearPowerPath(tableIndex);
	powerShiftState[powerRoutesSupplyRegister[tableIndex]] |= powerRoutesSuplly[tableIndex];
}

void copy_power_shift_state(uint8_t *shiftData) {
	for (uint8_t i = 0; i < NUM_OF_74HC595; i++) {
		shiftData[i] |= powerShiftState[i];
	}
}

static void writeCombinedShiftState(void) {
	uint8_t shiftData[NUM_OF_74HC595] = {0};

	copy_route_shift_state(shiftData);
	copy_power_shift_state(shiftData);
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
}

void LocoStop(void) {
	PORTB &= ~(1 << REVERS_PIN);
	PORTC &= ~(1 << PATH2_REVERS_PIN);
	PORTB &= ~(1 << PWM_PATH1_SWITCH_PIN);
	PORTB &= ~(1 << PWM_PATH2_SWITCH_PIN);
	PORTC &= ~(1 << PATH2_RAIL_POWER_ENABLE);

	disablePWM();

	for (uint8_t i = 0; i < NUM_OF_74HC595; i++) {
		powerShiftState[i] = 0;
	}

	reset_route_state();
	writeCombinedShiftState();
	routeSetupInProgress = 0;
	pathMode[1] = PATH_MODE_STOP;
	pathMode[2] = PATH_MODE_STOP;
	pathSelectedTable[1] = 0;
	pathSelectedTable[2] = 0;
	PowerSupplyOff();
}

void LocoStopPath(uint8_t path) {
	if (path == 2) {
		PORTB &= ~(1 << PWM_PATH2_SWITCH_PIN);
		PORTC &= ~(1 << PATH2_RAIL_POWER_ENABLE);
		PORTC &= ~(1 << PATH2_REVERS_PIN);
	} else {
		if (!shouldKeepSharedPathPowerForPath2()) {
			PORTB &= ~(1 << PWM_PATH1_SWITCH_PIN);
		}
	}

	disablePWMPath(path);
	clearPowerByPath(path);
	reset_route_path_state(path);
	writeCombinedShiftState();
	pathMode[path] = PATH_MODE_STOP;
	pathSelectedTable[path] = 0;

	if (path == 1 && !hasPath1PowerRoute()) {
		PORTB &= ~(1 << REVERS_PIN);
		if (!shouldKeepSharedPathPowerForPath2()) {
			PowerSupplyOff();
		}
	} else if (path == 2 && !isPathActive(1)) {
		PORTB &= ~(1 << PWM_PATH1_SWITCH_PIN);
		PowerSupplyOff();
	}
}

void LocoStopTable(uint8_t tableIndex) {
	if (tableIndex == 0 || tableIndex > 9) {
		LocoStop();
		return;
	}

	LocoStopPath(get_table_path(tableIndex - 1));
}

void MoveLocoForward(uint8_t tableIndex) {
	uint8_t path = get_table_path(tableIndex);

	if (path == 1) {
		PORTB &= ~(1 << REVERS_PIN);
	} else {
		PORTC &= ~(1 << PATH2_REVERS_PIN);
	}

	PowerSupplyOnPath(path);
	applyPowerRoute(tableIndex);
	writeCombinedShiftState();
	startPWMUpForPath(path);
	pathMode[path] = PATH_MODE_ACCELERATION;
}

void MoveLocoBackward(uint8_t tableIndex) {
	uint8_t shiftRegisterMask = tableIndex - 1;
	uint8_t path = get_table_path(shiftRegisterMask);

	LocoStopTable(tableIndex);
	if (path == 1) {
		ReversOn();
	} else {
		PORTC |= (1 << PATH2_REVERS_PIN);
	}
	PowerSupplyOnPath(path);
	startPWMUpForPath(path);

	applyPowerRoute(shiftRegisterMask);
	writeCombinedShiftState();
	pathMode[path] = PATH_MODE_ACCELERATION;
	routeSetupInProgress = 0;
}

void SlowMode(void){
	uint8_t path = (SelectedTable > 0) ? get_table_path(SelectedTable - 1) : 1;
	enablePWMPath(path);
	if (path == 2) {
		OCR1B = (PWM_MAX/2);
	} else {
		OCR1A = (PWM_MAX/2);
	}
}

void PowerSupplyOnPath(uint8_t path) {
	if (path == 2) {
		PORTB |= (1 << PWM_PATH2_SWITCH_PIN);
		return;
	}

	PORTB |= (1 << RAIL_POWER_ENABLE);
	PORTB |= (1 << PWM_PATH1_SWITCH_PIN);
}

void PowerSupplyOn() {
	PowerSupplyOnPath(1);
}

void PowerSupplyOff() {
	PORTB &= ~ (1 << RAIL_POWER_ENABLE);
}

void ReversOn(){
	PORTB |= (1 << REVERS_PIN);
}
