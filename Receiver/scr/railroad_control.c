#include "../include/shift_registers.h"
#include "../include/railroad_control.h"
#include "../include/loco_control.h"
#include "../include/config.h"
#include "../include/timer0.h"
#include "../include/PWM.h"
#include <avr/io.h>
#include <avr/delay.h>

static uint8_t initialized = 0;
static uint32_t routeStateMask = 0;
static uint32_t activeRouteMask = 0;

const uint32_t routeSwitches[] = {
	ROUTE_TO_TABLE_1,
	ROUTE_TO_TABLE_2,
	ROUTE_TO_TABLE_3,
	ROUTE_TO_TABLE_4,
	ROUTE_TO_TABLE_5,
	ROUTE_TO_TABLE_6,
	ROUTE_TO_TABLE_7,
	ROUTE_TO_TABLE_8,
	ROUTE_TO_TABLE_9
};

const uint8_t switchRegisterIndexes[] = {
	SWITCH_REGISTER_0,
	SWITCH_REGISTER_1,
	SWITCH_REGISTER_2
};

static uint8_t get_route_path(uint8_t tableIndex) {
	return (tableIndex < 4) ? 1 : 2;
}

static uint32_t get_route_path_mask(uint8_t tableIndex) {
	return (get_route_path(tableIndex) == 1) ? PATH1_ROUTE_MASK : PATH2_ROUTE_MASK;
}

static void apply_path1_route_setup_mode(void) {
	PORTB |= (1 << PWM_PATH1_SWITCH_PIN);
	PORTB |= (1 << RAIL_POWER_ENABLE);
}

static void apply_path2_route_setup_mode(void) {
	if (pathMode[1] != PATH_MODE_MOVING && !(PORTB & (1 << PWM_PATH1_SWITCH_PIN))) {
		PORTB |= (1 << PWM_PATH1_SWITCH_PIN);
	}
	PORTB |= (1 << PWM_PATH2_SWITCH_PIN);
	PORTB |= (1 << RAIL_POWER_ENABLE);
	PORTC &= ~(1 << PATH2_RAIL_POWER_ENABLE);
}

void copy_route_shift_state(uint8_t *shiftData) {
	for (uint8_t currentBit = 0; currentBit < (NUM_BITS_74HC595 * NUM_SWITCH_REGS); currentBit++) {
		if (routeStateMask & (1UL << currentBit)) {
			uint8_t routeRegister = currentBit / NUM_BITS_74HC595;
			uint8_t routeBit = currentBit % NUM_BITS_74HC595;
			shiftData[switchRegisterIndexes[routeRegister]] |= (1 << routeBit);
		}
	}
}

void reset_route_state(void) {
	initialized = 0;
	routeStateMask = 0;
	activeRouteMask = 0;
}

void reset_route_path_state(uint8_t path) {
	uint32_t pathMask = (path == 1) ? PATH1_ROUTE_MASK : PATH2_ROUTE_MASK;
	routeStateMask &= ~pathMask;
	activeRouteMask &= ~pathMask;

	if (activeRouteMask == 0) {
		initialized = 0;
	}
}

static void write_route_setup_state(void) {
	uint8_t shiftData[NUM_OF_74HC595] = {0};
	copy_route_shift_state(shiftData);
	copy_power_shift_state(shiftData);
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
}

void activate_route_non_blocking(uint8_t tableIndex) {
	tableIndex = tableIndex - 1;

	static uint32_t lastTick = 0;
	static uint8_t currentBit = 0;
	static uint8_t pulseActive = 0;

	if (!initialized) {
		currentBit = 0;
		pulseActive = 0;
		lastTick = rail_switch_step_counter;
		activeRouteMask = routeSwitches[tableIndex];
		routeStateMask &= ~get_route_path_mask(tableIndex);
		if (get_route_path(tableIndex) == 1) {
			apply_path1_route_setup_mode();
		} else {
			apply_path2_route_setup_mode();
		}
		initialized = 1;
	}

	if ((rail_switch_step_counter - lastTick) >= SWITCH_PAUSE_TIME) {
		lastTick = rail_switch_step_counter;

		if (pulseActive) {
			routeStateMask &= ~(1UL << currentBit);
			write_route_setup_state();
			pulseActive = 0;
			currentBit++;
			return;
		}

		while (currentBit < (NUM_BITS_74HC595 * NUM_SWITCH_REGS) && !(activeRouteMask & (1UL << currentBit))) {
			currentBit++;
		}

		if (currentBit < (NUM_BITS_74HC595 * NUM_SWITCH_REGS)) {
			routeStateMask |= (1UL << currentBit);
			write_route_setup_state();
			pulseActive = 1;
		} else {
			reset_route_state();
			MoveLocoForward(tableIndex);
			routeSetupInProgress = 0;
		}
	}
}
