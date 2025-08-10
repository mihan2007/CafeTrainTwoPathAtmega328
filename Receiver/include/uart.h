#ifndef UART_H
#define UART_H

#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
	#endif
	
	#define PACKET_SIZE 6
	#define STX 0x02
	#define ETX 0x03
	#define ACK_CMD 0x40   // Код подтверждения
	#define TIMEOUT_MS 100 // Таймаут ожидания в миллисекундах
	#define MAX_RETRIES 3  // Максимальное число попыток повторной передачи

	// Структура для представления полученного пакета
	typedef struct {
		uint8_t cmd;
		uint8_t table_id;
		uint8_t param;
		uint8_t valid; // 1 – пакет корректен, 0 – ошибка
	} UART_Packet;

	#define STX 0x02
	#define ETX 0x03

	// Дополнительные коды команд
	#define ACK  0x40    // Подтверждение получения
	#define NACK 0x41    // Отрицательное подтверждение
	
	void UART_init(void);
	void UART_send(uint8_t data);
	uint8_t UART_receive(void);
	void send_command(uint8_t cmd, uint8_t table_id, uint8_t data);
	uint8_t crc8(const uint8_t *data, uint8_t len);

	// Единая функция приёма пакета
	UART_Packet UART_receive_full_packet(void);
	
	// Функции для отправки ACK/NACK
	void send_ack(uint8_t cmd);
	void send_nack(uint8_t cmd);
	
	uint8_t send_command_with_ack(uint8_t cmd, uint8_t table_id, uint8_t data);

	#ifdef __cplusplus
}
#endif

#endif
