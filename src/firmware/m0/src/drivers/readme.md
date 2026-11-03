# 板级驱动源文件

该目录用于放置与开发板硬件直接相关的驱动实现，例如：

```text
led.c
key.c
uart.c
adc.c
pwm.c
```

新增的 `.c` 文件会被 makefile 递归发现，并在 `build/obj/drivers/` 中生成对应目标文件和依赖文件。
