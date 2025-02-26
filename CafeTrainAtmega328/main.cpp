#define F_CPU 16000000UL

#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>

#include "lcd.h"
#include "railroadSwitch.h"
#include "config.h"
#include "routes.h"
#include "PWM.h"

#define BAUD 9600
#define UBRR_VALUE ((unsigned long)(F_CPU / 16 / BAUD) - 1)

void UART_init(void) {
	UBRR0H = (UBRR_VALUE >> 8);
	UBRR0L = UBRR_VALUE;
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_send(char data) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

char UART_receive(void) {
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

char firstLineText[16]; // Глобальные строки
char secondLineText[16];

int main(void)
{
	// Инициализация модулей
	I2C_Init();
	LCD_Init();
	LCD_Clear();
	
	UART_init();
	initRailRoadSwitch();
	//initPWM();
	
	while (1) {
		char received;
		
		if (UCSR0A & (1 << RXC0)) {  // Проверяем, есть ли данные
			received = UART_receive();
			} else {
			received = '-';  // Если данных нет, записываем прочерк
		}

		
		snprintf(firstLineText, sizeof(firstLineText), "Control Panel" );
		snprintf(secondLineText, sizeof(secondLineText), "%c", received);

		LCD_PrintTwoLines(firstLineText, secondLineText, 0);
		_delay_ms(300);
		
		/*LocomotiveSpeedUp();
		moveLocomotive(1);
		_delay_ms(5000);
		stopLocomotive();
		_delay_ms(5000);*/
	}
	
	return 0;
}
