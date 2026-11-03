# 最终检查

完成 Python 项目修改前，检查以下事项：

- 是否理解并遵循了项目内更具体的约定。
- 是否避免了无关重构和不必要的依赖变更。
- 是否使用 Poetry 运行安装、脚本、测试或应用启动命令。
- CLI 入口是否位于 `<package_name>/commands/app.py`。
- Typer CLI 是否暴露 `app = typer.Typer()`，命令是否使用 `@app.command()` 注册。
- CLI 参数是否使用 `typer.Option` 或 `typer.Argument`，需要环境变量配置的参数是否显式设置 `envvar`。
- `app.py` 是否配置 `logging.basicConfig(...)`，各模块是否使用模块级 logger。
- `.vscode/launch.json` 中每个调试配置是否包含 `"envFile": "${workspaceFolder}/.env"`。
- `README.md` 是否使用中文。

最终回复中说明修改了哪些文件，以及运行过哪些验证命令。如果无法验证，简要说明原因。
