from pathlib import Path
import copy,xml.etree.ElementTree as ET,shutil
root=Path(__file__).resolve().parents[2]; fw=root/'Firmware'; project=fw/'MDK-ARM/LQB17_Tutorial.uvprojx'
backup=root/'Tests/build/before_phase4';backup.mkdir(exist_ok=True)
for name in ('LQB17_Tutorial.uvprojx','LQB17_Tutorial.uvoptx'):
    if not (backup/name).exists():shutil.copy2(project.parent/name,backup/name)
tree=ET.parse(project);targets=tree.find('Targets')
if not any(t.findtext('TargetName')=='LQB17_Final' for t in targets):
    final=copy.deepcopy(list(targets)[-1]);final.find('TargetName').text='LQB17_Final';targets.append(final)
for target in targets:
    name=target.findtext('TargetName')
    target.find('.//OutputDirectory').text='.\\'+name+'\\'
    target.find('.//OutputName').text=name
    target.find('.//ListingPath').text='.\\'+name+'\\'
    target.find('.//CreateHexFile').text='1'
    groups=target.find('Groups')
    for g in groups:
        if g.findtext('GroupName')=='BSP_Drivers':
            files=g.find('Files');files.clear()
            for folder,ext,kind in [('Inc','.h','5'),('Src','.c','1')]:
                for p in sorted((fw/'BSP_Drivers'/folder).glob('*'+ext)):
                    entry=ET.SubElement(files,'File')
                    for tag,value in [('FileName',p.name),('FileType',kind),('FilePath','..\\BSP_Drivers\\'+folder+'\\'+p.name)]:ET.SubElement(entry,tag).text=value
                    if p.name=='app.c':
                        common=ET.SubElement(ET.SubElement(entry,'FileOption'),'CommonProperty')
                        ET.SubElement(common,'IncludeInBuild').text='1' if name=='LQB17_Final' else '0'
        if g.findtext('GroupName')=='Stage_Verification':
            for entry in g.findall('Files/File'):
                entry.find('FileOption/CommonProperty/IncludeInBuild').text='1' if entry.findtext('FileName').lower()==name.lower()+'.c' else '0'
ET.indent(tree,space='  ');tree.write(project,encoding='utf-8',xml_declaration=True)
opt=project.with_suffix('.uvoptx');tree=ET.parse(opt);r=tree.getroot()
if not any(t.findtext('TargetName')=='LQB17_Final' for t in r.findall('Target')):
    final=copy.deepcopy(r.findall('Target')[-1]);final.find('TargetName').text='LQB17_Final';r.append(final)
for t in r.findall('Target'):
    current=t.find('.//IsCurrentTarget')
    if current is not None:current.text='1' if t.findtext('TargetName')=='LQB17_Final' else '0'
ET.indent(tree,space='  ');tree.write(opt,encoding='utf-8',xml_declaration=True)
p=fw/'BSP_Drivers/Inc/app.h';s=p.read_text(encoding='utf-8')
s=s.replace('当前目标只链接Tests中对应阶段的一个selftest实现；\n * 最终竞赛目标改为BSP_Drivers/Src/app.c，不得与测试入口同时链接。','Phase2/3目标各链接一个Tests入口；\n * LQB17_Final链接BSP_Drivers/Src/app.c，三个App不得同时链接。')
p.write_text(s,encoding='utf-8')
s=(root/'Tests/build_phase3.ps1').read_text(encoding='utf-8').replace('Phase3_SelfTest','LQB17_Final').replace('keil_phase3.log','keil_final.log')
(root/'Tests/build_final.ps1').write_text(s,encoding='utf-8')
print('Integrated three mutually exclusive App targets; Final is selected in uvoptx.')