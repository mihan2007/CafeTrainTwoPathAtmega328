#ifndef LCD_H
#define LCD_H

#ifdef __cplusplus
extern "C" {
	#endif

	#include <avr/io.h>

	// --- Макросы ---
	#define LCD_ADDRESS 0x27       // Адрес I2C LCD-дисплея
	#define LCD_BACKLIGHT 0x08     // Включение подсветки
	

	// --- Прототипы функций ---
	// Функции для работы с I2C


	void I2C_Init(void);

	void I2C_Start(void);

	void I2C_Stop(void);

	void I2C_Write(uint8_t data);

	// Функции для управления LCD
	void LCD_Send(uint8_t data, uint8_t mode);

	void LCD_Command(uint8_t command);

	void LCD_Data(uint8_t data);

	void LCD_Init(void);

	void LCD_Clear(void);

	void LCD_SetCursor(uint8_t col, uint8_t row);

	void LCD_Print(char* str);
	
	void LCD_Blink(char* buffer);
	
	void LCD_PrintTwoLines(char* firstLineText, char* secondLineText, int blink);

	#ifdef __cplusplus

}
#endif

#endif // LCD_H
