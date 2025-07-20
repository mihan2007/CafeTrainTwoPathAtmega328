#include "config.h"
#include "loco_control.h"
#include "shift_registers.h"
#include "railroad_control.h"

const uint8_t powerRoutesSuplly[] = {
	DC_SUPLLY_TABLE_1,
	DC_SUPLLY_TABLE_2,
	DC_SUPLLY_TABLE_3,
	DC_SUPLLY_TABLE_4,
	DC_SUPLLY_TABLE_5,
	DC_SUPLLY_TABLE_6,
	DC_SUPLLY_TABLE_7,
	DC_SUPLLY_TABLE_8

};

void LocoStop(void) {
	
	PORTB  &= ~(1 << REVERS_PIN);
	PORTB &= ~(1 << PWM_SWITCH_PIN);
	
	disablePWM();
	
	uint8_t shiftData[NUM_OF_74HC595] = {0};
		
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
	
	reset_route_state();
	
	routeSetupInProgress = 0;
	
	PowerSupplyOff();
}

void MoveLocoForward(uint8_t tableIndex) {
	
	PORTB &= ~(1 << REVERS_PIN);
	
	PowerSupplyOn();
	
	uint8_t shiftData[NUM_OF_74HC595] = {0};
	
	shiftData[2] = powerRoutesSuplly[tableIndex];
	
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
	
	startPWMUp();
}

void MoveLocoBackward(uint8_t tableIndex) {
	
	uint8_t shiftRegisterMask = tableIndex - 1;
	
	LocoStop();
	
	ReversOn();
	
	PowerSupplyOn();
	
	startPWMUp();
	
	uint8_t shiftData[NUM_OF_74HC595] = {0};
	
	shiftData[2] = powerRoutesSuplly[shiftRegisterMask];
			
	shiftOutMultiple(shiftData, NUM_OF_74HC595);
	
	routeSetupInProgress = 0;
	
}

void SlowMode(void){
	enablePWM();
	OCR1A = (PWM_MAX/2);
}

void PowerSupplyOn() {

	PORTB |= (1 << RAIL_POWER_ENABLE);
	PORTB |= (1 << PWM_SWITCH_PIN);	
}

void PowerSupplyOff() {
	
	PORTB &= ~ (1 << RAIL_POWER_ENABLE);
}

void ReversOn(){
		PORTB |= (1 << REVERS_PIN);
}