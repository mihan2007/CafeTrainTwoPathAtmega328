#define F_CPU 16000000UL
#include <util/delay.h>

#include <avr/io.h>
#include <stdio.h>
#include "lcd.h"
#include "railroadSwitch.h"
#include "config.h"
#include "routes.h"
#include "PWM.h"

char firstLineText[16]; // Text for the first line
char secondLineText[16]; // Text for the second line

int main(void)
{
	// Module initialization
	I2C_Init();
	LCD_Init();	
	LCD_Clear();
	
	initRailRoadSwitch();
	
	initPWM();
	
    char firstLineText[20];
    char secondLineText[20];

	
    while (1) {
        /*snprintf(firstLineText, sizeof(firstLineText), "Table" );
        snprintf(secondLineText, sizeof(secondLineText),"Right");

        LCD_PrintTwoLines(firstLineText, secondLineText, 0);
        _delay_ms(300);*/
		
		moveLocomotive(1);
		LocomotiveSpeedUp();
		_delay_ms(1000);
		stopLocomotive();
		moveLocomotive(0);
		_delay_ms(1000);
				
        }
	
    return 0;
}





