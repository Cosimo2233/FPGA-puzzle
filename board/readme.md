# AXU5EV-P 板端复现

`project/puzzle5ev.xpr` 与保存的 BD 为正式硬件入口；先执行根目录 README 中的 HLS 恢复，再打开工程。`runtime/` 包括板端启动、加载、相机/桥接别名、CPU/PL 验证和驱动源码；其中 Linux Kbuild 文件交付名为 `makefile`，`build_modules.sh` 在临时构建区恢复内核要求的 `Makefile` 名称。

`hardware/reference/puzzle5ev.xsa` 是压缩的已验证参考设计，`original_manifest.json` 为原构建证据，保持原始内容不改写；它记录的源目录为历史目录，不能作为新仓库的路径配置。执行 `build/package_hardware.py` 重新生成交付清单后才得到当前路径的可部署 bundle。

## 软件运行准备

板端环境为 AArch64 PetaLinux 2020.1、Linux 5.4、独立 CPython 3.11.17。准备匹配的 Python 与离线 wheelhouse；不能复用 ARMv7 包，也不覆盖厂商 Python。依赖来源和版本见 `runtime/readme.md`、`runtime/requirements-runtime-aarch64.txt`。OpenCV/NumPy 等平台包未全部打入仓库，由对应锁文件或 AArch64 清单获取。

1. 在开发机运行 `build/build.ps1 -Stage software`，取得 `board/software` 下应用及两个独立依赖 wheel。
2. 将 wheel、Python 运行时和 `runtime/` 安装脚本传到板端暂存区，校验哈希。按 `install_runtime_offline.sh` 创建独立应用环境。
3. 备份原始启动文件和恢复入口，选择 `boot/image-puzzle5ev.ub` 匹配的基底。默认不覆盖原 `BOOT.BIN`，不要把出厂设备树下的加载当作可用步骤。
4. 用根目录的 package 阶段重建硬件 bundle，上传到板端新的暂存目录；先运行 `load_hardware.py --bundle <目录>` 的只读预检。实际加 `--apply` 才会加载硬件，需单独执行板级验证。
5. 按包内脚本安装中文字体、启动 HDMI Xorg 和菜单；核对 HDMI OUT、USB 摄像头别名、分辨率和桥接身份。
6. 使用 `menu-config.example.json`、`board.env.example` 和 `tasks/`，保持 CPU、仅规划、禁用实体按键和运动。先验证识别、三模式完整规划与错误提示，再比较 CPU/PL 输出。

固定/随机与七巧板的实物状态以带日期的 `results/historical` 记录为准，不能从配置开关推导验收完成。USB3 Hub 热重启恢复仍需现场复核，异常恢复优先按已有安全关机和冷启动流程。

## M0 固件

完整手写固件与 SysConfig 输入在 `../src/firmware/m0`。使用 TI MSPM0 SDK 2.04.00.06、TI Arm Clang 4.0.4.LTS 和 SysConfig；工具路径均可用 Makefile 变量覆盖。在该目录使用 `gmake -f makefile all` 构建，`gmake -f makefile test` 为主机 C 行为测试。烧录前应检查固件版本、接线和供电。

`firmware/firmware.bin` 和 `firmware.hex` 为原发布参考，构建身份见来源清单；未把新源码编译等同于新板级验收。串口 ACK、转轴等待返回都不能证明真实 XYZ 到位或电磁铁吸起。
