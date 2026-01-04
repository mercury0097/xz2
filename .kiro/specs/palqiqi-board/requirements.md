# Requirements Document

## Introduction

本文档定义了创建新的开发板类型 "palqiqi" 的需求。该板子基于现有的 otto-robot 板子，但需要独立的标识和配置，以便在未来进行独立的开发和 OTA 升级。

## Glossary

- **palqiqi**: 新的开发板类型名称，用于替代 otto-robot
- **Board Type**: ESP-IDF 项目中用于区分不同硬件配置的标识符
- **Kconfig**: ESP-IDF 的配置系统，用于定义编译选项
- **CMakeLists.txt**: CMake 构建系统配置文件
- **DECLARE_BOARD**: 用于注册开发板类型的宏

## Requirements

### Requirement 1

**User Story:** As a developer, I want to create a new board type called "palqiqi" based on otto-robot, so that I can have an independent board identity for my customized project.

#### Acceptance Criteria

1. WHEN the build system is configured THEN the system SHALL provide a "palqiqi" option in the Board Type selection menu
2. WHEN "palqiqi" board type is selected THEN the system SHALL compile using the palqiqi board directory files
3. WHEN the palqiqi board is compiled THEN the system SHALL generate firmware with "palqiqi" as the board identifier

### Requirement 2

**User Story:** As a developer, I want all otto-robot source files to be copied and renamed for palqiqi, so that the new board has its own independent codebase.

#### Acceptance Criteria

1. WHEN the palqiqi board directory is created THEN the system SHALL contain all files from otto-robot with appropriate renaming
2. WHEN source files reference "otto" in class names THEN the system SHALL use "palqiqi" or "Palqiqi" instead
3. WHEN source files reference "otto-robot" in paths or identifiers THEN the system SHALL use "palqiqi" instead
4. WHEN the vector_eyes subdirectory is copied THEN the system SHALL preserve all functionality without modification

### Requirement 3

**User Story:** As a developer, I want the build configuration to be properly updated, so that the project compiles correctly with the new board type.

#### Acceptance Criteria

1. WHEN Kconfig.projbuild is updated THEN the system SHALL include BOARD_TYPE_PALQIQI option depending on IDF_TARGET_ESP32S3
2. WHEN CMakeLists.txt is updated THEN the system SHALL map CONFIG_BOARD_TYPE_PALQIQI to the "palqiqi" board directory
3. WHEN config.json is created THEN the system SHALL specify esp32s3 as target and "palqiqi" as the build name

### Requirement 4

**User Story:** As a developer, I want the project to compile and run correctly after the changes, so that I can continue development without issues.

#### Acceptance Criteria

1. WHEN the project is compiled with palqiqi board type THEN the system SHALL complete compilation without errors
2. WHEN the firmware runs on the device THEN the system SHALL function identically to the otto-robot configuration
3. WHEN DECLARE_BOARD macro is used THEN the system SHALL register PalqiqiBoard as the active board class
