#include "stm32f4xx.h"


// ------  адресация устройства на шине I2C ------------- 
#define I2C_DEV_ADDR	0xA0 // адрес микросхемы EEPROM = 1010_0000 в бинарном виде. Используются старшие 7 бит
#define I2C_WR_BIT		0x00 // запрос на запись данных в I2C-устройство (в EEPROM)
#define I2C_RD_BIT		0x01 // запрос на чтение данных из I2C-устройство (в EEPROM)
#define I2C_DEV_ADDR_RD	 (I2C_DEV_ADDR + I2C_RD_BIT)	// младший бит выставляем в RD = 1
#define I2C_DEV_ADDR_WR  (I2C_DEV_ADDR + I2C_WR_BIT)	// младший бит выставляем в WR = 0

// ----------- адресация внутри EEPROM -------------
#define EEPROM_WR_START_ADDR	0x08	// запись с 1 ячейки в страницу 2
#define EEPROM_WR_LEN			8	
#define EEPROM_PAGE_LEN_BYTES	8
#define EEPROM_RD_START_ADDR	0x08	// чтение с 1 ячейки в страницу 2
#define EEPROM_RD_LEN			8

#define WAIT_TIME		1024

enum I2C_ERR {
    	I2C_OK = 0,
    	I2C_WR_ERR,
		I2C_RD_ERR,
		I2C_DEV_ADDR_ERR,
		I2C_BUS_BUSY,
		I2C_ERR_DATA
	};




void I2C_Init(void);
void I2C1_StartGen(void);
void I2C1_StopGen(void);
void I2C1_ACK_Gen_Enable(void);
void I2C1_ACK_Gen_Disable(void);

uint16_t I2C1_Tx_DeviceADDR(char device_address, char RW_bit);
uint16_t I2C_Write(char start_addr, char data[], uint16_t data_len);
uint16_t EEPROM_PageClear(char start_addr);
uint16_t I2C_Read(char start_addr, char rd_data[], uint16_t data_len);

