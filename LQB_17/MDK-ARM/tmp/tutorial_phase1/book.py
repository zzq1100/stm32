from pathlib import Path
import re

ROOT = Path(r'E:\STM32Project\LQB_17\MDK-ARM\LQB17_Tutorial')
DOCS = ROOT / 'Docs'
SOURCES = Path(r'D:\蓝桥杯嵌入式资源包\嵌入式数据资源包\ES')
rows = []
chapters = []
parts = []
page = 0
count = 0
filename = ''

MODULES = {
 'project': 'CubeMX 配置、Core 初始化与工程管理',
 'submit': '最终提交清单与 HEX 构建',
 'app': 'app.c：初始化、调度与模块调用',
 'led': 'bsp_led.c：LED 锁存输出',
 'keys': 'bsp_keys.c：采样、防抖和事件',
 'adc': 'bsp_adc.c + measurement.c：电压采样与压力换算',
 'capture': 'bsp_capture.c + measurement.c：测频与瞬时流量',
 'pwm': 'bsp_pwm.c：PWM 启动与占空比输出',
 'control': 'control.c：手动/自动目标跟踪',
 'settings': 'settings.c：页面、选项、预设与提交',
 'display': 'bsp_lcd.c + display.c：LCD 接入与页面内容',
 'total': 'totalizer.c：累计整升和小数余量',
 'alarm': 'alarms.c：持续条件计时与灯状态',
}

def begin(n, title, intro):
    global page, count, parts, filename
    page, count = n, 0
    filename = f'{n:02d}-第{n}页-{title}-精讲博客版.md'
    chapters.append((n, filename, title))
    parts = [f'# 第{n}页逐句分析：{title}\n',
        '> **适用平台**：CT117E-M4 V1.1 / STM32G431RBT6。\n'
        '> **开发版本基线**：STM32CubeMX 6.17.0、STM32CubeG4 V1.6.3、Keil ARM Compiler 5.06 update 7。\n'
        '> **适合读者**：会 C 语言，尚未独立配置和调试 STM32 外设。\n'
        '> **本篇阶段**：读题与设计；实现位置均为配套工程的规划位置，尚未生成或验收固件。\n',
        f'[返回阅读目录](00-阅读路线与交付状态-精讲博客版.md) · [对照第{n}页原图](<D:/蓝桥杯嵌入式资源包/17届真题/{n}_6.png>) · [需求追踪表](07-需求追踪表与工程约定-精讲博客版.md)\n',
        intro.strip()+'\n',
        '阅读标记：**题目规定**来自原图；**资料确认**来自原理图、数据手册或驱动源码；**实现选择**是本工程采用的方法。文中的验收均是待执行步骤，示例计算不代表已上板测量。\n']

def section(title, text=''):
    parts.append(f'## {title}\n\n{text.strip()}\n')

def r(loc, quote, meaning, constraints, memory, missing, lookup, action, verify, module):
    global count
    count += 1
    rid = f'P{page:02d}-{count:02d}'
    parts.append(f'<a id="{rid.lower()}"></a>\n\n### {rid}｜{loc}\n\n'
        f'**1．题目原句**\n\n> {quote}\n\n'
        f'**2．白话含义**：{meaning}\n\n'
        f'**3．能确定的条件**：{constraints}\n\n'
        f'**4．程序需要记住什么**：{memory}\n\n'
        f'**5．还缺什么信息**：{missing}\n\n'
        f'**6．去哪里查、怎样核对**：{lookup}\n\n'
        f'**7．落实到实现与验收**：{action}\n\n'
        f'**待执行验收**：{verify}\n')
    rows.append(dict(id=rid, page=page, file=filename, location=loc, quote=quote,
                     module=MODULES[module], action=action, verify=verify))

def prose(text):
    parts.append(text.strip()+'\n')

def small(conclusion, reason, parameters, mistake):
    parts.append(f'**知识小结**：结论：{conclusion} 原因：{reason} 关键参数：{parameters} 常见误区：{mistake}\n')

def end(summary, params, questions, problems):
    parts.append('## 本页验证与排错\n\n'+problems.strip()+'\n')
    parts.append('## 一页纸总结\n\n'+summary.strip()+'\n')
    parts.append('## 关键参数表\n\n| 项目 | 要求或本方案选择 | 性质 |\n|---|---|---|\n'+params.strip()+'\n')
    parts.append('## 自测问题\n\n'+'\n'.join(f'{i+1}. {q}' for i,(q,a) in enumerate(questions))+
                 '\n\n<details>\n<summary>展开参考答案</summary>\n\n'+
                 '\n\n'.join(f'{i+1}. {a}' for i,(q,a) in enumerate(questions))+'\n\n</details>\n')
    parts.append('[返回阅读目录](00-阅读路线与交付状态-精讲博客版.md) · [核对需求追踪表](07-需求追踪表与工程约定-精讲博客版.md)\n')
    DOCS.mkdir(parents=True, exist_ok=True)
    (DOCS / filename).write_text('\n'.join(parts), encoding='utf-8')
