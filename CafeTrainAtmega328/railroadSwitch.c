#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "railroadSwitch.h"
#include "config.h"
#include "routes.h"
#include "timer0.h"

//extern uint8_t NUM_ROUTES; // Объявляем его перед использованием!

// Глобальные переменные (объявлены extern в .h)
volatile uint8_t processing_route = 0;
volatile uint8_t poceess_done     = 0;

// Индекс текущей стрелки в маршруте
static volatile uint8_t current_switch_index = 0;

// Функция инициализации стрелочных переводов
void initRailRoadSwitch() {
	// Настройка портов: S0-S3 как выходы
	DDRB |= SWITCH_MASK;
	// SIG как выход
	DDRD |= (1 << SIG);

	// Отключаем все стрелки, НЕ затрагивая PB1 (ШИМ)
	PORTB &= ~SWITCH_MASK;
	// SIG неактивен
	PORTD &= ~(1 << SIG);
}

// Запуск заданного маршрута
void start_route(uint8_t route_index) {
	//if (route_index >= NUM_ROUTES) return; // Проверяем, что маршрут существует
	current_switch_index = 0;
	processing_route = 1;  // Флаг, что процесс пошёл
	step_counter    = 0;   // Сбрасываем счётчик таймера
}

void process_route(void) {
	if (!processing_route) return;  // Если процесс не запущен, выходим

	Route selected_route = routes[current_switch_index];

	// Сработаем каждые 500 мс (примерно каждые 100 итераций по 5 мс)
	if (step_counter >= 100) {
		step_counter = 0; // сброс счётчика

		if (current_switch_index < selected_route.length) {
			uint8_t switch_index = selected_route.path[current_switch_index];

			// Включаем соответствующую стрелку
			selectChannel(switch_index);

			// Включаем SIG (PD7)
			PORTD |= (1 << SIG);
			} else {
			// Выключаем SIG (PD7)
			PORTD &= ~(1 << SIG);

			// Завершаем маршрут
			processing_route = 0;
			poceess_done = 0;
			// Вы можете выставить poceess_done = 1, если хотите обозначить «завершено».
			// Но судя по вашему циклу, вы используете "0" как сигнал к запуску
			// нового маршрута. Зависит от вашей логики.
		}

		current_switch_index++; // Переход к следующей стрелке
	}
}

// Функция выбора стрелочного канала
void selectChannel(uint8_t channel) {
	if (channel < 1 || channel > 16) return;

	PORTB &= ~SWITCH_MASK;  // Сброс текущего состояния
	PORTB |= (SWITCH_MASKS[channel - 1] & SWITCH_MASK); // Корректный доступ к массиву
}

void moveLocomotive(uint8_t forward) {
	stopLocomotive();
	
	uint8_t channel = forward ? 15 : 14;  // 15 - вперёд, 14 - назад

	selectChannel(channel);
	
	PORTD |= (1 << SIG);  // Включаем сигнал движения
}

void stopLocomotive() {
    PORTD &= ~(1 << SIG); // Отключаем сигнал SIG перед очисткой каналов
}

void clearChannels() {
	PORTD &= ~(1 << SIG);  // Отключаем сигнал движения
	PORTB &= ~SWITCH_MASK; // Сбрасываем все каналы (включая 14 и 15)
}





