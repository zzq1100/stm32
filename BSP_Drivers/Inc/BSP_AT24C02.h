#ifndef __BSP_AT24C02_H
#define __BSP_AT24C02_H


#ifdef __cplusplus
	extern "C" {
#endif
	
#include "stdint.h"
#include "stdbool.h"
#include "stm32f1xx_hal.h"


#define DEV_ADDRESS 0x50
#define PAGE_SIZE 8

#define SCAN_RETIES 2 
#define SCAN_TIME_MS 5
#define POLL_RETIES 10
#define POLL_TIME_MS 2

#define DEV_TOTAL_SIZE 256

#define READ_TIMEOUT_MS 50
#define Write_TIMEOUT_MS 50


typedef struct 
{
	I2C_HandleTypeDef *i2c;
	uint16_t dev_address;
	uint8_t page;
	bool is_init;
}epprom_t;



bool BSP_AT24C02_Init(I2C_HandleTypeDef *i2c);
bool BSP_AT24C02_Write(uint16_t start_address, void *data_in, uint8_t size);
bool BSP_AT24C02_Read(uint16_t start_address, void *data_out, uint8_t size);
		
		
		
#ifdef __cplusplus
	}
#endif



#endif