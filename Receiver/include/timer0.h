#ifndef TIMER0_H
#define TIMER0_H

#ifdef __cplusplus
extern "C" {
	#endif

	#include <stdint.h>

	void timer0_init(void);

	// Если счётчик шага нужен за пределами этого модуля,
	// можно объявить его extern (или хранить в другом модуле).
	extern volatile uint16_t rail_switch_step_counter;

	#ifdef __cplusplus
}
#endif

#endif /* TIMER0_H */
