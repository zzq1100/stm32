"""准备竞赛提交清单；提供准考证号时生成仅含自编/配置C、H及HEX的ZIP。
编译后 --record-build 记录输入哈希；普通运行核对源码与该次构建一致。
不下载、不提交到考试系统。--candidate-id 必须替换为真实准考证号。
"""
from pathlib import Path
import argparse,hashlib,json,re,zipfile,xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parent.parent
FW=ROOT/'Firmware'; BUILD=ROOT/'Tests/build'
HEX=FW/'MDK-ARM/LQB17_Final/LQB17_Final.hex'
LOG=BUILD/'keil_final.log'; RECORD=BUILD/'final_build_inputs.json'
PROJECT=FW/'MDK-ARM/LQB17_Tutorial.uvprojx'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise SystemExit(message)
def inputs():
    files=sorted(p for p in FW.rglob('*') if p.is_file() and p.suffix.lower() in ('.c','.h','.s','.ioc'))
    hashes={p.relative_to(ROOT).as_posix():sha(p) for p in files}
    tree=ET.parse(PROJECT)
    final=next(t for t in tree.findall('Targets/Target') if t.findtext('TargetName')=='LQB17_Final')
    # 仅比较最终目标；旧验证目标的折叠/调试界面状态不属于编译输入。
    status=final.find('.//TargetStatus')
    if status is not None:status.clear()
    hashes['@FinalTargetXML']=hashlib.sha256(ET.tostring(final)).hexdigest()
    return files,hashes
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--record-build',action='store_true')
parser.add_argument('--candidate-id')
parser.add_argument('--output-dir',type=Path,default=ROOT/'Tests/Submission')
args=parser.parse_args()
require(HEX.is_file() and LOG.is_file(),'先运行Tests/build_final.ps1生成最终HEX和构建日志。')
require('0 Error(s), 0 Warning(s).' in LOG.read_text(encoding='utf-8-sig'),'最终构建日志不是0错误0警告。')
files,hashes=inputs()
if args.record_build:
    newer=[p.relative_to(ROOT).as_posix() for p in files if p.stat().st_mtime_ns>HEX.stat().st_mtime_ns]
    require(not newer,'这些源文件晚于HEX，请先重新构建：'+', '.join(newer[:8]))
    RECORD.write_text(json.dumps({'hex_sha256':sha(HEX),'inputs':hashes},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
else:
    require(RECORD.is_file(),'缺少构建输入记录；请运行新的build_final.ps1。')
    recorded=json.loads(RECORD.read_text(encoding='utf-8'))
    require(recorded['hex_sha256']==sha(HEX) and recorded['inputs']==hashes,'源码/配置/HEX与构建记录不一致，请重建最终目标。')
# 原样官方LCD库与字库不属于选手修改源码；若它们被改动，必须重新核定清单。
original=json.loads((BUILD/'phase3_artifact_audit.json').read_text(encoding='utf-8'))['official_lcd_sha256']
for rel,expected in original.items():require(sha(FW/'BSP_Drivers'/rel)==expected,'官方LCD文件已变化，需重新核对提交范围：'+rel)
system=FW/'Core/Src/system_stm32g4xx.c'
template=FW/'Drivers/CMSIS/Device/ST/STM32G4xx/Source/Templates/system_stm32g4xx.c'
require(sha(system)==sha(template),'system_stm32g4xx.c已修改，不能按原样库文件排除。')
source_files=[]
for folder in ('BSP_Drivers/Inc','BSP_Drivers/Src','Core/Inc','Core/Src'):
    for p in sorted((FW/folder).iterdir()):
        if p.suffix in ('.c','.h') and p.name not in ('lcd.c','lcd.h','fonts.h','system_stm32g4xx.c'):source_files.append(p)
manifest={'target':'LQB17_Final','hardware_validation':'PENDING',
          'candidate_id':args.candidate_id,'hex_sha256':sha(HEX),
          'sources':[{'file':p.relative_to(FW).as_posix(),'sha256':sha(p)} for p in source_files],
          'excluded':['unchanged lcd.c/lcd.h/fonts.h','unchanged system_stm32g4xx.c','HAL/CMSIS libraries','Tests and tutorial documents','Keil project/UI/build intermediate files']}
(BUILD/'submission_manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Verified final build; submission source files:',len(source_files))
if args.candidate_id:
    require(re.fullmatch(r'[A-Za-z0-9_-]{1,64}',args.candidate_id) is not None,'准考证号仅允许字母、数字、下划线和短横线，长度1~64。')
    out=args.output_dir.resolve();require(out.is_relative_to(ROOT),'输出目录须位于本教程目录内。')
    out.mkdir(parents=True,exist_ok=True);archive=out/(args.candidate_id+'.zip')
    require(not archive.exists(),'已有同名ZIP；请先保留/移走旧文件，避免覆盖。')
    # 所有检查通过后才创建；失败时不留下一个看似成功的ZIP。
    try:
        with zipfile.ZipFile(archive,'x',zipfile.ZIP_DEFLATED) as z:
            for p in source_files:z.write(p,p.relative_to(FW).as_posix())
            z.write(HEX,args.candidate_id+'.hex')
        require(archive.stat().st_size<30*1000*1000,'ZIP超过30MB，请核对内容。')
        with zipfile.ZipFile(archive) as z:
            require(z.testzip() is None,'ZIP完整性检查失败。')
            for p in source_files:require(hashlib.sha256(z.read(p.relative_to(FW).as_posix())).hexdigest()==sha(p),'ZIP源码哈希不一致。')
            require(hashlib.sha256(z.read(args.candidate_id+'.hex')).hexdigest()==sha(HEX),'ZIP中HEX哈希不一致。')
        print('Created and verified:',archive)
    except BaseException:
        if archive.is_file():archive.unlink()
        raise
else:
    print('No candidate ID supplied; only submission_manifest.json prepared. No contest ZIP created.')