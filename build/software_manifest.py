"""记录部署 wheel 的依赖、来源提交和哈希，不要求交付目录已初始化 Git。"""
import hashlib
import json
import logging
from email.parser import Parser
from pathlib import Path
from zipfile import ZipFile
import typer

app=typer.Typer(add_completion=False)
@app.command()
def main(output: Path|None=typer.Option(None)):
    root=Path(__file__).resolve().parents[1]
    output=output or root/'board/software'
    records=[]
    for pattern in ['ec_26_e-*.whl','puzzle_solver-*.whl','puzzle_m0_bridge-*.whl']:
        matches=list(output.glob(pattern))
        if len(matches)!=1:raise ValueError(f'Expected one wheel: {pattern}')
        p=matches[0]
        with ZipFile(p) as archive:
            names=[n for n in archive.namelist() if n.endswith('.dist-info/METADATA')]
            if len(names)!=1:raise ValueError('Ambiguous wheel metadata')
            metadata=Parser().parsestr(archive.read(names[0]).decode())
        requirements=metadata.get_all('Requires-Dist',[])
        if any('file:' in r or '../' in r for r in requirements):raise ValueError('Wheel contains development path')
        records.append({'file':p.name,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'requires_dist':requirements})
    versions=json.loads((root/'data/dependencies/versions.json').read_text(encoding='utf-8'))
    (output/'manifest.json').write_text(json.dumps({'wheels':records,'repositories':versions['repositories'],
        'motion_tested':False,'hardware_changed':False},ensure_ascii=False,indent=2),encoding='utf-8')
    typer.echo(f'Wrote wheel manifest: {output}')
if __name__=='__main__':
    logging.basicConfig(level=logging.INFO,format='%(levelname)s %(name)s %(message)s')
    app()
