# 测试

`test_bridge.c` 在本机运行固件相同的协议和状态机，覆盖转发、错误恢复、
使能和超时行为。执行根目录 `gmake test`。

`wire_vectors.c` 由 `host/tests/test_protocol.py` 编译运行，将 C 编码结果
与 Python 编码结果逐字节比较，避免两端协议各自自洽却互不兼容。

嵌入式固件仍由根目录 makefile 构建；`tests/` 默认不会进入目标固件，也不会被 `src/` 的递归源码扫描收集。
