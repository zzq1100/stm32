#ifndef APP_H
#define APP_H

/* main.c的固定入口。Phase2/3目标各链接一个Tests入口；
 * LQB17_Final链接BSP_Drivers/Src/app.c，三个App不得同时链接。 */
void App_Init(void);
void App_Run(void);

#endif
