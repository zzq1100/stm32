/* 直接编译Firmware中的模块；只用替身模拟GPIO、ADC和TIM。
 * 能验证事件/换算/快照/异常路径，不能验证电气波形与ADC实际精度。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "test_hal.h"
#include "bsp_keys.h"
#include "bsp_adc.h"
#include "measurement.h"
#include "bsp_capture.h"
#include "bsp_pwm.h"

GPIO_TypeDef fake_gpio_a, fake_gpio_b;
ADC_HandleTypeDef hadc2;
TIM_TypeDef fake_tim2, fake_tim3;
TIM_HandleTypeDef htim2 = { &fake_tim2, {0U, 7999U}, 0U };
TIM_HandleTypeDef htim3 = { &fake_tim3, {79U, 65535U}, HAL_TIM_ACTIVE_CHANNEL_1 };
static uint32_t tick, primask, adc_raw, adc_stops, adc_starts;
static HAL_StatusTypeDef adc_cal_status, adc_start_status, adc_poll_status, adc_stop_status;
static HAL_StatusTypeDef capture_start_status, pwm_start_status;

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin) { return (port->IDR & pin) ? GPIO_PIN_SET : GPIO_PIN_RESET; }
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *h, uint32_t mode) { assert(h==&hadc2 && mode==ADC_SINGLE_ENDED); return adc_cal_status; }
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *h) { assert(h==&hadc2); ++adc_starts; return adc_start_status; }
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *h, uint32_t timeout) { assert(h==&hadc2 && timeout==1U); return adc_poll_status; }
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h) { assert(h==&hadc2); return adc_raw; }
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *h) { assert(h==&hadc2); ++adc_stops; return adc_stop_status; }
void FakeSetCompare(TIM_HandleTypeDef *h, uint32_t channel, uint32_t value) { assert(h==&htim2 && channel==TIM_CHANNEL_2); h->Instance->CCR2=value; }
uint32_t FakeGetCompare(TIM_HandleTypeDef *h, uint32_t channel) { assert(h==&htim2 && channel==TIM_CHANNEL_2); return h->Instance->CCR2; }
HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef *h, uint32_t channel) { assert(h==&htim3 && channel==TIM_CHANNEL_1); return capture_start_status; }
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *h, uint32_t channel) { assert(h==&htim2 && channel==TIM_CHANNEL_2); return pwm_start_status; }
uint32_t HAL_TIM_ReadCapturedValue(TIM_HandleTypeDef *h, uint32_t channel) { assert(h==&htim3 && channel==TIM_CHANNEL_1); return h->Instance->CCR1; }
uint32_t HAL_GetTick(void) { return tick; }
uint32_t __get_PRIMASK(void) { return primask; }
void __disable_irq(void) { primask=1U; }
void __set_PRIMASK(uint32_t value) { primask=value; }

static void near_value(float actual, float expected, float tolerance)
{
    assert(fabsf(actual-expected) <= tolerance);
}
static void keys_raw(uint8_t mask)
{
    fake_gpio_b.IDR=(uint32_t)(~mask) & 7U;
    fake_gpio_a.IDR=(mask & 8U) ? 0U : 1U;
}
static uint8_t scan(BSP_Keys *keys, BSP_KeyEvent *e, uint8_t mask, uint32_t now)
{
    keys_raw(mask); return BSP_Keys_Scan(keys,now,e);
}
static void test_keys(void)
{
    BSP_Keys keys;
    BSP_KeyEvent e[4];
    uint32_t t, held;
    unsigned i;
    keys_raw(0U); BSP_Keys_Init(&keys,0U);
    assert(!scan(&keys,e,4U,5U));
    assert(!scan(&keys,e,0U,10U));
    assert(!scan(&keys,e,4U,15U));
    assert(!scan(&keys,e,4U,30U));
    assert(scan(&keys,e,4U,35U)==1U && e[0].key==3U && e[0].type==BSP_KEY_PRESS);
    for(t=40U;t<=2010U;t+=5U) assert(!scan(&keys,e,4U,t));
    assert(!scan(&keys,e,0U,2015U));
    assert(scan(&keys,e,0U,2035U)==1U && e[0].type==BSP_KEY_RELEASE && e[0].held_ms==2000U);
    assert(!scan(&keys,e,0U,2040U));
    /* P05-09：不足/恰好/超过2秒；按住时均不伪造松开事件。 */
    for(held=1995U;held<=2005U;held+=5U) {
        keys_raw(0U); BSP_Keys_Init(&keys,0U);
        assert(!scan(&keys,e,4U,100U));
        assert(scan(&keys,e,4U,120U)==1U);
        assert(!scan(&keys,e,4U,100U+held));
        assert(!scan(&keys,e,0U,100U+held));
        assert(scan(&keys,e,0U,120U+held)==1U && e[0].held_ms==held);
    }
    keys_raw(15U); BSP_Keys_Init(&keys,0U);
    assert(!scan(&keys,e,15U,3000U));
    assert(!scan(&keys,e,0U,3010U));
    assert(!scan(&keys,e,0U,3030U)); /* 开机按住再松开，不算有效长按。 */
    assert(!scan(&keys,e,15U,3100U));
    assert(scan(&keys,e,15U,3120U)==4U);
    for(i=0U;i<4U;++i) assert(e[i].key==i+1U && e[i].type==BSP_KEY_PRESS);
    assert(BSP_Keys_PressedMask(&keys)==15U);
    assert(!scan(&keys,e,0U,3200U));
    assert(scan(&keys,e,0U,3220U)==4U && BSP_Keys_PressedMask(&keys)==0U);
    keys_raw(0U); BSP_Keys_Init(&keys,0xFFFFFF00U);
    assert(!scan(&keys,e,8U,0xFFFFFF10U));
    assert(scan(&keys,e,8U,0xFFFFFF24U)==1U);
    assert(!scan(&keys,e,0U,(uint32_t)(0xFFFFFF10U+2000U)));
    assert(scan(&keys,e,0U,(uint32_t)(0xFFFFFF24U+2000U))==1U && e[0].held_ms==2000U);
    puts("PASS KEYS: bounce, held/no-repeat, release duration, 2s boundaries, startup held, four keys, tick wrap");
}

static void test_adc_measurement(void)
{
    BSP_ADC_Sample s;
    float offset=0.3f;
    uint32_t before;
    assert(BSP_ADC_Init());
    adc_raw=0U; assert(BSP_ADC_Read(&s) && s.valid && s.raw==0U); near_value(s.voltage_v,0.0f,0.0f);
    adc_raw=4095U; assert(BSP_ADC_Read(&s)); near_value(s.voltage_v,3.3f,0.0f);
    assert(!Measurement_Calibrate(s.voltage_v,&offset) && offset==0.3f);
    adc_raw=2048U; assert(BSP_ADC_Read(&s)); near_value(s.voltage_v,1.650403f,0.00001f);
    adc_poll_status=HAL_TIMEOUT; before=adc_stops;
    assert(!BSP_ADC_Read(&s) && !s.valid && s.raw==0U && s.voltage_v==0.0f && adc_stops==before+1U);
    adc_poll_status=HAL_OK; adc_start_status=HAL_ERROR; assert(!BSP_ADC_Read(&s));
    adc_start_status=HAL_OK; adc_stop_status=HAL_ERROR; assert(!BSP_ADC_Read(&s) && !s.valid);
    adc_stop_status=HAL_OK; adc_raw=4096U; assert(!BSP_ADC_Read(&s));
    adc_cal_status=HAL_ERROR; assert(!BSP_ADC_Init()); before=adc_starts;
    assert(!BSP_ADC_Read(&s) && adc_starts==before);
    adc_cal_status=HAL_OK; assert(BSP_ADC_Init()); assert(!BSP_ADC_Read(0));
    near_value(Measurement_Pressure(1.65f,0.0f),5.0f,0.00001f);
    assert(Measurement_Calibrate(0.3f,&offset));
    near_value(Measurement_Pressure(1.8f,offset),5.0f,0.00001f);
    near_value(Measurement_Pressure(0.2f,offset),0.0f,0.0f);
    near_value(Measurement_Pressure(3.3f,offset),10.0f,0.0f);
    assert(!Measurement_Calibrate(3.3f,&offset) && offset==0.3f);
    assert(!Measurement_Calibrate(NAN,&offset) && !Measurement_Calibrate(INFINITY,&offset));
    assert(!Measurement_Calibrate(-1.0f,&offset) && !Measurement_Calibrate(0.2f,0));
    near_value(Measurement_Pressure(NAN,offset),0.0f,0.0f);
    near_value(Measurement_Pressure(2.0f,3.3f),0.0f,0.0f);
    near_value(Measurement_Flow(799.99f),0.0f,0.0f);
    near_value(Measurement_Flow(800.0f),4.0f,0.0f);
    near_value(Measurement_Flow(8000.0f),40.0f,0.0f);
    assert(!Measurement_Overrange(8000.0f) && Measurement_Overrange(8000.1f));
    near_value(Measurement_Flow(9000.0f),40.0f,0.0f);
    near_value(Measurement_Flow(NAN),0.0f,0.0f);
    assert(!Measurement_Overrange(INFINITY));
    puts("PASS ADC/MEASUREMENT: endpoints, failure validity, timeout cleanup, calibration slope/rejection, 800/8000 bounds");
}

static void edge(uint32_t micros, uint16_t base_count, uint32_t base_ms)
{
    tick=base_ms+micros/1000U;
    fake_tim3.CCR1=(uint16_t)(base_count+micros);
    htim3.Channel=HAL_TIM_ACTIVE_CHANNEL_1;
    HAL_TIM_IC_CaptureCallback(&htim3);
}
static void stream(uint16_t period, uint16_t base_count, uint32_t base_ms)
{
    unsigned i;
    assert(BSP_Capture_Init());
    for(i=0U;i<=16U;++i) edge(i*period,base_count,base_ms);
}
static void test_capture(void)
{
    BSP_CaptureSample s;
    uint32_t last;
    unsigned i;
    assert(BSP_Capture_Init()); BSP_Capture_Read(&s); assert(!s.valid);
    edge(0U,0U,0U); BSP_Capture_Read(&s); assert(!s.valid);
    stream(1250U,0U,0U); BSP_Capture_Read(&s);
    assert(s.valid && s.period_ticks==20000U && s.periods==16U); near_value(s.frequency_hz,800.0f,0.001f);
    stream(125U,65500U,0U); BSP_Capture_Read(&s); /* 跨16位计数回绕。 */
    near_value(s.frequency_hz,8000.0f,0.001f);
    stream(124U,0U,0U); BSP_Capture_Read(&s);
    assert(s.valid && Measurement_Overrange(s.frequency_hz) && Measurement_Flow(s.frequency_hz)==40.0f);
    last=tick; tick=last+49U; BSP_Capture_Read(&s); assert(s.valid);
    tick=last+50U; BSP_Capture_Read(&s); assert(!s.valid && s.frequency_hz==0.0f);
    edge(100000U,0U,0U); BSP_Capture_Read(&s); assert(!s.valid); /* 失联后第一个边沿只重新建起点。 */
    for(i=1U;i<=16U;++i) edge(100000U+i*125U,0U,0U);
    BSP_Capture_Read(&s); assert(s.valid); near_value(s.frequency_hz,8000.0f,0.001f);
    edge(104000U,0U,0U); BSP_Capture_Read(&s);
    assert(s.valid && s.periods==1U); near_value(s.frequency_hz,500.0f,0.001f);
    fake_tim3.SR|=TIM_FLAG_CC1OF;
    edge(104125U,0U,0U); BSP_Capture_Read(&s); assert(!s.valid && s.overcaptures==1U);
    for(i=1U;i<=16U;++i) edge(104125U+i*125U,0U,0U);
    BSP_Capture_Read(&s); assert(s.valid && s.overcaptures==1U);
    HAL_TIM_IC_CaptureCallback(&htim3); BSP_Capture_Read(&s); assert(!s.valid); /* 同一个CCR再次进入：零周期。 */
    stream(1250U,65000U,0xFFFFFFF8U); BSP_Capture_Read(&s); assert(s.valid); near_value(s.frequency_hz,800.0f,0.001f);
    htim3.Channel=HAL_TIM_ACTIVE_CHANNEL_2; HAL_TIM_IC_CaptureCallback(&htim3);
    HAL_TIM_IC_CaptureCallback(&htim2); BSP_Capture_Read(&s); assert(s.valid);
    primask=1U; BSP_Capture_Read(&s); assert(primask==1U);
    primask=0U; BSP_Capture_Read(&s); assert(primask==0U);
    capture_start_status=HAL_ERROR; assert(!BSP_Capture_Init()); BSP_Capture_Read(&s); assert(!s.valid);
    capture_start_status=HAL_OK; htim3.Init.Prescaler=0U; assert(!BSP_Capture_Init()); htim3.Init.Prescaler=79U;
    BSP_Capture_Read(0);
    puts("PASS CAPTURE: 17 edges, averaging, timer/tick wrap, low frequency, timeout/restart, overcapture, zero delta, IRQ restore");
}
static void test_pwm(void)
{
    uint32_t d;
    assert(BSP_PWM_Start() && fake_tim2.CCR2==400U && BSP_PWM_GetDuty()==500U);
    BSP_PWM_SetDuty(5000U); assert(fake_tim2.CCR2==4000U);
    BSP_PWM_SetDuty(9500U); assert(fake_tim2.CCR2==7600U);
    BSP_PWM_SetDuty(0U); assert(fake_tim2.CCR2==400U);
    BSP_PWM_SetDuty(65535U); assert(fake_tim2.CCR2==7600U);
    for(d=500U;d<=9500U;++d) {
        double target=d/10000.0;
        double actual, error;
        BSP_PWM_SetDuty((uint16_t)d);
        actual=fake_tim2.CCR2/8000.0; error=fabs(actual-target);
        assert(error <= 0.5/8000.0+1e-12 && error/target < 0.01);
    }
    htim2.Init.Period=7998U; assert(!BSP_PWM_Start()); htim2.Init.Period=7999U;
    pwm_start_status=HAL_ERROR; assert(!BSP_PWM_Start()); pwm_start_status=HAL_OK;
    puts("PASS PWM: 5/50/95%, clamp, 9001 duty steps rounding, configuration/start failure (not physical waveform)");
}
int main(void)
{
    test_keys(); test_adc_measurement(); test_capture(); test_pwm();
    puts("ALL PHASE3 HOST TESTS PASSED");
    return 0;
}
