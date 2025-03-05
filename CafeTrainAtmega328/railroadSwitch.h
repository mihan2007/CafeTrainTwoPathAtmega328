
#include <stdint.h>
#ifndef RAILROADSWITCH_H_
#define RAILROADSWITCH_H_

#ifdef __cplusplus
extern "C" {
	#endif

		void initRailRoadSwitch();

		void start_route(uint8_t route_index);

		void process_route(void);

		void moveLocomotive(uint8_t forward);
		void stopLocomotive();
		void selectChannel(uint8_t index);
				
	#ifdef __cplusplus
}
#endif

#endif /* RAILROADSWITCH_H_ */
