#include "uart.h"
#include "config.h"
#include "railroad_switch.h"
#include <avr/io.h>
#include <stdint.h>
#include <stddef.h>

#define TIMEOUT_MS 100  // Таймаут ожидания в миллисекундах

void UART_init(void) {
	UBRR0H = (UBRR_VALUE >> 8);
	UBRR0L = UBRR_VALUE;
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_send(uint8_t data) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

uint8_t UART_receive(void) {
	uint16_t timeout = TIMEOUT_MS * (F_CPU / 1000 / 64);
	while (!(UCSR0A & (1 << RXC0))) {
		if (--timeout == 0) {
			return 0xFF; // Ошибка: данные не пришли
		}
	}
	return UDR0;
}

void send_command(uint8_t cmd, uint8_t table_id, uint8_t data) {
	uint8_t packet[6];
	packet[0] = 0x02;   // STX
	packet[1] = cmd;
	packet[2] = table_id;
	packet[3] = data;
	packet[4] = crc8(&packet[1], 3);
	packet[5] = 0x03;   // ETX

	for (uint8_t i = 0; i < 6; i++) {
		UART_send(packet[i]);
	}
}

uint8_t crc8(const uint8_t *data, uint8_t len) {
	uint8_t crc = 0xFF;
	for (uint8_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (uint8_t j = 0; j < 8; j++) {
			if (crc & 0x80)
			crc = (crc << 1) ^ 0x07;
			else
			crc <<= 1;
		}
	}
	return crc;
}

// Единая функция приёма пакета: считывает, проверяет и возвращает структуру
UART_Packet UART_receive_full_packet(void) {
	UART_Packet packet = {0, 0, 0, 0};
	uint8_t received[6];

	// Ожидаем STX (0x02)
	if (UART_receive() != 0x02) {
		return packet; // Пакет невалиден
	}
	received[0] = 0x02;

	// Считываем оставшиеся 5 байт
	for (uint8_t i = 1; i < 6; i++) {
		received[i] = UART_receive();
	}

	// Проверяем контрольную сумму
	if (crc8(&received[1], 3) != received[4]) {
		return packet; // Ошибка CRC – пакет невалиден
	}

	// Заполняем структуру, если пакет корректен
	packet.cmd = received[1];
	packet.table_id = received[2];
	packet.param = received[3];
	packet.valid = 1;

	return packet;
}
