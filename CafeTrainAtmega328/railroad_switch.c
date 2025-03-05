#include <avr/io.h>
#include "railroad_switch.h"
#include "routes.h"     // для доступа к структурам маршрутов и т.д.
#include "timer0.h"     // чтобы видеть extern volatile uint16_t step_counter
#include "config.h"

// Глобальные переменные (объявлены extern в .h)
volatile uint8_t processing_route = 0;  
volatile uint8_t poceess_done     = 0;  
volatile uint8_t current_route = 0;

// Индекс текущей стрелки в маршруте
static volatile uint8_t current_switch_index = 0;

// Выбор канала: отключаем все стрелки, затем включаем нужные биты
void selectChannel(uint8_t channel) {
    if (channel < 1 || channel > 16) return;
    PORTB &= ~SWITCH_MASK;
    PORTB |= (SWITCH_MASKS[channel - 1] & SWITCH_MASK);
}

// Инициализация управления стрелками
void initRailRoadSwitch(void) {
    // Настройка портов: S0-S3 как выходы
    DDRB |= SWITCH_MASK;
    // SIG (PD7) как выход
    DDRD |= (1 << SIG);

    // Отключаем все стрелки, НЕ затрагивая PB1 (ШИМ)
    PORTB &= ~SWITCH_MASK;
    // SIG неактивен
    PORTD &= ~(1 << SIG);
}

// Запуск заданного маршрута
void start_route(uint8_t route_index) {
    if (route_index >= NUM_ROUTES) return; // Проверяем, что маршрут существует

    current_route = route_index;  // Сохраняем номер маршрута
    current_switch_index = 0;
	
    processing_route = 1;  // Флаг, что процесс пошёл
    step_counter    = 0;   // Сбрасываем счётчик таймера
}

// Функция, вызываемая из таймера (каждые ~5 мс). 
// Меняет стрелку раз в 500 мс (100*5 мс).
void process_route(void) {
	if (!processing_route) return;

	// Берем маршрут
	Route selected_route = routes[current_route];

	// Срабатываем только раз в 500 мс
	if (step_counter >= SWITCH_PAUSE_TIME) {
		step_counter = 0;

		// Проверяем, не все ли стрелки уже переключены
		if (current_switch_index < selected_route.length) {
			// Включаем очередную стрелку
			uint8_t switch_index = selected_route.path[current_switch_index];
			selectChannel(switch_index);
			PORTD |= (1 << SIG);
			
			// Переходим к следующей стрелке
			current_switch_index++;
			} else {
			// Если стрелок в маршруте больше нет — завершаем
			PORTD &= ~(1 << SIG);
			processing_route = 0;
			poceess_done = 0;
			
            moveLocomotive(1);
		}
	}
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