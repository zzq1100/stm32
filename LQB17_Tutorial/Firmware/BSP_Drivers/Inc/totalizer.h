#ifndef TOTALIZER_H
#define TOTALIZER_H
#include <stdint.h>
#define TOTALIZER_UNITS_PER_LITRE 600000000UL

typedef struct {
    uint32_t litres;              /* 整升，不与小增量一起保存在float中。 */
    uint32_t remainder;           /* 余量单位1/600000000 L，始终不足一升。 */
} Totalizer;
void Totalizer_Clear(Totalizer *q);
/* 每100ms一个真实采样槽；输入L/min，>40限幅，负值/NaN忽略。 */
void Totalizer_Sample100ms(Totalizer *q, float flow_lpm);
#endif
