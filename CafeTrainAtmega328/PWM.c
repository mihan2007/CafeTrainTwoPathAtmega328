#include "PWM.h"
#include "config.h"

// Инициализация ШИМ на PB1 (OC1A) с 10-битной разрядностью и частотой 20 кГц
void initPWM() {
	DDRB |= (1 << PWM_PIN);  // Настраиваем PB1 как выход
	DDRD |= (1<< PWM_SWITCH_PIN);
	// Включаем 10-битный Fast PWM
	TCCR1A = (1 << COM1A1) | (1 << WGM10) | (1 << WGM11);
	TCCR1B = (1 << WGM12) | (1 << CS10);  // Без делителя, частота 20 кГц

	OCR1A = 0;  // Начальное значение = 0 (двигатель выключен)
}

// Плавный разгон
void LocomotiveSpeedUp() {
	PORTD  |= (1 << PWM_SWITCH_PIN);
	for (uint16_t duty = PWM_MIN; duty <= PWM_MAX; duty += PWM_STEP) {
		OCR1A = duty;
		_delay_ms(PWM_DELAY);
	}
	PORTD &= ~ (1 << PWM_SWITCH_PIN);
}

// Плавное торможение
void LocomotiveSpeedDown() {
	for (uint16_t duty = PWM_MAX; duty >= PWM_MIN; duty -= PWM_STEP) {
		OCR1A = duty;
		_delay_ms(PWM_DELAY);
	}
	OCR1A = 0;  // Полная остановка
}