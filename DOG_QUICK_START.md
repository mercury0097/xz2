# 桌面小狗机器人快速入门指南

## 项目说明

这是基于palqiqi代码创建的桌面四足小狗机器人项目。小狗使用4个舵机，通过对角线蓄力的方法实现前进和后退动作。

## 硬件连接

### 舵机连接

| 舵机位置 | GPIO引脚 | 说明 |
|---------|---------|------|
| 左后腿 | GPIO 39 | 原palqiqi左腿位置 |
| 左前腿 | GPIO 38 | 原palqiqi左脚位置 |
| 右前腿 | GPIO 17 | 原palqiqi右腿位置 |
| 右后腿 | GPIO 18 | 原palqiqi右脚位置 |

### 舵机安装要求

- **安装方式**：舵机转轴平行于地面
- **运动方式**：脚可以垂直地面前后运动
- **初始角度**：所有舵机默认90度为休息位置

## 编译和烧录

### 1. 配置环境

确保已安装ESP-IDF v5.5或更高版本：

```bash
source ~/esp/esp-idf/export.sh
```

### 2. 配置板子类型

使用快捷脚本：

```bash
./compile_dog.sh
```

或手动配置：

```bash
idf.py set-target esp32s3
idf.py menuconfig
```

在menuconfig中选择：
- `Board Configuration` -> `Board Type` -> `Dog (Desktop Quadruped Robot)`

### 3. 编译

```bash
idf.py build
```

### 4. 烧录

```bash
idf.py flash monitor
```

## 运动原理

### 对角线蓄力方法

小狗通过**快慢组合**的动作来打破摩擦力平衡：

#### 前进

1. **第一步（对角线蓄力）**：
   - 左前腿 + 右后腿：**慢速**向后拨动（支撑腿，产生推力）
   - 右前腿 + 左后腿：**快速**向前划动（摆动腿，减少阻力）

2. **第二步（重心交替）**：
   - 右前腿 + 左后腿：**慢速**向后拨动（支撑腿，产生推力）
   - 左前腿 + 右后腿：**快速**向前收回（摆动腿，减少阻力）

#### 后退

运动方向相反，原理相同。

### 关键点

- **慢速拨动**：抓地力强，产生推力
- **快速划动**：静摩擦变动摩擦，阻力小
- **速度比例**：慢速占60%时间，快速占40%时间

## MCP控制接口

连接WiFi后，可通过MCP协议控制小狗：

### 基本动作

```javascript
// 前进（4步，每步1秒，步幅30度）
self.dog.walk_forward({steps: 4, speed: 1000, amount: 30})

// 后退（4步，每步1秒，步幅30度）
self.dog.walk_backward({steps: 4, speed: 1000, amount: 30})

// 停止
self.dog.stop()

// 回到休息姿态
self.dog.home()
```

### 参数说明

- **steps**: 步数（建议1-10）
- **speed**: 每步周期（毫秒，建议800-2000）
- **amount**: 步幅大小（度数，建议20-40）

### 舵机校准

如果小狗站立姿态不正，需要进行微调：

```javascript
// 设置单个舵机微调
self.dog.set_trim({servo: "left_rear_leg", value: 5})
self.dog.set_trim({servo: "left_front_leg", value: -3})
self.dog.set_trim({servo: "right_front_leg", value: 2})
self.dog.set_trim({servo: "right_rear_leg", value: -5})

// 获取当前微调值
self.dog.get_trims()

// 获取状态
self.dog.get_status()
```

## 调试技巧

### 1. 检查舵机连接

确保所有舵机正确连接到指定的GPIO引脚。

### 2. 调整步幅

如果小狗移动不理想，可以调整`amount`参数：
- 太小：移动不明显
- 太大：可能导致舵机堵转

### 3. 调整速度

如果动作不流畅，可以调整`speed`参数：
- 太快：摩擦力差异不明显
- 太慢：动作效率低

### 4. 微调舵机

使用`set_trim`命令微调每个舵机的初始位置，确保小狗站立时四条腿平衡。

## 常见问题

### Q: 小狗不动或抖动？
A: 检查舵机连接是否正确，微调值是否合适。

### Q: 小狗移动方向错误？
A: 可能需要调整某个舵机的微调值，或检查舵机安装方向。

### Q: 编译错误？
A: 确保选择了正确的板子类型（Dog），并且ESP-IDF版本正确。

### Q: 如何修改运动算法？
A: 编辑 `main/boards/dog/dog_movements.cc` 中的 `WalkForward` 和 `WalkBackward` 函数。

## 文件结构

```
main/boards/dog/
├── config.h                    # 硬件配置
├── config.json                 # 板子配置
├── dog_movements.h             # 运动控制头文件
├── dog_movements.cc            # 运动控制实现
├── dog_controller.cc           # MCP控制器
├── dog_board.cc                # 板子初始化
├── dog_vector_eye_display.h    # 矢量眼睛显示
├── dog_vector_eye_display.cc   # 矢量眼睛实现
├── oscillator.h                # 舵机振荡器
├── oscillator.cc               # 舵机振荡器实现
└── README.md                   # 说明文档
```

## 进阶开发

### 添加新动作

1. 在 `dog_movements.h` 中声明新函数
2. 在 `dog_movements.cc` 中实现
3. 在 `dog_controller.cc` 中注册MCP工具

### 修改运动参数

编辑 `dog_movements.cc` 中的参数：
- `slow_phase`: 慢速阶段时间比例（默认0.6）
- `fast_phase`: 快速阶段时间比例（默认0.4）

## 技术支持

基于xz1 (xiaozhi) 项目开发
- 原项目：https://github.com/78/xiaozhi-esp32
- 版本：Dog v1.0.0

## 舵机映射说明

为了不破坏palqiqi的其他功能，保持了相同的GPIO引脚配置，只是重新定义了舵机的功能含义：

| 功能 | Dog | Palqiqi |
|------|-----|---------|
| GPIO 39 | 左后腿 | 左腿 |
| GPIO 38 | 左前腿 | 左脚 |
| GPIO 17 | 右前腿 | 右腿 |
| GPIO 18 | 右后腿 | 右脚 |

这样做的好处是可以使用相同的硬件板，只需通过编译时选择不同的板子类型即可在palqiqi和dog之间切换。




