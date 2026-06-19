#include <avr/eeprom.h>
#include "../include/eeprom_settings.h"
#include "../include/config.h"

// EEPROM base: addr 0 = PATH1, addr 1 = PATH2
#define EEPROM_SLOW_PWM_BASE 0

uint8_t eeprom_slow_pwm_read(uint8_t path) {
    if (path < 1 || path > 2) return PWM_SLOW_DUTY;
    uint8_t raw = eeprom_read_byte(
        (const uint8_t *)(uintptr_t)(EEPROM_SLOW_PWM_BASE + path - 1));
    if (raw == 0xFF) return PWM_SLOW_DUTY;  // unset cell -> default
    return raw;
}

void eeprom_slow_pwm_write(uint8_t path, uint8_t value) {
    if (path < 1 || path > 2) return;
    // eeprom_update_byte skips write if value unchanged (protects endurance)
    eeprom_update_byte(
        (uint8_t *)(uintptr_t)(EEPROM_SLOW_PWM_BASE + path - 1),
        value);
}
