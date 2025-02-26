#include <avr/io.h>
#include <util/delay.h>

#include <avr/io.h>
#include "config.h"
#include "routes.h"

#include "railroadSwitch.h"
extern uint8_t NUM_ROUTES; // Объявляем его перед использованием!

void initRailRoadSwitch();
void railRoadSwitchTest();
void selectChannel(uint8_t channel);


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

	// Запуск теста стрелок
	//railRoadSwitchTest();
}

// Функция тестирования всех стрелок
void railRoadSwitchTest() {
	for (uint8_t i = 0; i <= 13; i++) {
		selectChannel(i);
		PORTD |= (1 << SIG);
		_delay_ms(SWITCH_PAUSE_TIME);
		PORTD &= ~(1 << SIG);
		_delay_ms(SWITCH_PAUSE_TIME);
	}
}

// Функция выбора стрелочного канала
void selectChannel(uint8_t channel) {
	if (channel < 1 || channel > 16) return;

	PORTB &= ~SWITCH_MASK;  // Сброс текущего состояния
	PORTB |= (SWITCH_MASKS[channel - 1] & SWITCH_MASK); // Корректный доступ к массиву
}

void setPath(uint8_t* path, uint8_t length) {
	for (uint8_t i = 0; i < length; i++) {
		selectChannel(path[i]); // Выбираем стрелку
		PORTD |= (1 << SIG);    // Активируем стрелочный механизм
		_delay_ms(SWITCH_PAUSE_TIME);         // Ждём переключения
		PORTD &= ~(1 << SIG);   // Отключаем сигнал
		_delay_ms(SWITCH_PAUSE_TIME);
	}
}

void setRouteByIndex(uint8_t index) {
	if (index >= NUM_ROUTES || routes[index].length > 8) return;

	clearChannels();

	setPath(routes[index].path, routes[index].length);
}

void moveLocomotive(uint8_t forward) {
	stopLocomotive();
	
	uint8_t channel = forward ? 15 : 14;  // 15 - вперёд, 14 - назад

	selectChannel(channel);
	
	PORTD |= (1 << SIG);  // Включаем сигнал движения
}

void stopLocomotive() {

	clearChannels();
}

void clearChannels() {
	PORTD &= ~(1 << SIG);  // Отключаем сигнал движения
	PORTB &= ~SWITCH_MASK; // Сбрасываем все каналы (включая 14 и 15)
}