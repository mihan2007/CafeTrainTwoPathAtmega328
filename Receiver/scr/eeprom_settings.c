#include <avr/eeprom.h>
#include "../include/eeprom_settings.h"
#include "../include/config.h"

// EEPROM layout:
//   addr 0 = PATH1 slow-mode PWM duty
//   addr 1 = PATH2 slow-mode PWM duty
//   addr 2 = overload threshold (stored as threshold/10, default 90 = 900 ADC)
//   0xFF in any cell = unset -> use compile-time default
#define EEPROM_SLOW_PWM_BASE     0
#define EEPROM_OVERLOAD_THR_ADDR 2

uint8_t eeprom_slow_pwm_read(uint8_t path) {
    if (path < 1 || path > 2) return PWM_SLOW_DUTY;
    uint8_t raw = eeprom_read_byte(
        (const uint8_t *)(uintptr_t)(EEPROM_SLOW_PWM_BASE + path - 1));
    if (raw == 0xFF) return PWM_SLOW_DUTY;  // unset cell -> default
    return raw;
}

void eeprom_slow_pwm_write(uint8_t path, uint8_t value) {
    if (path < 1 || path > 2) return;
    eeprom_update_byte(
        (uint8_t *)(uintptr_t)(EEPROM_SLOW_PWM_BASE + path - 1),
        value);
}

uint8_t eeprom_overload_threshold_read(void) {
    uint8_t raw = eeprom_read_byte((const uint8_t *)(uintptr_t)EEPROM_OVERLOAD_THR_ADDR);
    if (raw == 0xFF) return (uint8_t)(OVERLOAD_THRESHOLD / 10);  // unset -> default
    return raw;
}

void eeprom_overload_threshold_write(uint8_t storedVal) {
    eeprom_update_byte((uint8_t *)(uintptr_t)EEPROM_OVERLOAD_THR_ADDR, storedVal);
}
