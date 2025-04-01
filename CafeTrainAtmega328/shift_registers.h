#ifndef SHIFT_REGISTERS_H
#define SHIFT_REGISTERS_H

#ifdef __cplusplus
extern "C" {
	#endif

	#include <avr/io.h>
	#include <util/delay.h>
	



	void init_74HC165_ports(void);
	uint8_t read_74HC165(void);

	void init_74HC595_ports(void);
	void shiftOut(uint8_t data);
	void latchData(void);
	
	void shiftOutMultiple(uint8_t *data, uint8_t count);
	
	void shiftOutMultiple(uint8_t *shiftData, uint8_t count);

	#ifdef __cplusplus
}
#endif

#endif // SHIFT_REGISTERS_H