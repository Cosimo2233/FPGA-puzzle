# Puzzle5EV 多模式视觉拼图系统

Puzzle5EV 基于 AXU5EV-P 开发板，实现固定四片拼图、随机 2～4 片拼图和十二图案七巧板。系统完成图像采集、轮廓识别、几何规划、HDMI 预览及运动任务编排，并通过 PS/PL 协同加速图像预处理和部分几何计算。

目标器件为 `xczu5ev-sfvc784-2-i`，开发工具为 AMD Vivado/Vitis 2026.1。默认配置采用 CPU 后端和仅规划模式，关闭运动与实体按键。实物执行前需完成接线检查、相机和机械标定，并取得完整有效方案。

## 工程结构

目录按参赛工程推荐结构组织，各目录职责如下：

```text
FPGA-puzzle/
├─ README.md      # 项目简介、环境要求与复现步骤
├─ src/          # 设计源码：RTL、HLS、PS 侧软件及配套固件
├─ sim/          # 仿真与验证脚本、测试平台、结果索引
├─ build/        # 可复现构建脚本、综合与实现报告、当前验证结果
├─ board/        # 上板工程、运行脚本、匹配硬件文件与实测输出
├─ data/         # 测试数据、参考结果、固定依赖与校验清单
├─ skill/        # 技能包
│  ├─ README.md
│  ├─ xilinx-fpga/
│  └─ python-poetry/
└─ report/       # 设计报告归档位置与工程技术资料
```

| 目录 | 内容 |
|---|---|
| `src/` | RTL、HLS、约束、板端 Python 应用和 M0 桥接固件 |
| `sim/` | 软件测试、离线夹具、RTL/HLS 测试平台；验证结果由 `sim/readme.md` 索引 |
| `build/` | 构建、校验和清理脚本；`reports/reference/` 为综合与实现报告，`reports/current/` 为当前验证结果 |
| `board/` | Vivado 工程、上板脚本、Linux 模块、启动镜像和实测记录 |
| `data/` | HLS 向量、几何基准、独立依赖包和校验清单 |
| `skill/` | FPGA 工程与 Python 开发技能及参考资料 |
| `report/` | 设计报告待归档；现有工程说明、接口文档、来源索引和协作记录目录，见 `report/readme.md` |

Vivado 入口为 `board/project/puzzle5ev.xpr`，BD 位于 `board/project/puzzle5ev.srcs/sources_1/bd/system/system.bd`。首次打开工程前，先运行 HLS 构建恢复 IP。后续构建使用保存的 BD。

## 软件环境

需要 Python 3.10～3.12 和 Poetry 2。以下命令在仓库根目录的 PowerShell 中执行：

```powershell
$psProject = (Resolve-Path src/ps).Path
poetry -C $psProject env use (py -3.12 -c "import sys; print(sys.executable)")
poetry -C $psProject install
./build/build.ps1 -Stage check
./sim/run_tests.ps1 -All
./build/build.ps1 -Stage software
```

几何求解器和桥接客户端使用 `data/dependencies/` 中的固定 wheel，通过公共 API 调用。依赖版本及 SHA-256 见 `data/dependencies/versions.json`，公共 Python 依赖由 `src/ps/poetry.lock` 锁定。

默认测试入口按 `sim/host_profile.json` 列出平台要求和已知失败。`-Full -All` 运行完整单元与集成测试。相机、串口、运动和 GPIO 测试位于 `sim/tests/external/`，需在相应硬件环境中单独执行。测试范围见 [验证报告](build/reports/current/validation_summary.md)。

## 硬件构建

```powershell
$env:AMD_ROOT = 'D:/AMD/2026.1' # 按实际安装位置设置
./build/build.ps1 -Stage hls -AmdRoot $env:AMD_ROOT -Cosim
./build/build.ps1 -Stage project -AmdRoot $env:AMD_ROOT
./build/build.ps1 -Stage compile -AmdRoot $env:AMD_ROOT
./build/build.ps1 -Stage rtl -AmdRoot $env:AMD_ROOT
```

HLS 源码、测试平台和向量分别位于 `src/hls/`、`sim/hls/` 和 `data/hls/`。完整实现输出到 `board/generated/hardware/`。`build/reports/reference/` 保存参考实现报告，`build/reports/current/` 保存源码验证记录。

参考硬件以压缩 XSA 保存于 `board/hardware/reference/`。以下命令提取 bit/HWH，并生成 FPGA manager 所需的 `.bit.bin` 和匹配的部署清单：

```powershell
./build/build.ps1 -Stage package -AmdRoot $env:AMD_ROOT
```

输出目录为 `board/generated/bundle/`。匹配的启动镜像为 `board/boot/image-puzzle5ev.ub`，Linux 模块适用于 `5.4.0-xilinx-v2020.1`。出厂设备树仍绑定旧 PL 地址，加载前必须启动匹配镜像。操作步骤见 [板端部署](board/readme.md)。

## 操作与计时

菜单提供任务选择、摄像头预览、开始和取消。规划完成后显示源轮廓、目标落点、旋转角和取放顺序。

- **计算时间**：从点击运行到完整方案交给运动执行器之前。
- **总时间**：从同一次点击到执行器返回并完成任务收尾。

任务结束后保留两项时间，下次运行重新计时。仅规划模式的总时间截至规划完成；取消和失败分别显示对应状态。计时采用单调时钟，日志标记为 `TASK_TIMING`。

## 文件维护

`./build/clean.ps1 -WhatIf` 预览清理范围，`./build/clean.ps1` 删除生成目录。源码、BD、约束、测试输入、依赖和报告保留。修改文件后，运行以下命令更新并验证校验清单：

```powershell
poetry -C $psProject run python ../../build/validate_repository.py manifest
poetry -C $psProject run python ../../build/validate_repository.py manifest --verify
```

技能使用见 [开发技能](skill/README.md)，待完成材料见 [提交材料清单](report/submission_notes.md)。正式设计报告单独整理，不包含在当前仓库中。

## 开源许可

本项目原创代码及配套文档采用 [MIT License](LICENSE)，允许使用、修改和再分发，使用时须保留版权声明和许可文本。

第三方代码、字体、依赖包及厂商资料保留各自的版权声明和许可证，不因本项目采用 MIT 而变更。文泉驿字体许可见 [字体版权说明](board/fonts/copyright)，依赖版本和来源见 [依赖清单](data/dependencies/versions.json)。
