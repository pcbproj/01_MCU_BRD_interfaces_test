#include "gpio.h"




void GPIO_Init(void){
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
	
	//-------- GPIO for buttons -------------------
	GPIOE -> PUPDR |= GPIO_PUPDR_PUPD10_0;
	GPIOE -> PUPDR |= GPIO_PUPDR_PUPD11_0;
	GPIOE -> PUPDR |= GPIO_PUPDR_PUPD12_0;
	   
	//-------- GPIO settings for LED1 LED2 LED3 --------
	GPIOE -> MODER |=GPIO_MODER_MODE13_0;
	GPIOE -> MODER |=GPIO_MODER_MODE14_0;
	GPIOE -> MODER |=GPIO_MODER_MODE15_0;

}


void BTN_Check(uint16_t *ms_count,	// current ms counter value
				char *S1_state,
				char *S2_state,
				char *S3_state)
{
	static char S1_cnt, S2_cnt, S3_cnt;

	if ( *ms_count > BTN_CHECK_MS){
		*ms_count = 0;
		// Опрос кнопки S1
		if ((GPIOE->IDR & GPIO_IDR_ID10) == 0) {  // if S1 pressed
			if(S1_cnt < BTN_PRESS_CNT){  
				S1_cnt++;
				*S1_state = 0;	// считаем кнопку S1 не нажатой
			}
			else *S1_state = 1;	// считаем кнопку S1 нажатой
		}
		else{                   // if S1 released
			*S1_state = 0;	// считаем кнопку S1 не нажатой
			S1_cnt = 0;
		}
		
		// Опрос кнопки S2
		if ((GPIOE->IDR & GPIO_IDR_ID11) == 0) {  // if S2 pressed
			if(S2_cnt < BTN_PRESS_CNT){
				S2_cnt++;
				*S2_state = 0;
			}
			else *S2_state = 1;
		}
		else{                   // if S2 released
			*S2_state = 0;
			S2_cnt = 0;
		}
		
		// Опрос кнопки S3
		if ((GPIOE->IDR & GPIO_IDR_ID12) == 0) {  // if S3 pressed
			if(S3_cnt < BTN_PRESS_CNT){
				S3_cnt++;
				*S3_state = 0;
			}
			else *S3_state = 1;
		}
		else{                   // if S3 released
			*S3_state = 0;
			S3_cnt = 0;
		}

	}
 }