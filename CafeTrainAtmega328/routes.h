#ifndef ROUTES_H_
#define ROUTES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
	#endif

	// Таблица переключений стрелок (объявление)
	extern const uint8_t SWITCH_MASKS[16];

	// Структура маршрута
	typedef struct {
		char* name;       // Название маршрута
		uint8_t path[8];  // Номера стрелок на маршруте
		uint8_t length;   // Количество стрелок
	} Route;

	// Объявляем массив маршрутов (без определения)
	extern Route routes[];

	// Убираем `#define NUM_ROUTES` отсюда!
	extern const uint8_t NUM_ROUTES;
	
	#ifdef __cplusplus
}
#endif

#endif /* ROUTES_H_ */
