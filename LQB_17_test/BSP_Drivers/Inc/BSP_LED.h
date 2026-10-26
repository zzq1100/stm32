#ifndef BSP_LED_H
#define BSP_LED_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include "main.h"

void BSP_SetLEDs(uint8_t mask);


#ifdef __cplusplus
}
#endif

#endif
