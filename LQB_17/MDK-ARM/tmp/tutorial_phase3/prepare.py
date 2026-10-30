from pathlib import Path
import shutil,re
base=Path(r'E:\STM32Project\LQB_17\MDK-ARM')
fw=base/'LQB17_Tutorial/Firmware'
work=base/'tmp/tutorial_phase3'
backup=work/'phase2_backup'
backup.mkdir(exist_ok=True)
for rel in ['LQB17_Tutorial.ioc','MDK-ARM/LQB17_Tutorial.uvprojx','MDK-ARM/LQB17_Tutorial.uvoptx','Core/Src/main.c','Core/Src/gpio.c','Core/Src/stm32g4xx_it.c']:
    dst=backup/rel
    dst.parent.mkdir(parents=True,exist_ok=True)
    if not dst.exists(): shutil.copy2(fw/rel,dst)
data={}
for line in (fw/'LQB17_Tutorial.ioc').read_text().splitlines():
    if '=' in line:
        k,v=line.split('=',1); data[k]=v
for k in list(data):
    if re.fullmatch(r'Mcu\.(IP|Pin)\d+',k):del data[k]
for i,ip in enumerate(['ADC2','NVIC','RCC','SYS','TIM2','TIM3']):data[f'Mcu.IP{i}']=ip
data['Mcu.IPNb']='6'
pins=['PC13','PC14-OSC32_IN','PC15-OSC32_OUT','PF0-OSC_IN','PF1-OSC_OUT',
      'PA0','PA1','PB0','PB1','PB2','PB15','PC8','PC9','PA13','PA14','PC10','PC11','PC12','PD2','PB4',
      'VP_SYS_VS_Systick','VP_SYS_VS_DBSignals','VP_TIM2_VS_ClockSourceINT','VP_TIM3_VS_ClockSourceINT']
for i,p in enumerate(pins):data[f'Mcu.Pin{i}']=p
data['Mcu.PinsNb']=str(len(pins))
for i,p in enumerate(['PB0','PB1','PB2','PA0'],1):
    data.update({f'{p}.GPIOParameters':'GPIO_PuPd,GPIO_Label',f'{p}.GPIO_PuPd':'GPIO_PULLUP',f'{p}.GPIO_Label':f'KEY_B{i}',f'{p}.Locked':'true',f'{p}.Signal':'GPIO_Input'})
data.update({'PB15.Mode':'IN15-Single-Ended','PB15.Signal':'ADC2_IN15',
             'PA1.Signal':'S_TIM2_CH2','PB4.Signal':'S_TIM3_CH1',
             'PB4.GPIOParameters':'GPIO_PuPd','PB4.GPIO_PuPd':'GPIO_PULLDOWN',
             'SH.S_TIM2_CH2.0':'TIM2_CH2,PWM Generation2 CH2','SH.S_TIM2_CH2.ConfNb':'1',
             'SH.S_TIM3_CH1.0':'TIM3_CH1,Input_Capture1_from_TI1','SH.S_TIM3_CH1.ConfNb':'1',
             'NVIC.TIM3_IRQn':r'true\:1\:0\:false\:false\:true\:true\:true\:true'})
adc={'ClockPrescaler':'ADC_CLOCK_SYNC_PCLK_DIV4','Resolution':'ADC_RESOLUTION_12B','DataAlign':'ADC_DATAALIGN_RIGHT',
     'ScanConvMode':'ADC_SCAN_DISABLE','ContinuousConvMode':'DISABLE','DiscontinuousConvMode':'DISABLE',
     'DMAContinuousRequests':'DISABLE','EOCSelection':'ADC_EOC_SINGLE_CONV','Overrun':'ADC_OVR_DATA_PRESERVED',
     'LowPowerAutoWait':'DISABLE','OversamplingMode':'DISABLE','NbrOfConversion':'1','ExternalTrigConv':'ADC_SOFTWARE_START',
     'ExternalTrigConvEdge':'ADC_EXTERNALTRIGCONVEDGE_NONE','Rank-10\\#ChannelRegularConversion':'1',
     'Channel-10\\#ChannelRegularConversion':'ADC_CHANNEL_15','SamplingTime-10\\#ChannelRegularConversion':'ADC_SAMPLETIME_247CYCLES_5',
     'SingleDiff-10\\#ChannelRegularConversion':'ADC_SINGLE_ENDED','OffsetNumber-10\\#ChannelRegularConversion':'ADC_OFFSET_NONE',
     'Offset-10\\#ChannelRegularConversion':'0'}
for k,v in adc.items():data['ADC2.'+k]=v
data['ADC2.IPParameters']=','.join(adc)
for t in ['TIM2','TIM3']:
    data[f'VP_{t}_VS_ClockSourceINT.Mode']='Internal'
    data[f'VP_{t}_VS_ClockSourceINT.Signal']=t+'_VS_ClockSourceINT'
    params={'Prescaler':'0' if t=='TIM2' else '79','Period':'7999' if t=='TIM2' else '65535',
            'CounterMode':'TIM_COUNTERMODE_UP','ClockDivision':'TIM_CLOCKDIVISION_DIV1',
            'AutoReloadPreload':'TIM_AUTORELOAD_PRELOAD_DISABLE'}
    if t=='TIM2':
        params.update({'Channel-PWM Generation2 CH2':'TIM_CHANNEL_2','PulseNoDither_2':'400',
                       'OCMode_PWM-PWM Generation2 CH2':'TIM_OCMODE_PWM1','OCPolarity_2':'TIM_OCPOLARITY_HIGH',
                       'OCFastMode_PWM-PWM Generation2 CH2':'TIM_OCFAST_DISABLE'})
    else:
        params.update({'Channel-Input_Capture1_from_TI1':'TIM_CHANNEL_1','ICPolarity_CH1':'TIM_INPUTCHANNELPOLARITY_RISING',
                       'ICSelection-Input_Capture1_from_TI1':'TIM_ICSELECTION_DIRECTTI','ICPrescaler-Input_Capture1_from_TI1':'TIM_ICPSC_DIV1','ICFilter_CH1':'0'})
    for k,v in params.items():data[t+'.'+k.replace(' ','\\ ')]=v
    data[t+'.IPParameters']=','.join(params)
data['ProjectManager.functionlistsort']='1-SystemClock_Config-RCC-false-HAL-false,2-MX_GPIO_Init-GPIO-false-HAL-true,3-MX_ADC2_Init-ADC2-false-HAL-true,4-MX_TIM2_Init-TIM2-false-HAL-true,5-MX_TIM3_Init-TIM3-false-HAL-true'
(fw/'LQB17_Tutorial.ioc').write_text('#MicroXplorer Configuration settings - do not modify\n'+'\n'.join(k+'='+v for k,v in sorted(data.items()))+'\n',encoding='utf-8')
(work/'generate.txt').write_text(f'config load "{fw / "LQB17_Tutorial.ioc"}"\nproject generate\nexit\n',encoding='utf-8')
print('Prepared ADC2, TIM2, TIM3 and four keys; saved phase2 configuration backup.')
