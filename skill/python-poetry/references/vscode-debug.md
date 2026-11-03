# VS Code 调试配置

用于新增或调整 Python 项目的 `.vscode/launch.json`。

Python 项目应提供 `.vscode/launch.json`。该文件必须包含一个用于调试 app module 的配置：

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Python: App Module",
      "type": "python",
      "request": "launch",
      "module": "<package_name>.commands.app",
      "console": "integratedTerminal",
      "justMyCode": true,
      "envFile": "${workspaceFolder}/.env",
      "args": []
    }
  ]
}
```

新增任何 VS Code 调试配置时，都必须包含：

```json
"envFile": "${workspaceFolder}/.env"
```
