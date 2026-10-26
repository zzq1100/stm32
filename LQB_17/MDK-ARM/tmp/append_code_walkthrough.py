from pathlib import Path

workspace = Path.cwd()
project = workspace.parent
document = workspace / '第十七届蓝桥杯STM32管路监控从零实现-精讲博客版.md'
text = document.read_text(encoding='utf-8')
marker = '<!-- SOURCE_SNAPSHOT_START -->'
assert text.count(marker) == 1
text = text.split(marker)[0] + marker + '\n'

notes = {
    'bsp_board.h': '先读接口的单位约定。业务代码不直接操作硬件句柄，只调用这几个函数。电压接口通过指针输出结果，用返回值报告成功或失败。',
    'bsp_board.c': '初始化中的GPIO配置、ADC、TIM3、TIM2顺序与前文一致。TIM3中断入口与回调都在这个文件；不要在it.c重复定义。读取频率使用快照，LED与LCD在主循环串行访问总线。',
    'monitor.h': '这是所有业务变量的单位与含义约定。fraction在实际积分实现中的单位是1/600 L；target_d与duty的单位不同。阅读函数时先回这里查字段，不靠变量名字猜单位。',
    'monitor.c': '按初始化→换算→积分→控制→报警→按键业务阅读。每个函数都通过m访问同一份系统状态。Persistent是内部辅助函数，static使其他.c不能直接调用它。',
    'bsp_keys.h': '四个键各有一份KeyState。结构体不是硬件寄存器，而是软件在RAM里保存的历史记录。',
    'bsp_keys.c': '先比较采样变化并重新计时，再确认20 ms稳态。确认按下时记录起点；确认松开时计算时长；最后取消跨页松开动作。',
    'bsp_display.h': 'const Monitor *表示显示函数读取状态，不通过这个指针修改业务状态。Compose只组内容，Service负责逐字符绘制。',
    'bsp_display.c': 'wanted与shown是固定宽度字符画面，不是十条带结束符的字符串。Row用临时字符串格式化后再复制；Service的cursor循环扫描200个格子。',
    'app.h': 'main.c只需要知道初始化和持续运行这两个入口。其余模块由app内部组织。',
    'app.c': '文件级static对象持续保留；每次App_Run都会重新读now。采集失败保留旧电压，10/100 ms任务按计划相位推进，按键从实际扫描时刻重新安排。',
    'main.c': '此文件由CubeMX骨架加App接入组成。完整保留当前时钟配置和错误处理，只有USER CODE区域接入app。基础MX_GPIO_Init只使能部分端口，其余端口由BSP和LCD各自初始化。',
}

comments = {
    'bsp_board.c': {
        'static void Check(': '统一检查HAL状态；HAL_OK以外的结果进入可定位的错误入口。',
        'void BSP_SetLEDs(': 'mask的1表示亮，与LED低有效硬件相反；保存并恢复LCD共用数据总线。',
        'void BSP_BoardInit(': '先开时钟和配引脚，再初始化外设参数，最后启动捕获与PWM。',
        'void TIM3_IRQHandler(': '向量表调用的真实中断入口，交给HAL解析TIM3事件标志。',
        'void HAL_TIM_IC_CaptureCallback(': 'HAL调用的捕获业务回调；短小处理，只保存计数，不刷新LCD。',
        'float BSP_ReadFrequency(': '复制中断发布的数据并检查失联超时，主循环里才做浮点除法。',
        'uint8_t BSP_ReadVoltage(': '成功返回1并写入*voltage；失败返回0，不用无效值覆盖调用者记录。',
        'uint8_t BSP_ReadKeys(': '把低电平按下统一转换成1，并把B1~B4打包到低四位。',
        'void BSP_SetDuty(': 'duty单位0.01个百分点；先限幅，再按8000计数周期换算CCR2。',
    },
    'monitor.c': {
        'void Monitor_Init(': '清空业务状态，再按题目第6页填写非零默认值。',
        'void Monitor_Measure(': '硬件层传入V与Hz，此处完成压力映射与瞬时流量限幅。',
        'void Monitor_Integrate100ms(': '每100 ms只调用一次；fraction以1/600 L为单位保存未进位余量。',
        'void Monitor_Control10ms(': '每10 ms只走一小步；MAN跟踪占空比目标，AUTO根据压力死区选择方向。',
        'static uint8_t Persistent(': 'condition连续成立超过2秒才返回1；两个故障传入各自的计时记录。',
        'void Monitor_Alarms(': '读取active阈值与当前测量，重新构造LD1~LD4掩码，高四位保持0。',
        'static uint16_t Adjust(': '先在有符号整数中加减，再夹在上下限内，防止无符号减法绕回。',
        'void Monitor_Key(': 'down=1处理普通按下，down=0处理MAIN长按松手；切页时先提交旧页参数。',
    },
    'bsp_display.c': {
        'static void Row(': '把可变参数格式化为临时字符串，最多复制20字符；未用区域由空格填充。',
        'void Display_Init(': '只有启动时初始化并清屏一次；运行中不反复整屏擦除。',
        'void Display_Compose(': '将当前业务数据转换成10行20列的目标画面，不直接输出像素。',
        'void Display_Service(': '每次最多绘制一个变化字符，留出CPU时间处理采集与按键。',
    },
    'app.c': {
        'void App_Init(': '启动阶段完成硬件、业务、按键和显示准备，再记录调度起点。',
        'void App_Run(': 'while主循环反复调用；时间未到就跳过任务，不延时等待。',
    },
}

files = [
    project/'BSP_Drivers/Inc/bsp_board.h', project/'BSP_Drivers/Src/bsp_board.c',
    project/'BSP_Drivers/Inc/monitor.h', project/'BSP_Drivers/Src/monitor.c',
    project/'BSP_Drivers/Inc/bsp_keys.h', project/'BSP_Drivers/Src/bsp_keys.c',
    project/'BSP_Drivers/Inc/bsp_display.h', project/'BSP_Drivers/Src/bsp_display.c',
    project/'BSP_Drivers/Inc/app.h', project/'BSP_Drivers/Src/app.c',
    project/'Core/Src/main.c',
]
for number, path in enumerate(files, 1):
    source = path.read_text(encoding='utf-8-sig')
    for needle, explanation in comments.get(path.name, {}).items():
        assert needle in source, (path, needle)
        source = source.replace(needle, f'/* {explanation} */\n' + needle, 1)
    if path.name == 'monitor.h':
        source = source.replace('    double fraction;', '    double fraction; /* 未进位余量，单位1/600 L；600进1升 */')
    relative = path.relative_to(project).as_posix()
    text += f'\n### 8.{number} `{relative}`\n\n{notes[path.name]}\n\n```c\n{source.rstrip()}\n```\n'
text += '\n<!-- SOURCE_SNAPSHOT_END -->\n'
document.write_text(text, encoding='utf-8')
print(f'Embedded {len(files)} files; document now {len(text)} characters')
