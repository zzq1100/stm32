from pathlib import Path
import shutil
import re
import xml.etree.ElementTree as ET

root=Path(r'E:\STM32Project\LQB_17\MDK-ARM\LQB17_Tutorial')
fw=root/'Firmware'
resource=Path(r'D:\蓝桥杯嵌入式资源包\嵌入式数据资源包\ES\BSP\LCD_Driver\MDK5_LCD_HAL')
for source,dest in [('Src/lcd.c','Src/lcd.c'),('Inc/lcd.h','Inc/lcd.h'),('Inc/fonts.h','Inc/fonts.h')]:
    shutil.copy2(resource/source, fw/'BSP_Drivers'/dest)

main=fw/'Core/Src/main.c'
s=main.read_text(encoding='utf-8')
def region(text,name,body):
    pattern=r'(/\* USER CODE BEGIN '+re.escape(name)+r' \*/).*?(/\* USER CODE END '+re.escape(name)+r' \*/)'
    return re.sub(pattern,lambda m:m[1]+'\n'+body+'\n'+m[2],text,flags=re.S)
s=region(s,'Includes','#include "app.h"')
s=region(s,'2','  App_Init();')
s=region(s,'3','    App_Run();\n  }')
main.write_text(s,encoding='utf-8')

project=fw/'MDK-ARM/LQB17_Tutorial.uvprojx'
s=project.read_text(encoding='utf-8-sig')
if '<pCCUsed>' not in s:
    s=s.replace('</ToolsetName>', '</ToolsetName>\n      <pCCUsed></pCCUsed>\n      <uAC6>0</uAC6>', 1)
if '<PackID>' not in s:
    s=s.replace('</Vendor>', '</Vendor>\n          <PackID>Keil.STM32G4xx_DFP.2.2.0</PackID>\n          <PackURL>https://www.keil.com/pack/</PackURL>', 1)
def tag(name,value):
    global s
    s=re.sub(fr'<{name}>.*?</{name}>',lambda m:f'<{name}>{value}</{name}>',s,count=1,flags=re.S)
tag('TargetName','Phase2_SelfTest')
tag('pCCUsed',r'5060960::V5.06 update 7 (build 960)::.\ARMCC')
tag('uAC6','0')
tag('OutputName','Phase2_SelfTest')
tag('OutputDirectory','.\\Phase2_SelfTest\\')
tag('CreateHexFile','1')
tag('Optim','1')  # Keil XML 1 corresponds to -O0, preserving vendor busy-loop delays.
tag('uC99','1')
s=s.replace('CLOCK(8000000)','CLOCK(80000000)')
s=re.sub(r'<IncludePath>([^<]*Core[^<]*)</IncludePath>',
         lambda m:'<IncludePath>'+m[1].rstrip(';')+r';..\BSP_Drivers\Inc</IncludePath>' if 'BSP_Drivers' not in m[1] else m[0],s,count=1)

def group(name,files):
    out=f'      <Group>\n        <GroupName>{name}</GroupName>\n        <Files>\n'
    for path in files:
        typ='1' if path.endswith('.c') else '5'
        basename=path.replace('\\','/').split('/')[-1]
        out+=f'          <File><FileName>{basename}</FileName><FileType>{typ}</FileType><FilePath>{path}</FilePath></File>\n'
    return out+'        </Files>\n      </Group>\n'
for name in ('BSP_Drivers','Phase2_Verification'):
    s=re.sub(r'\s*<Group>\s*<GroupName>'+name+r'</GroupName>.*?</Group>','',s,flags=re.S)
files=['..\\BSP_Drivers\\'+str(p.relative_to(fw/'BSP_Drivers')).replace('/','\\')
       for p in sorted((fw/'BSP_Drivers').rglob('*')) if p.suffix in ('.c','.h')]
s=s.replace('</Groups>',group('BSP_Drivers',files)+group('Phase2_Verification',[r'..\..\Tests\phase2_selftest.c'])+'    </Groups>')
ET.fromstring(s)
project.write_bytes(s.replace('\n','\r\n').encode('utf-8'))

options=fw/'MDK-ARM/LQB17_Tutorial.uvoptx'
s=options.read_text(encoding='utf-8-sig')
s=re.sub(r'<TargetName>.*?</TargetName>','<TargetName>Phase2_SelfTest</TargetName>',s,count=1)
s=re.sub(r'<CLKADS>.*?</CLKADS>','<CLKADS>80000000</CLKADS>',s,count=1)
original_options=(root.parent/'LQB_17.uvoptx').read_text(encoding='utf-8-sig')
for section in ('DebugOpt','TargetDriverDllRegistry'):
    block=re.search(fr'<{section}>.*?</{section}>',original_options,re.S).group(0)
    s=re.sub(fr'<{section}>.*?</{section}>',lambda m:block,s,count=1,flags=re.S)
options.write_bytes(s.replace('\n','\r\n').encode('utf-8'))
(root/'Tests/build').mkdir(exist_ok=True)
shutil.copy2(root.parent/'tmp/tutorial_phase2/cubemx_final.log',root/'Tests/build/cubemx_generation.log')
print('Integrated production BSP and the separate Phase2_SelfTest target.')
