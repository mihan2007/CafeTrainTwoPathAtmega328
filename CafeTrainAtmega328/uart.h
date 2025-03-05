#ifndef UART_H
#define UART_H

#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
	#endif
/**
 * @brief Структура UART-протокола
 *
 * Каждое сообщение содержит фиксированные поля:
 *
 * | Поле       | Размер (байт) | Описание |
 * |------------|-------------|----------|
 * | `STX`      | 1           | Стартовый байт (0x02) |
 * | `CMD`      | 1           | Код команды |
 * | `TABLE_ID` | 1           | Номер стола (0-255) |
 * | `DATA`     | 1           | Данные (например, направление) |
 * | `CRC`      | 1           | Контрольная сумма (CRC-8) |
 * | `ETX`      | 1           | Конец сообщения (0x03) |
 *
 * @brief Описание команд
 *
 * | Код  | Команда               | Описание |
 * |------|-----------------------|----------|
 * | 0x10 | `HELLO`               | Проверка связи |
 * | 0x11 | `ACK_HELLO`           | Ответ системы управления |
 * | 0x20 | `MOVE_FORWARD`        | Движение вперед к `TABLE_ID` |
 * | 0x21 | `MOVE_BACKWARD`       | Движение назад к `TABLE_ID` |
 * | 0x30 | `EMERGENCY_STOP`      | Экстренное отключение |
 * | 0x40 | `COMMAND_ACK`         | Подтверждение получения команды |
 * | 0x50 | `COMMAND_COMPLETE`    | Отчет о завершении движения |
 */

		void UART_init(void);
		void UART_send(uint8_t data);
		uint8_t UART_receive(void);  // ? Объявление функции
		void send_command(uint8_t cmd, uint8_t table_id, uint8_t data);
		void process_command(uint8_t *data);
		uint8_t crc8(const uint8_t *data, uint8_t len);
		uint8_t UART_receive_packet(void);
		
		uint8_t UART_read_command(uint8_t *cmd, uint8_t *table_id, uint8_t *param);
	
	#ifdef __cplusplus
}
#endif

#endif
