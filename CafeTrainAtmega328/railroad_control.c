#include "shift_registers.h"
#include "railroad_control.h"
#include "config.h"
#include "timer0.h"
#include <avr/delay.h>


static uint8_t initialized = 0;

const uint16_t routeMasks[] = {
	ROUTE_TO_TABLE_1,
	ROUTE_TO_TABLE_4,
	ROUTE_TO_TABLE_2,
	ROUTE_TO_TABLE_3,
	ROUTE_TO_TABLE_5,
	ROUTE_TO_TABLE_6,
	ROUTE_TO_TABLE_7,
	ROUTE_TO_TABLE_8,
	ROUTE_TO_TABLE_9
};

void reset_route_state(void) {
	initialized = 0;
}

void activate_route_non_blocking(uint8_t tableIndex) {
	
		static uint32_t lastTick = 0;
		static uint8_t currentBit = 0;  // Индекс текущего проверяемого бита
		uint8_t shiftData[NUM_OF_74HC595] = {0};

		if (!initialized) {
			currentBit = 0;                  // Начинаем с первого бита
			lastTick = rail_switch_step_counter;
			initialized = 1;
		}

	if ((rail_switch_step_counter - lastTick) >= 500) {
		lastTick = rail_switch_step_counter;

		// Пропускаем нули
		while (currentBit < 16 && !(routeMasks[tableIndex] & (1 << currentBit))) {
			currentBit++;
		}

		if (currentBit < 16) {
			if (currentBit < 8) {
				shiftData[0] |= (1 << currentBit);
				} else {
				shiftData[1] |= (1 << (currentBit - 8));
			}
			shiftOutMultiple(shiftData, NUM_OF_74HC595);
			currentBit++;
			} 
			else {
			shiftData[0] = 0;
			shiftData[1] = 0;
			shiftData[2] = LOCO_FORWARD;
			shiftOutMultiple(shiftData, NUM_OF_74HC595);
			routeSetupInProgress = 0;
			initialized = 0;
		}
	}
}


