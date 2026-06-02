#ifndef PWM_H_
#define PWM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
	#endif

	void initPWM(void);
	void enablePWM(void);
	void enablePWMPath(uint8_t path);
	void startPWMUp(void);
	void startPWMUpForPath(uint8_t path);
	void processPWMUp(void);
	void disablePWM(void);
	void disablePWMPath(uint8_t path);
	uint8_t isPWMUpRunning(void);
	uint8_t isPWMUpRunningForPath(uint8_t path);
	void LocomotiveSpeedDown(void);

	#ifdef __cplusplus
}
#endif

#endif /* PWM_H_ */
