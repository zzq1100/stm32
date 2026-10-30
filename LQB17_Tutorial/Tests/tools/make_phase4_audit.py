from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=ROOT/'Tests/check_phase3_artifacts.py';s=p.read_text(encoding='utf-8')
import re
s=re.sub(r'if \(Path\(__file__\).*?raise SystemExit\(0\)\n', '', s, count=1, flags=re.S)
s=s.replace('第三阶段文档、源码、CubeMX配置与两个Keil目标','第四阶段文档、源码、CubeMX配置与三个Keil目标')
s=s.replace('phase3_artifact_audit.json','phase4_artifact_audit.json')
s=s.replace("root / 'Tests/第三阶段实操与验证记录.md',","root / 'Tests/第三阶段实操与验证记录.md',\n    root / 'Tests/第四阶段实操与验证记录.md',")
s=s.replace("== 17, 'Expected Docs00-16'","== 23, 'Expected Docs00-22'")
s=s.replace('<= 16:', '<= 22:').replace('range(1, 15)','range(1, 19)')
s=s.replace("{'Phase2_SelfTest', 'Phase3_SelfTest'}", "{'Phase2_SelfTest', 'Phase3_SelfTest', 'LQB17_Final'}")
s=s.replace("if actual.name.startswith('phase') and actual.name.endswith('_selftest.c'):","if actual.name in ('phase2_selftest.c', 'phase3_selftest.c', 'app.c'):")
s=s.replace("expected = '1' if actual.stem.lower() == name.lower() else '0'", "wanted_app = 'app.c' if name == 'LQB17_Final' else name.lower() + '.c'\n                expected = '1' if actual.name == wanted_app else '0'")
s=s.replace("check(apps == [name.lower() + '.c']", "check(apps == [('app.c' if name == 'LQB17_Final' else name.lower() + '.c')]")
s=s.replace("len(excluded_apps) == 1", "len(excluded_apps) == 2")
s=s.replace("('keil_phase2.log' if 'Phase2' in name else 'keil_phase3.log')", "({'Phase2_SelfTest':'keil_phase2.log','Phase3_SelfTest':'keil_phase3.log','LQB17_Final':'keil_final.log'}[name])")
s=s.replace("('第二阶段实操与验证记录.md' if 'Phase2' in name else '第三阶段实操与验证记录.md')", "({'Phase2_SelfTest':'第二阶段实操与验证记录.md','Phase3_SelfTest':'第三阶段实操与验证记录.md','LQB17_Final':'第四阶段实操与验证记录.md'}[name])")
s=s.replace("((2, 'host_results.txt'), (3, 'host_phase3_results.txt'))", "((2, 'host_results.txt'), (3, 'host_phase3_results.txt'), (4, 'host_phase4_results.txt'))")
s=s.replace('full_code_blocks = 0', 'full_code_blocks = 0\nfull_code_files = set()', 1)
s=s.replace('full_code_blocks += 1', 'full_code_blocks += 1\n        full_code_files.add(actual.resolve())', 1)
s=s.replace("trace = read(next(docs.glob('07-*.md')))", "for source in (fw/'BSP_Drivers').rglob('*'):\n    if source.suffix in ('.c','.h') and source.name not in ('lcd.c','lcd.h','fonts.h'):\n        check(source.resolve() in full_code_files, 'Self-written file lacks complete tutorial code: ' + source.name)\n\ntrace = read(next(docs.glob('07-*.md')))", 1)
s=s.replace("result = {", '''# 核对新增的需求映射和真实构建输入，打包检查同时验证41个文件清单。
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

result = {''')
s=s.replace("'unique_requirement_rows': len(set(ids)),", "'unique_requirement_rows': len(set(ids)), 'submission_source_files': len(manifest['sources']),")
(ROOT/'Tests/check_phase4_artifacts.py').write_text(s,encoding='utf-8')
# 旧检查入口在完整工程里转向新审计，保留之前的历史报告。
original=p.read_text(encoding='utf-8')
original=re.sub(r'if \(Path\(__file__\).*?raise SystemExit\(0\)\n', '', original, count=1, flags=re.S)
original=original.replace('from pathlib import Path', '''from pathlib import Path
if (Path(__file__).resolve().parent / 'check_phase4_artifacts.py').exists():
    import runpy
    print('Final target is present; running current phase4 audit instead of replacing historical phase3 report.')
    runpy.run_path(str(Path(__file__).resolve().parent / 'check_phase4_artifacts.py'), run_name='__main__')
    raise SystemExit(0)
''',1)
p.write_text(original,encoding='utf-8')
print('Created current three-target audit; old phase3 entry forwards to it.')