#include "BSP_W25Q64.h"
#include "BSP_LED.h"

spi_flash_t g_w25qxx;
static bool g_is_init = false;
const uint8_t dummy = 0x00;

// GPIOE, PIN6, 拉低片选
static void BSP_W25Qxx_CS_Enable(void)
{
    if (!g_is_init)
        return;
    HAL_GPIO_WritePin(g_w25qxx.gpio_port, g_w25qxx.nss_pin, GPIO_PIN_RESET);
}

static void BSP_W25Qxx_CS_Disable(void)
{
    if (!g_is_init)
        return;
    HAL_GPIO_WritePin(g_w25qxx.gpio_port, g_w25qxx.nss_pin, GPIO_PIN_SET);
}

static bool BSP_W25Qxx_Is_busy(void)
{
    return BSP_W25Qxx_Statue1_Reg() & W25Qxx_STATUS_BUSY;
}

static bool BSP_W25Qxx_Is_Wel_Enable(void)
{
    return BSP_W25Qxx_Statue1_Reg() & W25Qxx_STATUS_WEL;
}

bool BSP_W25Qxx_Init(spi_flash_t *init)
{
    if (init == NULL)
        return g_is_init;
    g_w25qxx.page_size = init->page_size;
    g_w25qxx.sector_size = init->sector_size;
    g_w25qxx.block_size = init->block_size;
    g_w25qxx.mem_total_size = init->mem_total_size;
    g_w25qxx.chip_id = init->chip_id;
    g_w25qxx.hspi = init->hspi;
    g_w25qxx.gpio_port = init->gpio_port;
    g_w25qxx.nss_pin = init->nss_pin;
    g_is_init = true;

    // TODO
    uint32_t id = BSP_W25Qxx_ID(); // 获取SPI flash内部的id
    if (g_w25qxx.chip_id != id)
    {
        g_is_init = false;
        printf(">> BSP_W25Qxx_Init error, ID error, set ID is : 0x%x, current ID is: 0x%x\n", g_w25qxx.chip_id, id);
        return g_is_init;
    }
    printf(">> BSP_W25Qxx_Init success! current ID is: 0x%x\n", id);
    return g_is_init;
}

uint32_t BSP_W25Qxx_ID(void)
{
    if (!g_is_init)
        return 0x0;
    BSP_W25Qxx_CS_Enable();

    uint8_t cmd = CMD_W25QXX_JEDEC_ID;
    uint8_t id[3] = {0};
    HAL_StatusTypeDef state = HAL_SPI_Transmit(g_w25qxx.hspi, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> BSP_W25Qxx_ID, HAL_SPI_Transmit error, cmd is : 0x%x failed, state: 0x%x\n", cmd, state);
        memset(id, 0, sizeof(id));
        goto END;
    }
    state = HAL_SPI_Receive(g_w25qxx.hspi, id, sizeof(id), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> BSP_W25Qxx_ID, HAL_SPI_Receive error, cmd is : 0x%x failed, state: 0x%x\n", cmd, state);
        memset(id, 0, sizeof(id));
        goto END;
    }
END:
    BSP_W25Qxx_CS_Disable();
    return ((id[0] << 16) | (id[1] << 8) | (id[2])); // 0000 0000 0000 0000 0000 0000 0000 0000
}

uint8_t BSP_W25Qxx_Statue1_Reg(void)
{
    if (!g_is_init)
        return 0xFF;
    uint8_t cmd = CMD_W25QXX_READ_STATUS_REG1;
    uint8_t status_reg = 0xFF;
    BSP_W25Qxx_CS_Enable();
    HAL_StatusTypeDef state = HAL_SPI_Transmit(g_w25qxx.hspi, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> BSP_W25Qxx_Statue1_Reg HAL_SPI_Transmit error, cmd is : 0x%x failed, state: 0x%x\n", cmd, state);
        goto END;
    }
    state = HAL_SPI_Receive(g_w25qxx.hspi, &status_reg, sizeof(status_reg), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        status_reg = 0xFF;
        printf(">> BSP_W25Qxx_Statue1_Reg HAL_SPI_Receive error, cmd is : 0x%x failed, state: 0x%x\n", cmd, state);
        goto END;
    }

END:
    BSP_W25Qxx_CS_Disable();
    return status_reg;
}

void BSP_W25Qxx_Write_Enable(void)
{
    if (!g_is_init)
        return;
    uint8_t cmd = CMD_W25QXX_WRITE_ENABLE;
    BSP_W25Qxx_CS_Enable();
    HAL_StatusTypeDef state = HAL_SPI_Transmit(g_w25qxx.hspi, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> BSP_W25Qxx_Write_Enable HAL_SPI_Transmit error, cmd is : 0x%x failed, state: 0x%x\n", cmd, state);
    }
    BSP_W25Qxx_CS_Disable();
}

void BSP_W25Qxx_Write_Disable(void)
{
    if (!g_is_init)
        return;
    uint8_t cmd = CMD_W25QXX_WRITE_DISABLE;
    BSP_W25Qxx_CS_Enable();
    HAL_StatusTypeDef state = HAL_SPI_Transmit(g_w25qxx.hspi, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> BSP_W25Qxx_Write_Enable HAL_SPI_Transmit error, cmd is : 0x%x failed, state: 0x%x\n", cmd, state);
    }
    BSP_W25Qxx_CS_Disable();
}

void BSP_W25Qxx_Waitfor_Write_Complete(void)
{
    if (!g_is_init)
        return;
    // while (BSP_W25Qxx_Statue1_Reg() & W25Qxx_STATUS_BUSY)
    //     ;

    int cnt  = 0;
    printf(">> 整盘擦除中\n");
    while (BSP_W25Qxx_Statue1_Reg() & W25Qxx_STATUS_BUSY)
    {
        printf(".");
        cnt++;
        LED_Blink(LED1, 1, 500);
    }
    printf("\n>> 整盘擦除完成, 花费时间: %dS\n", cnt);
}

void BSP_W25Qxx_Waitfor_Write_Complete_V2(void)
{
    if (!g_is_init)
        return;
    uint8_t cmd = CMD_W25QXX_READ_STATUS_REG1;
    uint8_t reg1 = 0xFF;
    BSP_W25Qxx_CS_Enable();
    HAL_StatusTypeDef state = HAL_SPI_Transmit(g_w25qxx.hspi, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> BSP_W25Qxx_Waitfor_Write_Complete_V2 HAL_SPI_Transmit error, cmd is : 0x%x failed, state: 0x%x\n", cmd, state);
        goto END;
    }
    do
    {
        HAL_SPI_TransmitReceive(g_w25qxx.hspi, &dummy, &reg1, sizeof(dummy), HAL_MAX_DELAY);
    } while (reg1 & W25Qxx_STATUS_BUSY);
END:
    BSP_W25Qxx_CS_Disable();
}

void BSP_W25Qxx_Sector_Erase(uint32_t sector_address)
{
    if (!g_is_init)
        return;
    if (sector_address & SECTOR_ADDRESS_MASK)
    {
        printf(">>> BSP_W25Qxx_Sector_Erase error, address 0x%x not sector aligned\n", sector_address);
        return;
    }
    if (sector_address >= g_w25qxx.mem_total_size)
    {
        printf(">>> BSP_W25Qxx_Sector_Erase error, address 0x%x out of range max flash size\n", sector_address);
        return;
    }
    // cmd[0] = CMD_W25QXX_SECTOR_ERASE;
    // cmd[1] = (sector_address>>16)
    // 1. 检测WEL位是否被设置1，如果为0才需要设置 2. 设置
    if (!BSP_W25Qxx_Is_Wel_Enable())
    {
        BSP_W25Qxx_Write_Enable();
    }

    uint8_t cmd[4] = {CMD_W25QXX_SECTOR_ERASE, (sector_address >> 16), (sector_address >> 8), sector_address};
    BSP_W25Qxx_CS_Enable();
    HAL_StatusTypeDef state = HAL_SPI_Transmit(g_w25qxx.hspi, cmd, sizeof(cmd) / sizeof(cmd[0]), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> BSP_W25Qxx_Sector_Erase HAL_SPI_Transmit error, cmd is : 0x%x failed, state: 0x%x\n", cmd[0], state);
        goto END;
    }
    printf(">> BSP_W25Qxx_Sector_Erase success\n"); // 擦除其实没有成功！！！！
END:
    BSP_W25Qxx_CS_Disable();

    if (state == HAL_OK)
        BSP_W25Qxx_Waitfor_Write_Complete_V2();
}

void BSP_W25Qxx_Chip_Erase(void)
{
    if (!g_is_init)
        return;

    // 1. 检测WEL位是否被设置1，如果为0才需要设置 2. 设置
    if (!BSP_W25Qxx_Is_Wel_Enable())
    {
        BSP_W25Qxx_Write_Enable();
    }

    // 正片擦除的逻辑
    uint8_t cmd = CMD_W25QXX_CHIP_ERASE;
    BSP_W25Qxx_CS_Enable();
    HAL_StatusTypeDef state = HAL_SPI_Transmit(g_w25qxx.hspi, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        printf(">> BSP_W25Qxx_Chip_Erase HAL_SPI_Transmit error, cmd is : 0x%x failed, state: 0x%x\n", cmd, state);
    }
    BSP_W25Qxx_CS_Disable();
    if (state == HAL_OK)
        BSP_W25Qxx_Waitfor_Write_Complete();
}

bool BSP_W25Qxx_Read_Data(uint32_t read_address, uint8_t *rbuffer, uint32_t rsize)
{
    if (!g_is_init)
        return false;
    if (rbuffer == NULL || rsize == 0)
        return false;
    if (read_address + rsize > g_w25qxx.mem_total_size)
        return false;

    bool ret = true;
    // 1. 检测W25qxx是否在忙 2. 等待忙完
    if (BSP_W25Qxx_Is_busy())
        BSP_W25Qxx_Waitfor_Write_Complete_V2();

    uint8_t cmd[] = {CMD_W25QXX_READ_DATA, (uint8_t)(read_address >> 16), (uint8_t)(read_address >> 8), (uint8_t)(read_address)};
    uint16_t cmdlen = sizeof(cmd) / sizeof(cmd[0]);

    // 2. 发送
    BSP_W25Qxx_CS_Enable();
    HAL_StatusTypeDef state = HAL_SPI_Transmit(g_w25qxx.hspi, cmd, cmdlen, HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        ret = false;
        printf(">> BSP_W25Qxx_Read_Data HAL_SPI_Transmit error, cmd is : 0x%x failed, state: 0x%x\n", cmd[0], state);
        goto END;
    }

    state = HAL_SPI_Receive(g_w25qxx.hspi, rbuffer, (uint16_t)rsize, HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        ret = false;
        printf(">> BSP_W25Qxx_Read_Data HAL_SPI_Receive error, cmd is : 0x%x failed, state: 0x%x\n", cmd[0], state);
        goto END;
    }
    printf(">> BSP_W25Qxx_Read_Data success, cmd is : 0x%x, state: 0x%x\n", cmd[0], state);

END:
    BSP_W25Qxx_CS_Disable();
    return ret;
}

// 这个是对页进行写入的函数
bool BSP_W25Qxx_Page_Program(uint32_t page_address, uint16_t page_inner_offset, uint8_t *wbuffer, uint16_t wsize)
{
    if (!g_is_init)
        return false;
    if (wbuffer == NULL || wsize == 0)
        return false;
    // 不能超过一页大小
    if (page_inner_offset + wsize > g_w25qxx.page_size)
        return false;
    // 真实的写入地址
    uint32_t write_address = page_address * g_w25qxx.page_size + page_inner_offset;

    bool ret = true;

    // 构建指令
    uint8_t cmd[] = {CMD_W25QXX_PAGE_PROGRAM, (uint8_t)(write_address >> 16), (uint8_t)(write_address >> 8), (uint8_t)(write_address)};
    uint16_t cmdlen = sizeof(cmd) / sizeof(cmd[0]);

    // 发送指令，首选需要关闭禁止写入
    // 1. 检测WEL位是否被设置1，如果为0才需要设置 2. 设置
    if (!BSP_W25Qxx_Is_Wel_Enable())
    {
        BSP_W25Qxx_Write_Enable();
    }

    BSP_W25Qxx_CS_Enable();

    HAL_StatusTypeDef state = HAL_SPI_Transmit(g_w25qxx.hspi, cmd, cmdlen, HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        ret = false;
        printf(">> BSP_W25Qxx_Page_Program HAL_SPI_Transmit cmd error, cmd is : 0x%x failed, state: 0x%x\n", cmd[0], state);
        goto END;
    }

    // 发送数据
    state = HAL_SPI_Transmit(g_w25qxx.hspi, wbuffer, wsize, HAL_MAX_DELAY);
    if (state != HAL_OK)
    {
        ret = false;
        printf(">> BSP_W25Qxx_Page_Program HAL_SPI_Transmit data error, cmd is : 0x%x failed, state: 0x%x\n", cmd[0], state);
        goto END;
    }
    printf(">> BSP_W25Qxx_Page_Program HAL_SPI_Transmit cmd && data success, cmd is : 0x%x, state: 0x%x\n", cmd[0], state);

END:
    BSP_W25Qxx_CS_Disable();
    if (state == HAL_OK)
        BSP_W25Qxx_Waitfor_Write_Complete_V2();
    return ret;
}

bool BSP_W25Qxx_Write_Data(uint32_t write_address, uint8_t *wbuffer, uint32_t wsize)
{
    if (!g_is_init)
        return false;
    if (wbuffer == NULL || wsize == 0)
        return false;
    if (write_address + wsize > g_w25qxx.mem_total_size)
    {
        printf("BSP_W25Qxx_Write_Data error, address out range!\n");
        return false;
    }

    // 已经写入的字节数
    uint32_t written_bytes = 0;

    while (written_bytes < wsize)
    {
        // 计算剩余数据
        uint32_t remaining_data = wsize - written_bytes;

        // 计算写入的页号
        uint16_t page_number = write_address / g_w25qxx.page_size;
        // 计算页内偏移
        uint16_t page_inner_offset = write_address % g_w25qxx.page_size;
        // 当前页的剩余空间
        uint16_t remaining_space = g_w25qxx.page_size - page_inner_offset;

        // 计算本次写入的数据量
        uint16_t to_written = remaining_data > remaining_space ? remaining_space : remaining_data;

        // 进行页编程
        bool ret = BSP_W25Qxx_Page_Program(page_number, page_inner_offset, &wbuffer[written_bytes], to_written);
        (void)ret;

        // 更新已经写入的数据量
        written_bytes += to_written;
        // 更新当前写入地址
        write_address += to_written;
    }
    return true;
}