---
name: python-poetry
description: Use this skill as the main entry point for Python project work, including creating, updating, running, testing, reviewing, or standardizing Python projects with Poetry, Typer CLI entry points, Chinese README documentation, logging conventions, and VS Code debugging configuration. For Python gRPC/Protobuf work, route to the protos skill. For deep learning training framework work, route to the deep-learning-training-framework skill.
metadata:
  short-description: Python project conventions with Poetry
---

# Python 工程约定

当任务涉及任何 Python 工程时，默认使用本 skill，包括创建、更新、运行、测试、调试、审查或规范化 Python 项目。修改项目文件前，先检查项目结构、入口模块、依赖工作流、日志配置和编辑器配置，确保项目约定一致。

## 沟通与编辑原则

- 默认使用中文与用户沟通。
- 修改代码前先理解项目结构、入口、调用链和已有风格。
- 优先做最小必要修改，避免无关重构。
- 如果项目已有更具体、更明确的约定，优先遵循项目内约定。
- `README.md` 必须使用中文编写。

## 标准工作流程

1. 修改前检查仓库结构、现有包名、`pyproject.toml`、CLI 模块、测试和 README。
2. 使用 Poetry 管理依赖、虚拟环境、脚本执行、测试和应用启动；不要假设系统 Python 环境可用。
3. 确保 Python CLI 入口模块位于 `<package_name>/commands/app.py`，使用 Typer，并暴露 `app = typer.Typer()`。
4. CLI 参数必须使用 `typer.Option` 和 `typer.Argument`；适合通过环境变量配置的参数必须显式设置 `envvar`。
5. 在 `app.py` 中使用 `logging.basicConfig(...)` 配置基础日志；各模块使用模块级 `logger = logging.getLogger(__name__)`。
6. 新增或更新 `.vscode/launch.json` 时，提供基于 module 的 `<package_name>.commands.app` 调试配置，并确保每个配置都包含 `"envFile": "${workspaceFolder}/.env"`。
7. 可行时通过 Poetry 运行验证命令，例如 `poetry install`、`poetry run pytest` 或项目对应命令。
8. 完成前说明修改了哪些文件，以及运行过哪些验证命令；如果无法验证，简要说明原因。

## 按需参考资料

不要在普通 Python 任务中预先加载所有参考资料。只有命中下列场景时，再读取对应文件：

- 新建或初始化 Python Poetry 项目时，读取 `references/python-project-init.md`。
- 需要创建、修复或规范化 Typer CLI、Poetry 入口脚本、日志模式或运行命令时，读取 `references/python-cli.md`。
- 需要新增或调整 VS Code 调试配置时，读取 `references/vscode-debug.md`。
- 需要收尾检查或统一交付说明时，读取 `references/final-check.md`。
- 任务涉及 `.proto`、gRPC、Protobuf、`grpcio-tools`、`mypy-protobuf`、`*_pb2.py`、`*_pb2_grpc.py`、生成脚本或内置 proto 文件时，使用 `protos` skill；本 skill 只提供 Poetry 环境和 Python 工程约定。
- 任务涉及深度学习训练框架、Lightning 风格训练、模型注册、数据管道、typed YAML 配置、checkpoint、resume、export、实验日志或训练测试时，使用 `deep-learning-training-framework` skill；本 skill 只提供 Poetry 环境和 Python 工程约定。

参考资料中的规则与本文件冲突时，优先遵循更具体的任务场景规则；如果项目内已有更明确约定，仍优先遵循项目内约定。
