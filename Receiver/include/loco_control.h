#ifndef LOCO_CONTROL_H_
#define LOCO_CONTROL_H_

#include <stdint.h>
#include "uart.h"

#ifdef __cplusplus
extern "C" {
	#endif

	void LocoStop(void);
	void LocoStopTable(uint8_t tableIndex);
	void LocoStopPath(uint8_t path);
	
	void MoveLocoBackward(uint8_t tableIndex);
	
	void MoveLocoForward(uint8_t tableIndex);
	
	void SlowMode(void);
	
	void PowerSupplyOff(void);
	
	void PowerSupplyOn(void);
	void PowerSupplyOnPath(uint8_t path);
	void copy_power_shift_state(uint8_t *shiftData);

	#ifdef __cplusplus
}
#endif

#endif /* LOCO_CONTROL_H_ */
