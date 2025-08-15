#include "../include/lcd.h"

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

// --- Определения ---
#define LCD_ADDRESS 0x27  // Адрес I2C LCD-дисплея
#define LCD_BACKLIGHT 0x08 // Включение подсветки

// --- Функции для работы с I2C ---
void I2C_Init(void) {
	TWSR = 0x00;             // Предделитель равен 1
	TWBR = 72;               // Устанавливаем скорость I2C = 100 кГц (16 МГц / 160)
	TWCR = (1 << TWEN);      // Включаем модуль TWI
}

void I2C_Start(void) {
	TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);  // Генерация START
	while (!(TWCR & (1 << TWINT)));                   // Ожидание завершения
}

void I2C_Stop(void) {
	TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);  // Генерация STOP
	while (TWCR & (1 << TWSTO));                      // Ждать завершения STOP
}

void I2C_Write(uint8_t data) {
	TWDR = data;                                      // Загружаем данные
	TWCR = (1 << TWEN) | (1 << TWINT);                // Начало передачи
	while (!(TWCR & (1 << TWINT)));                   // Ожидание завершения
}

// --- Функции для управления LCD ---
void LCD_Send(uint8_t data, uint8_t mode) {
	uint8_t highNibble = (data & 0xF0) | LCD_BACKLIGHT | mode;
	uint8_t lowNibble = ((data << 4) & 0xF0) | LCD_BACKLIGHT | mode;

	I2C_Start();
	I2C_Write(LCD_ADDRESS << 1); // Адрес устройства + бит записи
	I2C_Write(highNibble);       // Отправка старших 4 бит
	I2C_Write(highNibble | 0x04); // Установка EN = 1
	I2C_Write(highNibble);       // Сброс EN = 0
	I2C_Write(lowNibble);        // Отправка младших 4 бит
	I2C_Write(lowNibble | 0x04); // Установка EN = 1
	I2C_Write(lowNibble);        // Сброс EN = 0
	I2C_Stop();
}

void LCD_Command(uint8_t command) {
	LCD_Send(command, 0x00); // Передача команды (режим RS = 0)
	_delay_ms(2);            // Небольшая задержка
}

void LCD_Data(uint8_t data) {
	LCD_Send(data, 0x01);    // Передача данных (режим RS = 1)
	_delay_ms(2);            // Небольшая задержка
}

void LCD_Init(void) {
	_delay_ms(50);           // Задержка после подачи питания
	LCD_Command(0x03);       // Режим 8-битной шины
	_delay_ms(5);
	LCD_Command(0x03);       // Повтор команды
	_delay_us(150);
	LCD_Command(0x03);       // Повтор команды
	LCD_Command(0x02);       // Переход в 4-битный режим

	// Конфигурация дисплея
	LCD_Command(0x28);       // 4-битный режим, 2 строки, 5x8 точки
	LCD_Command(0x08);       // Выключить дисплей
	LCD_Command(0x01);       // Очистить дисплей
	_delay_ms(2);
	LCD_Command(0x06);       // Инкремент курсора
	LCD_Command(0x0C);       // Включить дисплей, курсор выключен
}

void LCD_Clear(void) {
	LCD_Command(0x01);       // Очистка дисплея
	_delay_ms(2);
}

void LCD_ClearLine(uint8_t row) {
	LCD_SetCursor(0, row);
	for (uint8_t i = 0; i < 16; i++) {
		LCD_Data(' ');
	}
	LCD_SetCursor(0, row);  // вернуться в начало строки
}

void LCD_SetCursor(uint8_t col, uint8_t row) {
	uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
	LCD_Command(0x80 | (col + row_offsets[row])); // Установка адреса DDRAM
}

void LCD_Print(char* str) {
	while (*str) {
		LCD_Data(*str++);
	}
}

void LCD_Blink(char* buffer) {
	static uint8_t toggle = 0; // Флаг переключения
	static uint8_t counter = 0; // Счетчик для контроля мигания
	uint8_t blinkCyclesNumber = 20; // Количество циклов между миганиями
	uint8_t length = strlen(buffer); // Длина строки
	
	counter = (counter + 1) % 255; // Предотвращение переполнения

	if (counter % blinkCyclesNumber == 0) {
		toggle = !toggle; // Меняем состояние
	}

	if (toggle) {
		// Создаем строку из пробелов такой же длины, как buffer
		char blankString[length + 1];
		memset(blankString, ' ', length);
		blankString[length] = '\0'; // Завершаем строку нулевым символом
		LCD_Print(blankString); // Печатаем пустую строку
		} else {
		LCD_Print(buffer); // Печатаем строку
	}
}

void LCD_PrintTwoLines(char* firstLineText, char* secondLineText, int blink) {
	
		LCD_SetCursor(0, 0);
		LCD_Print(firstLineText);
		LCD_SetCursor(0, 1);
		
		if (blink == 1)	{
			LCD_Blink(secondLineText);
		}
		else {
			LCD_Print(secondLineText);			
		}
}

void update_lcd(uint8_t cmd, uint8_t table_id) {
	char line1[17];
	snprintf(line1, sizeof(line1), "CMD:%03d TBL:%02d", cmd, table_id);

	// Заполняем оставшиеся символы пробелами
	uint8_t len = strlen(line1);
	for (uint8_t i = len; i < 16; i++) {
		line1[i] = ' ';
	}
	line1[16] = '\0';

	LCD_SetCursor(0, 0);
	LCD_Print(line1);
}

void print_triggered_sensor(uint8_t states) {
	char line2[17];

	for (uint8_t i = 0; i < 8; i++) {
		line2[i] = (states & (1 << (7 - i))) ? '1' : '0';
	}
	line2[8] = '\0';

	LCD_SetCursor(0, 1);  // только нижняя строка
	LCD_Print(line2);
}
