#include "i2c.h"

enum I2C_ERR I2C_error_code = I2C_OK;

void I2C_Init(void){
	
  	RCC -> APB1ENR |= RCC_APB1ENR_I2C1EN;	// включение тактирования модуля I2C1 
  	RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOBEN;	// включение тактирования порта GPIOB (PB8 = SCL, PB9 = SDA)
  	
  	// настройка выводов PB8 и PB9 для работы с модулем I2C1
  	GPIOB -> MODER 	|= 	GPIO_MODER_MODE8_1;		// PB8 в режиме альтернативной функции
  	GPIOB -> MODER 	|= 	GPIO_MODER_MODE9_1;		// PB9 в режиме альтернативной функции
  	GPIOB -> OTYPER	|=	(GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9);	// включение выводов PB8 PB9 в режим open-drain
			   
  	//GPIOB -> PUPDR	&=	~(GPIO_PUPDR_PUPD8 | GPIO_PUPDR_PUPD9);		// явно прописываем отключение всех подтягивающих резисторов
																		// хотя по умолчанию они и так отключены
  	
  	//GPIOB -> PUPDR	|=	(GPIO_PUPDR_PUPD8_0 | GPIO_PUPDR_PUPD9_0);	// включаем подтягивающие pull_up резисторы
																
  	
  	GPIOB -> AFR[1]	|=	GPIO_AFRH_AFRH0_2;	// для PB8 выбрана альтернативная ф-ия AF4 = I2C1
  	GPIOB -> AFR[1]	|=	GPIO_AFRH_AFRH1_2;	// для PB9 выбрана альтернативная ф-ия AF4 = I2C1
  
	/*======== настройка модуля I2C1 ============
	По умолчанию I2C работает в подчиненном режиме. Интерфейс автоматически переключается с подчиненного на
	мастер, после того как он сгенерирует условие START и от ведущего к ведомому переключается, в случае 
	арбитражного проигрыша или если на шине происходит генерация STOP-условия, что обеспечивает возможность 
	работы с несколькими ведущими устройствами.
	
	  	режим работы					  	  = мастер
	  	скорость передачи				  	  = 100 кбит/сек
	  	адресация устройств на шине I2C 	  = 7 битная
	  	DMA не используется			  	  = Эти биты по умолчанию равны 0
	  	прерывания не используются	  	  = Эти биты по умолчанию равны 0
	  	адрес микросхемы памяти на шине I2C = 0xA0 = 0b1010_0000. Используются старшие 7 бит!
	
	*/ 
	 
  	// настройка частоты тактирования I2C1 = частота шины APB1 = 42 МГц
  	I2C1 -> CR1	= 0x0000; // выставляем CR1 в default value
  	I2C1 -> CR2	|=	(42 << I2C_CR2_FREQ_Pos);  // CR2_FREQ = 42 т.к. Freq_APB1 = 42MHz
  	//I2C1 -> CR2	|=	(I2C_CR2_FREQ_5 | I2C_CR2_FREQ_3 | I2C_CR2_FREQ_1);  // CR2_FREQ = 42 пример побитной записи
  	
  	/*====== CCR вычисления: ======
	I2C работает на частоте 100 кГц - Standard mode
	Thigh = CCR * T_plck1
	Tlow = CCR * T_pclk1
	Tsm = 1/(I2C_freq) = 1/100000 = Thigh + Tlow;
	1/100000 = 2 * CCR * T_pclk1
	CCR = 1 / (2*100000*T_pclk1)
	T_pclk1 = 1 / Freq_APB1; 
	Freq_APB1 = 42 MHz
	T_Pclk1 = 1 / 42000000
	CCR = 42000000 / (2*100000) = 210;
  	*/

  	I2C1 -> CCR	|=	(210 << I2C_CCR_CCR_Pos);		// 100 КГц
  	I2C1 -> CCR	&=	~(I2C_CCR_FS);					// явный сброс бита FS = работа на чатоте 100 кГц (Standard Mode)	
  	
  	I2C1 -> TRISE |=  (43 << I2C_TRISE_TRISE_Pos);	// значение поля = I2C1_CR2_FREQ + 1 = 42+1 = 43
  	I2C1 -> OAR1  &=  ~(I2C_OAR1_ADDMODE);			// использование 7-ми битного адреса устройства на шине I2C
  	I2C1 -> CR1	|=	I2C_CR1_PE;						// I2C1 enabled. 
  	I2C1 -> CR1	|=	I2C_CR1_ACK;					// разрешение генерации ACK после приема байтов.
  	/* бит I2C_CR1_ACK можно выставлять в 1 только после включения бита I2C_CR1_PE. 
		 иначе бит I2C_CR1_ACK всегда будет сбрасываться в 0 аппаратно.
  	*/

}



void I2C1_StartGen(void){
	I2C1->CR1  |=  I2C_CR1_START;
	while((I2C1 -> SR1 & I2C_SR1_SB) == 0){};	// дождаться START-условия на шине I2C
}



void I2C1_StopGen(void){
	I2C1->CR1  |=  I2C_CR1_STOP;
}




void I2C1_ACK_Gen_Enable(void){
	I2C1->CR1  |=  I2C_CR1_ACK;
}




void I2C1_ACK_Gen_Disable(void){
	I2C1->CR1  &=  ~(I2C_CR1_ACK);
}




uint16_t I2C1_Tx_DeviceADDR(char device_address, char RW_bit){
	uint16_t wait_counter = 0;
	I2C1 -> DR = (device_address + RW_bit);				// отправить в I2C_DR адрес устройства и бит WR
	
	while((I2C1 -> SR1 & I2C_SR1_ADDR) == 0){ // ждем флаг I2C_SR1_ADDR = 1. Пока завершится передача байта адреса
		wait_counter++;
		if (wait_counter > WAIT_TIME){
			wait_counter = 0;
			return I2C_DEV_ADDR_ERR;
		}
	};		
	
	(void)I2C1 -> SR1; 
	(void)I2C1 -> SR2;	// очистка бита ADDR чтением регистров SR1 SR2
	return I2C_OK;
}


/* в случае EEPROM AT24C02B можно за раз записать не более 8 байт данных. 
т.к. размер страницы всего 8 байт. И если запись доходит до конца страницы,
то следующий байт пишется в первый адрес текущей страницы. 
Таким образом, данные могут быть повреждены / перезаписаны.
*/
uint16_t I2C_Write(char start_addr, char data[], uint16_t data_len){ // запись в EEPROM указанного массива, указанной длинны, с указанного адреса  
	uint16_t err_code;
	uint16_t wait_counter = 0;
	
	I2C1_ACK_Gen_Enable();						// включение генерации ACK
	
	while((I2C1 -> SR2 & I2C_SR2_BUSY) != 0){	// проверить занятость шины I2C по флагу I2C_SR2_BUSY
		wait_counter++;
		if(wait_counter > WAIT_TIME){
			wait_counter = 0;
			return I2C_BUS_BUSY;
		}
	};	
	
	I2C1_StartGen();							// генерация START-условия
	
	err_code = I2C1_Tx_DeviceADDR(I2C_DEV_ADDR, I2C_WR_BIT);
	if( err_code != I2C_OK ) return err_code;

	I2C1 -> DR = start_addr;	// отправить в I2C_DR адрес начальной ячейки памяти, куда хотим писать данные
	while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){
		wait_counter++;
		if(wait_counter > WAIT_TIME){
			wait_counter = 0;
			return I2C_WR_ERR;
		}
	};	// ждем флаг I2C_SR1_TXE = 1. Пока завершится передача байта данных

	// цикл сколько байт нужно передать: 
	// отправляем в I2C_DR байты данных, 
	// ждем флаг I2C_SR1_TXE = 1. Пока завершится передача байта данных
	for(uint16_t i = 0; i < data_len; i++){
		I2C1 -> DR = data[i];
		while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){
			if(wait_counter > WAIT_TIME){
				wait_counter = 0;
				return I2C_WR_ERR;
			}
		
		};
	}

	I2C1_StopGen();	// генерация STOP-условия

	return I2C_OK;
	
}




uint16_t EEPROM_PageClear(char start_addr){ // запись в EEPROM 0xFF на всю страницу 8 байт
	uint16_t wait_counter = 0;
	uint16_t err_code;

	I2C1_ACK_Gen_Enable();						// включение генерации ACK
	
	while((I2C1 -> SR2 & I2C_SR2_BUSY) != 0){		// проверить занятость шины I2C по флагу I2C_SR2_BUSY
		wait_counter++;
		if(wait_counter > WAIT_TIME){
			wait_counter = 0;
			return I2C_BUS_BUSY;
		}
	};	
	
	I2C1_StartGen();							// генерация START-условия
	
	err_code = I2C1_Tx_DeviceADDR(I2C_DEV_ADDR, I2C_WR_BIT);
	if( err_code != I2C_OK ) return err_code;

	I2C1 -> DR = start_addr;	// отправить в I2C_DR адрес начальной ячейки памяти, куда хотим писать данные
	
	while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){	// ждем флаг I2C_SR1_TXE = 1. Пока завершится передача байта данных
		if(wait_counter > WAIT_TIME){
			wait_counter = 0;
			return I2C_WR_ERR;
		}
	};	

	// цикл сколько байт нужно передать: 
	// отправляем в I2C_DR байты данных, 
	// ждем флаг I2C_SR1_TXE = 1. Пока завершится передача байта данных
	for(uint16_t i = 0; i < EEPROM_PAGE_LEN_BYTES-1; i++){
		I2C1 -> DR = 0xFF;
		while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){
			if(wait_counter > WAIT_TIME){
				wait_counter = 0;
				return I2C_WR_ERR;
			}
		};
	}

	I2C1_StopGen();	// генерация STOP-условия

	return I2C_OK;
}



uint16_t I2C_Read(char start_addr, char rd_data[], uint16_t data_len){  // чтение из EEPROM из указанной ячейки, указанной длины массив, сохранение в указагнный масссив
	
	uint16_t wait_counter = 0;
	uint16_t err_code;
	
	I2C1_ACK_Gen_Enable();						// включение генерации ACK
	
	while((I2C1 -> SR2 & I2C_SR2_BUSY) != 0){	// проверить занятость шины I2C по флагу I2C_SR2_BUSY
		wait_counter++;
		if(wait_counter > WAIT_TIME){
			wait_counter = 0;
			return I2C_BUS_BUSY;
		}
	};	
	

	I2C1_StartGen();							// генерация START-условия
  
	err_code = I2C1_Tx_DeviceADDR(I2C_DEV_ADDR, I2C_WR_BIT);	// передача адреса устройства и бита WR
	if( err_code != I2C_OK ) return err_code;

   /*
   передача адреса ячейки памяти с которой хотим читать
   так как у нас память всего 256 байт, то адрес ячейки памяти имеет разрядность 8-бит = 1 байт
   если объем памяти будет больше, то для передачи адреса ячейки памяти нужно будет использовать 2 байта 
	*/

	I2C1 -> DR = start_addr;					// отправить в I2C_DR адрес начальной ячейки памяти, откуда хотим читать данные
	
	while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){	// ждем флаг I2C_SR1_TXE = 1. Пока завершится передача байта данных
		if(wait_counter > WAIT_TIME){
			wait_counter = 0;
			return I2C_WR_ERR;
		}
	};

	
	/*========= Пример отправки 2-х байтного адреса ячейки памяти ===============

		I2C1 -> DR = (start_addr >> 8 );			// передача старшего байта адреса			
		while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){};	// ожидание отправки байта
		I2C1 -> DR = (start_addr);					// отправка младшего байта адреса	
		while((I2C1 -> SR1 & I2C_SR1_TXE) == 0){};	// ожидание отправки байта
	
	*/

	
	I2C1_StartGen();	// повторная генерация START-условия

	err_code = I2C1_Tx_DeviceADDR(I2C_DEV_ADDR, I2C_RD_BIT);		// передача адреса устройства и бита RD
	if( err_code != I2C_OK ) return err_code;

	// цикл чтения данных (кол-во байт - 1):
	// ожидание флаг I2C_SR1_RXNE = 1. - принят новый байт данных
	// чтение регистра I2C_DR
	for(uint16_t i = 0; i < data_len-1; i++){
		while((I2C1 -> SR1 & I2C_SR1_RXNE) == 0){
			if(wait_counter > WAIT_TIME){
				wait_counter = 0;
				return I2C_RD_ERR;
			}
		};
		rd_data[i] = I2C1 -> DR;
		
	}
	// отключение генерации ACK-бита после принятого байта, чтобы в конце отправить NACK
	I2C1_ACK_Gen_Disable();
	
	while((I2C1 -> SR1 & I2C_SR1_RXNE) == 0){	// ожидание принятия последнего байта
		if(wait_counter > WAIT_TIME){
			wait_counter = 0;
			return I2C_RD_ERR;
		}
	};	
	
	rd_data[data_len-1] = I2C1 -> DR;				// чтение регистра I2C_DR - чтение последнего принятого байта

	I2C1_ACK_Gen_Enable();

	I2C1_StopGen();	// генерация STOP-условия	
	
	return I2C_OK;

 }

