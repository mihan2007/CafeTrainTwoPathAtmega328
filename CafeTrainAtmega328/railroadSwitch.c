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
		_delay_ms(500);
		PORTD &= ~(1 << SIG);
		_delay_ms(500);
	}
}


void selectChannel(uint8_t ch) {
	if (ch < 1 || ch > 16) return;

	uint8_t masks[17] = {
		0,
		0b00000000, // 1 +
		0b00010000, // 2 +
		0b00001000, // 3 +
		0b00011000, // 4 +
		0b00000100, // 5 +
		0b00010100, // 6 +
		0b00001100, // 7 +
		0b00011100, // 8 +
		0b00000001, // 9 +
		0b00010001, // 10 +
		0b00001001, // 11 +
		0b00011001, // 12 +
		0b00000101, // 13 +
		0b00010101, // 14 +
		0b00001101, // 15 +
		0b00011101, // 16 +
	};

	PORTB = (PORTB & 0b11100010) | masks[ch]; // Устанавливаем только нужные биты
}
