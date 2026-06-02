#ifndef CAFE_TRAIN_RAILROAD_CONTROL_H_
#define CAFE_TRAIN_RAILROAD_CONTROL_H_

#include <stdint.h>
#include "uart.h"

#ifdef __cplusplus
extern "C" {
	#endif

	void activate_route_non_blocking(uint8_t tableIndex);
	void reset_route_state(void);
	void reset_route_path_state(uint8_t path);
	void copy_route_shift_state(uint8_t *shiftData);
	
	#ifdef __cplusplus
}
#endif

#endif /* LOCO_CONTROL_H_ */
