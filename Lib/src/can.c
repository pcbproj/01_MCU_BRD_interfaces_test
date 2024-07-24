#include "can.h"





void CAN2_Init(void){
	
	RCC -> AHB1ENR |=	RCC_AHB1ENR_GPIOBEN;			// включение тактирования GPIOB: PB5 = CAN2_RX, PB6 = CAN2_TX 
	RCC -> APB1ENR |=	RCC_APB1ENR_CAN1EN;				// включение тактирования CAN1
	RCC -> APB1ENR |=	RCC_APB1ENR_CAN2EN;				// включение тактирования CAN2
	
	
	GPIOB -> OSPEEDR |= GPIO_OSPEEDER_OSPEEDR5;			// максимальная частота работы вывода PB5
	GPIOB -> MODER |= GPIO_MODER_MODE5_1;				// настройка PB5 в альтернативный режим
	GPIOB -> AFR[0] |= (9U << GPIO_AFRL_AFSEL5_Pos);	// выбор альтернативной функции AF9 для PB5
	
	GPIOB -> OSPEEDR |= GPIO_OSPEEDER_OSPEEDR6;			// максимальная частота работы вывода PB6
	GPIOB -> MODER |= GPIO_MODER_MODE6_1;				// настройка PB6 в альтернативный режим
	GPIOB -> AFR[0] |= (9U << GPIO_AFRL_AFSEL6_Pos);	// выбор альтернативной функции AF9 для PB6


	
	CAN2 -> MCR |= CAN_MCR_INRQ;						// переключение CAN2 в режим инициализации
	while((CAN2 -> MSR & CAN_MSR_INAK) == 0){};			// ожидание, пока CAN2  не переключится в режим инициализации


	CAN2 -> MCR |= CAN_MCR_NART;						// Отключение автоматической ретрансляции сообщений, вероятность неудачной передачи мала
	CAN2 -> MCR |= CAN_MCR_AWUM;						// Включение автоматического выхода из спящего режима после приема сообщения 
	CAN2 -> BTR = 0x00;								// сброс регистра BTR
	CAN2 -> BTR &= ~(CAN_BTR_SILM | CAN_BTR_LBKM);	// Отключение режимов Loop и Silent, нормальный режим работы
	
	/*	Настройка скорости передачи данных 500 кБит/сек:
		предделитель 6. 
		freq tq = 42 / 6 = 7 MHz
		CAN_Bit_time_tq_number = 7_000_000 Hz / 500_000 bit/s  = 14
		CAN_Bit_time = (1 + BS1 * BS2 ) = 14*tq

		BS1 + BS2 = 13*tq
		BS2 = BS1 / 7;
		BS2 = 13*tq / 7 = 1.86 ~= 2*tq
		BS1 = (13-2) * tq = 11*tq

	*/
	
	CAN2 -> BTR |= (5 << CAN_BTR_BRP_Pos);			// предделитель равен 6: 42 / 6 = 7 МГц частота тактирования CAN
	CAN2 -> BTR |= (10 << CAN_BTR_TS1_Pos);			// TS1 = 10, BS1 = 11
	CAN2 -> BTR |= (1 << CAN_BTR_TS2_Pos);			// TS2 = 1 , BS2 = 2
		
	// настройка фильтрации по FRAME_ID
	// filter list mode ID
	// принимаем сообщения только с RX_FRAME_ID = 0x567;
	
	/*
		Если есть два модуля CAN, то они имеют в наличии 28 фильтров, 
		с 0 по 13 для CAN1, и с 14 по 28 для CAN2.
	*/
	
	CAN1 -> FMR |= CAN_FMR_FINIT;							// переводим фильтры в режим инициализации
	CAN1 -> FM1R |= CAN_FM1R_FBM14;							// выбираем банк 14 в режиме List mode
	CAN1 -> FS1R &= ~(CAN_FS1R_FSC14);						// явно выбираем разрядность фильтра 16 бит 
	CAN1 -> FFA1R &= ~(CAN_FFA1R_FFA14);					// явно выбираем, что сообщение после фильтра 14 сохранится в FIFO0. бит сброшен в 0.
	CAN1 -> sFilterRegister[14].FR1 = (RX_FRAME_ID << 5);	// записываем значение FRAME_ID, сообщения с которым мы принимаем
	CAN1 -> FA1R |= (1 << CAN_FA1R_FACT14_Pos);				// активируем фильтр 14 для работы
	CAN1 -> FMR &= ~CAN_FMR_FINIT;							// переводим фильтры в активный режим 												

	CAN2 -> MCR &= ~(CAN_MCR_INRQ);							// переключение CAN2 в нормальный режим работы
	while((CAN2 -> MSR & CAN_MSR_INAK) != 0){};				// ожидание, пока CAN2 не переключится в нормальный режим 

}


// функция чтения из FIFO принятого сообщения по CAN
char CAN2_ReceiveMSG(uint16_t *frame_ID,			// идентификатор фрейма CAN
						uint16_t *data_len_bytes,	// длина поля данных в байтах 0 - 8 байт
						char rx_array[]				// массив байтов, принятый по CAN
					){
	
	if ((CAN2 -> RF0R & CAN_RF0R_FMP0) != 0){	// проверка FIFO0 не пустое? 
		*frame_ID = ((CAN2 -> sFIFOMailBox[0].RIR >> CAN_RI0R_STID_Pos) & 0x0FFF);			// вычитывание идентификатора,	
		*data_len_bytes = ((CAN2 -> sFIFOMailBox[0].RDTR >> CAN_RDT0R_DLC_Pos) & 0x000F);		// вычитывание DLC 
		
		for(uint16_t i=0; i < *data_len_bytes; i++){		// вычитывание данных сообщения из FIFO0/1
			if (i < 4) {
				rx_array[i] = ((CAN2 -> sFIFOMailBox[0].RDLR >> 8*i) & 0x00FF);
			}
			else{
				rx_array[i] = ((CAN2 -> sFIFOMailBox[0].RDHR >> 8*(i-4)) & 0x00FF);
			}
		}
		CAN2 -> RF0R |= CAN_RF0R_RFOM0;		// освобождение FIFO0 выставлением бита FROM в 1 
		return CAN2_OK;
	}
	else{
		return CAN2_ERR;	// FIFO пустое. Ни чего не считали
	}
}






// функция передачи сообщения по CAN2
char CAN2_SendMSG(uint16_t frame_ID,			// идентификатор фрейма CAN 
					uint16_t data_len_bytes,	// длина поля данных в байтах 0 - 8 байт
					char tx_array[]				// массив байтов, для  отправки по CAN
					){

	// будем использовать для передачи собщений mailbox[0]
	if((CAN2 -> TSR & CAN_TSR_TME0) == 0){		// проверка, что mailbox[0] пустой
		return CAN2_ERR;								// возврат ошибки "mailbox[0] не пустой" завершение
	}
	else{
		CAN2 -> sTxMailBox[0].TIR = 0x0000;
		CAN2 -> sTxMailBox[0].TDTR = 0x0000;
		CAN2 -> sTxMailBox[0].TIR &= ~(CAN_TI0R_IDE);			// явно сбрасываем EXID, используем стандартный фрейм
		CAN2 -> sTxMailBox[0].TIR &= ~(CAN_TI0R_RTR);			// явно сбрасываем RTR, отправляем DATAT FRAME
		CAN2 -> sTxMailBox[0].TIR &= ~(CAN_TI0R_TXRQ);			// явно сбрасываем TXRQ, еще рано отправлять сообщение
		CAN2 -> sTxMailBox[0].TIR |= (frame_ID << 21);			// Запись идентификатора фрейма FRAME_ID
		
		CAN2 -> sTxMailBox[0].TDTR |= ((data_len_bytes & 0x000F) << CAN_TDT0R_DLC_Pos);	// Указать длину поля данных
			
		CAN2 -> sTxMailBox[0].TDLR = 0x0000;
		CAN2 -> sTxMailBox[0].TDHR = 0x0000;
		for(uint16_t i=0; i < data_len_bytes; i++){	// записать данные из массива в mailbox[0] для отправки 
			if(i < 4){
				CAN2 -> sTxMailBox[0].TDLR |= (tx_array[i] << 8*i);
			}
			else{
				CAN2 -> sTxMailBox[0].TDHR |= (tx_array[i] << 8*(i-4));
			}
		}
		

		CAN2 -> sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;		// Начать отправку сообщения. TXRQ = 1
		
		if((CAN2 -> TSR & CAN_TSR_RQCP0) == 0){			// проверка, что сообщение отправлено 
			return ((CAN2 -> ESR & CAN_ESR_LEC) >> CAN_ESR_LEC_Pos);	// возвращяем код ошибки 
		}
		else{
			CAN2 -> TSR |= CAN_TSR_RQCP0;					// сброс бита RQCP0
			return CAN2_OK;					
		}
	}
	
}

