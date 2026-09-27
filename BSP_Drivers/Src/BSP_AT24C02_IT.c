#include "BSP_AT24C02_IT.h"
#include "BSP_Usart_Redir.h"

epprom_t at24c02;

bool BSP_AT24C02_Init(I2C_HandleTypeDef *i2c)
{
    at24c02.i2c = i2c;
    at24c02.dev_address = DEV_ADDRESS << 1;
    at24c02.page = PAGE_SIZE;
    at24c02.is_init = false;

    at24c02.state = EEPROM_IDLE;
    at24c02.buffer = NULL;
    at24c02.len = 0;
    at24c02.operator = NONE;
    at24c02.user_callback = NULL;
    if (HAL_OK == HAL_I2C_IsDeviceReady(at24c02.i2c, at24c02.dev_address, SCAN_RETIES, SCAN_TIME_MS))
    {
        at24c02.is_init = true;
        at24c02.state = EEPROM_READY;
    }

    return at24c02.is_init;
}

static bool BSP_AT24C02_Ready()
{
    if (at24c02.is_init == false)
        return false;
    return HAL_OK == HAL_I2C_IsDeviceReady(at24c02.i2c, at24c02.dev_address, POLL_RETIES, POLL_TIME_MS);
}

bool BSP_AT24C02_Read_Page_IT(uint16_t start_address, void *data_out, uint8_t size, user_callback_t cb)
{
    if (at24c02.is_init == false || !data_out || size == 0)
    {
        if (cb)
            cb(false);
        return false;
    }
    if (start_address + size > DEV_TOTAL_SIZE)
    {
        if (cb)
            cb(false);
        return false;
    }
    if (at24c02.state == EEPROM_BUSY)
    {
        if (cb)
            cb(false);
        return false;
    }

    at24c02.state = EEPROM_BUSY;
    at24c02.buffer = data_out;
    at24c02.len = size;
    at24c02.operator = READ;
    at24c02.user_callback = cb;

    HAL_StatusTypeDef state = HAL_I2C_Mem_Read_IT(
        at24c02.i2c,
        at24c02.dev_address,
        start_address,
        I2C_MEMADD_SIZE_8BIT,
        data_out,
        size);

    if (state != HAL_OK)
    {
        if (cb)
            cb(false);
        return false;
    }
    return false;
}

// 单页写入
bool BSP_AT24C02_Write_Page_IT(uint16_t start_address, void *data_in, uint8_t size, user_callback_t cb)
{
    if (at24c02.is_init == false || !data_in || size == 0)
    {
        if (cb)
            cb(false);
        return false;
    }

    // 页的具体位置
    uint16_t page_offset = start_address % at24c02.page;
    // 检查写入是否跨页
    if (page_offset + size > at24c02.page)
    {
        if (cb)
            cb(false);
        return false;
    }

    if (at24c02.state == EEPROM_BUSY)
    {
        if (cb)
            cb(false);
        return false;
    }

    at24c02.state = EEPROM_BUSY;
    at24c02.buffer = data_in;
    at24c02.len = size;
    at24c02.operator = WRITE;
    at24c02.user_callback = cb;

    HAL_StatusTypeDef state = HAL_I2C_Mem_Write_IT(
        at24c02.i2c,
        at24c02.dev_address,
        start_address,
        I2C_MEMADD_SIZE_8BIT,
        data_in,
        size);
    if (HAL_OK != state)
    {
        if (cb)
            cb(false);
        return false;
    }

    return true;
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (at24c02.i2c->Instance == hi2c->Instance)
    {
        if (at24c02.operator == READ)
        {
            at24c02.state = EEPROM_SUCCESS;
            if (at24c02.user_callback)
            {
                at24c02.user_callback(true);
                at24c02.user_callback = NULL;
            }
        }
    }
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (at24c02.i2c->Instance == hi2c->Instance)
    {
        if (at24c02.operator == WRITE)
        {
            at24c02.state = EEPROM_SUCCESS;
            if (at24c02.user_callback)
            {
                at24c02.user_callback(true);
                at24c02.user_callback = NULL;
            }
        }
    }
}

static Status_t BSP_AT24C02_State(void)
{
    return at24c02.state;
}

bool BSP_AT24C02_WaitWrite_Process(void)
{
    if (at24c02.operator == WRITE && at24c02.state == EEPROM_SUCCESS)
    {
        BSP_AT24C02_Ready();
        printf("设备写入等待成功\n");
        at24c02.state = EEPROM_READY;
        return true;
    }
    return false;
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (at24c02.i2c->Instance == hi2c->Instance)
    {
        at24c02.state = EEPROM_ERROR;
        if (at24c02.user_callback)
        {
            at24c02.user_callback(false);
            at24c02.user_callback = NULL;
        }
    }
}