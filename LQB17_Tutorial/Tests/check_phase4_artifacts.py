"""检查第四阶段文档、源码、CubeMX配置与三个Keil目标的一致性。

只读取工程，不运行固件或下载程序；结果写入Tests/build。
在Tests同级项目目录下运行：python Tests/check_phase3_artifacts.py
原始资源目录可用--resources指定。
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import xml.etree.ElementTree as ET
from datetime import datetime, timezone

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--resources', type=Path,
                    default=Path('D:/蓝桥杯嵌入式资源包/嵌入式数据资源包/ES'))
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
docs, fw = root / 'Docs', root / 'Firmware'
report_path = root / 'Tests/build/phase4_artifact_audit.json'
report_path.parent.mkdir(parents=True, exist_ok=True)
report_path.write_text('{"status":"RUNNING"}\n', encoding='utf-8')
errors = []
links = 0
full_code_blocks = 0
full_code_files = set()
source_manifest = {}


def check(ok, message):
    if not ok:
        errors.append(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    if not path.is_file():
        errors.append(f'Missing file: {path}')
        return ''
    return path.read_text(encoding='utf-8-sig')


all_docs = sorted(docs.glob('*.md')) + [
    root / 'Tests/第二阶段实操与验证记录.md',
    root / 'Tests/第三阶段实操与验证记录.md',
    root / 'Tests/第四阶段实操与验证记录.md',
]
check(len(list(docs.glob('*.md'))) == 23, 'Expected Docs00-22')
for path in all_docs:
    content = read(path)
    check(not any(word in content for word in ('课件', '讲义', '面试', '必背')),
          f'Forbidden blog wording: {path.name}')
    check(sum(line.startswith('```') for line in content.splitlines()) % 2 == 0,
          f'Unpaired code fence: {path.name}')
    for raw in re.findall(r'\[[^\]\n]+\]\(([^\n]+?)\)', content):
        destination = raw.strip().strip('<>')
        if re.match(r'https?://', destination):
            continue
        relative, _, anchor = destination.partition('#')
        target = (path.parent / relative).resolve() if relative else path
        links += 1
        check(target.exists(), f'Broken link: {path.name}: {destination}')
        if anchor and target.suffix == '.md' and target.exists():
            check(f'id="{anchor}"' in read(target),
                  f'Missing anchor: {path.name}: {destination}')
    if path.parent == docs and 8 <= int(path.name[:2]) <= 22:
        for required in ('一页纸总结', '关键参数表', '自测问题', '```mermaid'):
            check(required in content, f'Missing {required}: {path.name}')
        count = content.count('**知识小结**')
        check(3 <= count <= 6, f'Core topic count {count}: {path.name}')
    for relative, block in re.findall(
            r'\[打开实际源码\]\(\.\./([^)]+)\)\s*```c\n(.*?)\n```', content, re.S):
        actual = root / relative
        check(read(actual).rstrip() == block,
              f'Complete source block differs: {path.name}: {relative}')
        full_code_blocks += 1
        full_code_files.add(actual.resolve())

for source in (fw/'BSP_Drivers').rglob('*'):
    if source.suffix in ('.c','.h') and source.name not in ('lcd.c','lcd.h','fonts.h'):
        check(source.resolve() in full_code_files, 'Self-written file lacks complete tutorial code: ' + source.name)

trace = read(next(docs.glob('07-*.md')))
ids = re.findall(r'^\| \[(P\d{2}-\d{2})\]', trace, re.M)
check(len(ids) == 97 and len(set(ids)) == 97, 'Expected 97 unique requirement rows')
check('|\n\n| A11 |' not in trace, 'A11-A14 detached from implementation-choice table')
for n in range(1, 19):
    check(f'| A{n:02d} |' in trace, f'Missing implementation choice A{n:02d}')

resource_hashes = {}
for relative in ('Src/lcd.c', 'Inc/lcd.h', 'Inc/fonts.h'):
    original = args.resources / 'BSP/LCD_Driver/MDK5_LCD_HAL' / relative
    actual = fw / 'BSP_Drivers' / relative
    if original.is_file() and actual.is_file():
        check(sha(original) == sha(actual), f'Official resource modified: {relative}')
        resource_hashes[relative] = sha(actual)
    else:
        errors.append(f'Missing original/copied LCD resource: {relative}')

project = fw / 'MDK-ARM/LQB17_Tutorial.uvprojx'
tree = ET.parse(project)
targets = tree.findall('./Targets/Target')
check({target.findtext('TargetName') for target in targets} ==
      {'Phase2_SelfTest', 'Phase3_SelfTest', 'LQB17_Final'}, 'Wrong verification targets')
target_reports = {}
for target in targets:
    name = target.findtext('TargetName')
    check(target.findtext('.//CreateHexFile') == '1', f'{name}: HEX disabled')
    check(target.findtext('.//Cads/Optim') == '1', f'{name}: expected ARMCC O0')
    check(target.findtext('uAC6') == '0', f'{name}: expected ARMCC5')
    includes = target.findtext('.//Cads/VariousControls/IncludePath', '')
    check('../BSP_Drivers/Inc' in includes.replace('\\', '/'), f'{name}: BSP include missing')
    compiled, excluded_apps = [], []
    for group in target.findall('./Groups/Group'):
        group_name = group.findtext('GroupName')
        for entry in group.findall('./Files/File'):
            relative = entry.findtext('FilePath', '')
            actual = (project.parent / relative.replace('\\', '/')).resolve()
            check(actual.is_file(), f'{name}: missing project file {relative}')
            check(actual.is_relative_to(root), f'{name}: dependency outside tutorial {relative}')
            if actual.parent.parent.name == 'BSP_Drivers':
                check(group_name == 'BSP_Drivers', f'{name}: wrong BSP group {relative}')
            enabled = entry.findtext('./FileOption/CommonProperty/IncludeInBuild', '1')
            if actual.name in ('phase2_selftest.c', 'phase3_selftest.c', 'app.c'):
                wanted_app = 'app.c' if name == 'LQB17_Final' else name.lower() + '.c'
                expected = '1' if actual.name == wanted_app else '0'
                check(enabled == expected, f'{name}: wrong App exclusion {actual.name}')
                if enabled == '0':
                    excluded_apps.append(actual.name)
            if actual.suffix.lower() in ('.c', '.s') and enabled != '0':
                compiled.append(actual)
                if actual.is_file():
                    source_manifest[str(actual.relative_to(root))] = sha(actual)
    check(len(compiled) == len(set(compiled)), f'{name}: duplicated compilation source')
    apps = [p.name for p in compiled if p.name.endswith('_selftest.c') or p.name == 'app.c']
    check(apps == [('app.c' if name == 'LQB17_Final' else name.lower() + '.c')], f'{name}: must link exactly its own App: {apps}')
    check(len(excluded_apps) == 2, f'{name}: alternate App must be explicitly excluded')
    build_log = root / 'Tests/build' / ({'Phase2_SelfTest':'keil_phase2.log','Phase3_SelfTest':'keil_phase3.log','LQB17_Final':'keil_final.log'}[name])
    log = read(build_log)
    check('0 Error(s), 0 Warning(s).' in log, f'{name}: no clean build evidence')
    image = project.parent / name / (name + '.hex')
    check(image.is_file() and image.stat().st_size > 1000, f'{name}: missing HEX')
    memory = re.search(r'Program Size: (.+)', log)
    target_reports[name] = {
        'compiled_sources': len(compiled), 'app': apps, 'explicitly_excluded_apps': excluded_apps,
        'memory': memory.group(1).strip() if memory else None,
        'hex_sha256': sha(image) if image.is_file() else None,
        'build_log_sha256': sha(build_log) if build_log.is_file() else None,
    }
    if memory:
        phase_record = root / 'Tests' / ({'Phase2_SelfTest':'第二阶段实操与验证记录.md','Phase3_SelfTest':'第三阶段实操与验证记录.md','LQB17_Final':'第四阶段实操与验证记录.md'}[name])
        for key, number in re.findall(r'(Code|RO-data|RW-data|ZI-data)=(\d+)', memory.group(1)):
            check(f'{key}={number}' in read(phase_record), f'{name}: stale record size {key}')

for phase, log_name in ((2, 'host_results.txt'), (3, 'host_phase3_results.txt'), (4, 'host_phase4_results.txt')):
    check(f'ALL PHASE{phase} HOST TESTS PASSED' in read(root / 'Tests/build' / log_name),
          f'Phase{phase}: missing successful host test evidence')
check(not (fw / 'LQB17_Tutorial').exists(), 'Accidental nested generated project')
for module in ('keys', 'adc', 'capture', 'pwm'):
    for folder, suffix in (('Inc', 'h'), ('Src', 'c')):
        check((fw / f'BSP_Drivers/{folder}/bsp_{module}.{suffix}').is_file(), f'Missing {module}.{suffix}')
for suffix in ('h', 'c'):
    check((fw / f'BSP_Drivers/{"Inc" if suffix == "h" else "Src"}/measurement.{suffix}').is_file(),
          f'Missing measurement.{suffix}')

ioc = read(fw / 'LQB17_Tutorial.ioc')
for setting in (
        'RCC.HSE_VALUE=24000000', 'RCC.PLLM=RCC_PLLM_DIV6', 'RCC.PLLN=40',
        'RCC.SYSCLKFreq_VALUE=80000000', 'ADC2.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4',
        'ADC2.Resolution=ADC_RESOLUTION_12B', r'ADC2.Channel-10\#ChannelRegularConversion=ADC_CHANNEL_15',
        r'ADC2.SamplingTime-10\#ChannelRegularConversion=ADC_SAMPLETIME_247CYCLES_5',
        'PB15.Signal=ADC2_IN15', 'PB4.Signal=S_TIM3_CH1', 'PB4.GPIO_PuPd=GPIO_PULLDOWN',
        'PA1.Signal=S_TIM2_CH2', 'TIM3.Prescaler=79', 'TIM3.Period=65535',
        'TIM2.Prescaler=0', 'TIM2.Period=7999', 'TIM2.PulseNoDither_2=400',
        r'NVIC.TIM3_IRQn=true\:1\:0', 'PD2.GPIO_Label=LED_LE'):
    check(setting in ioc, f'IOC missing: {setting}')
for pin, key in (('PB0', 1), ('PB1', 2), ('PB2', 3), ('PA0', 4)):
    check(f'{pin}.GPIO_Label=KEY_B{key}' in ioc and f'{pin}.GPIO_PuPd=GPIO_PULLUP' in ioc,
          f'Key configuration mismatch: B{key}')
generated = {
    'adc.c': ('ADC_CHANNEL_15', 'ADC_SAMPLETIME_247CYCLES_5', 'ADC_CLOCK_SYNC_PCLK_DIV4', 'GPIO_MODE_ANALOG'),
    'tim.c': ('htim2.Init.Period = 7999', 'htim3.Init.Period = 65535', 'GPIO_AF2_TIM3',
              'GPIO_AF1_TIM2', 'HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0)', 'TIM_CHANNEL_2'),
    'stm32g4xx_it.c': ('void TIM3_IRQHandler(void)', 'HAL_TIM_IRQHandler(&htim3)', 'HAL_IncTick()'),
    'main.c': ('MX_ADC2_Init()', 'MX_TIM2_Init()', 'MX_TIM3_Init()', 'App_Init()', 'App_Run()'),
}
for name, markers in generated.items():
    content = read(fw / 'Core/Src' / name)
    for marker in markers:
        check(marker in content, f'Generated {name} missing {marker}')
conf = read(fw / 'Core/Inc/stm32g4xx_hal_conf.h')
check('#define HAL_ADC_MODULE_ENABLED' in conf and '#define HAL_TIM_MODULE_ENABLED' in conf,
      'ADC/TIM HAL modules disabled')

# 核对新增的需求映射和真实构建输入，打包检查同时验证41个文件清单。
import subprocess, sys, zipfile
evidence = json.loads(read(root / 'Tests/build/requirement_evidence.json'))
check({row['id'] for row in evidence} == set(ids), 'Requirement evidence IDs differ')
for row in evidence:
    check(bool(row['implementation_files']), 'Requirement without implementation mapping: ' + row['id'])
    for relative in row['implementation_files']:
        check((root / relative).is_file(), 'Missing mapped implementation: ' + relative)
    check(row['hardware_status'] == 'PENDING', 'Hardware status must remain pending')
proc = subprocess.run([sys.executable, '-X', 'utf8', str(root/'Tests/package_submission.py')], capture_output=True, text=True, encoding='utf-8')
check(proc.returncode == 0, 'Build/input/package validation failed: ' + proc.stdout + proc.stderr)
manifest = json.loads(read(root/'Tests/build/submission_manifest.json'))
check(len(manifest['sources']) == 41, 'Expected 41 selected C/H files')
archive = root/'Tests/build/package_selftest/SELFTEST_NOT_FOR_SUBMISSION.zip'
if archive.is_file():
    with zipfile.ZipFile(archive) as package:
        check(package.testzip() is None, 'Package CRC error')
        check(len(package.namelist()) == 42, 'Expected 41 C/H plus one HEX')
        check(all(Path(name).suffix.lower() in ('.c','.h','.hex') for name in package.namelist()), 'Unexpected package content')
else: check(False, 'Missing package self-test artifact')

result = {
    'status': 'FAIL' if errors else 'PASS',
    'checked_at_utc': datetime.now(timezone.utc).isoformat(),
    'scope': 'artifact consistency and existing software evidence only; no board or waveform verification',
    'markdown_documents': len(all_docs), 'local_links_checked': links,
    'complete_source_blocks_checked': full_code_blocks, 'unique_requirement_rows': len(set(ids)), 'submission_source_files': len(manifest['sources']),
    'targets': target_reports, 'official_lcd_sha256': resource_hashes,
    'compiled_source_sha256': source_manifest, 'errors': errors,
}
report_path.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(json.dumps({k: v for k, v in result.items() if k != 'compiled_source_sha256'},
                 ensure_ascii=False, indent=2))
raise SystemExit(bool(errors))
