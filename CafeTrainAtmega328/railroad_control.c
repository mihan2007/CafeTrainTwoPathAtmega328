#include "shift_registers.h"
#include "railroad_control.h"
#include "config.h"
#include "timer0.h"

uint8_t moveForwardActive = 0;  // Флаг, что команда MOVE_FORWARD активна и процесс ещё не завершён

void activate_route_non_blocking(uint8_t tableIndex) {

	static uint32_t lastTick = 0;
	static uint8_t initialized = 0;
	static uint16_t mask = 0;
	static uint8_t currentBit = 0;  // Индекс текущего проверяемого бита
	uint8_t shiftData[NUM_OF_74HC595] = {0};

	if (!initialized) {
		mask = routeMasks[tableIndex];  // Загружаем выбранную маску из config.h
		currentBit = 0;                  // Начинаем с первого бита
		lastTick = rail_switch_step_counter;
		initialized = 1;
	}

	if ((rail_switch_step_counter - lastTick) >= 500) {  // Интервал переключения для каждого бита
		lastTick = rail_switch_step_counter;

		if (mask & (1 << currentBit)) {  // Если текущий бит установлен в 1
			if (currentBit < 8) {
				shiftData[0] |= (1 << currentBit);
				} else {
				shiftData[1] |= (1 << (currentBit - 8));
			}
		}

		shiftOutMultiple(shiftData, NUM_OF_74HC595);  // Отправляем текущий статус

		currentBit++;  // Переходим к следующему биту

		// Когда все биты пройдены, обнуляем их по очереди
		if (currentBit >= 16) {
			currentBit = 0;
			shiftData[0] = 0;
			shiftData[1] = 0;
			shiftData[2] = 0;
			shiftOutMultiple(shiftData, NUM_OF_74HC595);  // Гасим все биты
			moveForwardActive = 0;  // Заканчиваем процесс
			initialized = 0;        // Готовим к следующему запуску
		}
	}
}
