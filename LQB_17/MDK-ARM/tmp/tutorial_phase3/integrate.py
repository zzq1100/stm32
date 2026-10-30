from pathlib import Path
import copy,re,shutil,xml.etree.ElementTree as ET
base=Path(r'E:\STM32Project\LQB_17\MDK-ARM')
root=base/'LQB17_Tutorial'; fw=root/'Firmware'; backup=base/'tmp/tutorial_phase3/phase2_backup'
project=fw/'MDK-ARM/LQB17_Tutorial.uvprojx'
tree=ET.parse(project); targets=tree.getroot().find('Targets')
template=copy.deepcopy(targets[0])
for x in list(targets):targets.remove(x)

def set_text(parent,path,text):
    el=parent.find(path)
    if el is None:el=ET.SubElement(parent,path)
    el.text=text

for number in [2,3]:
    t=copy.deepcopy(template); name=f'Phase{number}_SelfTest'
    set_text(t,'TargetName',name)
    set_text(t,'pCCUsed',r'5060960::V5.06 update 7 (build 960)::.\ARMCC')
    set_text(t,'uAC6','0')
    common=t.find('TargetOption/TargetCommonOption')
    for k,v in [('OutputName',name),('OutputDirectory',f'.\\{name}\\'),('CreateHexFile','1'),('PackID','Keil.STM32G4xx_DFP.2.2.0'),('PackURL','https://www.keil.com/pack/')]:set_text(common,k,v)
    cpu=common.find('Cpu');cpu.text=re.sub(r'CLOCK\(\d+\)','CLOCK(80000000)',cpu.text)
    cads=t.find('.//Cads');set_text(cads,'Optim','1');set_text(cads,'uC99','1')
    inc=cads.find('VariousControls/IncludePath')
    if 'BSP_Drivers' not in (inc.text or ''):inc.text=(inc.text or '')+r';..\BSP_Drivers\Inc'
    groups=t.find('Groups')
    for g in list(groups):
        if g.findtext('GroupName')=='BSP_Drivers' or 'Verification' in g.findtext('GroupName',''):groups.remove(g)
    # CubeMX can relocate custom files to Application/User; remove test entries by filename too.
    for file_list in groups.findall('Group/Files'):
        for f in list(file_list):
            if f.findtext('FileName','') in ['phase2_selftest.c','phase3_selftest.c']:
                file_list.remove(f)
    group=ET.SubElement(groups,'Group');ET.SubElement(group,'GroupName').text='BSP_Drivers'
    files=ET.SubElement(group,'Files')
    # Both targets can compile all reusable modules. Only their verification entry differs.
    for p in sorted((fw/'BSP_Drivers').rglob('*')):
        if p.suffix not in ['.c','.h']:continue
        f=ET.SubElement(files,'File');ET.SubElement(f,'FileName').text=p.name
        ET.SubElement(f,'FileType').text='1' if p.suffix=='.c' else '5'
        ET.SubElement(f,'FilePath').text='..\\BSP_Drivers\\'+str(p.relative_to(fw/'BSP_Drivers'))
    # uVision shares the file tree across targets: exclusion must be explicit.
    group=ET.SubElement(groups,'Group');ET.SubElement(group,'GroupName').text='Stage_Verification'
    files=ET.SubElement(group,'Files')
    for stage in [2,3]:
        f=ET.SubElement(files,'File')
        ET.SubElement(f,'FileName').text=f'phase{stage}_selftest.c';ET.SubElement(f,'FileType').text='1'
        ET.SubElement(f,'FilePath').text=f'..\\..\\Tests\\phase{stage}_selftest.c'
        prop=ET.SubElement(ET.SubElement(f,'FileOption'),'CommonProperty')
        ET.SubElement(prop,'IncludeInBuild').text='1' if stage==number else '0'
    targets.append(t)
ET.indent(tree,space='  ');tree.write(project,encoding='utf-8',xml_declaration=True)

# Preserve the known CMSIS-DAP settings for both targets, without copying old file lists.
opt=fw/'MDK-ARM/LQB17_Tutorial.uvoptx';ot=ET.parse(opt);ort=ot.getroot()
old=ET.parse(backup/'MDK-ARM/LQB17_Tutorial.uvoptx').getroot().find('Target')
current=ort.find('Target')
for x in list(ort):
    if x.tag=='Target':ort.remove(x)
for number in [2,3]:
    t=copy.deepcopy(current);set_text(t,'TargetName',f'Phase{number}_SelfTest')
    options=t.find('TargetOption')
    for tag in ['DebugOpt','TargetDriverDllRegistry']:
        original=old.find('TargetOption/'+tag)
        existing=options.find(tag)
        if existing is not None:options.remove(existing)
        options.append(copy.deepcopy(original))
    clk=t.find('.//CLKADS')
    if clk is not None:clk.text='80000000'
    ort.append(t)
ET.indent(ot,space='  ');ot.write(opt,encoding='utf-8',xml_declaration=True)
shutil.copy2(base/'tmp/tutorial_phase3/cubemx.log',root/'Tests/build/cubemx_phase3.log')
print('Two targets share the generated peripherals and BSP; each links exactly one App entry.')
