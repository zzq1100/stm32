#include "App.h"

// 全局配置变量
config_t gconfig;

static void Config_Display(void)
{
    printf(">>-----------配置清单---------------<<\n");
    printf("config.magic: 0x%x\n", gconfig.magic);
    printf("config.version: 0x%x\n", gconfig.version);
    printf("config.led_id: %d\n", gconfig.led_id);
    printf("config.repeat_cnt: %d\n", gconfig.repeat_cnt);
    printf("config.delay_ms: %d\n", gconfig.delay_ms);
    printf("config.start_time: %d\n", gconfig.start_time);
}

// ret == EOF: 读取失败
static int Request_Input(const char *message)
{
    int value = 0;
    printf(">> %s\n", message);
    if (scanf("%d", &value) == 1)
    {
        return value;
    }
    printf("[!] 输入无效, 请重新尝试!\n");
    return EOF;
}

void App_Init(void)
{
    // 获取用户的选择 1. 重新配置参数 2. 使用现有的配置
    int choice = Request_Input("1. 重新配置参数 2. 使用现有的配置");
    if (choice == 1)
    {
        gconfig.magic = APP_MAGIC;
        gconfig.version = APP_VERSION;
        gconfig.led_id = (uint8_t)Request_Input("step1: 选择LED[1,4], 5表示全选>> ");
        gconfig.repeat_cnt = (uint8_t)Request_Input("step2: 设置闪灯次数, [1,255]>> ");
        gconfig.delay_ms = (uint16_t)Request_Input("step3: 设置闪灯间隔,单位ms>> ");
        gconfig.start_time = (uint16_t)Request_Input("step4: 设置开始时间,单位ms>> ");

        Config_Display();

        // 写入到EEPROM
        //SAVE_CONFIG(&gconfig, sizeof(gconfig)); // AT24C02写入
        //printf(">> write config done <<\n");
    }
    else if (choice == 2)
    {
        // 读取EEPROM
        //LOAD_CONFIG(&gconfig, sizeof(gconfig));
        //printf(">> load config done <<\n");
    }
    else
    {
        printf("[!] 输入无效, 选项只有1 || 2!\n");
    }
}
void App_Run(void)
{
    // if(gconfig.magic == APP_MAGIC && gconfig.version == APP_VERSION)
    // {
    //     printf(">> %d ms之后, 开始闪灯\n", (int)(gconfig.start_time));
    //     HAL_Delay(gconfig.start_time);
    //     printf(">> 闪灯中...\n");
    //     LED_Blink((LED_t)(gconfig.led_id), gconfig.repeat_cnt, gconfig.delay_ms);
    // }
}