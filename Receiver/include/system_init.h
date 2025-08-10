#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>

/**
 * @brief Инициализация всех аппаратных компонентов системы.
 * LCD, UART, таймер, сдвиговые регистры и глобальные прерывания.
 */
void system_init(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_INIT_H
