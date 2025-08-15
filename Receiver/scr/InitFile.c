#include "../include/system_init.h"

#include "../include/lcd.h"
#include "../include/uart.h"
#include "../include/timer0.h"
#include "../include/shift_registers.h"
#include "../include/config.h"
#include "../include/adcRead.h"

#include <avr/interrupt.h>

	uint8_t routeSetupInProgress = 0;
	volatile uint8_t emergencyStopActive = 0;
	uint8_t lastCmd = 0xFF;
	uint8_t isLocoMoving = 0;
	uint16_t tickCounter = 0;
	uint8_t timeCounter = 0;
	uint8_t sensorStates = 0;
	uint8_t triggeredBitsHistory = 0;
	uint8_t SelectedTable = 0;
	uint8_t previousSensorStates = 0xFF;
	
void system_init(void) {
	I2C_Init();
	LCD_Init();
	LCD_Clear();

	UART_init();
	timer0_init();

	init_74HC165_ports();
	init_74HC595_ports();
	
	ADC_Init();
	
	initPWM();
	disablePWM(); // избавляемся от паразитного свечения
	

	
	DDRB |= (1 << REVERS_PIN);
	DDRB |= (1 << RAIL_POWER_ENABLE);
	DDRB |= (1 << PWM_SWITCH_PIN);
	
	// Очистка регистров при старте
	uint8_t initData[NUM_OF_74HC595] = {0};
	shiftOutMultiple(initData, NUM_OF_74HC595);
	
	PowerSupplyOff();

	sei(); // Включаем глобальные прерывания

	LCD_PrintTwoLines("Waiting for Cmd", "", 0);
}
