#ifndef MONITOR_TYPES_H
#define MONITOR_TYPES_H
#include <stdint.h>

/* 公共类型先约定单位；目标整数%与实际输出0.01%不能混用。 */
typedef enum { MONITOR_MAIN, MONITOR_OUTP, MONITOR_PARA } MonitorPage;
typedef enum { MONITOR_MAN, MONITOR_AUTO } MonitorMode;
typedef struct {
    uint16_t tar_d_pct;           /* TarD：整数百分比，5~95，默认5。 */
    uint16_t tar_p_deci;          /* TarP：0.1BAR，10~95，默认50。 */
} OutputConfig;
typedef struct {
    uint16_t fh_deci, fl_deci;    /* FH/FL：0.1L/min，40~400。 */
    uint16_t pl_deci;             /* PL：0.1BAR，10~95。 */
    uint16_t dh_pct;              /* DH：整数百分比，5~95。 */
} SafetyConfig;
typedef struct {
    float voltage_v, offset_v, pressure_bar;
    float frequency_hz, flow_lpm;
    uint8_t pressure_valid;       /* ADC失败不能当作真实低压力控制。 */
    uint8_t capture_valid;        /* 无信号/无有效周期时F按0处理。 */
} MonitorMeasurements;
#endif
