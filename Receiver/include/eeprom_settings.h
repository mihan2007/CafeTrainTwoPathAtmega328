#ifndef EEPROM_SETTINGS_H_
#define EEPROM_SETTINGS_H_

#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
#endif

// EEPROM layout:
//   addr 0 = PATH1 slow-mode PWM duty
//   addr 1 = PATH2 slow-mode PWM duty
//   addr 2 = overload threshold (stored as threshold/10)
//   0xFF in any cell = unset -> returns compile-time default

uint8_t eeprom_slow_pwm_read(uint8_t path);
void    eeprom_slow_pwm_write(uint8_t path, uint8_t value);

// Overload threshold: stored value = real_threshold / 10
// e.g. 900 ADC -> stored as 90; display: storedVal * 10
uint8_t eeprom_overload_threshold_read(void);
void    eeprom_overload_threshold_write(uint8_t storedVal);

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_SETTINGS_H_ */
