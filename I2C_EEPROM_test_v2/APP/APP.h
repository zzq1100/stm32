#ifndef __APP_H
#define __APP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "BSP_LED.h"
#include "BSP_Usart_Redir.h"
#include "BSP_AT24C02.h"

#define CONFIG_MAGIC   0x50
#define CONFIG_VERSION 0x01

//8字节的配置结构体类型
typedef struct 
{
    uint8_t magic;        // 魔数，配置有效
    uint8_t version;      // 版本，配置有效
    uint8_t led;          // 选择LED
    uint8_t repeat;       // 闪几次
    uint16_t delay_ms;    // 时间间隔， ms
    uint16_t start_time;  // 开始闪灯时间, ms
}config_t;

#define CONFIG_ADDRESS 0x0

#define SAVE_CONFIG(config_address, config_len) BSP_AT24C02_Write(CONFIG_ADDRESS, config_address, config_len)
#define LOAD_CONFIG(config_address, config_len) BSP_AT24C02_Read(CONFIG_ADDRESS, config_address, config_len)


void App_Init();
void App_Run();

#ifdef __cplusplus
}
#endif

#endif