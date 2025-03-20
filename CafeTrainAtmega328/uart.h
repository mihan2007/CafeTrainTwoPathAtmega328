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

	void UART_init(void);
	void UART_send(uint8_t data);
	uint8_t UART_receive(void);
	void send_command(uint8_t cmd, uint8_t table_id, uint8_t data);
	uint8_t crc8(const uint8_t *data, uint8_t len);

	// Единая функция приёма пакета
	UART_Packet UART_receive_full_packet(void);

	#ifdef __cplusplus
}
#endif

#endif
