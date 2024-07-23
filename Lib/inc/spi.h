#include "stm32f4xx.h"

#define EN_RST	0x66
#define RST		0x99
#define	WR_EN	0x06
#define	SECT_ER	0x20
#define	RD_SR1	0x05
#define	PG_PROG	0x02
#define	RD_DATA	0x03

#define ADDR	0x21FF00

#define SPI2_TESTBYTE	0xAA

//======= SPI2 Error codes ================
#define SPI2_OK		0
#define SPI2_ERR	1



#define CSLOW	GPIOE->BSRR |= GPIO_BSRR_BR3	// nCS active 
#define CSHIGH	GPIOE->BSRR |= GPIO_BSRR_BS3	// nCS not active


void SPI2_Init(void);
uint8_t w25send(uint8_t data);

