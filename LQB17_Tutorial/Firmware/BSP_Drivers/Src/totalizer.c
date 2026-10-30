#include "totalizer.h"
#include <float.h>

void Totalizer_Clear(Totalizer *q)
{
    q->litres = 0U;
    q->remainder = 0U;            /* 清零必须同时清掉不足一升的余量。 */
}
void Totalizer_Sample100ms(Totalizer *q, float flow_lpm)
{
    uint32_t addition;
    if (!(flow_lpm >= 0.0f && flow_lpm <= FLT_MAX)) return;
    if (q->litres == UINT32_MAX) { q->remainder = 0U; return; }
    if (flow_lpm > 40.0f) flow_lpm = 40.0f;
    /* ΔQ=F/600 L。F先量化到百万分之一L/min，保留小余量，
     * 因此600000000个内部单位才进1L。double仅用于本次有界换算。 */
    addition = (uint32_t)((double)flow_lpm * 1000000.0 + 0.5);
    q->remainder += addition;    /* <600000000+40000000，不会uint32溢出。 */
    if (q->remainder >= TOTALIZER_UNITS_PER_LITRE) {
        q->remainder -= TOTALIZER_UNITS_PER_LITRE;
        ++q->litres;              /* 单次增量<=1/15L，只需进一升。 */
        if (q->litres == UINT32_MAX) q->remainder = 0U;
    }
}
