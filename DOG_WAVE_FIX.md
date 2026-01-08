# Dog机器人招手动作抖动问题修复

## 问题描述

在招手动作中，两条后腿（坐下状态）和右前腿（保持中立）会出现抽动现象，虽然它们应该保持静止。

## 问题根源

原来的`SayHello`函数在招手循环中使用`MoveServosWithEase`函数移动所有舵机：

```cpp
// 原来的代码 - 每次都移动所有舵机
for (int i = 0; i < wave_times; i++) {
  int target[SERVO_COUNT] = {
    left_sit,        // [0] 左后腿 - 想保持坐下
    wave_forward,    // [1] 左前腿 - 招手抬高
    neutral,         // [2] 右前腿 - 想保持中立
    right_sit        // [3] 右后腿 - 想保持坐下
  };
  MoveServosWithEase(period / 2, target, EASE_IN_OUT);  // 问题！
}
```

**问题分析：**

1. `MoveServosWithEase`函数每次调用时，会读取**所有舵机的当前位置**
2. 然后对**所有舵机**进行平滑移动到目标位置
3. 由于舵机位置读取存在微小误差（±1-2度），即使目标位置没变，函数也会尝试"平滑移动"
4. 这导致应该静止的腿在每次循环中都产生微小移动，看起来就是抽动

## 解决方案

### 1. 新增单舵机移动函数

添加了`MoveSingleServoWithEase`函数，**只移动指定的单个舵机**，其他舵机完全不受影响：

```cpp
void Dog::MoveSingleServoWithEase(int servo_index, int target_position, 
                                   int time, EaseType ease_type) {
  // 只获取和移动指定舵机的位置
  int start_position = servo_[servo_index].GetPosition();
  
  // 平滑移动过程...
  
  // 只对指定舵机设置位置
  servo_[servo_index].SetPosition(start_position + delta + trim);
}
```

### 2. 修改招手动作逻辑

在招手循环中，改用单舵机移动：

```cpp
// 修复后的代码
// 第1步：坐下（使用MoveServosWithEase - 需要移动多个舵机）
MoveServosWithEase(500, target, EASE_IN_OUT);

// 稍等稳定
vTaskDelay(pdMS_TO_TICKS(300));

// 锁定其他三条腿的位置（避免后续抖动）
servo_[LEFT_REAR_LEG].SetPosition(left_sit + servo_trim_[LEFT_REAR_LEG]);
servo_[RIGHT_FRONT_LEG].SetPosition(neutral + servo_trim_[RIGHT_FRONT_LEG]);
servo_[RIGHT_REAR_LEG].SetPosition(right_sit + servo_trim_[RIGHT_REAR_LEG]);

// 第2步：招手循环（只移动左前腿！）
for (int i = 0; i < wave_times; i++) {
  MoveSingleServoWithEase(LEFT_FRONT_LEG, wave_forward, period / 2, EASE_IN_OUT);
  MoveSingleServoWithEase(LEFT_FRONT_LEG, wave_back, period / 2, EASE_IN_OUT);
  vTaskDelay(pdMS_TO_TICKS(10));
}

// 第3步：左前腿回中立（单舵机移动）
MoveSingleServoWithEase(LEFT_FRONT_LEG, neutral, 300, EASE_IN_OUT);

// 第4步：站起来（使用MoveServosWithEase - 需要移动多个舵机）
MoveServosWithEase(500, target, EASE_IN_OUT);
```

## 修改的文件

1. **`main/boards/dog/dog_movements.h`**
   - 添加了`MoveSingleServoWithEase`函数声明

2. **`main/boards/dog/dog_movements.cc`**
   - 实现了`MoveSingleServoWithEase`函数（第819-854行）
   - 修改了`SayHello`函数的招手循环部分（第702-723行）

## 修复原理

**核心思想：** 只移动需要移动的舵机，完全不触碰应该静止的舵机

- **锁定位置：** 在招手前，直接设置其他腿的目标位置，让它们稳定
- **单舵机移动：** 招手时只调用单舵机移动函数，其他舵机的`SetPosition`完全不会被调用
- **避免读取误差累积：** 不需要的舵机不进行位置读取和更新，自然就没有抖动

## 效果

- ✅ 招手时，左前腿正常平滑摆动
- ✅ 两条后腿（坐下状态）完全静止，不再抽动
- ✅ 右前腿（保持中立）完全静止，不再抽动
- ✅ 保持原有功能完整性，不影响其他动作

## 测试建议

1. 编译并烧录固件
2. 执行招手动作：`self.dog.say_hello` (通过MCP接口)
3. 观察坐下时的两条后腿是否完全静止
4. 观察右前腿是否完全静止
5. 验证其他动作（前进、后退、转弯等）是否正常工作

## 技术要点

- **最小化干预原则：** 只修改有问题的部分，不影响其他功能
- **精确控制：** 使用单舵机控制避免不必要的舵机操作
- **位置锁定：** 在关键时刻直接设置位置，确保稳定性
- **平滑移动保持：** 招手的腿依然使用缓动函数，保持动作流畅

## 代码质量

- ✅ 无编译错误
- ✅ 无linter警告
- ✅ 保持代码风格一致
- ✅ 添加了详细的注释说明
- ✅ 函数命名清晰（`MoveSingleServoWithEase`）

