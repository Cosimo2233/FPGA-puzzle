"""从压缩 XSA 和匹配模块恢复上板包；只生成文件，不加载硬件或驱动。"""
import hashlib
import json
import logging
from pathlib import Path
import shutil
import subprocess
from zipfile import ZipFile
import typer

app = typer.Typer(add_completion=False)
logger = logging.getLogger(__name__)

@app.command()
def main(amd_root: Path = typer.Option(..., envvar='AMD_ROOT'),
         output: Path | None = typer.Option(None),
         hardware_dir: Path | None = typer.Option(None)):
    root = Path(__file__).resolve().parents[1]
    target = (output or root/'board/generated/bundle').resolve()
    if target.exists():
        raise ValueError('Use a new output directory; do not overwrite a hardware bundle')
    source = hardware_dir or root/'board/hardware/reference'
    with ZipFile(source/'puzzle5ev.xsa') as archive:
        bits = [n for n in archive.namelist() if n.endswith('.bit')]
        descriptions = [n for n in archive.namelist() if Path(n).name == 'system.hwh']
        if len(bits)!=1 or len(descriptions)!=1:
            raise ValueError('Ambiguous XSA bitstream/HWH')
        bit,hwh=archive.read(bits[0]),archive.read(descriptions[0])
    # 参考硬件须逐项校验身份，新实现的硬件不能冒用原有上板验收结论。
    original = json.loads((root/'board/hardware/reference/original_manifest.json').read_text())
    if hardware_dir is None:
        for name,data in [('puzzle5ev.bit',bit),('puzzle5ev.hwh',hwh)]:
            if hashlib.sha256(data).hexdigest()!=original['files'][name]:
                raise ValueError(f'Reference identity mismatch: {name}')
    bootgen=amd_root/'Vivado/bin/bootgen.bat'
    if not bootgen.is_file():raise FileNotFoundError(bootgen)
    # 拒绝覆盖已有包，保留每次发布的来源与失败现场。
    target.mkdir(parents=True)
    (target/'puzzle5ev.bit').write_bytes(bit)
    (target/'puzzle5ev.hwh').write_bytes(hwh)
    (target/'bitstream.bif').write_text('all:\n{\n  "puzzle5ev.bit"\n}\n',encoding='utf-8')
    subprocess.run([str(bootgen),'-arch','zynqmp','-image','bitstream.bif','-process_bitstream','bin','-w'],cwd=target,check=True)
    (target/'bitstream.bif').unlink()
    for p in (root/'board/hardware/linux').iterdir():
        if p.is_file():shutil.copyfile(p,target/p.name)
    for name in ['load_hardware.py','start_hdmi_x.sh','setup_x11_fonts.sh','setup_m0.sh',
                 'xorg-hdmi.conf','99-ec26e-camera.rules','99-ec26e-m0.rules',
                 'fix_ch341_baud.py','validate_accelerators.py']:
        shutil.copyfile(root/'board/runtime'/name,target/name)
    for name in ['wqy-microhei-ui.ttf','copyright']:
        shutil.copyfile(root/'board/fonts'/name,target/name)
    if hardware_dir is None:
        for name in ['puzzle5ev.bit.bin','puzzle5ev.dtbo','ec26e_accel.ko','ec26e_hdmi_fb.ko']:
            if hashlib.sha256((target/name).read_bytes()).hexdigest()!=original['files'][name]:
                raise ValueError(f'Preserved hardware identity mismatch: {name}')
    files={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(target.iterdir()) if p.is_file()}
    manifest={'board':'axu5ev-p','part':'xczu5ev-sfvc784-2-i','vivado':'2026.1',
              'kernel':'5.4.0-xilinx-v2020.1','files':files,'reference_hardware':hardware_dir is None,
              'hardware_acceptance':False,'active_boot_modified':False,'motion_executed':False}
    (target/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    typer.echo(f'Prepared only, not loaded: {target}')

if __name__=='__main__':
    logging.basicConfig(level=logging.INFO,format='%(levelname)s %(name)s %(message)s')
    app()
