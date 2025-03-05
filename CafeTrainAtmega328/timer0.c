#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer0.h"

// Можно хранить счётчик здесь
volatile uint16_t step_counter = 0;

// Нужно вызвать функцию process_route(), которая определена в другом файле.
extern void process_route(void);

void timer0_init(void) {
	// Настраиваем Timer0 в режиме CTC (Clear Timer on Compare)
	TCCR0A |= (1 << WGM01);               // Режим CTC
	TCCR0B |= (1 << CS02) | (1 << CS00);  // Предделитель 1024
	OCR0A = 77;                           // ~5 мс

	// Разрешаем прерывание по совпадению с OCR0A
	TIMSK0 |= (1 << OCIE0A);
}

// Обработчик прерывания по совпадению с OCR0A
ISR(TIMER0_COMPA_vect) {
	step_counter++;    // Увеличиваем счётчик каждые ~5 мс
	process_route();   // Проверяем, пора ли менять стрелку
}
