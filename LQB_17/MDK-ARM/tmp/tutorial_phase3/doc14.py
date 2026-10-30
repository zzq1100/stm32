from docs_common import *
t=header(14,'ADC采样、压力换算与零点校准','配置PB15/ADC2_IN15，从原始码得到电压和压力，理解硬件校准与用户零点校准的区别。','P02-11、P02-14～P02-16、P05-09、P06-18')
t+='''## 一、先从两点确定压力换算，而不是先找一个ADC函数

题目说输入电压范围是[V_offset,3.3V]，压力范围是0～10BAR，而且校准后动态更新斜率。它给了两点：(V_offset,0)与(3.3,10)。线性映射必须同时穿过这两点：

```text
P = 10 × (V - V_offset) / (3.3 - V_offset)
当V <= V_offset：P = 0
当V达到3.3V：P = 10
```

初始offset=0，1.65V对应5BAR。校准到0.3V后，有效电压跨度缩为3.0V，每伏对应10/3.0BAR；这时1.8V对应5BAR。若只减去0.3但仍除以3.3，满量程3.3V只能得到约9.09BAR，违背题意。

程序必须记住offset，因为旋钮后来改变时，零点仍保持上次校准的值。实时电压与offset不是同一个变量；LCD的V字段显示的是零点偏置，不是实时电压。阶段验证页另外显示实时V用于排错，不能把它的布局当作竞赛MAIN。

```mermaid
flowchart LR
    A["R37电压"] --> B["ADC原始码"]
    B --> C["实时电压V"]
    C --> D["压力公式"]
    Z["保存的零点offset"] --> D
    C --> E["B3有效长按释放：更新offset"]
    E --> Z
```

offset=3.3时分母为0，原题没有说明这种校准怎样处理。本方案拒绝该次校准并保留旧offset。越接近3.3，分母越小，电压噪声放大越明显；这是公式的物理含义，不应通过改错分母掩盖。零点应在模拟零压力条件下设置。
'''
t+=summary('压力是从两个端点推导的线性映射，校准同时改变跨度。','零点提高后，剩余电压范围仍需表示0～10BAR。','P=10(V-offset)/(3.3-offset)，初始offset=0。','只平移电压、不改变分母；或让满量程校准产生除零。')
t+='''## 二、ADC如何把连续电压变成整数

ADC是模数转换器：先让内部采样电容跟随输入电压，再进行逐次比较，最后给出数字码。本工程使用12位单端转换，单端是相对于模拟地测量这一个输入，不是同时测量两个引脚之差。

12位有2^12=4096种码，编号0～4095。参考电压决定量程，理想量化步距约3.3/4096≈0.806mV。这里程序选择端点归一化`V=raw/4095×3.3`，让0码映射0V、满码映射题目中的3.3V。4095是最大码而不是码的总数；两种分母约差0.0244%，必须知道在表达“量化步距”还是“显示端点映射”，不能混成同一个概念。

R37是10kΩ电位器，通过J11连接PB15，数据手册确认PB15是ADC2_IN15。J17把VREF+接到VDDA；参考端未正常连接时，只改软件公式没有用。调整跳线先断电，J11两针短接；不要照搬V1.2三针J11。

采样电容充电需要时间，外部信号源阻抗越高，越需要留出建立时间。本题R37变化很慢，选择247.5个ADC时钟采样，优先留余量，不追求最快转换。它不是“每247.5ms采一次”；一个参数描述单次转换内部的采样窗口，另一个10ms任务周期描述多久发起一次转换。

本次ADC同步时钟为AHB的80MHz/4=20MHz；247.5周期约12.375µs，12位转换阶段约12.5周期，合计约13µs的硬件时间，还不含HAL调用开销。这是参数推导，不是实测耗时。

`ADC_CLOCK_SYNC_PCLK_DIV4`宏名字带PCLK，但本地G4 HAL头文件注释明确写同步来源AHB；应按这颗芯片的实现理解，不能只按宏名猜总线。生成代码仍设置ADC12的SYSCLK时钟选择，但同步CKMODE路径由上述分频决定；不是再把两次分频都乘进去。
'''
t+=summary('原始码是有限分辨率的电压表示，量程依赖参考电压。','ADC采样和数字转换都不是无限精确、瞬时完成。','12位0～4095；20MHz ADC；采样247.5周期；程序用4095端点归一化。','把采样窗口当应用周期，或认为使用3.3常数就证明实际VDDA恰好3.3V。')
t+='''## 三、CubeMX逐项配置并沿初始化链核对

在Pinout & Configuration→Analog→ADC2，选择IN15 Single-ended。PB15应显示ADC2_IN15；不要选ADC1，不要把PB15配置成带上拉的普通数字输入。

| ADC2 Parameter Settings项目 | 设置 | 为什么 |
|---|---|---|
| Clock Prescaler | Synchronous clock mode divided by 4 | 80MHz/4=20MHz |
| Resolution / Data Alignment | 12 bits / Right alignment | 原始结果直接按0～4095读 |
| Scan Conversion Mode | Disabled | 只读一个通道 |
| Continuous Conversion Mode | Disabled | 每次由应用明确触发一个样本 |
| Number Of Conversion | 1 | 一个规则序列只有一个转换 |
| External Trigger Conversion Source / Edge | Software start / None | 不等待另一个定时器触发 |
| EOC Selection | End of single conversion | 轮询本次转换结束标志 |
| Discontinuous / DMA / Oversampling | Disabled | 当前不需要分组、DMA或硬件过采样 |
| Low Power Auto Wait | Disabled | 使用直接开始、完成、读取的流程 |
| Overrun | Data preserved | 配合单次读取，不默默覆盖未读旧结果 |
| Rank 1 / Channel | 1 / Channel 15 | Rank是序列顺序，Channel是外部输入通道 |
| Sampling Time | 247.5 Cycles | 慢速电位器留充电余量 |
| Single/Differential / Offset | Single-ended / None、0 | 用户零点由measurement保存，非ADC寄存器偏移 |

GPIO Settings核对PB15=Analog、No pull。NVIC不启用ADC中断，因为本篇采用短轮询。生成后看Core/Src/adc.c、Core/Inc/adc.h和main中的MX_ADC2_Init；hadc2是生成器创建的唯一ADC2句柄。

句柄是HAL保存外设实例、配置和运行状态的结构体，不是传感器数值。HAL函数接收&hadc2，是为了知道操作ADC2并更新其状态。不能在BSP里再声明一份同名hadc2，或用未初始化的局部句柄替代。

```mermaid
flowchart TD
    M["main：MX_ADC2_Init"] --> I["填写hadc2.Init"]
    I --> H["HAL_ADC_Init"]
    H --> S["HAL_ADC_MspInit：时钟、PB15模拟输入"]
    H --> R["ADC寄存器配置"]
    R --> C["HAL_ADC_ConfigChannel：CH15 / Rank1 / 247.5周期"]
    C --> B["App_Init：BSP_ADC_Init硬件校准"]
```

Msp可以先理解为HAL初始化时调用的底层板级准备入口。这里CubeMX已经生成它，不需要你再在其它文件复制一个同名函数。PB15被LCD影响的担心也可按端口核对：LCD数据用PC口、控制用PB5/PB8/PB9/PA8，没有配置PB15为LCD数据线。

### 实际生成的ADC配置文件
'''
t+=full('Firmware/Core/Src/adc.c')
t+=summary('配置由生成器管理，BSP调用生成的同一个hadc2。','配置、板级时钟GPIO准备与采样操作是不同阶段。','PB15 Analog；ADC2 CH15 Rank1；软件单次触发；无ADC中断。','把Rank=1误认为ADC_CHANNEL_1，或新建第二份hadc2。')
t+='''## 四、一次采样怎样开始、完成和失败

ADC硬件校准是芯片内部校正转换误差，在第一次转换前调用HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED)。它不读取你的“零压力”，也不会修改pressure offset。B3零点校准则在运行时保存当前电压并改变压力映射，两者不可互相代替。

| 调用 | 接口要点 | 成功后/失败后 |
|---|---|---|
| HAL_ADCEx_Calibration_Start | 句柄、单端模式，返回HAL状态 | BSP记录是否可用；失败时后续Read不伪造有效值 |
| HAL_ADC_Start | 指定已初始化ADC | 启动失败立即返回无效 |
| HAL_ADC_PollForConversion | 句柄、Timeout=1，单位ms | 完成返回HAL_OK；超时停止并返回无效 |
| HAL_ADC_GetValue | 返回规则转换数据寄存器值 | 核对12位范围，再换算电压 |
| HAL_ADC_Stop | 停止/关闭转换操作 | Stop失败也不能记为完整成功 |

“轮询”表示CPU短时间检查完成标志；不是中断，也不是完全不占CPU。1ms超时参数配合HAL tick限制等待，不是保证整个Read函数最多恰好1ms：HAL的超时比较、毫秒分辨率以及Start/Stop都有开销。正常硬件转换约13µs，异常路径与整机最坏延迟仍须实测。

为什么返回结构体而不只返回float？真实0V与读取失败都可能留下数值0，必须靠valid区分。结构体还保留raw，电压不对时能先判断采集是否正常。失败时清空数值并valid=0；最终自动控制必须检查有效性，不能把无效0当真实低压力不断加占空比。

### 采集模块完整文件
'''
t+=full('Firmware/BSP_Drivers/Inc/bsp_adc.h')
t+=full('Firmware/BSP_Drivers/Src/bsp_adc.c')
t+='''### 换算模块完整文件

measurement是纯计算，不包含HAL；同一文件同时提供压力与流量换算。下面给出最终文件的全部内容，流量函数在第15篇继续解释，不另生成第二份同名实现。

Measurement_Calibrate传入offset地址，是因为成功时要修改调用者保存的零点；失败时保持旧值。Measurement_Pressure每次使用新的分母，因此不用另存一份容易忘记更新的斜率。非法浮点值检查避免NaN/无穷大进入正常结果；这不能代替上层判断sample.valid。
'''
t+=full('Firmware/BSP_Drivers/Inc/measurement.h')
t+=full('Firmware/BSP_Drivers/Src/measurement.c')
t+=summary('ADC硬件校准、实时采样、压力零点是三个不同操作。','它们修正的对象、执行时机和返回含义都不同。','Init一次硬件校准；Read单次转换；valid标记结果；offset只在有效请求时更新。','用旧电压校准，或把转换失败的0V当真实压力输入。')
t+='''## 五、在没有电压表的条件下，哪些结论可以验证

选择Phase3_SelfTest下载并全速运行，调R37。验证页V是实时电压、P是压力、Z是保存的零点；A=1表示最近一次ADC读取成功。Keil观察phase3_adc.raw、phase3_adc.voltage_v、phase3_adc.valid、phase3_offset和phase3_zero_result。

把R37转到低端再复位，offset初值应为0；调高旋钮，raw与V、P总体上升。中间位置不保证恰好1.650V，旋钮机械中心和真实分压误差都不能从肉眼精确确认。没有电压表时可验证变化关系，不能填写电压绝对精度合格。

校准实验：将V调到约0.3V，按住B3至少2秒，期间Z不应因到时而改变；松开后，验证程序重新调用Read取当前电压，再更新offset。ZR=1成功，2表示满量程等非法校准被拒绝，3表示这次读取失败。之后在V约1.8V处P应约5.0；这些“约”是手工操作预期，精确数值边界由主机测试注入。

| 用例 | 已执行的软件检查 | 板上如何观察 |
|---|---|---|
| raw=0/4095 | 0V/3.3V端点通过 | 两端趋势；不等于实际电压已测准 |
| offset=0、V=1.65 | P≈5通过 | Watch或旋钮近似观察 |
| offset=0.3、V=1.8 | 新斜率P≈5通过 | 校准后观察P随V变化 |
| V<offset | P=0通过 | 校准后把旋钮再调低 |
| offset校准到3.3 | 拒绝且旧值保持通过 | 满码条件下观察ZR；实际ADC能否到满码要看板上结果 |
| Start/Poll/Stop/硬件校准失败 | 不返回valid=1；超时清理通过 | 真故障先查句柄状态和时钟，不故意损坏接线来制造 |

显示保留一位并不意味着内部先舍入到一位。后续P与目标±0.5比较时使用未按显示精度舍入的计算值。

排错顺序是：J11/J17和PB15接线→PB15模拟模式→ADC2时钟/CH15→raw→电压换算→offset→压力公式→显示格式。raw不动时先查前半段；raw正确而P校准后满量程不到10，查分母；只在B3操作后出错，查释放触发与新样本有效性。
'''
t+=summary('分开验证原始码、换算关系和真实电压精度。','软件公式正确并不消除参考电压和硬件误差。','Watch raw/voltage/valid/offset；ZR=1成功、2拒绝、3读取失败。','仅看到屏幕显示3.3就宣称引脚电压实测为3.300V。')
finish(14,'ADC采样、压力换算与零点校准',t,
'''- 题目两端点决定P=10(V-offset)/(3.3-offset)。
- R37经V1.1两针J11到PB15/ADC2_IN15，J17提供参考连接。
- ADC2为12位、软件单次、同步AHB/4=20MHz、采样247.5周期。
- 先做ADC内部校准，再按任务周期Start→Poll→Get→Stop。
- raw/voltage/valid一起保存，转换失败不能伪装成真实0V。
- 用户长按释放时重新采样并更新offset，满量程校准拒绝。
- measurement无硬件依赖，文中完整文件与工程同一份。
- 软件边界已验证，电压绝对精度和真实响应尚未实测。''',
'''| 输入 | R37 / J11 / PB15 / ADC2_IN15 |
| 参考 | J17接通VREF+与VDDA；程序按3.3V |
| 分辨率 | 12位，4096种码，0～4095 |
| 时钟 | ADC_CLOCK_SYNC_PCLK_DIV4，实际为AHB/4=20MHz |
| 采样 | 247.5周期；12位转换另需约12.5周期 |
| 周期 / 超时 | 验证App每10ms读一次；Poll参数1ms |
| 压力 | 0～10BAR，offset初值0V |
| 满量程零点 | 拒绝更新，保留旧值 |''',
[('校准后为什么分母也必须改变？','有效电压跨度由3.3变成3.3-offset，但仍需映射到10BAR。'),
 ('ADC内部校准会把当前R37电压设为压力零点吗？','不会，内部校准修正ADC自身误差；压力零点由应用保存当前有效电压。'),
 ('Rank1和Channel15分别是什么？','Rank是规则转换序列中的第几个，Channel是要测哪个模拟输入。'),
 ('为什么raw/4095与3.3/4096同时出现？','前者是程序端点归一化，后者近似理想量化步距，4095是最大码而4096是码的数量。'),
 ('sample.voltage_v为0能否直接判断真实压力为0？','不能，还要看valid；失败被清零与真实低电压不是同一情况。'),
 ('调R37时raw变化而校准后P满量程偏低，优先查什么？','优先查是否只减offset却仍除以3.3，然后检查显示舍入和实际参考电压。')])
