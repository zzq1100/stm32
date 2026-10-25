#ifndef BSP_DISPLAY_H
#define BSP_DISPLAY_H
#include "monitor.h"
void Display_Init(void);
void Display_Compose(const Monitor *m);
void Display_Service(void);
#endif
