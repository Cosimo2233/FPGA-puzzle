"""解压有授权来源的字体输入，按当前菜单与图案名称重新生成中文子集。"""
import gzip
import logging
from pathlib import Path
import subprocess
import sys
import typer

app=typer.Typer(add_completion=False)
@app.command()
def main():
    root=Path(__file__).resolve().parents[1]
    temporary=root/'build/generated/font'
    temporary.mkdir(parents=True,exist_ok=True)
    font=temporary/'wqy-microhei.ttc'
    # 完整字体仅在生成区展开，交付时保留压缩输入以减少体积。
    with gzip.open(root/'data/fonts/wqy-microhei.ttc.gz','rb') as compressed:
        font.write_bytes(compressed.read())
    subprocess.run([sys.executable,str(root/'build/axu5ev/prepare_ui_font.py'),
        '--font',str(font),'--license',str(root/'board/fonts/copyright'),
        '--root',str(root),'--output',str(root/'board/fonts')],check=True)
    typer.echo('Regenerated board/fonts; no board was accessed.')
if __name__=='__main__':
    logging.basicConfig(level=logging.INFO,format='%(levelname)s %(name)s %(message)s')
    app()
