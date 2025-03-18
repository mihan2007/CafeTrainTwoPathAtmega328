#include "uart.h"
#include "config.h"
#include "railroad_switch.h"
#include <avr/io.h>
#include <stdint.h>
#include <stddef.h>
//#include <util/delay.h>

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
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

void send_command(uint8_t cmd, uint8_t table_id, uint8_t data) {
	uint8_t packet[6];
	packet[0] = 0x02;   // STX (начало пакета)
	packet[1] = cmd;    // Код команды
	packet[2] = table_id; // Номер стола
	packet[3] = data;   // Данные (направление, скорость и т. д.)
	packet[4] = crc8(&packet[1], 3);  // Контрольная сумма
	packet[5] = 0x03;   // ETX (конец пакета)

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

void process_command(uint8_t *data) {
	if (data == NULL) {
		return; // Проверка на пустой указатель
	}

	uint8_t cmd = data[1];
	uint8_t table_id = data[2];
	uint8_t param = data[3];
	uint8_t received_crc = data[4];

	// Проверяем контрольную сумму
	if (crc8(&data[1], 3) != received_crc) {
		return; // Ошибка CRC, игнорируем пакет
	}

	switch (cmd) {
		
	case 0x30: // EMERGENCY_STOP
		stopLocomotive();
		//send_command(0x40, 0x30, 0x00); // Подтверждение экстренной остановки
		return; // Завершаем выполнение без отправки COMMAND_COMPLETE

	case 0x20: // MOVE_FORWARD
		routSetup(table_id-1);
		//moveLocomotive(1);
		break;

		case 0x21: // MOVE_BACKWARD
		moveLocomotive(0);
		//send_command(0x40, 0x21, table_id); // Подтверждение с номером стола
		break;
	}

	// Отчет о завершении команды
	//send_command(0x50, cmd, table_id);
}

uint8_t UART_receive_packet(void) {
	uint8_t received_packet[6];
	uint8_t cmd_code = 0xFF; // Код команды по умолчанию

	// Ожидаем STX (0x02)
	if (UART_receive() == 0x02) {
		received_packet[0] = 0x02; // Сохраняем STX

		// Считываем оставшиеся 5 байт
		for (uint8_t i = 1; i < 6; i++) {
			received_packet[i] = UART_receive();
		}

		// Проверяем корректность пакета (CRC, структура)
		if (crc8(&received_packet[1], 3) == received_packet[4]) {
			// Обрабатываем команду
			process_command(received_packet);
			// Сохраняем код команды
			cmd_code = received_packet[1];
		}
	}

	return cmd_code;
}

uint8_t UART_read_command(uint8_t *cmd, uint8_t *table_id, uint8_t *param) {
	uint8_t received_packet[6];

	// Ожидаем STX (0x02)
	if (UART_receive() != 0x02) {
		return 0; // Нет корректного STX
	}

	received_packet[0] = 0x02; // Сохраняем STX

	// Считываем оставшиеся 5 байт
	for (uint8_t i = 1; i < 6; i++) {
		received_packet[i] = UART_receive();
	}

	// Проверяем контрольную сумму
	if (crc8(&received_packet[1], 3) == received_packet[4]) {
		// Если CRC верен — возвращаем значения «наверх»
		*cmd       = received_packet[1];
		*table_id  = received_packet[2];
		*param     = received_packet[3];
		return 1; // Успешно
		} else {
		return 0; // Ошибка CRC
	}
}
