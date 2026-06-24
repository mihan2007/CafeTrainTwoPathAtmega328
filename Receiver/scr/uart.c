#include "../include/uart.h"
#include "../include/config.h"
#include <avr/io.h>
#include <stdint.h>
#include <stddef.h>

#define TIMEOUT_MS 100  // ??????? ???????? ? ?????????????




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
			return 0xFF; // ??????: ?????? ?? ??????
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

// ?????? ??????? ????? ??????: ?????????, ????????? ? ?????????? ?????????
UART_Packet UART_receive_full_packet(void) {
	UART_Packet packet = {0, 0, 0, 0};
	uint8_t received[6];
	uint8_t byte;

	do {
		byte = UART_receive();
		if (byte == 0xFF) {
			return packet;
		}
	} while (byte != STX);

	received[0] = STX;

	for (uint8_t i = 1; i < 6; i++) {
		received[i] = UART_receive();
	}

	if (received[5] != ETX) {
		return packet;
	}

	if (crc8(&received[1], 3) != received[4]) {
		return packet;
	}

	packet.cmd = received[1];
	packet.table_id = received[2];
	packet.param = received[3];
	packet.valid = 1;

	return packet;
}
void send_ack(uint8_t cmd, uint8_t seq) {
	send_command(ACK, cmd, seq);
}

void send_nack(uint8_t cmd, uint8_t seq) {
	send_command(NACK, cmd, seq);
}

uint8_t UART_receive_packet(uint8_t *packet_buffer) {
	uint8_t byte;

	do {
		byte = UART_receive();
		if (byte == 0xFF) {
			return 0;
		}
	} while (byte != STX);

	packet_buffer[0] = byte;
	for (uint8_t i = 1; i < PACKET_SIZE; i++) {
		byte = UART_receive();
		packet_buffer[i] = byte;
	}
	// ???????? ETX
	if (packet_buffer[5] != ETX) {
		return 0;
	}
	// ???????? CRC
	if (crc8(&packet_buffer[1], 3) != packet_buffer[4]) {
		return 0;
	}
	return 1;
}

uint8_t send_command_with_ack(uint8_t cmd, uint8_t table_id, uint8_t data) {
	static uint8_t next_seq = 1;
	uint8_t retries = 0;
	uint8_t ack_received = 0;
	uint8_t response[PACKET_SIZE];
	uint8_t seq = next_seq++;

	if (next_seq == 0) {
		next_seq = 1;
	}

	(void)data;

	while (retries < MAX_RETRIES && !ack_received) {
		send_command(cmd, table_id, seq);

		if (UART_receive_packet(response)) {
			if (response[1] == ACK_CMD && response[2] == cmd && response[3] == seq) {
				ack_received = 1;
				break;
			}
		}
		retries++;
	}

	return ack_received;
}
