#ifndef ADCREAD_H_
#define ADCREAD_H_

#ifdef __cplusplus
extern "C" {
	#endif

	#include <stdint.h>  // Äëÿ uint8_t è uint16_t

	void ADC_Init(void);
	uint16_t ADC_Read(uint8_t channel);

	#ifdef __cplusplus
}
#endif

#endif /* ADCREAD_H_ */
