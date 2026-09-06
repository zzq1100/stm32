#include "BSP_AT24C02.h"


epprom_t at24c02;


bool BSP_AT24C02_Init(I2C_HandleTypeDef *i2c)
{
    at24c02.i2c = i2c;
    at24c02.dev_address = DEV_ADDRESS << 1;
    at24c02.page = PAGE_SIZE;
    at24c02.is_init = false;

    if (HAL_OK == HAL_I2C_IsDeviceReady(at24c02.i2c, at24c02.dev_address, SCAN_RETIES, SCAN_TIME_MS))
    {
        at24c02.is_init = true;
    }

    return at24c02.is_init;
}


bool BSP_AT24C02_Read(uint16_t start_address, void *data_out, uint8_t size)
{
    if (at24c02.is_init == false || !data_out || size == 0) return false;
    if (start_address + size > DEV_TOTAL_SIZE) return false;

    HAL_StatusTypeDef state = HAL_I2C_Mem_Read(
        at24c02.i2c,
        at24c02.dev_address,
        start_address,
        I2C_MEMADD_SIZE_8BIT,
        data_out,
        size,
        READ_TIMEOUT_MS
    );

    return HAL_OK == state;
}

static bool BSP_AT24C02_Ready()
{
    if (at24c02.is_init == false) return false;
    return HAL_OK == HAL_I2C_IsDeviceReady(at24c02.i2c, at24c02.dev_address, POLL_RETIES, POLL_TIME_MS);
}

// 单页写入
static bool BSP_AT24C02_Write_Page(uint16_t start_address, void *data_in, uint8_t size)
{
    if (at24c02.is_init == false || !data_in || size == 0) return false;

    // 页的具体位置
    uint16_t page_offset = start_address % at24c02.page;
    // 检查写入是否跨页
    if (page_offset + size > at24c02.page) return false;

    HAL_StatusTypeDef state =  HAL_I2C_Mem_Write(
        at24c02.i2c,
        at24c02.dev_address,
        start_address,
        I2C_MEMADD_SIZE_8BIT,
        data_in,
        size,
        Write_TIMEOUT_MS
    );
    if (HAL_OK != state) return false;

    return BSP_AT24C02_Ready();
}



bool BSP_AT24C02_Write(uint16_t start_address, void *data_in, uint8_t size)
{
    if (at24c02.is_init == false || data_in == NULL || size == 0) return false;
    if (start_address + size > at24c02.page) return false;

    // 已经写入字节量
    uint16_t bytes_writen = 0;

    while (bytes_writen < size)
    {
        // 数据还有多少没发送
        uint16_t data_left = size - bytes_writen;
        // 新的开始地址
        uint16_t new_start_address = start_address + bytes_writen;
        // 当前页写到哪个位置
        uint16_t page_offset = new_start_address % at24c02.page;
        // 当前页剩余空间
        uint16_t page_space = at24c02.page - page_offset;
        // 本次写入的数据量
        uint16_t write_bytes = data_left;
        if (write_bytes > page_space)
        {
            write_bytes = page_space;
        }

        if (BSP_AT24C02_Write_Page(new_start_address, (uint16_t*)data_in + bytes_writen, write_bytes) == false) return false;

        bytes_writen += write_bytes;
    }
    return true;
}
