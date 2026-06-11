// protection.h
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
	#endif
	void check_and_send_overload_stop(void);
	void checkLocoMovementTimeout(void);
	void stopLocoDueToTimeout(void);
	void stopLocoDueToTimeout(void);
	void resetLocoTimer(void);
	void overload_led_sync(void);
	void clear_overload_emergency(uint8_t notifyTransmitter);
	#ifdef __cplusplus
}
#endif
