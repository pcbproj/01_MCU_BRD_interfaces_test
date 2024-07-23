#include "stm32f4xx.h"


#define USART6_TESTBYTE (char)0xA5;

//======= USART6 Error codes ================
#define USART6_OK		0
#define USART6_ERR		1





void USART1_Init(void);

void USART6_Init(void);

void usart1_send(char data[], uint32_t len);
void usart6_send(char data[], uint32_t len);
