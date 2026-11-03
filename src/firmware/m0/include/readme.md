# 用户头文件

该目录是用户头文件的搜索根目录：

```text
include/app/       应用层接口
include/drivers/   板级驱动接口
include/modules/   可复用模块接口
```

引用子目录头文件时使用完整相对路径：

```c
#include "app/app.h"
#include "drivers/uart.h"
#include "modules/control.h"
```

SysConfig 生成的头文件位于 `build/syscfg/`，不要复制到这里，也不要手工修改生成文件。
