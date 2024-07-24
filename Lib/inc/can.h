#include "stm32f4xx.h"

#define CAN_TX_TIME_MS		300		// время ожидания в мс для отправки сообщения по CAN
//#define BTN_CHECK_MS		10		// период опроса кнопок в мс
//#define	BTN_PRESS_CNT		4		// кол-во последовательных проверок состояния кнопки


#define RX_FRAME_ID			0x567	// FRAME_ID сообщений, которые мы принимаем. остальные игнорируем
#define TX_FRAME_ID			0x567	// FRAME_ID отправляемого сообщения
#define CAN_RX_DATA_LEN		8		// количество байт данных в принимаемом сообщении CAN
#define CAN_TX_DATA_LEN		8		// количество байт данных в отправляемом сообщении CAN

#define CAN2_OK				0
#define CAN2_ERR			1

void CAN2_Init(void);


// функция передачи сообщения по CAN2
char CAN2_SendMSG(uint16_t frame_ID,			// идентификатор фрейма CAN 
					uint16_t data_len_bytes,	// длина поля данных в байтах 0 - 8 байт
					char tx_array[]				// массив байтов, для  отправки по CAN
					);


// функция чтения из FIFO принятого сообщения по CAN
char CAN2_ReceiveMSG(uint16_t *frame_ID,			// идентификатор фрейма CAN
						uint16_t *data_len_bytes,	// длина поля данных в байтах 0 - 8 байт
						char rx_array[]				// массив байтов, принятый по CAN
					);
