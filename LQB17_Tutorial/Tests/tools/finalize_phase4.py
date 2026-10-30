from pathlib import Path
import re,json
ROOT=Path(__file__).resolve().parents[2];DOCS=ROOT/'Docs';TESTS=ROOT/'Tests'
# 构建后记录源文件/目标配置指纹，之后打包时与它比较。
p=TESTS/'build_final.ps1';s=p.read_text(encoding='utf-8')
s=s.replace("param([string]$UV4Path = 'D:\\Tools\\Keil\\UV4\\UV4.exe')", "param([string]$UV4Path = 'D:\\Tools\\Keil\\UV4\\UV4.exe',\n      [string]$PythonPath = 'D:\\Tools\\Deepseek Harness\\resources\\runtime\\primary-runtime\\dependencies\\python\\python.exe')")
if '--record-build' not in s:s+='\n& $PythonPath -X utf8 (Join-Path $PSScriptRoot \'package_submission.py\') --record-build\nif ($LASTEXITCODE -ne 0) { throw "Build input recording failed: $LASTEXITCODE" }\n'
p.write_text(s,encoding='utf-8')
# 补充模拟硬件的关键事实，同时更新生成器以便将来复现文档。
p=TESTS/'tools/phase4_docs_a.py';s=p.read_text(encoding='utf-8')
needle='自动调节在5%和95%处停止越界，但“到限位”不等于“压力达到目标”。'
extra='本板R37是手动模拟的压力传感器，PA1不会自动改变它的电压。测试AUTO时需要手动转R37模拟压力响应；输入始终低压而D最终停在95%，本身可以是正确行为。\n\n'
if extra not in s:s=s.replace(needle,extra+needle)
p.write_text(s,encoding='utf-8')
import phase4_docs_a,phase4_docs_b
phase4_docs_a.doc18()

p=DOCS/'00-阅读路线与交付状态-精讲博客版.md';s=p.read_text(encoding='utf-8')
s=re.sub(r'> \*\*当前交付：.*?(?=\n>\n)', '> **当前交付：第四阶段（完整功能与验收准备）。** 第17～22篇与页面设置、手动/自动控制、累计、报警、显示、最终App已交付。LQB17_Final及两个阶段目标均0错误0警告，三组主机测试通过；真实开发板/波形验收尚待执行。已准备41个C/H与最终HEX的提交清单，准考证号命名的正式压缩包待填写真实号码后生成。',s,count=1,flags=re.S)
if '> **第四阶段入口**' not in s:
    s=s.replace('## 一、先学会', '> **第四阶段入口**：[17页面与提交](17-页面状态、参数预设与退出后生效-精讲博客版.md) → [18控制](18-手动渐变、自动压力控制与目标跟踪-精讲博客版.md) → [19累计](19-100ms累计流量、余量保存与大数计量-精讲博客版.md) → [20报警](20-连续故障计时、恢复清除与LED报警-精讲博客版.md) → [21整机](21-三页面显示、主程序调度与最终工程集成-精讲博客版.md) → [22验收与提交](22-整机验收、故障定位与竞赛提交-精讲博客版.md)。[第四阶段验证记录](../Tests/第四阶段实操与验证记录.md)提供软件证据和板上待填表。\n\n## 一、先学会',1)
s=s.replace('第08～16篇已经交付，可点击阅读；第17～22篇仍是后续计划。','第08～22篇均已交付，可点击阅读；接下来结合开发板操作结果继续验收和定位问题。')
titles={17:'页面状态、参数预设与退出后生效',18:'手动渐变、自动压力控制与目标跟踪',19:'100ms累计流量、余量保存与大数计量',20:'连续故障计时、恢复清除与LED报警',21:'三页面显示、主程序调度与最终工程集成',22:'整机验收、故障定位与竞赛提交'}
for n,title in titles.items():s=re.sub(r'^\| '+str(n)+r' \| [^|]+ \|',f'| {n} | [{title}]({n}-{title}-精讲博客版.md)（已交付） |',s,flags=re.M)
s=s.replace('第三阶段已实现按键事件、ADC与压力、输入捕获与瞬时流量、PWM输出；页面提交、控制、累计和报警业务仍待后续阶段。','第三阶段实现按键、测量和PWM，第四阶段已连接页面提交、控制、累计、报警与最终显示。最终固件为LQB17_Final；实板行为、精度和严格响应时间仍待验证。')
s=s.replace('Docs共17篇：目录、6页分析、追踪表、9篇模块；Tests保留两个阶段验证记录','Docs共23篇：目录、6页分析、追踪表、15篇模块与验收文章；Tests保留三个阶段验证记录')
s=s.replace('基础及采集输出两个目标均已构建；最终竞赛业务待第17～22篇实现','三个目标均已构建；最终软件功能已实现，实际开发板与波形待验收')
p.write_text(s,encoding='utf-8')

p=DOCS/'07-需求追踪表与工程约定-精讲博客版.md';s=p.read_text(encoding='utf-8')
s=re.sub(r'> \*\*状态\*\*：.*', '> **状态**：第四阶段已完成全部功能模块、最终App和第17～22篇。三个Keil目标与三组主机测试通过；真实开发板、波形、严格时序及现场提交仍待执行，不能将97条整机要求全标为实测通过。',s,count=1)
s=s.replace('已实现模块的接口与完整源码见第10～16篇；稳定入口为App_Init/App_Run。Phase2和Phase3目标分别只链接Tests中对应的一个selftest.c，最终改为BSP_Drivers/Src/app.c。Keil共享文件树，因此用Include in Target Build显式排除非当前入口。','已实现模块接口和完整源码见第10～21篇。稳定入口为App_Init/App_Run：Phase2/Phase3各链接对应selftest.c，LQB17_Final只链接BSP_Drivers/Src/app.c。三个目标共享文件树，非当前入口用Include in Target Build明确排除。')
s=s.replace('完整竞赛业务与实板验收待执行','完整业务已实现，实板验收待执行')
s=s.replace('关键参数：A01～A14单独编号','关键参数：A01～A18单独编号')
if '| A15 |' not in s:
    end='| A14 | PWM细分与读回 | 0.01个百分点请求量化到8000计数；GetDuty读回CCR配置而非实测波形；控制保存自己的细分状态 |'
    s=s.replace(end,end+'''\n| A15 | 运行测量失效 | ADC无效时AUTO保持、泄漏条件清除；无有效捕获时F=0，允许高D低流量持续触发堵塞；不另占用LD5～8 |
| A16 | 任务漏槽 | 正常5/10/100ms按节拍执行；落后多槽只处理当前一次并累计诊断，不能用当前样本补造历史；漏槽时速率/计量需排错 |
| A17 | 阈值提交后的持续时间 | FL/DH改变只重启堵塞，FH/PL改变只重启泄漏；草稿和无关阈值不影响该计时 |
| A18 | 累计余量及显示舍入 | Q用uint32整升，余量一升600000000单位；F量化到百万分之一L/min；D显示整数就近舍入，内部仍0.01% |''')
s=s.replace('- 第三阶段已接入按键、压力、流量和PWM；下一步按第17～22篇实现设置、控制、累计、报警与最终验收。','- 第四阶段已接入设置、控制、累计、报警和最终显示；下一步按第22篇开展实板验收，填写真实准考证号后打包。')
if '### 第四阶段已实现部分与证据' not in s:
    marker='### 第1页：先确定平台、交付物和系统边界（11条）'
    s=s.replace(marker,'''### 第四阶段已实现部分与证据

| 需求组 | 最终实现 | 已执行的软件证据 | 尚待执行 |
|---|---|---|---|
| 第4/5/6页页面与按键 | settings、app、bsp_keys | 草稿/提交、六范围、模式/选项、释放与跨页取消 | 实际手感、响应与全场景操作 |
| 第3页控制、第6页LD4 | control、bsp_pwm | 10ms步进、死区端点、限幅、无效压力、目标过程 | 1个百分点/秒的真实时间和波形 |
| 第2页累计、第3/6页Q | totalizer、app | 100ms公式、余量进位、千万升、饱和与清零 | 实际采样节拍和累计观察 |
| 第6页报警 | alarms、bsp_led | 严格阈值、2000/2001ms、恢复、回绕、参数改变 | 真实LED响应和连续条件 |
| 第3/4页显示与整机 | display、app | 行列/单位/补空格，真实App的主机调用链 | LCD颜色、无噪点、整屏更新时间 |
| 第1页提交 | package_submission.py、Final HEX | 41个C/H清单、构建输入哈希、测试ZIP完整性 | 真实准考证号、实板验收、本人上传 |

[第四阶段验证记录](../Tests/第四阶段实操与验证记录.md)列出证据与待填表；[97条机器可读映射](../Tests/build/requirement_evidence.json)逐条记录实现文件、软件证据范围及原来的最终验收方法。它不把源码存在等同于实物合格。

'''+marker)
s=s.replace('| 规划负责位置 |','| 实现位置与职责 |')
module_tests={
 'settings.c':'host_phase4_results.txt: SETTINGS / APP', 'control.c':'host_phase4_results.txt: CONTROL / APP',
 'totalizer.c':'host_phase4_results.txt: TOTALIZER / APP', 'alarms.c':'host_phase4_results.txt: ALARMS / APP',
 'display.c':'host_phase4_results.txt: DISPLAY / APP','app.c':'host_phase4_results.txt: APP',
 'bsp_keys.c':'host_phase3_results.txt: KEYS; host_phase4_results.txt: SETTINGS / APP',
 'bsp_adc.c':'host_phase3_results.txt: ADC/MEASUREMENT','measurement.c':'host_phase3_results.txt: ADC/MEASUREMENT',
 'bsp_capture.c':'host_phase3_results.txt: CAPTURE','bsp_pwm.c':'host_phase3_results.txt: PWM',
 'bsp_lcd.c':'host_results.txt: LCD','bsp_led.c':'host_results.txt: LED'}
evidence=[];out=[]
for line in s.splitlines():
    m=re.match(r'^\| \[(P\d{2}-\d{2})\]\(([^)]+)\)(.*?)\| (.*?) \| (.*?) \|$',line)
    if m:
        rid,link,label,resp,verify=m.groups()
        resp=re.sub(r'\[([^\]]+)\]\([^)]+\)', r'\1', resp)
        if '配置与初始化' in resp: resp='CubeMX 配置、Core 初始化与工程管理'
        if '打包脚本' in resp: resp='最终提交清单与 HEX 构建'
        names=list(dict.fromkeys(re.findall(r'\b[a-z_]+\.c\b',resp)))
        files=['Firmware/BSP_Drivers/Src/'+name for name in names]
        tests=[module_tests[name] for name in names if name in module_tests]
        if 'CubeMX' in resp:files=['Firmware/LQB17_Tutorial.ioc','Firmware/Core/Src/main.c'];tests=['IOC/generated source audit and keil_final.log; process/hardware rules still require manual checks']
        if '最终提交' in resp:files=['Tests/package_submission.py','Firmware/MDK-ARM/LQB17_Final/LQB17_Final.hex'];tests=['submission_manifest.json; SELFTEST_NOT_FOR_SUBMISSION.zip integrity only']
        evidence.append({'id':rid,'source':link,'responsibility':resp,'implementation_files':files,'software_evidence':tests,'final_acceptance':verify,'hardware_status':'PENDING','claim':'Listed module checks only; not a per-requirement physical PASS'})
        for name in names:resp=resp.replace(name,f'[{name}](../Firmware/BSP_Drivers/Src/{name})')
        if 'CubeMX' in resp:resp='[IOC](../Firmware/LQB17_Tutorial.ioc)、[Core/main.c](../Firmware/Core/Src/main.c)：配置与初始化'
        if '最终提交' in resp:resp='[打包脚本](../Tests/package_submission.py)与[Final HEX](../Firmware/MDK-ARM/LQB17_Final/LQB17_Final.hex)'
        line='| ['+rid+']('+link+')'+label+'| '+resp+' | '+verify+' |'
    out.append(line)
assert len(evidence)==97
p.write_text('\n'.join(out)+'\n',encoding='utf-8')
(TESTS/'build/requirement_evidence.json').write_text(json.dumps(evidence,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
# 历史阶段文章继续按教学阶段阅读，顶部明确当前工程已有Final。
for p in sorted(DOCS.glob('*.md')):
    n=int(p.name[:2]);s=p.read_text(encoding='utf-8')
    if 8<=n<=16 and '> **完整工程进度**' not in s:
        lines=s.splitlines();lines.insert(2,'> **完整工程进度**：本篇按第'+('二' if n<=12 else '三')+'阶段讲解；当前第17～22篇和LQB17_Final已完成。文中阶段测试的范围保持不变，最终业务见[第21篇](21-三页面显示、主程序调度与最终工程集成-精讲博客版.md)。\n');s='\n'.join(lines)+'\n'
    if n==13:
        s=s.replace('最终设置模块还要实现页面资格：','第17篇的最终设置模块实现页面资格：')
        s=s.replace('尚未验证跨页取消。','其跨页取消由第四阶段SETTINGS/APP用例验证。')
        s=s.replace('本阶段尚未实现累计流量清零。','此阶段测试入口不执行累计清零；Final入口已在第19/21篇接入。')
    if n==16:
        s=s.replace('当前工程保留Phase2_SelfTest，新增Phase3_SelfTest。','本篇使用Phase3_SelfTest，并保留Phase2_SelfTest；完整工程另外提供第21篇的LQB17_Final。')
        s=s.replace('下一阶段再接入页面与参数提交、手动/自动控制、累计流量和报警；这些业务使用本阶段接口，不另换一套ADC/TIM/按键驱动。','第17～22篇已经接入页面与参数提交、手动/自动控制、累计流量和报警；这些业务继续使用本阶段接口。')
    # 所有完整代码块逐一重新读取，尤其app.h的最终目标说明。
    def refresh(m):return m.group(1)+(ROOT/m.group(2)).read_text(encoding='utf-8').rstrip()+m.group(3)
    s=re.sub(r'(\[打开实际源码\]\(\.\./([^)]+)\)\s*```c\n).*?(\n```)',refresh,s,flags=re.S)
    p.write_text(s,encoding='utf-8')
for name in ('第二阶段实操与验证记录.md','第三阶段实操与验证记录.md'):
    p=TESTS/name;s=p.read_text(encoding='utf-8')
    if '> **第四阶段更新**' not in s:
        s=s.replace('\n\n','\n\n> **第四阶段更新**：本记录保留对应阶段的测试范围；当前已有LQB17_Final及最终业务，见[第四阶段记录](第四阶段实操与验证记录.md)。历史审计报告保留，当前整包以[phase4_artifact_audit.json](build/phase4_artifact_audit.json)为准。\n\n',1)
    # 现在约定工作目录是教程根，不再要求从其父目录运行。
    s=s.replace('E:\\STM32Project`','E:\\STM32Project\\LQB17_Tutorial`').replace('.\\LQB17_Tutorial\\Tests\\','.\\Tests\\')
    p.write_text(s,encoding='utf-8')
print('Updated current navigation, 97 requirement links, historical scope and source excerpts.')