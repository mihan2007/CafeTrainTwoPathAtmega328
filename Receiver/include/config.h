#ifndef CONFIG_H_
#define CONFIG_H_

#include <avr/io.h>

#define F_CPU 16000000UL

// Подстройте под свой датчик:
#define OVERLOAD_THRESHOLD      700   // порог "перегруз"
#define OVERLOAD_HYSTERESIS     30    // гистерезис отпускания
#define OVERLOAD_HOLD_TICKS     50    // сколько тиков выше порога до срабатывания
#define OVERLOAD_RESEND_TICKS   60000     // период повтора команды в перегрузе

// Защита по времени от работы на холостом ходу 
#define TICKS_PER_UNIT 10000
#define MAX_TIME_UNITS 30

// Удержание кнопки для перехода в режим отстройки аналогового значения
#define HOLD_TICKS_BUTTON 1000 // сколько тиков ~1с при 1мс таймере

// Определение линий управления стрелками
#define REVERS_PIN PB5  // 

//PWM settings
#define PWM_INITIAL_DUTY 100
#define PWM_PATH1_PIN PB1			// ШИМ на выводе OC1A (D9) 
#define PWM_PATH1_SWITCH_PIN PB4	// Switch into PWM mode pin (D12)
#define PWM_PATH2_PIN PB2		// ШИМ путь 2 на OC1B (D10)
#define PWM_PATH2_SWITCH_PIN PB3	// Switch into PWM mode path 2 (D11)
#define PWM_PIN PWM_PATH1_PIN
#define PWM_SWITCH_PIN PWM_PATH1_SWITCH_PIN
#define RAIL_POWER_ENABLE PB0// Power supply PIN (D8)
#define PWM_MAX 255			// Максимальный уровень ШИМ (10 бит)
#define PWM_STEP 1			// Шаг увеличения ШИМ
#define PWM_DELAY 50		// Задержка между шагами (мягкость разгона)

// Маска для управления стрелками (исключая PB1 - ШИМ)
#define SWITCH_PAUSE_TIME 500 // Время ожидания после переключения стрелки

//  UART
#define BAUD 9600
#define UBRR_VALUE ((unsigned long)(F_CPU / 16 / BAUD) - 1)

#define NUM_BITS_74HC595 8  // Количество бит в байте
#define NUM_OF_74HC595 4    // Количество включенных в каскад 74HC595
#define NUM_SWITCH_REGS 3 // Количество сдвиговых регистров подключенных к стрелкам
#define POWER_SUPPLY_REGISTER_A 2
#define POWER_SUPPLY_REGISTER_B 3
#define SWITCH_REGISTER_0 0
#define SWITCH_REGISTER_1 1
#define SWITCH_REGISTER_2 2 

// Shift register PINs 74HC165
#define LATCH_74HC165 PD6   // LATCH для 74HC165
#define CLOCK_74HC165 PD5   // CLOCK для 74HC165
#define DATA_74HC165  PD7   // DATA для 74HC165

// Shift register PINs 74HC595
#define DATA_74HC595    PD2
#define CLOCK_74HC595   PD3
#define LATCH_74HC595   PD4

// Alarm
#define ALARM_ENABLED 0
#define ALARM_PIN PB3

// Определение маршрутов
#define ROUTE_TO_TABLE_1 0b00000000000000001UL
#define ROUTE_TO_TABLE_2 0b00000000000000110UL
#define ROUTE_TO_TABLE_3 0b00000000000011010UL
#define ROUTE_TO_TABLE_4 0b00000000001101010UL
#define ROUTE_TO_TABLE_5 0b00000000110000000UL
#define ROUTE_TO_TABLE_6 0b00000011010000000UL
#define ROUTE_TO_TABLE_7 0b00001101010000000UL
#define ROUTE_TO_TABLE_8 0b00110101010000000UL
#define ROUTE_TO_TABLE_9 0b11010101010000000UL
#define PATH1_ROUTE_MASK 0b00000000001111111UL
#define PATH2_ROUTE_MASK 0b11111111100000000UL

#define DC_SUPLLY_TABLE_1 0b00000100
#define DC_SUPLLY_TABLE_2 0b00001000
#define DC_SUPLLY_TABLE_3 0b00010000
#define DC_SUPLLY_TABLE_4 0b00100000
#define DC_SUPLLY_TABLE_5 0b01000000
#define DC_SUPLLY_TABLE_6 0b10000000
#define DC_SUPLLY_TABLE_7 0b00000001
#define DC_SUPLLY_TABLE_8 0b00000010
#define DC_SUPLLY_TABLE_9 0b00000100
// Uart Comands 
#define CMD_FORWARD		0x20
#define CMD_BACKWARD	0x21
#define CMD_SLOW_MODE	0x22
#define CMD_ARRIVED		0x23
#define CMD_STOP		0x30
#define CMD_IDEL_STOP	0x31
#define CMD_OVER_LOAD_STOP	0x32
#define CMD_CLEAR_EMERGENCY 0x33
#define CMD_STOP_PATH1 0x34
#define CMD_STOP_PATH2 0x35

#define PATH_MODE_STOP 0
#define PATH_MODE_ROUTE_SETUP 1
#define PATH_MODE_ACCELERATION 2
#define PATH_MODE_MOVING 3

extern uint8_t routeSetupInProgress;
extern uint8_t pathMode[3];
extern uint8_t pathSelectedTable[3];

// Sensors bytes
#define ADC_MODE_BUTTON (1 << 0)
#define SENSOR_2 (1 << 1)
#define SENSOR_3 (1 << 2)
#define SENSOR_4 (1 << 3)
#define KITCHEN_STOP_SENSOR (1 << 4)
#define KITCHEN_SLOW_SENSOR (1 << 5)
#define TABLE_SLOW_SENSOR (1 << 6)
#define TABLE_STOP_SENSOR (1 << 7)

// ==== Runtime state (externs) ====
extern volatile uint8_t emergencyStopActive;
extern uint8_t lastCmd;
extern uint8_t isLocoMoving;
extern uint16_t tickCounter;
extern uint8_t timeCounter;

extern uint8_t sensorStates;
extern uint8_t triggeredBitsHistory;
extern uint8_t SelectedTable;
extern uint8_t previousSensorStates;

#endif /* CONFIG_H_ */


