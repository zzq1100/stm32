from pathlib import Path
import re, hashlib, json, xml.etree.ElementTree as ET

root=Path(r'E:\STM32Project\LQB_17\MDK-ARM\LQB17_Tutorial')
docs=root/'Docs'
fw=root/'Firmware'
es=Path(r'D:\蓝桥杯嵌入式资源包\嵌入式数据资源包\ES')
errors=[]
links=0
def check(ok,msg):
    if not ok: errors.append(msg)

# Historic analyses describe their phase, while pointing readers to current state.
for p in sorted(docs.glob('0[1-6]-*.md')):
    s=p.read_text(encoding='utf-8')
    s=s.replace('> **本篇阶段**：读题与设计；实现位置均为配套工程的规划位置，尚未生成或验收固件。',
                '> **本篇阶段**：第一阶段的读题与设计分析。文中实现位置用于规划；第二阶段基础工程的当前进度与验证结果见[阅读目录](00-阅读路线与交付状态-精讲博客版.md)和[需求追踪表](07-需求追踪表与工程约定-精讲博客版.md)。整题及实板验收尚未完成。')
    p.write_text(s,encoding='utf-8')

all_docs=list(docs.glob('*.md'))+[root/'Tests/第二阶段实操与验证记录.md']
for p in all_docs:
    s=p.read_text(encoding='utf-8')
    check(not any(word in s for word in ('课件','讲义','面试','必背')),f'Banned word: {p.name}')
    check(sum(1 for ln in s.splitlines() if ln.startswith('```'))%2==0,f'Fence: {p.name}')
    for raw in re.findall(r'\[[^\]\n]+\]\(([^\n]+?)\)',s):
        dest=raw.strip().strip('<>')
        if re.match(r'https?://',dest):continue
        path,_,anchor=dest.partition('#')
        target=(p.parent/path).resolve() if path else p
        links+=1
        check(target.exists(),f'Link missing: {p.name}: {dest}')
        if anchor and target.suffix=='.md' and target.exists():
            target_text=target.read_text(encoding='utf-8')
            check(f'id="{anchor}"' in target_text,f'Anchor missing: {dest}')
    if p.parent==docs and 8<=int(p.name[:2])<=12:
        for term in ['一页纸总结','关键参数表','自测问题','知识小结','```mermaid']:
            check(term in s,f'Missing {term}: {p.name}')
        count=s.count('**知识小结**')
        check(3<=count<=6,f'Knowledge points {count}: {p.name}')
    for path,block in re.findall(r'\[打开实际源码\]\(\.\./([^)]+)\)\s*```c\n(.*?)\n```',s,re.S):
        actual=(root/path).read_text(encoding='utf-8').rstrip()
        check(actual==block,f'Source differs: {p.name} -> {path}')

trace=next(docs.glob('07-*.md')).read_text(encoding='utf-8')
ids=re.findall(r'^\| \[(P\d{2}-\d{2})\]',trace,re.M)
check(len(ids)==97 and len(set(ids))==97,'Trace must retain 97 unique rows')

resource_hashes={}
for rel in ['Src/lcd.c','Inc/lcd.h','Inc/fonts.h']:
    src=es/'BSP/LCD_Driver/MDK5_LCD_HAL'/rel
    dst=fw/'BSP_Drivers'/rel
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    check(sha(src)==sha(dst),f'Official resource changed: {rel}')
    resource_hashes[rel]=sha(dst)

project=fw/'MDK-ARM/LQB17_Tutorial.uvprojx'
tree=ET.parse(project)
check(tree.findtext('.//TargetName')=='Phase2_SelfTest','Wrong target')
check(tree.findtext('.//CreateHexFile')=='1','HEX disabled')
check(tree.findtext('.//Cads/Optim')=='1','Keil must use O0')
sources=[]
for item in tree.findall('.//File'):
    path=item.findtext('FilePath')
    if not path:continue
    full=(project.parent/path.replace('\\','/')).resolve()
    check(full.exists(),f'Project missing: {path}')
    check(full.is_relative_to(root),f'External old-project dependency: {path}')
    if full.suffix.lower() in ('.c','.s'):sources.append(full)
check(len(sources)==len(set(sources)),'Duplicate compilation source')
check((root/'Tests/phase2_selftest.c').resolve() in sources,'Missing test app')
check(not any(p.name=='app.c' for p in sources),'Unexpected second app')
check(not (fw/'LQB17_Tutorial').exists(),'Accidental nested CubeMX project')
hexfile=fw/'MDK-ARM/Phase2_SelfTest/Phase2_SelfTest.hex'
check(hexfile.exists() and hexfile.stat().st_size>1000,'HEX missing or empty')
check('0 Error(s), 0 Warning(s).' in (root/'Tests/build/keil_phase2.log').read_text(encoding='utf-8-sig'),'Build did not pass')
check('ALL PHASE2 HOST TESTS PASSED' in (root/'Tests/build/host_results.txt').read_text(encoding='utf-8-sig'),'Host tests did not pass')
for rel in ['Inc/bsp_led.h','Src/bsp_led.c','Inc/bsp_lcd.h','Src/bsp_lcd.c','Inc/bsp_time.h','Src/bsp_time.c','Inc/app.h']:
    check((fw/'BSP_Drivers'/rel).exists(),f'Module missing {rel}')

ioc=(fw/'LQB17_Tutorial.ioc').read_text()
for setting in ['RCC.HSE_VALUE=24000000','RCC.PLLM=RCC_PLLM_DIV6','RCC.PLLN=40','RCC.SYSCLKFreq_VALUE=80000000','PD2.GPIO_Label=LED_LE','ProjectManager.CoupleFile=true']:
    check(setting in ioc,f'IOC missing {setting}')

result={'status':'PASS' if not errors else 'FAIL','markdown_documents':len(all_docs),
        'local_links_checked':links,'unique_requirement_rows':len(set(ids)),
        'linked_compilation_sources':len(sources),'official_lcd_sha256':resource_hashes,
        'hex_sha256':hashlib.sha256(hexfile.read_bytes()).hexdigest(),'errors':errors}
(root/'Tests/build/artifact_audit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2))
raise SystemExit(bool(errors))
