"""校验参赛目录、文件命名、硬件身份与独立依赖，默认不访问任何外设。"""
from __future__ import annotations
import ast
import hashlib
import json
import logging
from pathlib import Path
import re
import xml.etree.ElementTree as ET
from zipfile import ZipFile
import typer

app=typer.Typer(add_completion=False)
logger=logging.getLogger(__name__)
ROOT=Path(__file__).resolve().parents[1]
GENERATED={'build/generated','board/generated'}
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def delivery_files():
    # 生成目录不作为交付源码计入清单；清理后仍须保留全部重建输入。
    for p in sorted(ROOT.rglob('*')):
        if not p.is_file():continue
        rel=p.relative_to(ROOT)
        if any(part in {'.git','.venv','__pycache__','.pytest_cache','.Xil','xsim.dir'} for part in rel.parts):continue
        if any(rel.as_posix().startswith(x+'/') for x in GENERATED):continue
        if rel.as_posix().startswith('board/project/puzzle5ev.srcs/sources_1/bd/system/') and p.name!='system.bd':continue
        if any(part.startswith('puzzle5ev.') and part.split('.')[-1] in {'cache','gen','hw','runs','sim','ip_user_files'} for part in rel.parts):continue
        yield p
def inspect():
    failures=[]
    for folder in ['src','sim','build','board','data','skill','report']:
        if not (ROOT/folder).is_dir():failures.append('Missing directory: '+folder)
    if not (ROOT/'README.md').is_file():failures.append('Missing root README.md')
    files=list(delivery_files())
    for p in files:
        rel=p.relative_to(ROOT)
        for part in rel.parts:
            if part in {'README.md','SKILL.md'}:continue
            if not re.fullmatch(r'[a-z0-9_.-]+',part):failures.append('Invalid path: '+rel.as_posix())
        if p.suffix=='.py':
            try:ast.parse(p.read_text(encoding='utf-8-sig'),filename=str(rel))
            except (SyntaxError,UnicodeError) as e:failures.append(f'Python syntax: {rel}: {e}')
    for skill in ['xilinx-fpga','python-poetry']:
        if not (ROOT/'skill'/skill/'SKILL.md').is_file():failures.append('Missing original skill: '+skill)
    versions=json.loads((ROOT/'data/dependencies/versions.json').read_text(encoding='utf-8'))
    for name,value in versions['dependencies'].items():
        p=ROOT/'data/dependencies'/name
        if not p.is_file() or digest(p)!=value:failures.append('Dependency identity: '+name)
    reference=ROOT/'board/hardware/reference'
    manifest=json.loads((reference/'original_manifest.json').read_text())
    # XSA 内部 bit/HWH 必须与同一原始构建清单匹配，不能混用旧版本。
    with ZipFile(reference/'puzzle5ev.xsa') as z:
        bits=[n for n in z.namelist() if n.endswith('.bit')]
        hwh=[n for n in z.namelist() if Path(n).name=='system.hwh']
        if len(bits)!=1 or len(hwh)!=1:failures.append('XSA membership is ambiguous')
        else:
            for name,entry in [('puzzle5ev.bit',bits[0]),('puzzle5ev.hwh',hwh[0])]:
                if hashlib.sha256(z.read(entry)).hexdigest()!=manifest['files'][name]:failures.append('XSA mismatch: '+name)
    for p in (ROOT/'board/hardware/linux').glob('*'):
        if p.is_file() and digest(p)!=manifest['files'].get(p.name):failures.append('Linux module mismatch: '+p.name)
    boot=json.loads((ROOT/'board/boot/image-puzzle5ev.json').read_text())
    if digest(ROOT/'board/boot/image-puzzle5ev.ub')!=boot['output_sha256']:failures.append('Boot FIT mismatch')
    xpr=ET.parse(ROOT/'board/project/puzzle5ev.xpr')
    if not any(e.attrib.get('Name')=='Part' and e.attrib.get('Val')=='xczu5ev-sfvc784-2-i' for e in xpr.iter()):failures.append('Wrong XPR part')
    bd=json.loads((ROOT/'board/project/puzzle5ev.srcs/sources_1/bd/system/system.bd').read_text())
    if 'puzzle_candidate_scan_0' not in json.dumps(bd):failures.append('Missing saved BD candidate core')
    for p in files:
        rel=p.relative_to(ROOT).as_posix()
        if rel.startswith(('build/','src/ps/','board/runtime/')) and p.suffix in {'.py','.ps1','.tcl','.toml'}:
            s=p.read_text(encoding='utf-8-sig')
            if re.search(r'[dD]:[/\\]FPGA[/\\](puzzle5ev|puzzle7020)([/\\])',s):failures.append('Development workspace dependency: '+rel)
    return {'passed':not failures,'file_count':len(files),'bytes':sum(p.stat().st_size for p in files),'failures':failures,
            'naming_exceptions':['README.md required by guide','SKILL.md is the skill entry filename'],
            'motion_executed':False}

@app.command()
def check(output: Path|None=typer.Option(None)):
    result=inspect()
    if output:
        output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    typer.echo(json.dumps(result,ensure_ascii=False,indent=2))
    if not result['passed']:raise typer.Exit(1)

@app.command()
def manifest(verify: bool=typer.Option(False,'--verify')):
    target=ROOT/'data/submission_manifest.json'
    if verify:
        expected=json.loads(target.read_text())['files']
        errors=[name for name,value in expected.items() if not (ROOT/name).is_file() or digest(ROOT/name)!=value]
        actual={p.relative_to(ROOT).as_posix() for p in delivery_files() if p!=target and not p.relative_to(ROOT).as_posix().startswith('build/reports/current/')}
        errors.extend('unlisted: '+name for name in sorted(actual-set(expected)))
        if errors:raise ValueError('File identity mismatch: '+', '.join(errors))
        typer.echo(f'PASS: {len(expected)} manifest entries')
        return
    result=inspect()
    if not result['passed']:raise ValueError(result['failures'])
    hashes={p.relative_to(ROOT).as_posix():digest(p) for p in delivery_files() if p!=target and not p.relative_to(ROOT).as_posix().startswith('build/reports/current/')}
    target.write_text(json.dumps({'date':'2026-11-01','files':hashes},indent=2),encoding='utf-8')
    typer.echo(f'Wrote {len(hashes)} entries: {target}')

if __name__=='__main__':
    logging.basicConfig(level=logging.INFO,format='%(levelname)s %(name)s %(message)s')
    app()
