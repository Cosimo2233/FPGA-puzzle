# 应用层源文件

该目录用于放置应用初始化、任务调度和产品业务流程，例如：

```text
app.c
state_machine.c
```

程序入口仍保留在 `src/main.c`。应用层可以调用 `drivers/` 和 `modules/`，不建议直接堆积底层寄存器操作。
