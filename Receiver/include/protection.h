// protection.h
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void check_and_send_overload_stop(void);
void checkLocoMovementTimeout(void);
void stopLocoDueToTimeout(void);
void resetLocoTimer(void);
void overload_led_sync(void);
void clear_overload_emergency(uint8_t notifyTransmitter);
// Call once at startup (and after CMD_MENU_SET for threshold) to update
// the cached threshold from EEPROM. storedVal = real_threshold / 10.
void overload_update_threshold(uint8_t storedVal);
#ifdef __cplusplus
}
#endif
