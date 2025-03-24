#ifndef SHIFT_REGISTERS_H
#define SHIFT_REGISTERS_H

#ifdef __cplusplus
extern "C" {
	#endif

	#include <avr/io.h>
	#include <util/delay.h>
	
	extern uint8_t LOCO_CTRL;
	extern uint8_t SWITCH_A_CTRL;
	extern uint8_t SWITCH_B_CTRL;

	#define NUM_BITS_74HC595 8  // Количество бит в байте
	#define MUM_OF_74HC595 3    // Количество включенных в каскад 74HC595

	// Определение пинов для 74HC165
	#define LATCH_74HC165 PD6   // LATCH для 74HC165
	#define CLOCK_74HC165 PD5   // CLOCK для 74HC165
	#define DATA_74HC165  PD7   // DATA для 74HC165

	// Определение пинов для 74HC595
	#define DATA_74HC595    PD2
	#define CLOCK_74HC595   PD3
	#define LATCH_74HC595   PD4

	void init_74HC165_ports(void);
	uint8_t read_74HC165(void);

	void init_74HC595_ports(void);
	void shiftOut(uint8_t data);
	void latchData(void);
	
	void shiftOutMultiple(uint8_t *data, uint8_t count);
	
	void update_shift_registers();

	#ifdef __cplusplus
}
#endif

#endif // SHIFT_REGISTERS_H