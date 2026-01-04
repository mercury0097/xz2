# Implementation Plan

根据 docs/custom-board.md 文档指南创建新的开发板类型。

- [x] 1. 创建 palqiqi 板子目录结构
  - [x] 1.1 创建 main/boards/palqiqi 目录
    - 创建新的板子目录（按文档建议的命名方式）
    - _Requirements: 2.1_
  - [x] 1.2 复制 vector_eyes 子目录
    - 将 otto-robot/vector_eyes 整个目录复制到 palqiqi/vector_eyes
    - 这些文件不包含 otto 特定命名，无需修改
    - _Requirements: 2.4_

- [x] 2. 创建核心配置文件（按文档步骤2）
  - [x] 2.1 创建 config.h
    - 复制 otto-robot/config.h 并修改版本号宏名称
    - OTTO_ROBOT_VERSION → PALQIQI_VERSION
    - 保持所有硬件引脚配置不变
    - _Requirements: 2.3, 3.3_
  - [x] 2.2 创建 config.json
    - 修改 name 为 "palqiqi"
    - 保持 target 为 "esp32s3"
    - 保持 sdkconfig_append 配置不变
    - _Requirements: 3.3_

- [x] 3. 创建主板类文件（按文档步骤3）
  - [x] 3.1 创建 palqiqi_board.cc
    - 复制 otto_robot.cc 并重命名类
    - OttoRobot → PalqiqiBoard
    - 更新 TAG 为 "Palqiqi"
    - 更新 include 语句（palqiqi_vector_eye_display.h 等）
    - 更新 DECLARE_BOARD(PalqiqiBoard) 宏注册开发板
    - _Requirements: 2.2, 4.3_

- [x] 4. 创建显示相关文件
  - [x] 4.1 创建 palqiqi_vector_eye_display.h
    - 复制 otto_vector_eye_display.h
    - 重命名类 OttoVectorEyeDisplay → PalqiqiVectorEyeDisplay
    - 更新头文件保护宏
    - _Requirements: 2.2_
  - [x] 4.2 创建 palqiqi_vector_eye_display.cc
    - 复制 otto_vector_eye_display.cc
    - 更新 include 和类名
    - _Requirements: 2.2_
  - [x] 4.3 创建 palqiqi_emoji_display.h
    - 复制 otto_emoji_display.h
    - 重命名类 OttoEmojiDisplay → PalqiqiEmojiDisplay
    - _Requirements: 2.2_
  - [x] 4.4 创建 palqiqi_emoji_display.cc
    - 复制 otto_emoji_display.cc
    - 更新 include 和类名
    - _Requirements: 2.2_

- [x] 5. 创建控制器和动作文件
  - [x] 5.1 创建 palqiqi_controller.cc
    - 复制 otto_controller.cc
    - 更新函数名 InitializeOttoController → InitializePalqiqiController
    - 更新 TAG 和相关标识符
    - _Requirements: 2.2_
  - [x] 5.2 创建 palqiqi_movements.h
    - 复制 otto_movements.h
    - 重命名类 OttoMovements → PalqiqiMovements
    - _Requirements: 2.2_
  - [x] 5.3 创建 palqiqi_movements.cc
    - 复制 otto_movements.cc
    - 更新 include 和类名
    - _Requirements: 2.2_

- [x] 6. 复制辅助文件
  - [x] 6.1 复制 oscillator.cc 和 oscillator.h
    - 直接复制，无需修改
    - _Requirements: 2.1_
  - [x] 6.2 复制 power_manager.h
    - 直接复制，无需修改
    - _Requirements: 2.1_

- [x] 7. 更新构建系统配置（按文档步骤4）
  - [x] 7.1 更新 Kconfig.projbuild
    - 在 BOARD_TYPE_OTTO_ROBOT 配置项后添加 BOARD_TYPE_PALQIQI
    - 设置 depends on IDF_TARGET_ESP32S3
    - 添加 select LV_USE_GIF 和 select LV_GIF_CACHE_DECODE_DATA
    - 描述文字设为 "Palqiqi"
    - _Requirements: 3.1_
  - [x] 7.2 更新 CMakeLists.txt
    - 在 CONFIG_BOARD_TYPE_OTTO_ROBOT 配置后添加 CONFIG_BOARD_TYPE_PALQIQI
    - 设置 BOARD_TYPE 为 "palqiqi"（与目录名一致）
    - 设置 BUILTIN_TEXT_FONT 为 font_puhui_16_4（240x240 屏幕）
    - 设置 BUILTIN_ICON_FONT 为 font_awesome_16_4
    - _Requirements: 3.2_

- [x] 8. 创建 README.md 文档（按文档步骤6）
  - [x] 8.1 创建 main/boards/palqiqi/README.md
    - 说明开发板特性
    - 说明硬件要求
    - 说明编译和烧录步骤
    - _Requirements: 2.1_

- [ ] 9. Checkpoint - 验证编译（按文档步骤5）
  - Ensure all tests pass, ask the user if questions arise.
  - 方法一：使用 idf.py 手动配置
    - 运行 `idf.py menuconfig` 验证 Palqiqi 选项出现
    - 选择 Palqiqi 板子类型
    - 运行 `idf.py build` 验证编译成功
  - 方法二：使用 release.py 脚本（推荐）
    - 运行 `python scripts/release.py palqiqi`
  - _Requirements: 4.1_
