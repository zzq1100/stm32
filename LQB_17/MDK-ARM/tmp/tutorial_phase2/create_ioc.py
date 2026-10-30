from pathlib import Path

root = Path(r'E:\STM32Project\LQB_17\MDK-ARM')
firmware = root/'LQB17_Tutorial/Firmware'
base = (root.parent/'LQB_17.ioc').read_text(encoding='utf-8')
values = dict(line.split('=', 1) for line in base.splitlines() if '=' in line)
values.update({
    'ProjectManager.ProjectName': 'LQB17_Tutorial',
    'ProjectManager.ProjectFileName': 'LQB17_Tutorial.ioc',
    'ProjectManager.CoupleFile': 'true',
    'ProjectManager.DeletePrevious': 'false',
    'ProjectManager.LastFirmware': 'false',
    'ProjectManager.CompilerLinker': 'ARMCC',
    'ProjectManager.StackSize': '0x800',
    'ProjectManager.HeapSize': '0x200',
    'RCC.HSE_VALUE': '24000000',
    'RCC.PLLSourceVirtual': 'RCC_PLLSOURCE_HSE',
    'RCC.PLLM': 'RCC_PLLM_DIV6',
    'RCC.PLLN': '40',
    'RCC.VCOInputFreq_Value': '4000000',
    'RCC.MCO1PinFreq_Value': '24000000',
})
for key in list(values):
    if key.startswith('Mcu.Pin') and key != 'Mcu.PinsNb':
        del values[key]
pins=['PF0-OSC_IN','PF1-OSC_OUT','PA13','PA14']
pins += [f'PC{i}' for i in range(8,16)] + ['PD2','VP_SYS_VS_Systick','VP_SYS_VS_DBSignals']
for i,pin in enumerate(pins):
    values[f'Mcu.Pin{i}']=pin
values['Mcu.PinsNb']=str(len(pins))
for i in range(8,16):
    pin=f'PC{i}'
    values[f'{pin}.Signal']='GPIO_Output'
    values[f'{pin}.Locked']='true'
    values[f'{pin}.PinState']='GPIO_PIN_SET'
    values[f'{pin}.GPIO_Speed']='GPIO_SPEED_FREQ_HIGH'
    values[f'{pin}.GPIO_Label']=f'LED_D{i-8}'
    values[f'{pin}.GPIOParameters']='GPIO_Speed,PinState,GPIO_Label'
values.update({'PD2.Signal':'GPIO_Output','PD2.Locked':'true',
               'PD2.GPIO_Label':'LED_LE','PD2.GPIO_Speed':'GPIO_SPEED_FREQ_HIGH',
               'PD2.PinState':'GPIO_PIN_RESET',
               'PD2.GPIOParameters':'GPIO_Speed,PinState,GPIO_Label'})
params=values['RCC.IPParameters'].split(',')
values['RCC.IPParameters']=','.join(sorted(set(params+['PLLM','PLLSourceVirtual'])))
path=firmware/'LQB17_Tutorial.ioc'
path.write_text('#MicroXplorer Configuration settings - do not modify\n'+
                '\n'.join(k+'='+v for k,v in sorted(values.items()))+'\n',encoding='utf-8')
script=root/'tmp/tutorial_phase2/generate.txt'
script.write_text(f'config load "{path}"\nproject name LQB17_Tutorial\n'
                  f'project path "{firmware}"\nproject toolchain "MDK-ARM V5.32"\n'
                  f'project couplefilesbyip 1\nconfig save "{path}"\nproject generate\nexit\n',encoding='utf-8')
print(path)
