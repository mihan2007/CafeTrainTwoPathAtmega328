#ifndef RAILROAD_CONTROL_H
#define RAILROAD_CONTROL_H

#ifdef __cplusplus
extern "C" {
	#endif

	#include <stdint.h>

	// ќбъ€вление функций управлени€
	void calculate_mask(uint8_t table_id, uint8_t *maskBuffer, uint8_t bufferSize);
	void move_forward(uint8_t table_id);
	void updat_switches();
	void animate_switches(uint8_t *switchMask, uint8_t numSwitchRegs);	// «десь можно добавить дополнительные объ€влени€ функций

	#ifdef __cplusplus
}
#endif

#endif // RAILROAD_CONTROL_H
