#include "../include/PWM.h"
#include "../include/config.h"
#include "../include/timer0.h"
#include <util/delay.h>

static uint8_t pwmIncreasingPath1 = 0;
static uint8_t pwmIncreasingPath2 = 0;
static uint16_t currentDutyPath1 = PWM_INITIAL_DUTY;
static uint16_t currentDutyPath2 = PWM_INITIAL_DUTY;
static uint32_t pwmLastTickPath1 = 0;
static uint32_t pwmLastTickPath2 = 0;

void initPWM(void) {
	DDRB |= (1 << PWM_PATH1_PIN) | (1 << PWM_PATH2_PIN);
	DDRB |= (1 << PWM_PATH1_SWITCH_PIN) | (1 << PWM_PATH2_SWITCH_PIN);

	TCCR1A = (1 << WGM10);
	TCCR1B = (1 << WGM12) | (1 << CS10);

	OCR1A = 0;
	OCR1B = 0;
	PORTB &= ~((1 << PWM_PATH1_PIN) | (1 << PWM_PATH2_PIN));
	PORTB &= ~((1 << PWM_PATH1_SWITCH_PIN) | (1 << PWM_PATH2_SWITCH_PIN));
}

void enablePWMPath(uint8_t path) {
	if (path == 2) {
		OCR1B = 0;
		PORTB |= (1 << PWM_PATH2_SWITCH_PIN);
		TCCR1A |= (1 << COM1B1);
		DDRB |= (1 << PWM_PATH2_PIN);
	} else {
		OCR1A = 0;
		PORTB |= (1 << PWM_PATH1_SWITCH_PIN);
		TCCR1A |= (1 << COM1A1);
		DDRB |= (1 << PWM_PATH1_PIN);
	}
}

void enablePWM(void) {
	enablePWMPath(1);
}

void disablePWMPath(uint8_t path) {
	if (path == 2) {
		pwmIncreasingPath2 = 0;
		OCR1B = 0;
		TCCR1A &= ~(1 << COM1B1);
		PORTB &= ~(1 << PWM_PATH2_PIN);
	} else {
		pwmIncreasingPath1 = 0;
		OCR1A = 0;
		TCCR1A &= ~(1 << COM1A1);
		PORTB &= ~(1 << PWM_PATH1_PIN);
	}
}

void disablePWM(void) {
	disablePWMPath(1);
	disablePWMPath(2);
}

void startPWMUpForPath(uint8_t path) {
	if (path == 2) {
		currentDutyPath2 = 0;
		OCR1B = 0;
		enablePWMPath(2);
		pwmLastTickPath2 = rail_switch_step_counter;
		pwmIncreasingPath2 = 1;
	} else {
		currentDutyPath1 = 0;
		OCR1A = 0;
		enablePWMPath(1);
		pwmLastTickPath1 = rail_switch_step_counter;
		pwmIncreasingPath1 = 1;
	}
}

void startPWMUp(void) {
	startPWMUpForPath(1);
}

static void processPWMUpPath1(void) {
	if (!pwmIncreasingPath1) return;

	if ((rail_switch_step_counter - pwmLastTickPath1) >= PWM_DELAY) {
		pwmLastTickPath1 = rail_switch_step_counter;

		if (currentDutyPath1 <= PWM_MAX) {
			OCR1A = currentDutyPath1++;
		} else {
			PORTB &= ~(1 << PWM_PATH1_SWITCH_PIN);
			disablePWMPath(1);
			pwmIncreasingPath1 = 0;
		}
	}
}

static void processPWMUpPath2(void) {
	if (!pwmIncreasingPath2) return;

	if ((rail_switch_step_counter - pwmLastTickPath2) >= PWM_DELAY) {
		pwmLastTickPath2 = rail_switch_step_counter;

		if (currentDutyPath2 <= PWM_MAX) {
			OCR1B = currentDutyPath2++;
		} else {
			PORTB &= ~(1 << PWM_PATH2_SWITCH_PIN);
			disablePWMPath(2);
			pwmIncreasingPath2 = 0;
		}
	}
}

void processPWMUp(void) {
	processPWMUpPath1();
	processPWMUpPath2();
}

uint8_t isPWMUpRunning(void) {
	return pwmIncreasingPath1 || pwmIncreasingPath2;
}

uint8_t isPWMUpRunningForPath(uint8_t path) {
	return (path == 2) ? pwmIncreasingPath2 : pwmIncreasingPath1;
}

void LocomotiveSpeedDown(void) {

}
