
#include <stdint.h>
#ifndef RAILROADSWITCH_H_
#define RAILROADSWITCH_H_

#ifdef __cplusplus
extern "C" {
#endif

	void initRailRoadSwitch();
	void setRouteByIndex(uint8_t index);
	void moveLocomotive(uint8_t forward);
	void stopLocomotive();
#ifdef __cplusplus
}
#endif

#endif /* RAILROADSWITCH_H_ */
