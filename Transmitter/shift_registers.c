#include "shift_registers.h"
#include "config.h"

void init_74HC165_ports(void) {
	DDRD |= (1 << CLOCK_74HC165) | (1 << LATCH_74HC165); // PD5 ? PD6 - ??????
	DDRD &= ~(1 << DATA_74HC165);                        // PD7 - ????
	PORTD &= ~(1 << DATA_74HC165);
	
	uint8_t initialData[NUM_OF_74HC595] = {0};
	shiftOutMultiple(initialData, NUM_OF_74HC595);
}

uint8_t read_74HC165(void) {
	uint8_t data = 0;

	PORTD &= ~(1 << LATCH_74HC165);
	_delay_us(5);
	PORTD |= (1 << LATCH_74HC165);

	for (uint8_t i = 0; i < NUM_BITS_74HC595; i++) {
		data <<= 1;
		PORTD &= ~(1 << CLOCK_74HC165);
		_delay_us(5);

		if (PIND & (1 << DATA_74HC165)) {
			data |= 1;
		}

		PORTD |= (1 << CLOCK_74HC165);
		_delay_us(5);
	}

	return data;
}

void init_74HC595_ports(void) {
	DDRD |= (1 << DATA_74HC595) | (1 << CLOCK_74HC595) | (1 << LATCH_74HC595);
	PORTD &= ~((1 << DATA_74HC595) | (1 << CLOCK_74HC595) | (1 << LATCH_74HC595));
}

void shiftOut(uint8_t data) {
	for (uint8_t i = 0; i < NUM_BITS_74HC595; i++) {
		if (data & (1 << (NUM_BITS_74HC595 - 1 - i))) {
			PORTD |= (1 << DATA_74HC595);
			} else {
			PORTD &= ~(1 << DATA_74HC595);
		}

		PORTD |= (1 << CLOCK_74HC595);
		_delay_us(1);
		PORTD &= ~(1 << CLOCK_74HC595);
	}
}

void latchData(void) {
	PORTD |= (1 << LATCH_74HC595);
	_delay_us(1);
	PORTD &= ~(1 << LATCH_74HC595);
}

void shiftOutMultiple(uint8_t *data, uint8_t count) {
	// Активируем питание рельс
	PORTB |= (1 << RAIL_POWER_ENABLE);

	for (uint8_t i = 0; i < count; i++) {
		shiftOut(data[i]);
	}

	latchData();
}
