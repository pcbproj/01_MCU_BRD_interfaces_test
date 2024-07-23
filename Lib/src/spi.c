#include "spi.h"



uint8_t w25send( uint8_t data ){
	SPI2->DR = data;
	//while( ( SPI2->SR & ( SPI_SR_TXE | SPI_SR_RXNE ) ) == 0 ){};		// wait for transmitter empty and receiver not empty
	while( ( SPI2->SR & SPI_SR_TXE ) == 0 ){};		// wait for transmitter empty and receiver not empty
	while( ( SPI2->SR & SPI_SR_RXNE ) == 0 ){};		// wait for receiver not empty

	return SPI2->DR;		// return received data through SPI2
	
}


void SPI2_Init(void){
	
	RCC->AHB1ENR  |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOEEN;
	GPIOC->MODER  |= GPIO_MODER_MODE3_1 | GPIO_MODER_MODE2_1;	// PC2 PC3 alternate function modes 
	GPIOC->AFR[0] |= GPIO_AFRL_AFRL3_2 | GPIO_AFRL_AFRL3_0 | GPIO_AFRL_AFRL2_2 | GPIO_AFRL_AFRL2_0; // selecter SPI2 alternate functions
	GPIOC->PUPDR  |= GPIO_PUPDR_PUPD3_1 | GPIO_PUPDR_PUPD2_1;  // turn on pull-down registers
	
	GPIOB->MODER  |= GPIO_MODER_MODE10_1 ;	// PB10 alternate function modes (SCLK)
	GPIOB->AFR[1] |= GPIO_AFRH_AFRH2_2 | GPIO_AFRH_AFRH2_0; // selecter SPI2 alternate function for PB10
	GPIOB->PUPDR  |= GPIO_PUPDR_PUPD10_1;  // turn on pull-down registers for PB10

	GPIOE->MODER |= GPIO_MODER_MODE3_0;		// PE3 = nCS output
	GPIOE->OTYPER &= ~(GPIO_OTYPER_OT3);		// output mode
	GPIOE->PUPDR |= GPIO_PUPDR_PUPD3_1; 
	GPIOE->BSRR  |= GPIO_BSRR_BS3;			// out 1 to nCS. not active 

	// SPI2 configuration
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
	SPI2->CR1 |= SPI_CR1_BR_2 | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;	// baud 1.3 MHz, Master mode, NSS application drive mode
	SPI2->CR1 &= ~(SPI_CR1_DFF);	// 8 bit frame mode
	SPI2->CR1 |= SPI_CR1_SPE;		// SPI2 enable
	

	CSLOW;
	w25send(EN_RST);		// send comand enable Reset to FLASH memory
	CSHIGH;
	
	CSLOW;
	w25send(RST);		// send comand Reset to FLASH memory
	CSHIGH;

	CSLOW;
	w25send(WR_EN);		// send comand WriteEnable to FLASH memory
	CSHIGH;
	
	CSLOW;
	w25send(SECT_ER);		// send comand Sector Erase to FLASH memory
	
	// send adddres sector to be erased
	w25send( ( ADDR >> 16 ) & 0xFF );		
	w25send( ( ADDR >> 8 ) & 0xFF );	
	w25send( ( ADDR ) & 0xFF );	
	CSHIGH;

	// check for sector erase complete
	CSLOW;
	w25send(RD_SR1);	// send command "read status register1"
	while( ( w25send(0x00) & 0x01 ) == 1 ){};	// wait while BUSY-bit in cleared
	CSHIGH;

	CSLOW;
	w25send(WR_EN);		// send comand WriteEnable to FLASH memory
	CSHIGH;
}
 