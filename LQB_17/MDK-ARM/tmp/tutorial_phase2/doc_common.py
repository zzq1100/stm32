from pathlib import Path
import re

ROOT=Path(r'E:\STM32Project\LQB_17\MDK-ARM\LQB17_Tutorial')
DOCS=ROOT/'Docs'
FW=ROOT/'Firmware'
pages=[]

def code(path, start=None, end=None):
    text=(ROOT/path).read_text(encoding='utf-8')
    if start is not None:
        text=text[text.index(start):]
    if end is not None:
        text=text[:text.index(end)]
    return '\n```c\n'+text.rstrip()+'\n```\n'

def full(path):
    return '\n### '+Path(path).name+' 完整文件\n\n[打开实际源码](../'+path+')\n'+code(path)

def summary(a,b,c,d):
    return f'\n**知识小结**：结论：{a} 原因：{b} 关键参数：{c} 常见误区：{d}\n\n'

def header(n,title,scope,refs):
    return f'''# {n:02d}｜{title}

> **平台/版本**：CT117E-M4 V1.1 / STM32G431RBT6，STM32CubeMX 6.17.0、STM32CubeG4 V1.6.3、Keil ARM Compiler 5.06 update 7。
> **适合读者**：只会C语言，按本文步骤配置、观察和理解外设。
> **本篇技术点与效果**：{scope}
> **需求关联**：{refs}，可在[需求追踪表](07-需求追踪表与工程约定-精讲博客版.md)定位原句。
> **验证边界**：第二阶段工程已进行软件构建与主机测试；文中的接线、下载、屏幕和灯的现象是供你操作的预期，尚未代你上板验证。阶段目标是基础验证，不是完整竞赛固件。
> **版本衔接**：这些篇章保留第二阶段的学习范围；当前配套工程已扩展第三阶段，共用Core增加ADC/TIM初始化。Phase2_SelfTest仍保留基础验证行为，新增外设操作见第13～16篇；文中完整代码已同步到当前源码。

[阅读目录](00-阅读路线与交付状态-精讲博客版.md) · [第二阶段实操与验证记录](../Tests/第二阶段实操与验证记录.md)

'''

def finish(n,title,text,onepage,params,qa):
    text+='\n## 一页纸总结\n\n'+onepage.strip()+'\n'
    text+='\n## 关键参数表\n\n| 项目 | 参数与作用 |\n|---|---|\n'+params.strip()+'\n'
    text+='\n## 自测问题\n\n'+'\n'.join(f'{i+1}. {q}' for i,(q,a) in enumerate(qa))
    text+='\n\n<details>\n<summary>展开参考答案</summary>\n\n'
    text+='\n\n'.join(f'{i+1}. {a}' for i,(q,a) in enumerate(qa))+'\n\n</details>\n'
    text+='\n[回到阅读目录](00-阅读路线与交付状态-精讲博客版.md)\n'
    name=f'{n:02d}-{title}-精讲博客版.md'
    (DOCS/name).write_text(text,encoding='utf-8')
    pages.append((n,name,title))
