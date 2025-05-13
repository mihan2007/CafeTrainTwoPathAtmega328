#ifndef PWM_H_
#define PWM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
	#endif

	void initPWM();
	void startPWMUp();
	void processPWMUp(void);
	void disablePWM(void);
	uint8_t isPWMUpRunning(void);
	void LocomotiveSpeedDown();

	#ifdef __cplusplus
}
#endif

#endif /* PWM_H_ */
