#define F_CPU 16000000UL

#ifndef CONFIG_H_
#define CONFIG_H_

#include <avr/io.h>

#define LOCO_STOP        0b00000000  // Остановка локомотива
#define LOCO_FORWARD     0b00000001  // Движение вперёд
#define LOCO_BACKWARD    0b00000010  // Движение назад

// Определение линий управления стрелками
#define S0 PB4  // connected to к D12 (PB4)
#define S1 PB3  // connected to D11 (PB3)
#define S2 PB2  // connected to к D10 (PB2)
#define S3 PB0  // connected to D8 (PB0)
#define SIG PD7 // Sinal (D7 - PD7)

#define PWM_PIN PB1   // ШИМ на выводе OC1A (PB1)
#define PWM_SWITCH_PIN PD6 // Switch into PWM mode pin
#define PWM_MAX 1023  // Максимальный уровень ШИМ (10 бит)
#define PWM_MIN 50    // Минимальный уровень ШИМ (начало разгона)
#define PWM_STEP 10   // Шаг увеличения ШИМ
#define PWM_DELAY 1000  // Задержка между шагами (мягкость разгона)

#define SWITCH_PAUSE_TIME 100 // Время ожидания после переключения стрелки

// Маска для управления стрелками (исключая PB1 - ШИМ)
#define SWITCH_MASK ((1 << S0) | (1 << S1) | (1 << S2) | (1 << S3))

//  UART

#define BAUD 9600
#define UBRR_VALUE ((unsigned long)(F_CPU / 16 / BAUD) - 1)

#define NUM_BITS_74HC595 8  // Количество бит в байте
#define NUM_OF_74HC595 3    // Количество включенных в каскад 74HC595
#define NUM_SWITCH_REGS (NUM_OF_74HC595 - 1) // Колличесиво сдвиговых регистров подключенных к стрелкам 

	// Определение пинов для 74HC165
#define LATCH_74HC165 PD6   // LATCH для 74HC165
#define CLOCK_74HC165 PD5   // CLOCK для 74HC165
#define DATA_74HC165  PD7   // DATA для 74HC165

	// Определение пинов для 74HC595
#define DATA_74HC595    PD2
#define CLOCK_74HC595   PD3
#define LATCH_74HC595   PD4

#endif /* CONFIG_H_ */