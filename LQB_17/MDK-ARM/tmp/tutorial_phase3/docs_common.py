import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent.parent/'tutorial_phase2'))
from doc_common import code,full,summary,finish,ROOT,DOCS,FW

def header(n,title,scope,refs):
    return f'''# {n:02d}｜{title}

> **平台与版本**：CT117E-M4 V1.1 / STM32G431RBT6；CubeMX 6.17.0、STM32CubeG4 V1.6.3、Keil ARMCC 5.06 update 7，80MHz系统时钟，O0编译。
> **读者与前置知识**：会C语言，已读第08～12篇；新外设机制在使用前解释。
> **本篇效果**：{scope}
> **需求关联**：{refs}，原句见[需求追踪表](07-需求追踪表与工程约定-精讲博客版.md)。
> **当前边界**：第三阶段源码已通过Keil构建和所列主机测试，尚未上板或测量引脚波形。Phase3_SelfTest验证采集与输出，不是最终竞赛程序。

[阅读目录](00-阅读路线与交付状态-精讲博客版.md) · [第三阶段实操与验证记录](../Tests/第三阶段实操与验证记录.md)

'''
