#include <stdint.h>
#include <avr/eeprom.h>
#include "../include/eeprom_settings.h"
#include "../include/config.h"

#define EEPROM_SLOW_PWM_PATH1_ADDR 0
#define EEPROM_SLOW_PWM_PATH2_ADDR 1
#define EEPROM_OVERLOAD_THR_ADDR   2

static uint8_t eeprom_default_if_empty(uint8_t value, uint8_t defaultValue) {
	return (value == 0xFF) ? defaultValue : value;
}

uint8_t eeprom_slow_pwm_read(uint8_t path) {
	uint8_t address = (path == 2) ? EEPROM_SLOW_PWM_PATH2_ADDR : EEPROM_SLOW_PWM_PATH1_ADDR;
	uint8_t value = eeprom_read_byte((const uint8_t *)(uintptr_t)address);
	return eeprom_default_if_empty(value, PWM_SLOW_DUTY);
}

void eeprom_slow_pwm_write(uint8_t path, uint8_t value) {
	uint8_t address = (path == 2) ? EEPROM_SLOW_PWM_PATH2_ADDR : EEPROM_SLOW_PWM_PATH1_ADDR;
	eeprom_update_byte((uint8_t *)(uintptr_t)address, value);
}

uint8_t eeprom_overload_threshold_read(void) {
	uint8_t value = eeprom_read_byte((const uint8_t *)(uintptr_t)EEPROM_OVERLOAD_THR_ADDR);
	return eeprom_default_if_empty(value, (uint8_t)(OVERLOAD_THRESHOLD / 10));
}

void eeprom_overload_threshold_write(uint8_t storedVal) {
	eeprom_update_byte((uint8_t *)(uintptr_t)EEPROM_OVERLOAD_THR_ADDR, storedVal);
}
