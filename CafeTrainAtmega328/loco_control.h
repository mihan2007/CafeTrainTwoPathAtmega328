#ifndef LOCO_CONTROL_H_
#define LOCO_CONTROL_H_

#include <stdint.h>
#include "uart.h"

#ifdef __cplusplus
extern "C" {
	#endif

	void LocoStop(void);
	void MoveLocoBackward(void);
	void MoveLocoForward(void);

	#ifdef __cplusplus
}
#endif

#endif /* LOCO_CONTROL_H_ */
