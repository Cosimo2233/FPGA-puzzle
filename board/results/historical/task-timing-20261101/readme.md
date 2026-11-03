# 点击运行后的任务计时

2026-11-01，在 AXU5EV-P 原应用 `020d423` 基础上增加菜单计时，覆盖固定四片、随机拼图和七巧板。

计算时间从菜单收到运行点击（键盘、实体键确认同样适用）开始，包含释放预览相机、Python/配置加载、采图、识别、求解、软件检查和执行器构造，至方案交给原运动执行器之前。总时间从同一次点击计至执行器返回并完成任务函数的收尾。仅规划时总时间计至规划返回，不表示完成了实物拼装。

使用跨进程共享基准的单调时钟，避免板端日期不准影响计时。每次任务使用独立令牌和原子 `.timing.json`，重复点击运行中的任务不会重置时间。正常结束后保持两个数值；取消、失败和任务异常退出有独立显示。计时写入失败不会中断原任务。日志新增 `TASK_TIMING`。

## 验证

在 `sw/` 下通过 Poetry 执行以下相关测试，结果 **147 passed，34 subtests passed**：

```powershell
poetry run pytest tests/unit/test_task_timing.py tests/unit/test_board_menu.py tests/unit/test_plan_preview.py tests/unit/test_tangram.py tests/unit/test_warm_task.py tests/unit/test_menu_model.py tests/unit/tasks/test_task1_actuation.py tests/integration/tasks/test_task1_application.py tests/integration/tasks/test_task2_application.py -q
poetry build --format wheel
```

受控时钟测试覆盖点击到子进程启动的延迟、计算/运动分界、结束后冻结、重复启动、取消、运动异常、非法启动时间、计时写入故障及旧令牌隔离。七巧板链路测试使用真实分割/求解和假执行器验证计算时间在执行器调用前固定。

在 FPGA 上核对已安装 Python 源码与发布 wheel，使用保存的白片照片和已有标定执行一次只规划验证，没有重新打开相机，也没有构造运动执行器。计算用时 7.328 s，总用时 7.331 s；两者均显示 7.33 s，属于无运动的单次回放结果，不代表实际搬运性能。菜单计时标签完整位于 1280×720 HDMI 范围内。

- [板端结果](result.json)
- [HDMI 计时显示](timing-replay.png)
- [部署文件及配置哈希](deployed.json)

## 部署与恢复

最终发布目录为 `/opt/ec26e/releases/task-timing-20261101-r2`。应用 wheel SHA256 为 `d91014fbba31d1b88b8a02e5092c42c5cd750602971d2464540cf08062d55a93`。求解器 wheel 保持 `511c237` 对应产物，界面字体已重新生成并核对计时文字字符覆盖。

安装前核验板端应用/求解器源码与上次恢复的可信 wheel 一致，无额外 Python 源码。两次安装均通过执行锁、空闲状态检查及现有备份/回滚安装器完成，保留当前全部配置、机械标定和试机权限。第二次仅调整计时文字与任务状态共用底部一行，保留预览和健康状态的显示空间。没有修改硬件、PL、启动脚本或启动镜像。生产菜单已重新运行，未由本次变更启动实物运动。

第一份发布目录 `/opt/ec26e/releases/task-timing-20261101/production-before` 保存增加计时前的完整应用、求解器、配置及字体备份；`-r2/production-before` 保存第一次计时版本。若需恢复到增加计时前的版本，在菜单空闲时使用第一份备份：

```sh
. /opt/ec26e/env.sh
python /opt/ec26e/releases/task-timing-20261101/install_tangram_release.py \
  --release /opt/ec26e/releases/task-timing-20261101 --rollback --apply
```

真实运动结束后的计时显示仍需用户下一次实物运行验收；本次保留原有 1° 启动、后续拾取 2° 的运动行为。
