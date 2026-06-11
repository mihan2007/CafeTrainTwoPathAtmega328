#include "uart.h"
#include "config.h"

#include <avr/io.h>
#include <stdint.h>
#include <stddef.h>

#define TIMEOUT_COUNT (TIMEOUT_MS * (F_CPU / 1000 / 64))

static uint8_t pending_packet[PACKET_SIZE];
static uint8_t pending_packet_valid = 0;

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

// Функция приёма с таймаутом: если данные не поступают, возвращает 0xFF
uint8_t UART_receive(void) {
    uint16_t timeout = TIMEOUT_COUNT;
    while (!(UCSR0A & (1 << RXC0))) {
        if (--timeout == 0) {
            return 0xFF;
        }
    }
    return UDR0;
}

// Отправка пакета по протоколу: STX, CMD, TABLE_ID, DATA, CRC, ETX
void send_command(uint8_t cmd, uint8_t table_id, uint8_t data) {
    uint8_t packet[PACKET_SIZE];
    packet[0] = STX;
    packet[1] = cmd;
    packet[2] = table_id;
    packet[3] = data;
    packet[4] = crc8(&packet[1], 3);
    packet[5] = ETX;
    
    for (uint8_t i = 0; i < PACKET_SIZE; i++) {
        UART_send(packet[i]);
    }
}

// Вычисление CRC-8 для 3 байт (CMD, TABLE_ID, DATA)
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

/*
 * Функция UART_receive_packet() ожидает поступления полного пакета (6 байт)
 * и проверяет:
 *  - первый байт должен быть STX,
 *  - последний байт должен быть ETX,
 *  - CRC (пятый байт) должен совпадать с вычисленным по CMD, TABLE_ID и DATA.
 * Если пакет корректен, функция возвращает 1, иначе – 0.
 */
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
    // Проверка ETX
    if (packet_buffer[5] != ETX) {
        return 0;
    }
    // Проверка CRC
    if (crc8(&packet_buffer[1], 3) != packet_buffer[4]) {
        return 0;
    }
    return 1;
}

/*
 * Функция send_command_with_ack() отправляет команду и ожидает получения ACK.
 * ACK считается полученным, если полученный пакет имеет:
 *  - CMD равный ACK_CMD (0x40),
 *  - поле TABLE_ID равное исходной команде, для которой подтверждение.
 * Функция повторяет отправку до MAX_RETRIES раз и возвращает 1, если ACK получен,
 * иначе – 0.
 */
static void save_pending_packet(uint8_t *packet_buffer) {
    for (uint8_t i = 0; i < PACKET_SIZE; i++) {
        pending_packet[i] = packet_buffer[i];
    }
    pending_packet_valid = 1;
}

uint8_t UART_get_packet(uint8_t *packet_buffer) {
    if (pending_packet_valid) {
        for (uint8_t i = 0; i < PACKET_SIZE; i++) {
            packet_buffer[i] = pending_packet[i];
        }
        pending_packet_valid = 0;
        return 1;
    }

    return UART_receive_packet(packet_buffer);
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
            save_pending_packet(response);
        }
        retries++;
    }
    
    return ack_received;
}

// Пример функции обработки входящего пакета (на стороне передатчика, если требуется)
void process_command(uint8_t *data) {
    if (data == NULL) {
        return;
    }

    uint8_t cmd = data[1];
    uint8_t table_id = data[2];
    uint8_t param = data[3];
    uint8_t received_crc = data[4];

    if (crc8(&data[1], 3) != received_crc) {
        return;
    }

    switch (cmd) {
        case 0x20: // MOVE_FORWARD
            setRouteByIndex(table_id);
            moveLocomotive(1);
            // Отправляем ACK для команды MOVE_FORWARD
            send_command(ACK_CMD, 0x20, param);
            break;

        case 0x21: // MOVE_BACKWARD
            moveLocomotive(0);
            send_command(ACK_CMD, 0x21, param);
            break;

        case 0x30: // EMERGENCY_STOP
            stopLocomotive();
            send_command(ACK_CMD, 0x30, param);
            return;
    }

    // Отчет о завершении команды (COMMAND_COMPLETE)
    send_command(0x50, cmd, table_id);
}
