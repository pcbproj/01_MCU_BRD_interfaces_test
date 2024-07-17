#include "stm32f4xx.h"

#define BTN_CHECK_MS			10
#define	BTN_PRESS_CNT			4 


void GPIO_Init(void);

void BTN_Check(uint16_t *ms_count,	// current ms counter value
				char *BTN_state);



