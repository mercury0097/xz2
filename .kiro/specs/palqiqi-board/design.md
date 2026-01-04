# Design Document: Palqiqi Board

## Overview

本设计文档描述了如何创建一个名为 "palqiqi" 的新开发板类型，基于现有的 otto-robot 板子。这个新板子将拥有独立的标识符，使其能够独立进行开发、编译和 OTA 升级。

主要工作包括：
1. 复制 otto-robot 目录到新的 palqiqi 目录
2. 重命名所有类名和标识符
3. 更新构建系统配置（Kconfig 和 CMakeLists.txt）
4. 确保项目能正确编译

## Architecture

```
main/boards/
├── otto-robot/          # 原有板子（保留）
│   ├── config.h
│   ├── config.json
│   ├── otto_robot.cc
│   ├── otto_*.cc/h
│   └── vector_eyes/
└── palqiqi/             # 新板子
    ├── config.h
    ├── config.json
    ├── palqiqi_board.cc
    ├── palqiqi_*.cc/h
    └── vector_eyes/     # 直接复制，无需修改
```

### 构建系统集成

```
main/
├── Kconfig.projbuild    # 添加 BOARD_TYPE_PALQIQI 选项
└── CMakeLists.txt       # 添加 CONFIG_BOARD_TYPE_PALQIQI 映射
```

## Components and Interfaces

### 1. 板子目录结构

需要创建的文件：

| 原文件 | 新文件 | 说明 |
|--------|--------|------|
| otto_robot.cc | palqiqi_board.cc | 主板类，类名改为 PalqiqiBoard |
| otto_vector_eye_display.cc | palqiqi_vector_eye_display.cc | 矢量眼睛显示类 |
| otto_vector_eye_display.h | palqiqi_vector_eye_display.h | 矢量眼睛显示头文件 |
| otto_emoji_display.cc | palqiqi_emoji_display.cc | GIF表情显示类 |
| otto_emoji_display.h | palqiqi_emoji_display.h | GIF表情显示头文件 |
| otto_controller.cc | palqiqi_controller.cc | MCP控制器 |
| otto_movements.cc | palqiqi_movements.cc | 动作控制 |
| otto_movements.h | palqiqi_movements.h | 动作控制头文件 |
| config.h | config.h | 硬件配置（修改版本号） |
| config.json | config.json | 编译配置（修改名称） |
| oscillator.cc/h | oscillator.cc/h | 直接复制 |
| power_manager.h | power_manager.h | 直接复制 |
| vector_eyes/* | vector_eyes/* | 整个目录直接复制 |

### 2. 类名映射

| 原类名 | 新类名 |
|--------|--------|
| OttoRobot | PalqiqiBoard |
| OttoVectorEyeDisplay | PalqiqiVectorEyeDisplay |
| OttoEmojiDisplay | PalqiqiEmojiDisplay |
| OttoMovements | PalqiqiMovements |

### 3. 标识符映射

| 原标识符 | 新标识符 |
|----------|----------|
| BOARD_TYPE_OTTO_ROBOT | BOARD_TYPE_PALQIQI |
| CONFIG_BOARD_TYPE_OTTO_ROBOT | CONFIG_BOARD_TYPE_PALQIQI |
| otto-robot | palqiqi |
| OTTO_ROBOT_VERSION | PALQIQI_VERSION |
| TAG "OttoRobot" | TAG "Palqiqi" |

## Data Models

### config.json 结构

```json
{
    "target": "esp32s3",
    "builds": [
        {
            "name": "palqiqi",
            "sdkconfig_append": [
                "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions/v1/16m.csv\"",
                "CONFIG_LVGL_USE_GIF=y"
            ]
        }
    ]
}
```

### Kconfig 配置项

```kconfig
config BOARD_TYPE_PALQIQI
    bool "Palqiqi"
    depends on IDF_TARGET_ESP32S3
    select LV_USE_GIF
    select LV_GIF_CACHE_DECODE_DATA
```

### CMakeLists.txt 配置

```cmake
elseif(CONFIG_BOARD_TYPE_PALQIQI)
    set(BOARD_TYPE "palqiqi")
    set(BUILTIN_TEXT_FONT font_puhui_16_4)
    set(BUILTIN_ICON_FONT font_awesome_16_4)
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

由于本任务主要是文件复制和重命名操作，大部分验收标准是通过示例测试（检查文件存在、内容正确）来验证的，而非属性测试。

Property 1: 文件完整性
*For any* 文件在 otto-robot 目录中，palqiqi 目录中应存在对应的文件（经过适当重命名）
**Validates: Requirements 2.1**

Property 2: 标识符一致性
*For any* 源文件在 palqiqi 目录中，该文件不应包含 "otto" 或 "Otto" 字符串（除了注释中的历史说明）
**Validates: Requirements 2.2, 2.3**

## Error Handling

### 编译错误处理

1. **头文件找不到**: 确保所有 `#include` 语句中的文件名已正确更新
2. **类名未定义**: 确保所有类名引用已从 Otto* 更新为 Palqiqi*
3. **宏未定义**: 确保 config.h 中的宏名称已更新
4. **链接错误**: 确保 DECLARE_BOARD 宏使用了正确的类名

### 运行时错误处理

1. **板子未注册**: 检查 DECLARE_BOARD(PalqiqiBoard) 是否正确
2. **显示异常**: 检查显示类名是否正确更新

## 回退策略

### 设计原则

1. **保留原有 otto-robot 目录**: 不修改或删除任何 otto-robot 文件，确保可以随时切换回原板子
2. **增量添加**: 只添加新文件和配置，不修改现有功能代码
3. **独立配置**: palqiqi 板子使用独立的 Kconfig 选项，与 otto-robot 互不影响

### 快速回退方法

如果 palqiqi 板子出现问题，可以通过以下步骤快速回退：

**方法 1: 切换板子类型（推荐）**
```bash
# 进入配置菜单
idf.py menuconfig
# 导航到 Xiaozhi Assistant -> Board Type
# 选择 "ottoRobot" 而不是 "Palqiqi"
# 保存并退出
idf.py build
```

**方法 2: 删除 palqiqi 目录**
```bash
# 删除新创建的板子目录
rm -rf main/boards/palqiqi

# 撤销 Kconfig.projbuild 和 CMakeLists.txt 的修改
git checkout main/Kconfig.projbuild main/CMakeLists.txt

# 重新编译
idf.py build
```

**方法 3: 使用 Git 回退**
```bash
# 如果已提交，可以回退到之前的提交
git log --oneline  # 查看提交历史
git checkout <commit-hash>  # 回退到指定提交
```

### 验证回退成功

回退后，运行以下命令验证：
```bash
idf.py menuconfig  # 确认 Palqiqi 选项已移除（如果删除了目录）
idf.py build       # 确认编译成功
```

## Testing Strategy

### 验证方法

由于这是一个配置和重命名任务，主要通过以下方式验证：

1. **文件存在性检查**: 验证所有必需文件已创建
2. **内容正确性检查**: 验证文件内容中的标识符已正确更新
3. **编译测试**: 使用 `idf.py build` 验证项目能正确编译
4. **配置检查**: 验证 Kconfig 和 CMakeLists.txt 配置正确

### 手动测试步骤

1. 运行 `idf.py menuconfig`，验证 "Palqiqi" 选项出现在 Board Type 菜单中
2. 选择 Palqiqi 板子类型
3. 运行 `idf.py build` 验证编译成功
4. 烧录固件到设备验证功能正常
