#ifndef EEPROM_SETTINGS_H_
#define EEPROM_SETTINGS_H_

#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
#endif

// EEPROM layout:
//   addr 0 = PATH1 slow-mode PWM duty
//   addr 1 = PATH2 slow-mode PWM duty
//   0xFF = unset -> returns PWM_SLOW_DUTY default

uint8_t eeprom_slow_pwm_read(uint8_t path);          // path = 1 or 2
void    eeprom_slow_pwm_write(uint8_t path, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_SETTINGS_H_ */
