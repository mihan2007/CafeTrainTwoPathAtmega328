#include "railroadSwitch.h"
#include <avr/io.h>
#include <util/delay.h>


#define S0 PB4  // S0 подключен к D12 (PB4)
#define S1 PB3  // S1 подключен к D11 (PB3)
#define S2 PB2  // S2 подключен к D10 (PB2)
#define S3 PB0  // S3 подключен к D8 (PB0)
#define SIG PD7 // SIG подключен к D7 (PD7)

#define switchPauseTime 1000

uint8_t testExecuted = 0; 
uint8_t i = 0;

void initRailRoadSwitch() {
	DDRB |= (1 << S0) | (1 << S1) | (1 << S2) | (1 << S3); // Линии S0-S3 как выходы
	DDRD |= (1 << SIG); // SIG как выход
	PORTB &= ~((1 << S0) | (1 << S1) | (1 << S2) | (1 << S3)); // Сбрасываем линии управления
	PORTD &= ~(1 << SIG); // Выключаем SIG (если он активен HIGH)
	
	RailRoadSwitchTest();
}

void RailRoadSwitchTest(){
	
	for (uint8_t i = 1; i <= 16; i++) {
		selectChannel(i);
		PORTD |= (1 << SIG);
		_delay_ms(switchPauseTime);
		PORTD &= ~(1 << SIG);
		_delay_ms(switchPauseTime);
	}
}


void selectChannel(uint8_t ch) {
	if (ch < 1 || ch > 16) return;

	uint8_t masks[17] = {
		0,
		0b00000000, // C1  Table 1 Left
		0b00010000, // C2  Table 1 Right
		0b00001000, // C3  Table 2 Left
		0b00011000, // C4  Table 2 Right
		0b00000100, // C5  Table 3 Left
		0b00010100, // C6  Table 3 Right
		0b00001100, // C7  Table 4 Left
		0b00011100, // C8  Table 4 Right
		0b00000001, // C9  Table 5 Left
		0b00010001, // C10 Table 5 Right
		0b00001001, // C11 Table 6 Left
		0b00011001, // C12 Table 6 Right
		0b00000101, // C13 Free
		0b00010101, // C14 Short circuit switch off
		0b00001101, // C15 Move Backward
		0b00011101, // C16 Move Forward
	};

	PORTB = (PORTB & 0b00000000) | masks[ch]; // Устанавливаем только нужные биты
}
