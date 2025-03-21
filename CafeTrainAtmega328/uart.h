#ifndef UART_H
#define UART_H

#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
	#endif

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

	#ifdef __cplusplus
}
#endif

#endif
