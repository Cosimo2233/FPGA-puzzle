# 提交材料清单

## 功能与验证资料

当前软件支持十二图案七巧板，并包含轮廓识别、旋转零位容差和运行计时功能。实验记录按日期保存在 `board/results/historical/`，构建与测试结果位于 `build/reports/`。编写设计报告时，各组实验应注明对应版本、输入、平台和计时范围。

十二图案与软件接口见 [七巧板说明](engineering/tangram.md)，运行计时的板端记录见 [计时验证](../board/results/historical/task-timing-20261101/readme.md)。

## 材料清单

| 材料 | 位置或状态 |
|---|---|
| 工程源码与保存的 XPR/BD | `src/`、`board/project/` |
| 构建、仿真及实现报告 | `build/`、`sim/` |
| 上板脚本与实测记录 | `board/` |
| 测试输入与参考结果 | `data/`、`sim/tests/fixtures/` |
| 开发技能 | `skill/` |
| 正式设计报告 | 单独编写，当前仓库不包含 Word 或 PDF 设计报告 |
| 大模型协作原始记录 | `collaboration/`，待补充完整导出 |
| 开源许可 | 已采用 MIT，见根目录 `LICENSE`；第三方组件保留各自许可 |
| 团队信息、决赛英文海报 | 待完成 |

## 第三方资料

AMD 工具和 TI SDK 按各自许可安装。厂家 HDMI 初始化参考保留原版权声明；字体许可证位于 `board/fonts/copyright`。依赖版本和来源统一记录在 `data/dependencies/versions.json`，公开发布前需确认各组件的授权范围。
