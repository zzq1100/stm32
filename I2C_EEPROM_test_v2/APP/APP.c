#include "App.h"

// 全局配置变量
config_t config;

static int Request_Input(const char *message)
{
    printf("%s\n", message);
    int choice = 0;
    if (scanf("%d", &choice) == 1)
    {
        return choice;
    }
    return EOF;
}

static void Config_Display()
{
    printf("\n\n<<----------配置信息---------->>\n");
    printf("魔数: 0x%02x\n", config.magic);
    printf("版本: 0x%02x\n", config.version);
    if (config.led == 0)
        printf("亮灯: [1-4] 所有灯\n");
    else
        printf("亮灯: %d 号灯\n", config.led);
    printf("闪灯次数: %d\n", config.repeat);
    printf("闪灯间隔: %d ms\n", config.delay_ms);
    printf("开始闪灯时间: %d ms\n\n", config.start_time);
}

void App_Init()
{
    int choice = Request_Input("\n\n请选择配置选项: 1.重新进行配置 2.使用原有配置");
    while (choice != 1 && choice != 2)
    {
        choice = Request_Input("\n\n【!!!选项错误!!!】\n请重新选择配置选项: 1.重新进行配置 2.使用原有配置");
    }
    if (choice == 1)
    {
        config.magic = CONFIG_MAGIC;
        config.version = CONFIG_VERSION;
        config.led = (uint8_t)Request_Input("\n\n选择led灯: [1-4]号灯 [0]全选");
        config.repeat = (uint8_t)Request_Input("选择闪灯次数: ");
        config.delay_ms = (uint16_t)Request_Input("选择闪灯间隔(ms): ");
        config.start_time = (uint16_t)Request_Input("选择开始闪灯时间(ms): ");

        // 展示配置信息
        Config_Display();

        // 写入到EEPROM
        SAVE_CONFIG(&config, sizeof(config));
        printf("\n>> write config done <<\n");
    }
    else if (choice == 2)
    {
        // 读取到EEPROM
        LOAD_CONFIG(&config, sizeof(config));
        printf("\n>> read config done <<\n");
    }
}

static void Show_StartTime(uint16_t time)
{
    for (uint16_t i = time / 1000; i > 0; i--)
    {
        printf("%d s后闪灯\n", i);
        HAL_Delay(1000);
    }
}

void App_Run()
{
    if (config.magic == CONFIG_MAGIC && config.version == CONFIG_VERSION)
    {
        printf(">> %d ms后开始闪灯 <<\n", (int)config.start_time);
        //HAL_Delay(config.start_time);
        Show_StartTime(config.start_time);
        printf("-----> 闪灯中 <-----\n");
        LED_Blink2((LED_t)config.led, config.repeat, config.delay_ms);
        printf("-----> 闪灯结束 <-----\n");
    }
    
}