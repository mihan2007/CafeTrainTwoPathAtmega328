#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "system_init.h"
#include "lcd.h"
#include "config.h"
#include "uart.h"
#include "shift_registers.h"

static uint8_t sensorStates = 0;
uint8_t triggeredBitMask = 0;
uint8_t triggeredBitsHistory = 0;
uint8_t previousSensorStates = 0xFF;

bool isForwardDirection() {

}
void handleForwardSensors(uint8_t mask) {

	print_triggered_sensor(triggeredBitsHistory);
}

void handleReverseSensors(uint8_t mask) {

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

int main(void) {
	UART_init();


	static uint8_t selected_table = 0; // Хранит номер выбранного стола

	while (1) {
	checkSensorsState();
	}

	return 0;
}
