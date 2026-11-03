# Poetry 与 Typer CLI 约定

用于创建、修复或规范化 Python CLI 入口、Poetry 脚本、运行命令和日志模式。

## Poetry 使用

默认使用 Poetry 命令：

```bash
poetry install
poetry run python -m <package_name>.commands.app
poetry run pytest
```

不要假设系统 Python 环境可用。需要运行 Python 命令时，优先使用 Poetry 虚拟环境。

## Poetry scripts

如果需要在 `pyproject.toml` 中暴露命令行入口，优先使用 Poetry scripts，并将入口指向 Typer app 对象：

```toml
[tool.poetry.scripts]
app = "<package_name>.commands.app:app"
```

如果项目已有更具体的命令名，可以保留该命令名，但入口模块仍应遵循 `<package_name>.commands.app` 约定。

如果既有项目已经使用 PEP 621 的 `[project.scripts]` 且项目约定明确，可以保留现状，避免为了统一而做无关迁移。

## CLI 模块

创建或规范化如下 CLI 入口：

```text
<package_name>/commands/app.py
```

基础模式：

```python
from __future__ import annotations

import logging
from pathlib import Path

import typer

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s [%(name)s] %(message)s",
)
logger = logging.getLogger(__name__)

app = typer.Typer()


@app.command()
def main(
    input_path: Path = typer.Argument(
        ...,
        help="输入文件路径",
        envvar="APP_INPUT_PATH",
    ),
    output_dir: Path = typer.Option(
        Path("outputs"),
        "--output-dir",
        "-o",
        help="输出目录",
        envvar="APP_OUTPUT_DIR",
    ),
) -> None:
    logger.info("Starting app")
    logger.info("Input path: %s", input_path)
    logger.info("Output dir: %s", output_dir)


if __name__ == "__main__":
    app()
```

命令必须使用 `@app.command()` 注册。尽量避免 `print(...)`；只有明确需要标准输出作为程序结果时才使用。
