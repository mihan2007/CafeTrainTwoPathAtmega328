#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "system_init.h"
#include "lcd.h"
#include "shift_registers.h"
#include "config.h"

#define SENSOR_INDEX_OFFSET 1

int8_t get_triggered_sensor(uint8_t states) {
	for (uint8_t i = 0; i < 8; i++) {
		if (states & (1 << i)) {
			return i + SENSOR_INDEX_OFFSET;
		}
	}
	return -1;
}

int main(void) {
	system_init();
	
	LCD_Clear();
	LCD_PrintTwoLines("Welcome", "Select table", 0);
	
	int8_t currentTable = -1;

while (1) {
		PORTB |= (1 << LIGHT_INDICATION_ENABLE);  // Светодиод

		uint8_t rawBits = read_74HC165();
		uint8_t invertedBits = ~rawBits;

		int8_t detectedTable = get_triggered_sensor(invertedBits);

		if (detectedTable >= 0 && detectedTable != currentTable) {
			currentTable = detectedTable;

			// Формируем строку TABLE: X
			char line2[17];
			snprintf(line2, sizeof(line2), "TABLE: %-2d        ", currentTable);

			// Печатаем обе строки
			LCD_PrintTwoLines("Select command", line2, 0);
		}

		_delay_ms(100);
	}
}

