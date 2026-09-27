#ifndef __BSP_AT24C02_IT_H
#define __BSP_AT24C02_IT_H


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



// 设备状态
typedef enum
{
	EEPROM_IDLE = 0,     // 空闲,没有被初始化
	EEPROM_READY,        // 就绪状态
	EEPROM_BUSY,         // 忙状态
	EEPROM_SUCCESS,      // 成功
	EEPROM_ERROR         // 错误
}Status_t;

// 操作类型
typedef enum
{
	NONE = 0,
	WRITE,
	READ
}Operator_t;

typedef void(*user_callback_t)(bool);

typedef struct 
{
	I2C_HandleTypeDef *i2c;
	uint16_t dev_address;
	uint8_t page;
	bool is_init;

	// 添加中断标志位
	volatile Status_t state;
	uint8_t *buffer;
	uint8_t len;
	Operator_t operator;
	user_callback_t user_callback;
}epprom_t;



bool BSP_AT24C02_Init(I2C_HandleTypeDef *i2c);
bool BSP_AT24C02_Write_Page_IT(uint16_t start_address, void *data_in, uint8_t size, user_callback_t cb);
bool BSP_AT24C02_Read_Page_IT(uint16_t start_address, void *data_out, uint8_t size, user_callback_t cb);
bool BSP_AT24C02_WaitWrite_Process(void);
		
		
#ifdef __cplusplus
	}
#endif

#endif