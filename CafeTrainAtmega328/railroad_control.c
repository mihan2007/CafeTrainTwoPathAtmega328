#include "railroad_control.h"
#include "shift_registers.h"   // Для update_shift_registers
#include "config.h"
#include "timer0.h"
#include <string.h>
#include <util/delay.h>        // Для _delay_ms(), если используется



static void animate_switch(uint8_t reg, uint8_t bit) {
    uint8_t tempData[NUM_OF_74HC595] = {0};
    tempData[reg + 1] = (1 << bit);
    update_shift_registers(tempData, NUM_OF_74HC595);
}


void calculate_mask(uint8_t table_id, uint8_t *maskBuffer) {
	memset(maskBuffer, 0, NUM_SWITCH_REGS);

	uint32_t enableBit = (table_id - 1) * 2;

	// Устанавливаем биты на нечётных позициях до enableBit
	for (uint32_t i = 1; i < enableBit; i += 2) {
		uint32_t byteIndex = i / 8;
		if (byteIndex >= NUM_SWITCH_REGS) break;

		maskBuffer[byteIndex] |= (1 << (i % 8));
	}

	// Устанавливаем бит в позиции enableBit
	if (enableBit < NUM_SWITCH_REGS * 8) {
		uint32_t byteIndex = enableBit / 8;
		maskBuffer[byteIndex] |= (1 << (enableBit % 8));
	}
}



void animate_switches(uint8_t *switchMask, uint8_t numSwitchRegs) {
	for (uint8_t reg = 0; reg < numSwitchRegs; reg++) 

		for (uint8_t bit = 0; bit < NUM_BITS_74HC595; bit++) {
			if (switchMask[reg] & (1 << bit)) {
				animate_switch(reg, bit);
				_delay_ms(5000);  // 0.5 секунды – бит активен.
			}
		}
	}

/**
 * Переводит локомотив в состояние "вперёд" с анимацией переключения стрелок.
 */
void move_forward(uint8_t table_id) {

    uint8_t switchMask[NUM_SWITCH_REGS];
    
    // Вычисляем маску для переключателей
    calculate_mask(table_id, switchMask);
    
    animate_switches(switchMask, NUM_SWITCH_REGS);
    
    uint8_t finalData[NUM_OF_74HC595] = {0};
    finalData[0] = LOCO_FORWARD;
    update_shift_registers(finalData, NUM_OF_74HC595);
}

void updat_switches(){
	if (rail_switch_step_counter <= 500) return;
	rail_switch_step_counter = 0;
}