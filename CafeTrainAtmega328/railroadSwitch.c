#include <avr/io.h>
#include <util/delay.h>

#include <avr/io.h>
#include "config.h"
#include "routes.h"

#include "railroadSwitch.h"
extern uint8_t NUM_ROUTES; // Объявляем его перед использованием!
/*
// #include "config.h" true table
const uint8_t SWITCH_MASKS[16] = {
	0b00000000, // C0  RailroadSwitch 1 Left
	0b00010000, // C1  RailroadSwitch 1 Right
	0b00001000, // C2  RailroadSwitch 2 Left
	0b00011000, // C3  RailroadSwitch 2 Right
	0b00000100, // C4  RailroadSwitch 3 Left
	0b00010100, // C5  RailroadSwitch 3 Right
	0b00001100, // C6  RailroadSwitch 4 Left
	0b00011100, // C7  RailroadSwitch 4 Right
	0b00000001, // C8  RailroadSwitch 5 Left
	0b00010001, // C9  RailroadSwitch 5 Right
	0b00001001, // C10 RailroadSwitch 6 Left
	0b00011001, // C11 RailroadSwitch 6 Right
	0b00000101, // C12 RailroadSwitch 7 Left
	0b00010101, // C13 RailroadSwitch 7 Left
	0b00001101, // C14 Move Backward
	0b00011101  // C15 Move Forward
};

//Structure of the Rout
typedef struct {
	char* name;          // Rout name
	uint8_t path[8];     // Number of rail switches in the rout 
	uint8_t length;      // Quantity of rail switches in the rout 
} Route;

Route routes[] = {
	{"Table 1", {2}, 1},					// Rout to table 1 through C1
	{"Table 2", {1, 4}, 2},					// Rout to table 2 through C1 -> C4
	{"Table 3", {1, 3, 6}, 3},				// Rout to table 3 through C1 -> C3 -> C5
	{"Table 4", {1, 3, 5, 8}, 4},			// Rout to table 4 through C1 -> C3 -> C5 -> C7
	{"Table 5", {1, 3, 5, 7, 10}, 5},		// Rout to table 5 through C1 -> C3 -> C5 -> C7 -> C10
	{"Table 6", {1, 3, 5, 7, 9, 12}, 6},	// Rout to table 6 through C1 -> C3 -> C5 -> C7 -> C9 -> C12	
	{"Table 7", {1, 3, 5, 7, 9, 11, 14}, 7},// Rout to table 7 through C1 -> C3 -> C5 -> C7 -> C9 -> C11 -> C14
	{"Table 8", {1, 3, 5, 7, 9, 11, 13}, 7},// Rout to table 8 through C1 -> C3 -> C5 -> C7 -> C9 -> C11 -> C13		
};

#define NUM_ROUTES (sizeof(routes) / sizeof(routes[0]))
*/
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