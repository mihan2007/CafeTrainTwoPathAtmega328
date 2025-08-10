#ifndef CAFE_TRAIN_RAILROAD_CONTROL_H_
#define CAFE_TRAIN_RAILROAD_CONTROL_H_

#include <stdint.h>
#include "uart.h"

#ifdef __cplusplus
extern "C" {
	#endif

	void activate_route_non_blocking(uint8_t tableIndex);
	
	#ifdef __cplusplus
}
#endif

#endif /* LOCO_CONTROL_H_ */
