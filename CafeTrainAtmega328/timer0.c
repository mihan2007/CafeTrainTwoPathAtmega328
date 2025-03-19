#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer0.h"

// Можно хранить счётчик здесь
volatile uint16_t step_counter = 0;

// Нужно вызвать функцию process_route(), которая определена в другом файле.
extern void process_route(void);

void timer0_init(void) {
	TCCR0A |= (1 << WGM01); // CTC Mode
	TCCR0B |= (1 << CS01) | (1 << CS00); // Предделитель 64
	OCR0A = 249; // Прерывание раз в 1 мс
	TIMSK0 |= (1 << OCIE0A); // Включаем прерывание по совпадению

	// Разрешаем прерывание по совпадению с OCR0A
	TIMSK0 |= (1 << OCIE0A);
}

// Обработчик прерывания по совпадению с OCR0A
ISR(TIMER0_COMPA_vect) {
	step_counter++;    // Увеличиваем счётчик каждые ~5 мс
}
