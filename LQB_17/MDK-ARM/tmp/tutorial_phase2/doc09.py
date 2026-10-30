from doc_common import *

t=header(9,'CubeMX工程、80MHz时钟与Keil下载','建立独立工程，理解启动顺序，将24MHz晶振变成80MHz，并找到生成代码和调试入口。','P01-01、P01-08、P02-06～P02-10')
t+='''## 一、先理解我们要建立什么

题目要求在80MHz系统主频下运行，因此工程第一项任务是让时钟明确、可检查。否则后续定时器频率、毫秒计时和LCD初始化时序都缺少可靠基础。

MCU内部有CPU、存储器和外设。时钟是驱动这些部分工作的节拍；“80MHz”表示每秒8000万个时钟周期，单个周期为12.5ns，**不表示每条C语句都只执行12.5ns**。一条语句可能包含多条指令、访存和分支。

HSE是外部高速时钟源。本板主控页X1标24MHz，连接PF0/PF1。PLL是锁相环，可以先分频再倍频，得到所需系统频率。SYSCLK是系统时钟源输出，HCLK是AHB/CPU侧时钟，PCLK1/PCLK2是APB总线时钟。它们有联系，但名字不同不能自动当成相同数值。

本方案选择：

```text
24 MHz HSE / M(6) = 4 MHz
4 MHz × N(40) = 160 MHz，PLL内部VCO频率
160 MHz / R(2) = 80 MHz，选为SYSCLK
AHB分频=1 → HCLK=80 MHz
APB1、APB2分频=1 → 两路PCLK均为80 MHz
```

```mermaid
flowchart LR
    H["主控X1：24MHz"] --> M["PLL M=6：4MHz"]
    M --> N["PLL N=40：160MHz"]
    N --> R["PLL R=2：80MHz"]
    R --> S["SYSCLK / HCLK：80MHz"]
    S --> P["APB1、APB2：80MHz"]
```

这组数值是根据板上晶振和题目主频选出的可用组合，不是要求你死记所有STM32都用M=6。换晶振后必须重新推导。APB分频改变时定时器时钟还可能有倍频关系，后续TIM章节会根据实际配置计算；本阶段APB不分频，不需要引入另一套时钟假设。
'''
t+=summary('先建立时钟，再建立依赖时钟的模块。','延时、定时器和任务节拍都由它派生。','24/6×40/2=80MHz；AHB、APB1、APB2分频均1。','把晶振频率当成最终CPU频率，或把一条C语句当成一个时钟周期。')
t+='''## 二、从空工程到CubeMX配置

### 1. 两种操作入口

如果目的是先确认开发环境能跑通，直接用CubeMX打开配套的 [LQB17_Tutorial.ioc](../Firmware/LQB17_Tutorial.ioc)，逐项核对下面的参数即可。它是本次用CubeMX实际生成工程的配置文件。

如果要练习从零操作，先在另一个练习目录创建项目，避免覆盖配套源码。CubeMX首页选择 **ACCESS TO MCU SELECTOR**，搜索STM32G431RBT6，核对LQFP64封装，选中后Start Project。第一次保存时注意CubeMX可能在Project Location下再建立项目名目录；以最终`.ioc`所在目录为工程根目录。配套工程将该根目录统一命名为Firmware，不能把父目录、工程名目录和MDK-ARM目录混成同一层。

先不要开启ADC、TIM2、TIM3。第二阶段只建立能显示、能点灯、能计时的基础，这样发现故障时范围比较小。

### 2. Pinout & Configuration页面

| 左侧分类/位置 | 选择 | 需求与原因 |
|---|---|---|
| System Core → SYS → Debug | Serial Wire | 保留PA13/PA14用于下载调试，释放不必要的JTAG占用 |
| System Core → SYS → Timebase Source | SysTick | 使用HAL默认1ms时间基准 |
| System Core → RCC → HSE | Crystal/Ceramic Resonator | 本板主控有晶体，不选择外部时钟旁路Bypass |
| RCC → LSE | Disable | 本题当前不需要RTC低速晶振 |
| 右侧芯片PC8～PC15 | GPIO_Output | LED数据总线；稍后LCD驱动接管整个PC口 |
| 右侧芯片PD2 | GPIO_Output | 控制573锁存器LE |

点击PC8后选择GPIO_Output，再给PC9～PC15同样操作。到System Core→GPIO的配置表逐个或多选设置：

| 引脚 | User Label | Output Level | Mode | Pull-up/Pull-down | Maximum output speed |
|---|---|---|---|---|---|
| PC8～PC15 | 依次LED_D0～LED_D7 | High | Output Push Pull | No pull-up and no pull-down | High |
| PD2 | LED_LE | Low | Output Push Pull | No pull-up and no pull-down | High |

PC数据初始高是为低有效LED准备全灭数据；PD2初始低使锁存器保持。仅设置初值还不等于把全灭状态送入锁存器，后续BSP_LED_Init会完成一次真正写入。

User Label不是备注装饰：本工程会在main.h生成LED_LE_Pin和LED_LE_GPIO_Port，bsp_led.c正是引用这两个名字。自己练习时如果改名，接口也要同步修改。

LCD的PB5、PB8、PB9、PA8以及PC0～PC7不在这里另建初始化。官方LCD_Init会配置它们，避免不理解其流程时再复制一套配置。本阶段没有把B1～B4配置为按键输入，因此此时按键没有功能是预期。

### 3. Clock Configuration页面

先将HSE输入值设为24MHz；选PLL Source为HSE，M为6、N为40、R为2；System Clock Mux选PLLCLK。AHB Prescaler选1，APB1/2 Prescaler选1。确认SYSCLK、HCLK和PCLK1/PCLK2显示80MHz且无红色冲突。

本工程同时生成P/Q分频2，但当前未使用对应输出；决定SYSCLK的是R输出。不要为了“都一样”去开启无关外设。

电压调节范围与Flash等待周期交给CubeMX根据该芯片和主频生成。当前结果为Voltage Scale 1、FLASH_LATENCY_2。等待周期是Flash访问需要的节拍数，不能认为“越小程序越快”就擅自降到0。

### 4. Project Manager页面

| 配置 | 本工程选择 | 生成后的作用 |
|---|---|---|
| Project Name | LQB17_Tutorial | IOC、Keil工程的基本名称 |
| Toolchain / IDE | MDK-ARM V5 | 生成Keil工程文件 |
| Firmware Package | STM32Cube FW_G4 V1.6.3 | 固定本次HAL/CMSIS版本 |
| Code Generator → Generate peripheral initialization as a pair of .c/.h files | 勾选 | GPIO初始化出现在gpio.c/gpio.h |
| Code Generator → Keep User Code when re-generating | 勾选 | 保留USER CODE区域中的接入代码 |
| Minimum Stack Size | 0x800 | 为函数调用、局部变量等预留2KiB栈 |

点击Generate Code。已提供的Firmware目录就是生成后接入模块的结果；初学者首次阅读不必立即重复生成。后续改变外设配置再生成时，核对USER CODE保留、BSP搜索路径、BSP文件组、测试目标和编译器选项。CubeMX生成器并不理解我们的竞赛模块边界，不能把“保留用户代码”理解为会保留所有手动工程选项。

### 5. 从空工程练习时，怎样判断这一步已经成功

先只编译CubeMX刚生成的工程。此时while循环中没有业务，能编译、下载、进入main，并在SystemClock_Config之后观察到预期RCC配置，就是这一小步的目标；屏幕不显示、灯不轮换都不能据此判断时钟配置失败。

**不要只复制下一节main中的App_Init/App_Run，就立即期待链接通过。** 这两个函数的实现放在第12篇的阶段测试文件里，还依赖第10、11篇的BSP模块。在自己从空工程逐步搭建时，先保持生成器的空循环，写齐第10～12篇模块后，再一次接好下面的文件树与入口。这是搭建顺序，不是再维护一套不同的最终驱动。

已经提供的配套工程包含所有这些文件，因此可以直接构建和上板，再按章节逐个理解内部机制。
'''
t+=summary('配置必须能在生成文件中找到对应结果。','图形界面的选项最终要落实为C代码、宏和工程参数。','SYS Serial Wire、HSE Crystal、PC8～15高、PD2低、80MHz。','勾了GPIO_Output却不核对初始电平，或以为头文件会自动加入所有自建目录。')
t+='''## 三、生成文件怎样衔接到自己的模块

先建立文件地图，再读main，不必从HAL库的第一行开始。

| 路径 | 谁生成/维护 | 现在负责什么 |
|---|---|---|
| Firmware/LQB17_Tutorial.ioc | CubeMX配置 | 记录芯片、引脚、时钟和生成选项 |
| Core/Src/main.c | CubeMX + USER CODE | HAL初始化、系统时钟、GPIO、App入口 |
| Core/Src/gpio.c | CubeMX | GPIO时钟、初值和模式 |
| Core/Inc/main.h | CubeMX | LED_LE等引脚宏、Error_Handler声明 |
| Core/Src/stm32g4xx_it.c | CubeMX | 包含SysTick_Handler等中断入口 |
| Drivers | ST库 | CMSIS寄存器定义、HAL实现 |
| BSP_Drivers/Inc、Src | 自编模块 + 官方LCD文件 | 可复用的LED、LCD、时间模块 |
| Tests/phase2_selftest.c | 阶段验证程序 | 本阶段App_Init/App_Run的实现 |
| MDK-ARM/LQB17_Tutorial.uvprojx | Keil工程 | 编译哪些文件、包含哪些目录、输出什么镜像 |

CPU复位后并不是凭空从main第一行开始：先进入启动文件的复位入口，完成基础系统和C运行环境准备，最后调用main。C运行环境会建立全局变量初值；随后main中的HAL_Init建立HAL与默认时基，SystemClock_Config切换到80MHz，MX_GPIO_Init打开GPIO时钟并配置引脚，App_Init初始化模块，然后循环App_Run。

```mermaid
flowchart TD
    A["复位向量 / 启动文件"] --> B["系统与C运行环境初始化"]
    B --> C["main：HAL_Init"]
    C --> D["SystemClock_Config：80MHz"]
    D --> E["MX_GPIO_Init"]
    E --> F["App_Init：LED→LCD→计时起点"]
    F --> G["while循环：App_Run"]
    G --> G
```

自己添加的入口只有三处：USER CODE Includes中包含app.h，USER CODE 2中调用App_Init，USER CODE 3中调用App_Run。它们都位于CubeMX允许保留的区域。

当前main的完整代码已经包含第三阶段生成的adc.h/tim.h及MX_ADC2_Init、MX_TIM2_Init、MX_TIM3_Init。它们分别在第14～16篇解释；本篇从空工程练习时先完成时钟与GPIO，不需要提前手写这些函数。共享Core让两个测试目标都进行寄存器初始化，但只有Phase3 App真正开始ADC采样、输入捕获和PWM输出。

下列完整main.c来自实际已编译文件。先重点读初始化调用顺序，再读SystemClock_Config中的字段：RCC_OscInitStruct描述振荡器和PLL；HAL_RCC_OscConfig返回HAL_OK表示配置成功，否则进入Error_Handler；RCC_ClkInitStruct选择时钟源和总线分频；HAL_RCC_ClockConfig同时配置Flash等待周期。Error_Handler是出错后停住的入口，不是所有问题都应该忽略并继续运行。
'''
t+=full('Firmware/Core/Src/main.c')
t+='''`HSE_VALUE`也要核对：Core/Inc/stm32g4xx_hal_conf.h中应为24000000UL。这个宏帮助软件计算时钟，不会把一个实际8MHz晶振“变成24MHz”。如果物理晶振、PLL参数、宏三者矛盾，SystemCoreClock变量可能给出误导结果。

本阶段app.h只定义稳定接口；当前由Tests中的验证程序实现。以后完成竞赛业务时，移除测试文件，换成BSP_Drivers/Src/app.c。两份都定义App_Init/App_Run，不能同时链接。
'''
t+=summary('main负责接线式的调用顺序，模块负责具体行为。','把时钟、GPIO、应用分开，生成器和手写逻辑才容易共同维护。','HAL_Init→SystemClock_Config→MX_GPIO_Init→App_Init→App_Run。','把业务代码写在USER CODE区域外，或把测试App和最终App同时加入目标。')
t+='''## 四、Keil编译、下载和第一次观察

### 1. 先打开正确文件

打开 [Firmware/MDK-ARM/LQB17_Tutorial.uvprojx](../Firmware/MDK-ARM/LQB17_Tutorial.uvprojx)，选择目标 **Phase2_SelfTest**。不要打开原工程的uvprojx，也不要把uvguix当作工程；uvguix主要保存界面状态。

本机实际构建用ARM Compiler 5.06 update 7 build 960和STM32G4xx_DFP 2.2.0。你的Keil应安装STM32G431器件支持包。Options for Target→Device核对STM32G431RBTx；Target中核对所选编译器。这里Target的晶振/时钟输入用于工程和调试信息，**它不能代替SystemClock_Config中的真正硬件初始化**。

Options for Target→C/C++中将Optimization设为**Level 0 (-O0)**，启用C99。本次官方LCD驱动里有普通空循环延时，优化器可能删除它们；在本阶段保留O0是明确的实现条件。以后改优化等级必须检查驱动延时并重新上板验证，不能仅凭编译成功判断没影响。

Include Paths中包含 `../BSP_Drivers/Inc`，还应保留生成器已有的Core、HAL、CMSIS目录。Options for Target→Output勾选Create HEX File；本目标输出目录为`Phase2_SelfTest`。

### 2. 磁盘目录、工程组、包含路径是三件事

磁盘的Inc/Src决定文件保存在哪里。Keil的BSP_Drivers组决定左侧树如何组织；右键组→Add Existing Files to Group，把模块.c及.h加入，选择头文件过滤器才能看到.h。Include Paths决定编译器在`#include "bsp_led.h"`时去哪里找。

把.h加入树不会让编译器自动增加搜索目录；反过来，头文件没显示在树中也不一定不能编译。当前配套工程已经同时设置文件组和搜索路径。

Tests/phase2_selftest.c现在与第三阶段测试入口一起显示在Stage_Verification组。Keil多目标共享文件树，两个测试.c必须按目标分别设置Include in Target Build，只启用对应的一个；第16篇给出完整说明。它们在磁盘上属于Tests；所有可复用BSP代码仍在Firmware/BSP_Drivers中。

### 从空工程补齐文件：按这个顺序操作

将实际`.ioc`所在目录记为Firmware。先在文件管理器创建BSP_Drivers/Inc和BSP_Drivers/Src；它们与Core、Drivers并列。Tests则位于Firmware的上一层，与Firmware并列。写齐后的最小结构为：

```text
LQB17_Tutorial/
├─ Firmware/
│  ├─ LQB17_Tutorial.ioc
│  ├─ Core/
│  ├─ Drivers/
│  ├─ BSP_Drivers/
│  │  ├─ Inc/  app.h、bsp_led.h、bsp_lcd.h、bsp_time.h、lcd.h、fonts.h
│  │  └─ Src/  bsp_led.c、bsp_lcd.c、bsp_time.c、lcd.c
│  └─ MDK-ARM/
│     └─ LQB17_Tutorial.uvprojx
└─ Tests/
   └─ phase2_selftest.c
```

1. 用编辑器新建并保存第10～12篇给出的完整.h/.c，文件名及大小写与代码中的include一致。另将三份官方LCD资源复制到图中的目录；不要把代码块的三个反引号也粘进C文件。
2. Keil右键目标→Add Group，建立BSP_Drivers组。右键该组→Add Existing Files to Group，选择Src里的四个.c；切换文件类型过滤器，再加入Inc里的六个.h。已有文件不用重复加入。
3. 建立Stage_Verification组，加入Tests/phase2_selftest.c。相对于MDK-ARM，其路径是`../../Tests/phase2_selftest.c`；写成`../Tests`会去找Firmware内部不存在的目录。从零建立第二阶段时先只加此文件；使用当前多目标工程则按上段检查另一入口被排除。
4. Options for Target→C/C++→Include Paths添加`../BSP_Drivers/Inc`，保留原有HAL/CMSIS/Core路径。路径从工程文件目录解析，不从当前编辑的.c所在目录解析。
5. 按前文完整main的三个USER CODE位置添加app.h、App_Init和App_Run，再核对ARMCC5、C99、O0和HEX输出选项。
6. 完整Rebuild。若报未定义符号App_Init，说明头文件声明虽已找到，实现它的测试.c没有参与链接；若报找不到bsp_led.h，则是头文件或搜索路径问题。这两类错误的修复位置不同。

Keil将每个.c分别编译成目标文件，再把这些目标文件链接成固件。`.h`提供声明，不能代替函数实现；因此不要用`#include "bsp_led.c"`来掩盖漏加源文件的问题，那样很容易在正式加入.c后产生重复定义。

这里有两种同名但位置不同的“源文件目录”：Core/Src放生成器管理的main、gpio和中断文件；BSP_Drivers/Src放自编模块和接入的LCD驱动。我们只向BSP目录添加模块，不把原有生成文件全部搬走。

### 3. 编译和读取结果

点击Rebuild（完整重编译）。本次实际结果为：

```text
Program Size: Code=16458 RO-data=5090 RW-data=104 ZI-data=3320
0 Error(s), 0 Warning(s).
FromELF: creating hex file...
```

Code是指令，RO-data是只读数据（字库占其中一部分），RW-data是带初值的可写数据，ZI-data是需清零的RAM数据。HEX用于烧录；AXF带符号等调试信息；MAP便于查函数、变量和内存分布。不是所有生成文件都属于题目要求提交的文件。

本次HEX位于 [Phase2_SelfTest.hex](../Firmware/MDK-ARM/Phase2_SelfTest/Phase2_SelfTest.hex)。它用于检验时钟、LCD、LED和任务时基，**没有完整竞赛功能，不能作为最终参赛HEX提交**。

### 4. 你在开发板上的操作

1. 接好板卡供电和已可用的下载器。Options for Target→Debug选择板载CMSIS-DAP对应驱动，Settings中选择SWD，确认能识别目标。
2. Utilities页使用同一调试驱动。Flash Download中核对STM32G43x/G44x的128KiB Flash算法、起始地址0x08000000；不要误用F1算法。可以勾选下载后复位运行。
3. 点击Download；以Keil报告擦除、编程、校验成功为下载依据。
4. 进入调试，运行到main确认复位入口正确。跨过SystemClock_Config后查看SystemCoreClock=80000000，查看RCC寄存器的HSE/PLL就绪以及系统时钟选择。
5. 全速运行。完成LCD初始化后应出现PHASE2 CHECK、SYS=80MHz、TEST ONLY，时间持续增长，LD1～LD4依次各亮约1秒，LD5～LD8灭。
6. 注意SYS=80MHz是验证页固定文字，另有SystemCoreClock检查，但两者都不能代替频率测量。实际主频和输出精度仍需仪器验证。

| 现象 | 确认方法 | 修复方向 |
|---|---|---|
| Cannot read project file | 确认打开uvprojx、文件存在且XML未被截断 | 使用这里的新工程路径；不要用ui状态文件代替工程 |
| 无法选择编译器/设备 | 看Target编译器与Pack Installer | 安装对应编译器或器件包；切换版本后重新构建验证 |
| Cannot open source input file | 看报错的文件名和Include Paths | 添加BSP_Drivers/Inc；区分缺文件和缺搜索路径 |
| App_Init重复定义 | 看参与编译的两个.c | 只保留当前阶段测试App或最终App中的一个 |
| 下载器找不到芯片 | 看SWD连线、供电与Debug Settings | 恢复正确SWD连接和驱动设置，必要时在复位下连接 |
| 停在Error_Handler | 查看调用栈 | 若来自时钟，查HSE模式、晶振和PLL；若来自App检查，查SystemCoreClock |
| 编译/下载成功但屏不亮 | 先看是否已经进入App_Run | 进入则按第11篇查LCD；未进入则先查初始化卡在哪 |

调试器停住CPU会改变时间关系，观察正常节拍时应让程序全速运行；一次断点停留不能拿来计算响应时间。
'''
t+=summary('工程可构建、可下载、可运行、精度达标分别验证。','各阶段检查的是不同故障。','本次编译0错误0警告；O0；HEX是Phase2_SelfTest。','看到HEX生成就认为整场比赛功能和硬件精度都已验收。')
finish(9,'CubeMX工程、80MHz时钟与Keil下载',t,
'''- 从STM32G431RBT6和V1.1原理图出发，选择24MHz晶体。
- HSE经M=6、N=40、R=2得到80MHz；AHB/APB均不分频。
- CubeMX生成GPIO、时钟和中断基础；USER CODE中只接入App接口。
- 磁盘Inc/Src、Keil组、Include Paths分别核对。
- 先构建，再下载，再全速观察；基础HEX不用于最终提交。
- 下一篇用实际bsp_led模块解释GPIO和锁存器。''',
'''| MCU / 工具链 | STM32G431RBT6 / ARMCC 5.06u7 |
| HSE / PLL | 24MHz / M6 N40 R2 |
| SYSCLK / HCLK / PCLK | 80MHz / 80MHz / 80MHz |
| Flash / 电压范围 | LATENCY_2 / Scale 1 |
| 编译优化 | O0，保留官方LCD空循环延时 |
| 栈 / Flash | 0x800字节 / 128KiB |
| 当前目标 | Phase2_SelfTest |''',
[('为什么Target页填80MHz不能代替时钟配置？','它是工程/调试信息，真正改变RCC的是运行中的初始化代码。'),
 ('RCC中HSE为什么选Crystal而不是Bypass？','主控接的是晶体，Bypass用于外部有源时钟直接输入，两者硬件条件不同。'),
 ('头文件显示在Keil树里仍提示找不到，应查哪里？','查C/C++的Include Paths，以及相对于MDK-ARM目录的路径。'),
 ('为什么保留O0？','官方LCD含可被优化删除的空循环延时，本阶段没有重写该驱动，固定优化条件并要求上板确认。'),
 ('main中的三个自编接入位置在哪里？','Includes加入app.h，USER CODE 2调用App_Init，USER CODE 3调用App_Run。'),
 ('当前HEX能提交竞赛吗？','不能，它只验证基础设施，采集、PWM、按键、控制和竞赛页面尚未实现。')])
