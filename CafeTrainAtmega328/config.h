
#ifndef CONFIG_H_
#define CONFIG_H_

#include <avr/io.h>

// Определение линий управления стрелками
#define S0 PB4  // connected to к D12 (PB4)
#define S1 PB3  // connected to D11 (PB3)
#define S2 PB2  // connected to к D10 (PB2)
#define S3 PB0  // connected to D8 (PB0)
#define SIG PD7 // Sinal (D7 - PD7)

#define PWMpin PB1 // Подключено к MOSFET-транзистору (ШИМ)

#define SWITCH_PAUSE_TIME 3000 // Время ожидания после переключения стрелки

// Маска для управления стрелками (исключая PB1 - ШИМ)
#define SWITCH_MASK ((1 << S0) | (1 << S1) | (1 << S2) | (1 << S3))


#endif /* CONFIG_H_ */