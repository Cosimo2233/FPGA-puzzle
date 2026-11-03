# MSPM0G3507 standalone TI Arm Clang build

PROJECT_NAME ?= firmware
MSPM0_SDK_ROOT ?= D:/ti/mspm0_sdk_2_04_00_06
TICLANG_ROOT ?= D:/ti/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS
SYSCONFIG_ROOT ?= D:/ti/SYSCONFIG
BUILD_DIR ?= ../../../build/generated/m0
OPT_LEVEL ?= -O2

SOURCE_DIR := src
INCLUDE_DIR := include
CONFIG_DIR := config
OBJECT_DIR := $(BUILD_DIR)/obj
SYSCONFIG_DIR := $(BUILD_DIR)/syscfg

SYSCONFIG_FILE := $(CONFIG_DIR)/app.syscfg
PRODUCT_JSON := $(MSPM0_SDK_ROOT)/.metadata/product.json
STARTUP_SOURCE := $(MSPM0_SDK_ROOT)/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c

CC := "$(TICLANG_ROOT)/bin/tiarmclang.exe"
OBJCOPY := "$(TICLANG_ROOT)/bin/tiarmobjcopy.exe"
SYSCONFIG_CLI := "$(SYSCONFIG_ROOT)/sysconfig_cli.bat"

# Recursively collect C sources while preserving their path below src/.
# Example: src/drivers/uart.c -> build/obj/drivers/uart.obj
rwildcard = $(foreach item,$(wildcard $1*),$(call rwildcard,$(item)/,$2) $(filter $(subst *,%,$2),$(item)))
USER_SOURCES := $(strip $(call rwildcard,$(SOURCE_DIR)/,*.c))
USER_OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(OBJECT_DIR)/%.obj,$(USER_SOURCES))
SYSCONFIG_SOURCE := $(SYSCONFIG_DIR)/ti_msp_dl_config.c
SYSCONFIG_HEADER := $(SYSCONFIG_DIR)/ti_msp_dl_config.h
SYSCONFIG_OBJECT := $(OBJECT_DIR)/ti_msp_dl_config.obj
STARTUP_OBJECT := $(OBJECT_DIR)/startup_mspm0g350x_ticlang.obj
OBJECTS := $(USER_OBJECTS) $(SYSCONFIG_OBJECT) $(STARTUP_OBJECT)
DEPENDENCIES := $(OBJECTS:.obj=.d)

DEVICE_OPT := $(SYSCONFIG_DIR)/device.opt
LINKER_COMMAND := $(SYSCONFIG_DIR)/device_linker.cmd
GENERATED_LIBS := $(SYSCONFIG_DIR)/device.cmd.genlibs
SYSCONFIG_OUTPUTS := $(SYSCONFIG_SOURCE) $(SYSCONFIG_HEADER) $(DEVICE_OPT) $(LINKER_COMMAND) $(GENERATED_LIBS)

OUTPUT := $(BUILD_DIR)/$(PROJECT_NAME).out
MAP_FILE := $(BUILD_DIR)/$(PROJECT_NAME).map
HEX_FILE := $(BUILD_DIR)/$(PROJECT_NAME).hex
BIN_FILE := $(BUILD_DIR)/$(PROJECT_NAME).bin

CFLAGS := \
    -I$(INCLUDE_DIR) \
    -I$(SYSCONFIG_DIR) \
    @$(DEVICE_OPT) \
    $(OPT_LEVEL) \
    -isystem "$(MSPM0_SDK_ROOT)/source/third_party/CMSIS/Core/Include" \
    -isystem "$(MSPM0_SDK_ROOT)/source" \
    -gdwarf-3 \
    -mcpu=cortex-m0plus \
    -march=thumbv6m \
    -mfloat-abi=soft \
    -mthumb \
    -std=c11 \
    -Wall -Wextra -Werror \
    -MMD \
    -MP

LFLAGS := \
    -L$(SYSCONFIG_DIR) \
    -ldevice.cmd.genlibs \
    "-L$(MSPM0_SDK_ROOT)/source" \
    $(LINKER_COMMAND) \
    -Wl,--stack_size=2048 \
    "-Wl,-m,$(MAP_FILE)" \
    -Wl,--rom_model \
    -Wl,--warn_sections \
    "-L$(TICLANG_ROOT)/lib" \
    -llibc.a

.PHONY: all syscfg clean test

all: $(OUTPUT) $(HEX_FILE) $(BIN_FILE)

# Native tests exercise the same protocol/state machine used in the MCU.
HOST_CC ?= gcc
test: | $(BUILD_DIR)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iinclude tests/test_bridge.c src/modules/bridge.c -o $(BUILD_DIR)/test_bridge.exe
	$(BUILD_DIR)/test_bridge.exe
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Itests/fake_hw -Iinclude tests/test_uart_faults.c src/modules/bridge.c src/modules/uart_diagnostics.c -o $(BUILD_DIR)/test_uart_faults.exe
	$(BUILD_DIR)/test_uart_faults.exe

syscfg: $(SYSCONFIG_OUTPUTS)

$(BUILD_DIR) $(OBJECT_DIR) $(SYSCONFIG_DIR):
	@ powershell.exe -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$@' | Out-Null"

$(SYSCONFIG_OUTPUTS) &: $(SYSCONFIG_FILE) | $(SYSCONFIG_DIR)
	@ echo Generating SysConfig files...
	@ $(SYSCONFIG_CLI) --compiler ticlang --product "$(PRODUCT_JSON)" --output "$(SYSCONFIG_DIR)" "$(SYSCONFIG_FILE)"
	@ powershell.exe -NoProfile -Command "Get-Item '$(SYSCONFIG_DIR)/ti_msp_dl_config.c','$(SYSCONFIG_DIR)/ti_msp_dl_config.h','$(SYSCONFIG_DIR)/device.opt','$(SYSCONFIG_DIR)/device_linker.cmd','$(SYSCONFIG_DIR)/device.cmd.genlibs' | ForEach-Object { $$_.LastWriteTime = Get-Date }"

$(OBJECT_DIR)/%.obj: $(SOURCE_DIR)/%.c $(SYSCONFIG_HEADER) makefile | $(OBJECT_DIR)
	@ echo Building $@
	@ powershell.exe -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(@D)' | Out-Null"
	@ $(CC) $(CFLAGS) -MF "$(@:.obj=.d)" -c "$<" -o "$@"

$(SYSCONFIG_OBJECT): $(SYSCONFIG_SOURCE) $(SYSCONFIG_HEADER) makefile | $(OBJECT_DIR)
	@ echo Building $@
	@ $(CC) $(CFLAGS) -MF "$(@:.obj=.d)" -c "$(SYSCONFIG_SOURCE)" -o "$@"

$(STARTUP_OBJECT): $(STARTUP_SOURCE) $(SYSCONFIG_HEADER) makefile | $(OBJECT_DIR)
	@ echo Building $@
	@ $(CC) $(CFLAGS) -MF "$(@:.obj=.d)" -c "$(STARTUP_SOURCE)" -o "$@"

$(OUTPUT): $(OBJECTS) $(LINKER_COMMAND) $(GENERATED_LIBS) | $(BUILD_DIR)
	@ echo Linking $@
	@ $(CC) -Wl,-u,_c_int00 $(OBJECTS) $(LFLAGS) -o "$@"

$(HEX_FILE): $(OUTPUT)
	@ echo Generating $@
	@ $(OBJCOPY) -O ihex "$<" "$@"

$(BIN_FILE): $(OUTPUT)
	@ echo Generating $@
	@ $(OBJCOPY) -O binary "$<" "$@"

clean:
	@ powershell.exe -NoProfile -ExecutionPolicy Bypass -File "tools/clean.ps1" -BuildDir "$(BUILD_DIR)"

-include $(DEPENDENCIES)
