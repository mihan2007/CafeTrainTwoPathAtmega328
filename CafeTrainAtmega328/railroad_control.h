#ifndef RAILROAD_CONTROL_H
#define RAILROAD_CONTROL_H

#ifdef __cplusplus
extern "C" {
	#endif

		#include <stdint.h>

		void activate_route_non_blocking(uint8_t tableIndex);
		extern uint8_t moveForwardActive;

	#ifdef __cplusplus
}
#endif

#endif // RAILROAD_CONTROL_H
