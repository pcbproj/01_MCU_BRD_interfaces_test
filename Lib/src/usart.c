#include "usart.h"




// настройка USART1 для передачи принятых данных в ПК по USART1
void USART1_Init(void){	
	
	RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOAEN;							// включение тактирования GPIOA: PA9 = TX, PA10 = RX
	RCC -> APB2ENR |= RCC_APB2ENR_USART1EN;							// включение тактирования USART1 от шины APB2

	
	GPIOA -> MODER  |= GPIO_MODER_MODE9_1;							// Альтернативная функция для PA9 (USART1 - TX)
	GPIOA -> AFR[1] |= (7 << GPIO_AFRH_AFSEL9_Pos);					// AF7 для PA9
	GPIOA -> MODER  |= GPIO_MODER_MODE10_1;                           // Альтернативная функция для PA10 (USART1 - RX)
	GPIOA -> AFR[1] |= (7 << GPIO_AFRH_AFSEL10_Pos);					// AF7 для PA10

	
	/* Расчет скорости передачи данных:
		(84МГц/115200)/16 = 45.57; 
		Целая часть = 45 = 0x2D; 
		Дробная часть = 0.57*16 = 9 = 0x09 
	*/

	USART1 -> BRR |= 0x2D9;	// 115200
	
	/* Включение приемника и передатчика */
	USART1 -> CR1 |= USART_CR1_TE | USART_CR1_RE; 
	USART1 -> CR1 &= ~(USART_CR1_M) | ~(USART_CR1_PCE);              // 8-бит, без контроля четности
	USART1 -> CR2 &= ~(USART_CR2_STOP);                              // 1 стоповый бит
	USART1 -> CR1 |= USART_CR1_UE;                                   // Включение USART1

}





void usart1_send(char data[], uint32_t len) {
	for (uint32_t i=0; i < len; i++){
		USART1 -> DR = data[i];
		while ((USART1 -> SR & USART_SR_TXE) == 0){};
	}
}

void usart6_send(char data[], uint32_t len) {
	for (uint32_t i=0; i < len; i++){
		USART6 -> DR = data[i];
		while ((USART6 -> SR & USART_SR_TXE) == 0){};
	}
}

// retarget the C library printf function to the USART 
//retarget the C library printf function to the USART 
int __SEGGER_RTL_X_file_write(__SEGGER_RTL_FILE *__stream, const char *__s, unsigned __len) {
  
  // Send string over USART1 in pending mode 
  for (; __len != 0; --__len) {
    USART1->DR = * __s++;
    while (RESET == READ_BIT(USART1->SR, USART_SR_TXE));
  } 

  return 0;
  }
