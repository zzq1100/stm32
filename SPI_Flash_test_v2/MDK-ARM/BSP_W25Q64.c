#include "BSP_W25Q64.h"
#include "string.h"

spi_falsh_t w25qxx;

static void SPI_Flash_Init(SPI_HandleTypeDef *spi1)
{
    w25qxx.page_size           = PAGE_SIZE;
    w25qxx.sector_size         = SECTOR_SIZE;
    w25qxx.block_size          = BLOCK_SIZE;
    w25qxx.mem_total_size      = MEM_TOTAL_SIZE;
    w25qxx.chip_id             = CHIP_ID;
    w25qxx.hspi1               = spi1;
    w25qxx.gpio_port           = GPIO_PORT;
    w25qxx.nss_pin             = NSS_PIN;
    w25qxx.is_init             = true;
}

static void BSP_W25Qxx_CS_Enable()
{
    HAL_GPIO_WritePin(w25qxx.gpio_port, w25qxx.nss_pin, GPIO_PIN_RESET);
}

static void BSP_W25Qxx_CS_Disable()
{
    HAL_GPIO_WritePin(w25qxx.gpio_port, w25qxx.nss_pin, GPIO_PIN_SET);
}

static bool BSP_W25Qxx_Write_Is_Busy()
{
    return BSP_W25Qxx_Get_SR1() & W25Qxx_WRITE_BUSY_BIT;
}

static bool BSP_W25Qxx_WEL_Is_Enable()
{
    return BSP_W25Qxx_Get_SR1() & W25Qxx_WEL_ENABLE_BIT;
}

bool BSP_W25Qxx_Init(SPI_HandleTypeDef *spi1)
{
    SPI_Flash_Init(spi1);

    uint32_t id = BSP_W25Qxx_Get_ID();
    if (id != w25qxx.chip_id)
    {
        w25qxx.is_init = false;
        printf(">> BSP_W25Qxx_Init Error <<\n");
        return false;
    }

    printf(">> BSP_W25Qxx_Init Success & ID: 0x%x <<\n", w25qxx.chip_id);
    return true;
}

uint32_t BSP_W25Qxx_Get_ID(void)
{
    if (w25qxx.is_init == false)
        return 0;

    uint8_t cmd = CMD_W25Qxx_GET_JEDIC_ID;
    uint8_t id[3] = {0};
    HAL_StatusTypeDef state = HAL_OK;

    BSP_W25Qxx_CS_Enable();

    state = HAL_SPI_Transmit(w25qxx.hspi1, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> Get_ID's HAL_SPI_Transmit Error <<\n");
        goto EXIT;
    }
    state = HAL_SPI_Receive(w25qxx.hspi1, id, sizeof(id), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        memset(id, 0, sizeof(id));
        printf(">> Get_ID's HAL_SPI_Receive Error <<\n");
        goto EXIT;
    }

EXIT:
    BSP_W25Qxx_CS_Disable();
    return (id[0] << 16) | (id[1] << 8) | (id[2]);
}

uint8_t BSP_W25Qxx_Get_SR1(void)
{
    if (w25qxx.is_init == false)
        return 0xFF;

    uint8_t cmd = CMD_W25Qxx_READ_SR1;
    uint8_t SR1 = 0xFF;
    HAL_StatusTypeDef state = HAL_OK;

    BSP_W25Qxx_CS_Enable();

    state = HAL_SPI_Transmit(w25qxx.hspi1, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> Get_SR1's HAL_SPI_Transmit Error <<\n");
        goto EXIT;
    }
    state = HAL_SPI_Receive(w25qxx.hspi1, &SR1, sizeof(SR1), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        SR1 = 0xFF;
        printf(">> Get_SR1's HAL_SPI_Receive Error <<\n");
        goto EXIT;
    }

EXIT:
    BSP_W25Qxx_CS_Disable();
    printf(">> BSP_W25Qxx_SR1: 0x%x <<\n", SR1);
    return SR1;
}

void BSP_W25Qxx_Write_Enable(void)
{
    if (w25qxx.is_init == false)
        return;

    BSP_W25Qxx_CS_Enable();

    uint8_t cmd = CMD_W25Qxx_WRITE_ENABLE;
    HAL_StatusTypeDef state = HAL_OK;

    state = HAL_SPI_Transmit(w25qxx.hspi1, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> Write_Enable's HAL_SPI_Transmit Error <<\n");
        goto EXIT;
    }
    printf(">> BSP_W25Qxx_Write_Enable Success <<\n");
EXIT:
    BSP_W25Qxx_CS_Disable();
}

void BSP_W25Qxx_Write_Disable(void)
{
    if (w25qxx.is_init == false)
        return;

    BSP_W25Qxx_CS_Enable();

    uint8_t cmd = CMD_W25Qxx_WRITE_DISABLE;
    HAL_StatusTypeDef state = HAL_OK;

    state = HAL_SPI_Transmit(w25qxx.hspi1, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> Write_Disable's HAL_SPI_Transmit Error <<\n");
        goto EXIT;
    }
    printf(">> BSP_W25Qxx_Write_Disable Success <<\n");
EXIT:
    BSP_W25Qxx_CS_Disable();
}

void BSP_W25Qxx_Wait_BUSY(void)
{
    if (w25qxx.is_init == false)
        return;

    BSP_W25Qxx_CS_Enable();

    uint8_t cmd = CMD_W25Qxx_READ_SR1;
    uint8_t SR1 = 0xFF;
    HAL_StatusTypeDef state = HAL_OK;

    state = HAL_SPI_Transmit(w25qxx.hspi1, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> Wait_Write's HAL_SPI_Transmit Error <<\n");
        goto EXIT;
    }

    do
    {
        state = HAL_SPI_Receive(w25qxx.hspi1, &SR1, sizeof(SR1), HAL_MAX_DELAY);
        if (state != HAL_OK)
        {
            printf(">> Wait_Write's HAL_SPI_Receive Error <<\n");
            goto EXIT;
        }
    } while (SR1 & W25Qxx_WRITE_BUSY_BIT);

EXIT:
    BSP_W25Qxx_CS_Disable();
}

void BSP_W25Qxx_Sector_Erase(uint32_t sector_address)
{
    if (w25qxx.is_init == false) return;

    if (sector_address & W25Qxx_SECTOR_ADDRESS_MASK)
    {
        printf(">> Sector Erase Error: address(0x%x) is not sector aligned <<\n", sector_address);
        return;
    }

    if (sector_address > w25qxx.mem_total_size)
    {
        printf(">> Sector Erase Error: address(0x%x) is out of flash max size range <<\n", sector_address);
        return;
    }

    uint8_t cmd[4] = {CMD_W25Qxx_SECTOR_ERASE, (sector_address >> 16), (sector_address >> 8), (sector_address)};
    HAL_StatusTypeDef state = HAL_OK;

    if (BSP_W25Qxx_WEL_Is_Enable() == false) BSP_W25Qxx_Write_Enable();

    BSP_W25Qxx_CS_Enable();

    state = HAL_SPI_Transmit(w25qxx.hspi1, cmd, sizeof(cmd)/sizeof(cmd[0]), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> Sector_Erase's HAL_SPI_Transmit Error <<\n");
        goto EXIT;
    }

EXIT:
    BSP_W25Qxx_CS_Disable();
    if (state != HAL_OK) return;
    BSP_W25Qxx_Wait_BUSY();
}

bool BSP_W25Qxx_Read_Data(uint32_t data_address, uint8_t *rx_buffer, uint32_t rx_size)
{
    if (w25qxx.is_init == false) return false;

    if (rx_buffer == NULL || rx_size == 0) return false;

    if (BSP_W25Qxx_Write_Is_Busy()) BSP_W25Qxx_Wait_BUSY();

    uint8_t cmd[4] = {CMD_W25Qxx_READ_DATA, (data_address >> 16), (data_address >> 8), (data_address)};
    HAL_StatusTypeDef state = HAL_OK;
    bool ret = true;

    BSP_W25Qxx_CS_Enable();

    state = HAL_SPI_Transmit(w25qxx.hspi1, cmd, sizeof(cmd)/sizeof(cmd[0]), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        ret = false;
        printf(">> Read_Data's HAL_SPI_Transmit Error <<\n");
        goto EXIT;
    }

    state = HAL_SPI_Receive(w25qxx.hspi1, rx_buffer, rx_size, HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        ret = false;
        printf(">> Read_Data's HAL_SPI_Receive Error <<\n");
        goto EXIT;
    }

EXIT:
    BSP_W25Qxx_CS_Disable();
    if (ret == true) printf(">> Read Data Success <<\n");
    else printf(">> Read Data Error <<\n");
    return ret;
}

bool BSP_W25Qxx_Page_Program(uint32_t page_address, uint16_t page_offset, uint8_t *wx_buffer, uint32_t wx_size)
{
    if (w25qxx.is_init == false) return false;
    if (wx_buffer == NULL || wx_size == 0) return false;
    if (wx_size + page_offset > w25qxx.page_size) 
    {
        printf(">> Page Program Error: address_total(0x%x) is out of range <<\n", page_address);
        return false;
    }

    uint8_t cmd[4] = {CMD_W25Qxx_PAGE_PROGRAM, (page_address >> 16), (page_address >> 8), (page_address)};
    HAL_StatusTypeDef state = HAL_OK;
    bool ret = true;

    if (BSP_W25Qxx_WEL_Is_Enable() == false) BSP_W25Qxx_Write_Enable();

    BSP_W25Qxx_CS_Enable();

    state = HAL_SPI_Transmit(w25qxx.hspi1, cmd, sizeof(cmd)/sizeof(cmd[0]), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        ret = false;
        printf(">> Page_Program's HAL_SPI_Transmit_cmd Error <<\n");
        goto EXIT;
    }

    state = HAL_SPI_Transmit(w25qxx.hspi1, wx_buffer, wx_size, HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        ret = false;
        printf(">> Page_Program's HAL_SPI_Transmit_wx_buffer Error <<\n");
        goto EXIT;
    }

EXIT:
    BSP_W25Qxx_CS_Disable();
    if (ret == true) printf(">> Page_Program Success <<\n");
    BSP_W25Qxx_Wait_BUSY();
    return ret;
}

bool BSP_W25Qxx_Write_Data(uint32_t page_address, uint8_t *wx_buffer, uint32_t wx_size)
{
    if (w25qxx.is_init == false) return false;

    if (wx_buffer == NULL || wx_size == 0) return false;

    if (page_address + wx_size > w25qxx.mem_total_size)
    {
        printf(">> Write Data Error: address_total(0x%x) is out of range <<\n", page_address);
        return false;
    }

    uint32_t written_byte = 0;
    bool ret = false;

    while (written_byte < wx_size)
    {
        uint32_t remaining_data  = wx_size - written_byte;
        uint32_t page_offset     = page_address % w25qxx.page_size;
        uint32_t remaining_space = w25qxx.page_size - page_offset;
        uint32_t to_write        = (remaining_data > remaining_space) ? remaining_space : remaining_data;

        /* 第一个参数要传绝对地址（Page_Program 内部直接拼成 24 位地址发给 Flash） */
        ret = BSP_W25Qxx_Page_Program(page_address, (uint16_t)page_offset,
                                      &wx_buffer[written_byte], to_write);
        if (ret == false) break;

        written_byte += to_write;
        page_address += to_write;
    }
    return ret;
}