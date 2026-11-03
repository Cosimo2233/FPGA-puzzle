# 构建与清理说明

正式入口为 `build.ps1`，在仓库根运行。`-Stage check` 做源码交付结构及身份检查，`hls` 重新生成四核 IP，`project` 打开保存的工程并校验 BD，`compile` 执行综合/布局布线，`rtl` 执行 HDMI 仿真，`software` 打包应用，`package` 从压缩参考设计恢复上板包。工具根目录通过 `-AmdRoot` 或 `AMD_ROOT` 指定，不依赖原开发工程。

各阶段会在 `build/generated`、`board/generated` 和 Vivado 工程的标准生成目录写入中间文件。`clean.ps1` 先检查工具进程，再核对待删绝对路径确实位于本仓库下，只删除这些生成内容。保存的 XPR、BD、源码、依赖锁、测试向量、技能和验证报告不属于清理对象。先用 `-WhatIf` 查看范围。

`rebuild_font.py` 从 `data/fonts` 的压缩字体源恢复完整 TTC，然后按当前软件和求解器公共图案名称生成 UI 子集；原字体许可证保留在 `board/fonts/copyright`。只保留子集会影响未来新增汉字后的重建，因此压缩字体源是必要输入，不是缓存。

`package_hardware.py` 默认组装原已验证 XSA 对应的参考包，只恢复文件、不访问板卡；`--hardware-dir` 可选择本次重新实现的产物目录，但新设计必须另行核对设备树、驱动与上板结果，不能沿用原实测结论。

## 报告区分

- `reports/reference/`：原成功完整实现的资源、时序、DRC、HLS 综合和协同仿真报告。
- `reports/current/`：源码检查的软件、C 仿真、RTL、固件主机测试、工程引用和文件校验结果。
- 原始测试失败保持原断言；主机适用入口的排除项见 `sim/host_profile.json`。最终通过数和限制统一见验证报告。

`validate_repository.py manifest` 生成交付 SHA-256 清单；任何修改后都需重跑检查，再生成新清单。`manifest --verify` 只读验证，不改文件。
