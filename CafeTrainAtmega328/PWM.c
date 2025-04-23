#include "PWM.h"
#include "config.h"
#include "timer0.h"
#include <util/delay.h>

static uint8_t pwmIncreasing = 0;
static uint16_t currentDuty = 0;
static uint32_t pwmLastTick = 0;

// Инициализация ШИМ на PB1 (OC1A) с 10-битной разрядностью и частотой 20 кГц
void initPWM() {

	DDRB |= (1 << PWM_PIN);  // Настраиваем PB1 как выход

	// Включаем 8-битный Fast PWM
	TCCR1A = (1 << COM1A1) | (1 << WGM10);  // 8-bit
	TCCR1B = (1 << WGM12) | (1 << CS10);   // Fast PWM, без делителя

	OCR1A = 0;  // Стартовое значение
}

void enablePWM() {
	OCR1A = 0; // ? на всякий случай	
	TCCR1A |= (1 << COM1A1); // Включаем выход ШИМ
	DDRB |= (1 << PWM_PIN);  // Устанавливаем пин как выход
}

void disablePWM() {
	OCR1A = 0;
	TCCR1A &= ~(1 << COM1A1); // Отключаем выход ШИМ
	PORTB &= ~(1 << PWM_PIN); // Принудительно ставим 0
}

// Soft Start
void startPWMUp() {
	currentDuty = 0;
	OCR1A = 0;          // ? ОБЯЗАТЕЛЬНО, иначе будет всплеск
	enablePWM();
	pwmLastTick = rail_switch_step_counter;
	pwmIncreasing = 1;	
}



void processPWMUp() {
	if (!pwmIncreasing) return;

	if ((rail_switch_step_counter - pwmLastTick) >= PWM_DELAY) {
		pwmLastTick = rail_switch_step_counter;

		if (currentDuty <= PWM_MAX) {
			OCR1A = currentDuty++;
			} else {
			
			pwmIncreasing = 0;
		}
	}
}

uint8_t isPWMUpRunning() {
	return pwmIncreasing;
}

// Soft Slow Down
void LocomotiveSpeedDown() {

}