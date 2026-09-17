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

#define PAGE_SIZE                       (256)
#define SECTOR_SIZE                     (256 * 16)
#define BLOCK_SIZE                      (256 * 16 * 16)
#define MEM_TOTAL_SIZE                  (256 * 16 * 16 * 128)
#define CHIP_ID                         0xEF4017
#define GPIO_PORT                       GPIOE
#define NSS_PIN                         GPIO_PIN_6


#define CMD_W25Qxx_GET_JEDIC_ID         0x9F
#define CMD_W25Qxx_READ_SR1             0x05
#define CMD_W25Qxx_READ_SR2             0x35
#define CMD_W25Qxx_READ_SR3             0x15
#define CMD_W25Qxx_WRITE_ENABLE         0x06
#define CMD_W25Qxx_WRITE_DISABLE        0x04
#define CMD_W25Qxx_SECTOR_ERASE         0x20
#define CMD_W25Qxx_READ_DATA            0x03
#define CMD_W25Qxx_PAGE_PROGRAM         0x02
#define CMD_W25Qxx_CHIP_ERASE           0xC7


#define W25Qxx_WRITE_BUSY_BIT           0x01
#define W25Qxx_WEL_ENABLE_BIT           0x02


#define W25Qxx_SECTOR_ADDRESS_MASK      0xFFF

typedef struct
{
    uint32_t             page_size;
    uint32_t             sector_size;
    uint32_t             block_size;
    uint32_t             mem_total_size;
    uint32_t             chip_id;
    SPI_HandleTypeDef    *hspi1;
    GPIO_TypeDef         *gpio_port;
    uint16_t             nss_pin;
    bool                 is_init;
}spi_falsh_t;



bool      BSP_W25Qxx_Init(SPI_HandleTypeDef *spi1);
uint32_t  BSP_W25Qxx_Get_ID(void);
uint8_t   BSP_W25Qxx_Get_SR1(void);
void      BSP_W25Qxx_Write_Enable(void);
void      BSP_W25Qxx_Write_Disable(void);
void      BSP_W25Qxx_Wait_BUSY(void);
void      BSP_W25Qxx_Sector_Erase(uint32_t sector_address);
bool      BSP_W25Qxx_Read_Data(uint32_t data_address, uint8_t *rx_buffer, uint32_t rx_size);
bool      BSP_W25Qxx_Page_Program(uint32_t page_address, uint16_t page_offset, uint8_t *wx_buffer, uint32_t wx_size);
bool      BSP_W25Qxx_Write_Data(uint32_t page_address, uint8_t *wx_buffer, uint32_t wx_size);


#ifdef __cplusplus
}
#endif

#endif