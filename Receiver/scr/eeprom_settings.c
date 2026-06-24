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

static uint8_t clamp_u8(uint8_t value, uint8_t minValue, uint8_t maxValue) {
	if (value < minValue) return minValue;
	if (value > maxValue) return maxValue;
	return value;
}

uint8_t eeprom_slow_pwm_read(uint8_t path) {
	uint8_t address = (path == 2) ? EEPROM_SLOW_PWM_PATH2_ADDR : EEPROM_SLOW_PWM_PATH1_ADDR;
	uint8_t value = eeprom_read_byte((const uint8_t *)(uintptr_t)address);
	value = eeprom_default_if_empty(value, PWM_SLOW_DUTY);
	return clamp_u8(value, MENU_PWM_SLOW_MIN, MENU_PWM_SLOW_MAX);
}

void eeprom_slow_pwm_write(uint8_t path, uint8_t value) {
	uint8_t address = (path == 2) ? EEPROM_SLOW_PWM_PATH2_ADDR : EEPROM_SLOW_PWM_PATH1_ADDR;
	value = clamp_u8(value, MENU_PWM_SLOW_MIN, MENU_PWM_SLOW_MAX);
	eeprom_update_byte((uint8_t *)(uintptr_t)address, value);
}

uint8_t eeprom_overload_threshold_read(void) {
	uint8_t value = eeprom_read_byte((const uint8_t *)(uintptr_t)EEPROM_OVERLOAD_THR_ADDR);
	value = eeprom_default_if_empty(value, (uint8_t)(OVERLOAD_THRESHOLD / 10));
	return clamp_u8(value, (uint8_t)(MENU_OVERLOAD_ADC_MIN / 10), (uint8_t)(MENU_OVERLOAD_ADC_MAX / 10));
}

void eeprom_overload_threshold_write(uint8_t storedVal) {
	storedVal = clamp_u8(storedVal, (uint8_t)(MENU_OVERLOAD_ADC_MIN / 10), (uint8_t)(MENU_OVERLOAD_ADC_MAX / 10));
	eeprom_update_byte((uint8_t *)(uintptr_t)EEPROM_OVERLOAD_THR_ADDR, storedVal);
}
