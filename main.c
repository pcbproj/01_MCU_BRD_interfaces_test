/*
Тест интерфейсов платы и МК:

GPIO: кнопки и светодиоды
I2C1

USART1 - для RS232 - ПК связь. 
	baudrate = 115200, 
	data bits = 8 
	stop bit = 1
	parity = NO

USART6 - для RS485 подключения
	baudrate = 115200, 
	data bits = 8 
	stop bit = 1
	parity = NO

SPI:
	bit rate = 


CAN2:
	bit rate = 500 kBit/s;
	FRAME_ID = 0x234;
	data_len = 1;


Алгоритм проверки следующий:
	++ На ветодиоды выводятся нажатые кнопки.

	В USART1 отправляется лог состояния проверок каждые 1000 мс
		Лог содержит:
		1. Нажатие кнопок
		2. Принятый пакет по CAN2 поле данных
		3. проверка I2C пройден или провал. (Таймауты чтобы не повиснуть)
			запись тестовых данных и чтение их. 
			Сравнение с оригиналом.
		4. Проверка SPI пройден или провал. (Таймауты чтобы не повиснуть)
			запись тестовых данных и чтение их. 
			Сравнение с оригиналом.

	В CAN2 каждые 1000 мс отправлется пакет с кодом нажатых кнопок
	В USART6 каждые 1000 мс отправлять тестовые данные

*/


#include <stdio.h>
#include <stdlib.h>
#include "stm32f4xx.h"
#include "main.h"


#define COUNTER_1000_MS		1000	// 1000 ms for 1 second 

#define I2C_ARRAYS_LEN		8

const char Hello_str[25]		= "+++ System started! +++ \n"; 
const char Buttons_str[19]		= "+++ Buttons Code = "; 
const char I2C_Error_str[26]	= "--- I2C1 Test FAILED --- \n"; 
const char I2C_Ok_str[39]		= "+++ I2C1 Test PASSED SECCESSFULLY +++ \n"; 


char i2c_tx_array[I2C_ARRAYS_LEN] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };	// Массив записываетмый в EEPROM
char i2c_rx_array[I2C_ARRAYS_LEN] = {};	// Массив, куда будут читаться данные из EEPROM


uint16_t btn_count = 0;	// счетчик мс для опроса кнопок
uint16_t sec_count = 0;	// счетчик секунды для оотправки лога и сообщений
uint16_t delay1_cnt = 0; // счетчик мс для задержек
uint16_t delay2_cnt = 0; // счетчик мс для задержек


void RCC_Init(void);



void Delay_ms(uint16_t ms){
	delay1_cnt = 0;
	while(delay1_cnt < ms){}; 
}





void SysTick_Handler(void){		// прервание от Systick таймера, выполняющееся с периодом 1000 мкс
	btn_count++;
	sec_count++;
	delay1_cnt++;
	delay2_cnt++;
}


int main(void) {
	enum I2C_ERR I2C_ErrCode = I2C_OK;

	const char eeprom_addr = EEPROM_RD_START_ADDR;	// адрес чтения и записи в EEPROM, передаем по I2C

	char btn_state_byte = 0;   
	/* в байте состояний кнопок btn_state_byte выставляются в 1 и сбрасываются в 0 соответствующие биты, при нажатии кнопок
	бит 0 - кнопка S1. Если кнопка нажата, то бит равен 1. Если кнопка отпущена, то бит равен 0.
	бит 1 - кнопка S2. Если кнопка нажата, то бит равен 1. Если кнопка отпущена, то бит равен 0.
	бит 2 - кнопка S3. Если кнопка нажата, то бит равен 1. Если кнопка отпущена, то бит равен 0.
	*/

	char I2C_ErrorCode = 0;
	char test_start = 0;
  	
  	
  	
  	RCC_Init();
  	
  	GPIO_Init();
  	
  	I2C_Init();
  	
	USART1_Init();
  	
  	SysTick_Config(84000);		// настройка SysTick таймера на время отрабатывания = 1 мс
								// 84000 = (AHB_freq / время_отрабатывания_таймера_в_мкс)
								// 84000 = 84_000_000 Гц / 1000 мкс; 
   
	//---- turn off leds ---------- 
	GPIOE -> BSRR |= GPIO_BSRR_BS13;
	GPIOE -> BSRR |= GPIO_BSRR_BS14;
	GPIOE -> BSRR |= GPIO_BSRR_BS15;

	//EEPROM_PageClear(EEPROM_RD_START_ADDR);	// предварительная очистка страницы
											// чтобы можно было видеть записанные новые данные в массиве
  
	//usart1_send(Hello_str, sizeof(Hello_str));
	printf(">>> System started! \n");
	printf(">>> Pressed buttons indicated on LEDS \n");
	printf(">>> For start testing, send byte 0x31 in HEX or character '1' \n");

	while (1){
		
		//============= Buttons testing ================
		BTN_Check(&btn_count, &btn_state_byte);		// проверка нажатия кнопок
		
		GPIOE->ODR = (~(btn_state_byte << 13));		// выдаем состояние кнопок на LED1-LED3


		if(USART1 -> SR & USART_SR_RXNE){		// IF USART1 received '1' 
			if(USART1->DR == 0x31){
				test_start = 1;
			}
			else{
				test_start = 0;
			}
		}
		else{
			test_start = 0;
		}

		if{test_start){

			//============= I2C1 testing ==================
			I2C_ErrCode = I2C_Write(eeprom_addr, i2c_tx_array, EEPROM_WR_LEN);	
			Delay_ms(5);
			
			if(I2C_ErrCode == I2C_OK){
				I2C_ErrCode = I2C_Read(eeprom_addr, i2c_rx_array, EEPROM_RD_LEN);
			}
			
			if(I2C_ErrCode == I2C_OK){
				for(uint16_t i = 0; i < EEPROM_RD_LEN; i++){
					if(i2c_tx_array[i] != i2c_rx_array[i]) I2C_ErrCode = I2C_ERR_DATA;
				}
			}
			
			
			
			//========= SPI testing ========================
			
			
			//========= CAN2 testing ========================
			
			
			
			//========= USART1 LOG Sending =================
			printf(">>> Testing FINISHED. Writing test LOG: \n");
			
			switch(I2C_ErrCode){
				//case I2C_OK: 
				//	printf("+++ I2C1 Test PASSED SECCESSFULLY +++ \n");
				//break;
			
				case I2C_BUS_BUSY:
					printf("--- I2C1 Test FAILED = I2C BUS BUSY --- \n");
				break;
			
				case I2C_DEV_ADDR_ERR:
					printf("--- I2C1 Test FAILED = I2C ERROR DEVICE ADDRESS --- \n");
				break;
			
				case I2C_WR_ERR:
					printf("--- I2C1 Test FAILED = I2C WRITE ERROR --- \n");
				break;
			
				case I2C_RD_ERR:
					printf("--- I2C1 Test FAILED = I2C READ ERROR --- \n");
				break;
				
				case I2C_ERR_DATA:
					printf("--- I2C1 Test FAILED = I2C RX TX DATA COMPARE ERROR --- \n");
				break;
				
				default:
					printf("+++ I2C1 Test PASSED +++ \n");
			}	// switch(I2C_ErrCode)
			
			// ======== LOG USART1 send ==========
			
			//======== LOG USART6 send ============
			
			//======== LOG SPI send ===============
			
			
			//========= LOG CAN2 send ===========
		


		}	// if (test_start)
	}	// while(1)
	  
}	// main()






/*************************** End of file ****************************/
