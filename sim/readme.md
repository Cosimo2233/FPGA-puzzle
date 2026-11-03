# 仿真与软件测试

| 目录 | 内容 |
|---|---|
| `tests/unit/` | 单元测试 |
| `tests/integration/` | 应用链路集成测试 |
| `tests/fixtures/` | 离线图像和轮廓输入 |
| `tests/external/` | 相机、串口、运动和 GPIO 现场测试 |
| `hls/` | 四个 HLS 核的 C++ 测试平台 |
| `rtl/` | 两个 HDMI 扫描测试平台 |

以下命令在仓库根目录执行：

```powershell
./sim/run_tests.ps1 -All
./sim/run_tests.ps1 -Full -All
./build/build.ps1 -Stage rtl -AmdRoot $env:AMD_ROOT
& "$env:AMD_ROOT/Vitis/bin/vitis-run.bat" --mode hls --tcl sim/hls_csim.tcl
```

HLS 输入向量位于 `../data/hls/`。主机测试直接引用仓库内的应用、配置和几何基准，不需要相邻源码目录。

默认入口按 `host_profile.json` 列出平台要求和已知失败；`-Full` 不采用该配置中的排除项。Linux 文件锁、设备路径和符号链接相关测试需在匹配环境中运行。现场测试不纳入默认收集。

测试结果及已知问题见 [验证报告](../build/reports/current/validation_summary.md)。
