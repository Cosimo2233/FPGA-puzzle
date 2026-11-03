# 开发技能

本目录包含 FPGA 工程和 Python 软件开发的两套工作流程。各子目录中的 `SKILL.md` 为技能定义，`references/` 提供配套参考资料。

| 技能 | 主要内容 | 使用条件 |
|---|---|---|
| `xilinx-fpga/` | Vivado/Vitis 工程、RTL/HLS、IP、约束、构建与板级验证 | 需要对应工具、器件支持和板级资料；上板检查还需要目标硬件 |
| `python-poetry/` | Poetry 依赖管理、Typer 入口、日志、测试与调试配置 | 需要匹配的 Python 环境；设备、文件锁和显示测试需满足平台要求 |

使用时先阅读对应的 `SKILL.md`，再按任务选择参考章节。FPGA 工程检查涵盖 XPR/BD、HLS ABI、时钟约束和硬件交付文件；Python 检查涵盖依赖锁、公共 API 和软件回归。

项目中的故障案例与验证记录分别位于 `report/engineering/` 和 `board/results/historical/`，测试结果见 `build/reports/current/`。更换板卡时，应重新核对地址映射、时钟复位、内核和外设配置。
