#include "config.h"
#include "loco_control.h"
#include "shift_registers.h"
#include "railroad_control.h"

void LocoStop(void) {
	disablePWM();
	uint8_t shiftData[NUM_OF_74HC595] = {0};
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
	reset_route_state();
	routeSetupInProgress = 0;
}

void MoveLocoForward(void) {
	uint8_t shiftData[NUM_OF_74HC595] = {0};
	shiftData[2] = LOCO_FORWARD;
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
	startPWMUp();
}

void MoveLocoBackward(void) {
	uint8_t shiftData[NUM_OF_74HC595] = {0};
	shiftData[NUM_OF_74HC595 - 1] = LOCO_BACKWARD;
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
	routeSetupInProgress = 0;
}

void SlowMode(void){
	enablePWM();
	OCR1A = (PWM_MAX/2);
}

