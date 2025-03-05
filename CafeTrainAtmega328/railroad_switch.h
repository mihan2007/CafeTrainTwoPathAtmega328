#ifndef RAILROAD_SWITCH_H
#define RAILROAD_SWITCH_H

#ifdef __cplusplus
extern "C" {
	#endif

	#include <stdint.h>

	// Глобальные флаги/переменные, влияющие на состояние процессов
	extern volatile uint8_t processing_route;
	extern volatile uint8_t poceess_done;
	extern volatile uint8_t current_route;
	
	// Инициализация стрелок
	void initRailRoadSwitch(void);

	// Выбор канала (переключение стрелки)
	void selectChannel(uint8_t channel);

	// Запуск маршрута
	void routSetup(uint8_t route_index);

	// Обработка маршрута (вызывается из прерывания)
	void process_route(void);

	void moveLocomotive(uint8_t forward);
	
	void stopLocomotive();

	#ifdef __cplusplus
}
#endif

#endif /* RAILROAD_SWITCH_H */
