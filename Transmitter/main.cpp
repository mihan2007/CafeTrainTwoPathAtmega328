#include <avr/io.h>
#include <util/delay.h>
#include "system_init.h"
#include "lcd.h"
#include "shift_registers.h"
#include "config.h"

int main(void) {
	system_init();

	while (1) {
		PORTB |= (1 << LIGHT_INDICATION_ENABLE);  // Зажечь светодиод на D8

		uint8_t rawBits = read_74HC165();        // Считать с регистра
		uint8_t invertedBits = ~rawBits;         // Инвертировать
		print_triggered_sensor(invertedBits);    // Показать на 2-й строке

		_delay_ms(100); // Обновлять каждые 100 мс
	}
}
