/*
Тест интерфейсов платы и МК:

GPIO: кнопки и светодиоды

I2C1: микросхема EEPROM

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

SPI2:
	bit rate = 1.3 MHz

CAN2:
	bit rate = 500 kBit/s;
	TX_FRAME_ID = 0x567;
	data_len = 1;

	
Алгоритм проверки следующий:
	0. Подключение:
		- Закоротить контакты С7 и С6 на разъеме Р5
		- Подключить переходник RS232-USB
		- подключить переходник CAN-USB
		- Подключить питание через micro-USB разъем

	1. На светодиоды выводятся нажатые кнопки.
	
	2. В CAN2 отвечаем тем же пакетом данных на принятый фрейм:
		RX_FRAME_ID = 0x567 - принимает фремы только с таким ID
		TX_FRAME_ID = 0x567 - отвечает фреймами только с таким ID
	
	3. При подаче питания или сброве в USART1 выдается приглашение:
		>>> System started! 
		>>> Pressed buttons indicated on LEDS 
		>>> For start testing, send byte 0x31 in HEX or character '1' 
		при этом нажатия кнопок индицируются на светодиодах.
		
		Для проверки CAN требуется отправить DATA_FRAME с ID = 0x567 на плату на CAN2 через приложение CANgaroo.
		И в приложении CANgaroo наблюдать ответный фрейм от платы.
		
		Для запуска тестов остальных интерфейсов по USART1 отправить символ '1' или ASCII-код = 0x31.

	4. В USART6 один раз отправляется тестовый байт данных и сравнивается 
		отправленный байт и принятый байт. Если сходятся, то USART6 ОК.

	5. В USART1 отправляется лог состояния проверок каждые 1000 мс
		Лог содержит:
		1. проверка I2C пройден или провал. (Таймауты чтобы не повиснуть)
			запись тестовых данных и чтение их. 
			Сравнение с оригиналом.
		
		2. проверка работы USART6 по закольцованному интерфейсу: PC6 <-> PC7 перемычкой закорочены 

		3. Проверка SPI пройден или провал. (Таймауты чтобы не повиснуть)
			запись тестовых данных и чтение их. 
			Сравнение с оригиналом.

		4. Приходил ли пакет по CAN2. 
	
	


*/


#include <stdio.h>
#include <stdlib.h>
#include "stm32f4xx.h"
#include "main.h"


#define COUNTER_1000_MS		1000	// 1000 ms for 1 second 

#define I2C_ARRAYS_LEN		8


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
	char usart6_ErrorCode = USART6_OK;
	char test_start = 0;
	char SPI2_ErrCode = SPI2_OK;
	char SPI2_rd_byte;
  	
	uint16_t CAN2_RxFrameID, CAN2_RxDataLen;
	uint16_t CAN2_TxFrameID = TX_FRAME_ID, CAN2_TxDataLen;
	
	char CAN2_RxState = 0;
	char CAN2_TxState = 0;
	char CAN2_RxFlag = 0;
	char const usart6_test_byte = 0x5A;

	char CAN2_TxData[CAN_TX_DATA_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	char CAN2_RxData[CAN_RX_DATA_LEN] = {};

	//char usart6_tx_array[1] = {0xA5};
  	
  	
  	RCC_Init();
  	
  	GPIO_Init();
  	
  	I2C_Init();
  	
	USART1_Init();

	USART6_Init();

	SPI2_Init();

	CAN2_Init();
  	
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
		
	

		//========= CAN2 testing ========================
		CAN2_RxState = CAN2_ReceiveMSG(&CAN2_RxFrameID,			// идентификатор фрейма CAN
						&CAN2_RxDataLen,			// длина поля данных в байтах 0 - 8 байт
						CAN2_RxData						// массив байтов, принятый по CAN
					);
		
		Delay_ms(1);

		if(CAN2_RxState == CAN2_OK){		// CAN2 frame received
				CAN2_TxState = CAN2_SendMSG(CAN2_RxFrameID,			// идентификатор фрейма CAN 
											CAN2_RxDataLen,		// длина поля данных в байтах 0 - 8 байт
											CAN2_RxData				// массив байтов, для  отправки по CAN
											);
				if(!CAN2_TxState) CAN2_RxFlag = 1;	// flag for succsessfull CAN2 reception and answer

		}

		

		if(USART1 -> SR & USART_SR_RXNE){		// IF USART1 received '1' 
			if(USART1->DR == 0x31){
				test_start = 1;
				printf(" ==== Testing Started.... \n");
			}
			else{
				test_start = 0;
			}
		}
		else{
			test_start = 0;
		}

		if(test_start){
			
			//========= USART6 testing: 0xA5 - test byte =============
			usart6_send(&usart6_test_byte, 1);
			Delay_ms(10);
			
			if(USART6->SR & USART_SR_RXNE) {
				if(USART6->DR == usart6_test_byte) usart6_ErrorCode = USART6_OK;
				else usart6_ErrorCode = USART6_ERR;
			} 
			else{
				usart6_ErrorCode = USART6_ERR;
			}
			
			//======== LOG USART6 send ============
			switch(usart6_ErrorCode){
				case USART6_OK: 
					printf("+++ USART6 Test PASSED +++ \n");
					break;
					
				case USART6_ERR:
					printf("--- USART6 Test FAILED --- \n");

			} // switch(usart6_ErrorCode)




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

				I2C_ErrCode = EEPROM_PageClear(eeprom_addr);	// clear EEPROM page after testing
			}
			
			//========= I2C1 LOG Sending =================
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
			
			

			
			//========= SPI testing ========================
			// writing data into FLASH memory
			CSLOW;
			w25send(PG_PROG);		// send comand Page Prog to FLASH memory
			// send adddres sector to be writed
			w25send( ( ADDR >> 16 ) & 0xFF );		
			w25send( ( ADDR >> 8 ) & 0xFF );	
			w25send( ( ADDR ) & 0xFF );	
			w25send(SPI2_TESTBYTE);
			CSHIGH;
			
			// check for data writing complete
			CSLOW;
			w25send(RD_SR1);	// send command "read status register1"
			while( ( w25send(0x00) & 0x01 ) == 1 ){};	// wait while BUSY-bit in cleared
			CSHIGH;
			
			Delay_ms(10);
			CSLOW;
			w25send(RD_DATA);		// send comand "Read data" to FLASH memory
			// send adddres to be readed
			w25send( ( ADDR >> 16 ) & 0xFF );		
			w25send( ( ADDR >> 8 ) & 0xFF );	
			w25send( ( ADDR ) & 0xFF );	
			SPI2_rd_byte = w25send(0x00);		// read data byte from FLASH
			CSHIGH;
		
			Delay_ms(10);
			
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

			//======== LOG SPI send ===============
			if(SPI2_rd_byte == SPI2_TESTBYTE){
				printf("+++ SPI2 Test PASSED +++ \n");
			}
			else{
				printf("--- SPI2 Test FAILED --- \n");
			};

			
			//========= LOG CAN2 send ===========
			if(CAN2_RxFlag){
				printf("+++ CAN2 Rx Tx Test PASSED +++ \n");
			}
			else{
				printf("--- CAN2 Test FAILED OR NOT EXECUTED ---\n");
			}
		

		}	// if (test_start)
	}	// while(1)
	  
}	// main()






/*************************** End of file ****************************/
