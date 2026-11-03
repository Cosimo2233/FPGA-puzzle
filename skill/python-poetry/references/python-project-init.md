# Python 项目初始化

用于新建或初始化 Poetry Python 项目。

## pyproject.toml

首先编辑 `pyproject.toml`，引入清华源。源配置放在 `[build-system]` 上方，即 TOML 文件的倒数第二项：

```toml
[[tool.poetry.source]]
name = "tuna"
url = "https://pypi.tuna.tsinghua.edu.cn/simple"
priority = "primary"
```

`requires-python` 统一为：

```toml
requires-python = ">=3.10,<3.13"
```

如果项目已有更严格且合理的 Python 版本约束，先遵循项目内约定。

## 虚拟环境

使用 Python 3.10 创建 Poetry 虚拟环境：

```bash
poetry env use python3.10
poetry install
```

## .gitignore

创建并编辑 `.gitignore`。如果当前 skill 目录已有可复用的 `.gitignore` 模板，优先复制其内容到目标项目。

## Git 分支

初始化完成后提交一次：

```bash
git add .
git commit -m init
```

创建 `dev` 和 `feature/<项目名>` 分支，并切换到 `feature/<项目名>`：

```bash
git branch dev
git switch -c feature/<项目名>
```
