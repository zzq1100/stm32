from pathlib import Path
import sys,re
sys.path.insert(0,str(Path(__file__).resolve().parent))
import doc13,doc14,doc15,doc16
# Refresh earlier complete code blocks against shared, evolved generated files.
import doc08,doc09,doc10,doc11,doc12
root=Path(r'E:\STM32Project\LQB_17\MDK-ARM\LQB17_Tutorial')
docs=root/'Docs'

p=docs/'00-阅读路线与交付状态-精讲博客版.md';s=p.read_text(encoding='utf-8')
s=re.sub(r'> \*\*当前交付：第二阶段.*?(?=\n>\n)',
''' > **当前交付：第三阶段（采集与输出）。** 第13～16篇及按键、ADC、压力/流量换算、输入捕获、PWM模块已交付；第二阶段基础代码与验证目标保留。两个Keil目标均已构建，主机边界测试通过；实板及波形测量未执行。当前仍是阶段验证HEX，不是完整竞赛固件。'''.lstrip(),s,count=1,flags=re.S)
if '> **第三阶段入口**' not in s:
    marker='## 一、先学会把一句要求变成一个可验证的行为'
    links='> **第三阶段入口**：[13按键](13-按键输入、防抖与松开后长按事件-精讲博客版.md) → [14压力](14-ADC采样、压力换算与零点校准-精讲博客版.md) → [15流量](15-输入捕获、频率测量与瞬时流量-精讲博客版.md) → [16 PWM与集成](16-PA1的10kHz PWM与第三阶段集成-精讲博客版.md)。上板前先看[第三阶段实操与验证记录](../Tests/第三阶段实操与验证记录.md)。\n\n'
    s=s.replace(marker,links+marker)
s=s.replace('第08～12篇已经交付，可点击阅读；第13～22篇仍是后续计划。','第08～16篇已经交付，可点击阅读；第17～22篇仍是后续计划。')
topics={13:('按键防抖与长按','按键输入、防抖与松开后长按事件'),14:('ADC与压力','ADC采样、压力换算与零点校准'),15:('输入捕获与瞬时流量','输入捕获、频率测量与瞬时流量'),16:('PWM输出','PA1的10kHz PWM与第三阶段集成')}
for n,(old,title) in topics.items():s=s.replace(f'| {n} | {old} |',f'| {n} | [{title}]({n}-{title}-精讲博客版.md)（已交付） |')
s=s.replace('按键、ADC、输入捕获、PWM及竞赛业务仍待后续阶段。','第三阶段已实现按键事件、ADC与压力、输入捕获与瞬时流量、PWM输出；页面提交、控制、累计和报警业务仍待后续阶段。')
s=s.replace('Docs共13篇：目录、6页分析、追踪表、5篇基础模块；另有Tests验证记录','Docs共17篇：目录、6页分析、追踪表、9篇模块；Tests保留两个阶段验证记录')
s=s.replace('基础验证工程已构建；最终竞赛业务待第13～22篇实现','基础及采集输出两个目标均已构建；最终竞赛业务待第17～22篇实现')
p.write_text(s,encoding='utf-8')

p=docs/'07-需求追踪表与工程约定-精讲博客版.md';s=p.read_text(encoding='utf-8')
s=re.sub(r'> \*\*状态\*\*：.*', '> **状态**：前三阶段已交付读题、基础工程及按键/ADC/测频/PWM模块，软件构建和所列主机测试通过；页面/控制/累计/报警尚待实现，所有实板与整题验收尚待实施。',s,count=1)
s=s.replace('以下是整机配置约定。第二阶段IOC已生成HSE/PLL、SWD、PC8～PC15和PD2配置；官方LCD驱动负责其总线初始化。按键、ADC和TIM配置尚未加入，表中这些项是后续目标。',
'''以下是整机配置约定。第三阶段IOC已配置HSE/PLL、SWD、LED、四键输入、ADC2、TIM2 PWM和TIM3捕获；官方LCD驱动负责其总线初始化。各外设配置已生成并编译，尚未在实物验证。''')
s=s.replace('| 压力R37 | J11→PB15 | ADC2_IN15，模拟、无上下拉；J11短接，J17接通参考电压 |','| 压力R37 | J11→PB15 | ADC2_IN15，模拟无上下拉；同步AHB/4=20MHz，247.5周期采样；J11/J17接通 |')
s=s.replace('| 流量R39 | J9→PB4 | AF2 TIM3_CH1，上升沿输入捕获；J9短接 |','| 流量R39 | J9→PB4 | AF2 TIM3_CH1，上升沿、下拉；PSC79、ARR65535，常规16周期平均；J9短接 |')
s=s.replace('已实现的三个BSP模块，其接口与完整源码见第10～12篇；稳定入口为app.h中的App_Init/App_Run。第二阶段由Tests/phase2_selftest.c实现该入口，最终由BSP_Drivers/Src/app.c实现，二者不能同时链接。其它模块接口在各自编码章节明确，不创建互相矛盾的同名实现。',
'''已实现模块的接口与完整源码见第10～16篇；稳定入口为App_Init/App_Run。Phase2和Phase3目标分别只链接Tests中对应的一个selftest.c，最终改为BSP_Drivers/Src/app.c。Keil共享文件树，因此用Include in Target Build显式排除非当前入口。''')
if '| A11 |' not in s:
    s=s.replace('**知识小结**：结论：把实现选择写明',
'''| A11 | 开机已按住的键 | 初始化记录当前电平但不建立有效按下历史；先松开再重新按下才报告正常事件 |
| A12 | 测频窗口与异常 | 常规16周期平均；间隔>1250us快速发布低频；50ms无边沿失效；CC1OF或零差丢弃窗口 |
| A13 | ADC端点与硬件采样 | 12位raw/4095×3.3作端点归一化；20MHz、247.5周期；失败另标valid，不当真实0V |
| A14 | PWM细分与读回 | 0.01个百分点请求量化到8000计数；GetDuty读回CCR配置而非实测波形；控制保存自己的细分状态 |

**知识小结**：结论：把实现选择写明''')
    s=s.replace('关键参数：A01～A10单独编号','关键参数：A01～A14单独编号')
s=s.replace('基准 |\n\n| A11 |','基准 |\n| A11 |')
s=s.replace('**原图已核对，设计已分析；基础实现部分完成，完整竞赛行为与实板验收待执行**','**原图已核对，设计已分析；基础及采集输出已实现，完整竞赛业务与实板验收待执行**')
if '### 第三阶段已实现部分与证据' not in s:
    marker='### 第1页：先确定平台、交付物和系统边界（11条）'
    block='''### 第三阶段已实现部分与证据

| 需求关联 | 当前实现 | 软件证据 | 尚待实现/验证 |
|---|---|---|---|
| P05-01、P05-09、P05-10、P06-05 | [bsp_keys](../Firmware/BSP_Drivers/Src/bsp_keys.c) | 防抖/释放持续时间/回绕测试通过 | 页面动作、跨页取消、真实响应；Q清零未实现 |
| P02-11、P02-14～16、P06-18 | [bsp_adc](../Firmware/BSP_Drivers/Src/bsp_adc.c)、[measurement](../Firmware/BSP_Drivers/Src/measurement.c) | ADC配置构建；采样失败、端点、斜率与校准拒绝测试通过 | 实际电压与误差、最终MAIN显示 |
| P02-12、P02-17～20、P06-09输入依据 | [bsp_capture](../Firmware/BSP_Drivers/Src/bsp_capture.c)、measurement | 窗口、回绕、超时/恢复、覆盖和800/8000边界通过 | 实际捕获链、LD1报警业务 |
| P02-08、P02-09、P02-13、P03-01～03、P06-20输出基础 | [bsp_pwm](../Firmware/BSP_Drivers/Src/bsp_pwm.c) | 5%启动、限幅、全部9001细分请求量化测试；Keil通过 | 真实波形精度、渐变与自动控制 |

[第三阶段实操与验证记录](../Tests/第三阶段实操与验证记录.md)给出两个构建目标、测试日志和实物待执行表。通用模块具备相应能力，不等于这些需求的完整业务链全部通过。

'''
    s=s.replace(marker,block+marker)
s=s.replace('- 第二阶段完成资源、CubeMX、GPIO/LED、LCD和时间基础；下一步按第13篇计划接入按键，再扩展采集与PWM。','- 第三阶段已接入按键、压力、流量和PWM；下一步按第17～22篇实现设置、控制、累计、报警与最终验收。')
p.write_text(s,encoding='utf-8')

for p in sorted(docs.glob('0[1-6]-*.md')):
    s=p.read_text(encoding='utf-8').replace('第二阶段基础工程的当前进度与验证结果','配套工程的当前进度与验证结果')
    p.write_text(s,encoding='utf-8')
p=root/'Tests/第二阶段实操与验证记录.md';s=p.read_text(encoding='utf-8')
s=s.replace('Code=8206、RO-data=5090、RW-data=80、ZI-data=3056','Code=16458、RO-data=5090、RW-data=104、ZI-data=3320')
s=s.replace('来自当前独立工程构建，不沿用旧项目日志','第三阶段共用Core扩展后重建Phase2的当前结果；基础行为保留')
s=s.replace('Phase2_Verification组中的测试源文件','Stage_Verification组中的阶段测试源文件')
if '> **后续版本说明**' not in s:
    s=s.replace('## 从这里开始','> **后续版本说明**：当前工程已加入第三阶段ADC/TIM及按键配置，Phase2目标仍保留。本记录中的功能验证范围仍是基础模块；上方构建日志为共享Core扩展后的重建结果。最初的artifact_audit.json保留为当时检查记录，当前整包一致性以[第三阶段审计](build/phase3_artifact_audit.json)为准。\n\n## 从这里开始')
p.write_text(s,encoding='utf-8')
print('Generated 13-16, refreshed earlier source blocks, updated status and trace links.')
