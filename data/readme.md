# 测试输入与依赖

`hls/` 保存硬件数值测试向量，`baselines/upstream/` 保存固定与随机拼图的几何输入及参考输出。图像与七巧板夹具位于 `../sim/tests/fixtures/`。

`dependencies/` 包含几何求解器和桥接客户端的固定 wheel。其版本、来源和 SHA-256 见 `versions.json`；公共 Python 依赖由 `../src/ps/poetry.lock` 锁定。

`submission_manifest.json` 记录交付文件的 SHA-256。修改文件后应更新清单，再运行 `build/validate_repository.py manifest --verify` 检查一致性。
