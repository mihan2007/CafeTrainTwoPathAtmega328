#ifndef RAILROAD_SWITCH_H
#define RAILROAD_SWITCH_H

#ifdef __cplusplus
extern "C" {
	#endif

	#include <stdint.h>



	// Обработка маршрута (вызывается из прерывания)
	void process_route(void);

	void moveLocomotive(uint8_t forward);
	
	void stopLocomotive();

	#ifdef __cplusplus
}
#endif

#endif /* RAILROAD_SWITCH_H */
