#ifndef EEPROM_SETTINGS_H_
#define EEPROM_SETTINGS_H_

#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t eeprom_slow_pwm_read(uint8_t path);
void eeprom_slow_pwm_write(uint8_t path, uint8_t value);
uint8_t eeprom_accel_delay_read(uint8_t path);
void eeprom_accel_delay_write(uint8_t path, uint8_t value);
uint8_t eeprom_overload_threshold_read(void);
void eeprom_overload_threshold_write(uint8_t storedVal);

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_SETTINGS_H_ */
