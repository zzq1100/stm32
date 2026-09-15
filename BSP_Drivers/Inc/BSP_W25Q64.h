#ifndef __BSP_W25Q64_H
#define __BSP_W25Q64_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f1xx_hal.h"
#include "stdint.h"
#include "stdbool.h"
#include "stdio.h"

#define PAGE_SIZE (256)
#define SECTOR_SIZE (256 * 16)
#define BLOCK_SIZE (256 * 16 * 16)
#define MEM_TOTAL_SIZE (256 * 16 * 16 * 128)
#define CHIP_ID 0xef4017
#define GPIO_PORT GPIOE
#define NSS_PIN GPIO_PIN_6

typedef struct
{
    uint32_t page_size;
    uint32_t sector_size;
    uint32_t block_size;
    uint32_t mem_total_size;
    uint32_t chip_id;
    SPI_HandleTypeDef *hspi1;
    GPIO_TypeDef *gpio_port;
    uint16_t nss_pin;
    bool is_init;
}spi_falsh_t;



bool BSP_W25Qxx_Init(SPI_HandleTypeDef *spi1);
uint32_t BSP_W25Qxx_ID(void);

#ifdef __cplusplus
}
#endif

#endif