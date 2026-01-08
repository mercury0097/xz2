# Dog招手动作修复前后对比

## 可视化对比

### 修复前（有抖动问题）

```
招手循环中的每一次：
┌─────────────────────────────────────────────────────────┐
│ MoveServosWithEase() 调用                                 │
│                                                          │
│  1. 读取所有4个舵机的当前位置                              │
│     - LEFT_REAR_LEG:  读取 → 109.8° (目标110°)          │
│     - LEFT_FRONT_LEG: 读取 → 89.5°  (目标90°)           │
│     - RIGHT_FRONT_LEG: 读取 → 90.3° (目标90°)           │
│     - RIGHT_REAR_LEG: 读取 → 69.7°  (目标70°)           │
│                                                          │
│  2. 为所有4个舵机计算平滑路径                             │
│     - LEFT_REAR_LEG:  109.8° → 110° (移动0.2°) ⚠️       │
│     - LEFT_FRONT_LEG: 89.5° → 150°  (移动60.5°) ✅      │
│     - RIGHT_FRONT_LEG: 90.3° → 90°  (移动-0.3°) ⚠️      │
│     - RIGHT_REAR_LEG: 69.7° → 70°   (移动0.3°) ⚠️       │
│                                                          │
│  3. 执行平滑移动（每10ms更新一次）                         │
│     - 所有舵机同时移动                                    │
│     - 即使是0.2°的微小移动也会执行！                      │
│                                                          │
│  结果：其他三条腿产生微小抖动 ❌                          │
└─────────────────────────────────────────────────────────┘

下一次循环重复，累积误差 → 持续抖动！
```

### 修复后（无抖动）

```
第1步：坐下（移动多个舵机）
┌─────────────────────────────────────────────────────────┐
│ MoveServosWithEase() 调用                                 │
│  - 所有4个舵机移动到坐下姿势                              │
└─────────────────────────────────────────────────────────┘

第2步：锁定其他三条腿
┌─────────────────────────────────────────────────────────┐
│ 直接设置位置（不读取，不平滑移动）                         │
│  servo_[LEFT_REAR_LEG].SetPosition(110°)                │
│  servo_[RIGHT_FRONT_LEG].SetPosition(90°)               │
│  servo_[RIGHT_REAR_LEG].SetPosition(70°)                │
│                                                          │
│  → 这三条腿被"锁定"在目标位置 🔒                         │
└─────────────────────────────────────────────────────────┘

第3步：招手循环（每次只移动一个舵机）
┌─────────────────────────────────────────────────────────┐
│ MoveSingleServoWithEase(LEFT_FRONT_LEG, 150°)           │
│                                                          │
│  1. 只读取LEFT_FRONT_LEG的位置: 90°                      │
│  2. 只为LEFT_FRONT_LEG计算路径: 90° → 150°              │
│  3. 只移动LEFT_FRONT_LEG                                 │
│                                                          │
│  其他三条腿：完全不触碰！✅                               │
│  - LEFT_REAR_LEG: 不读取，不计算，不移动                 │
│  - RIGHT_FRONT_LEG: 不读取，不计算，不移动               │
│  - RIGHT_REAR_LEG: 不读取，不计算，不移动                │
│                                                          │
│  结果：其他三条腿完全静止 ✅                              │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ MoveSingleServoWithEase(LEFT_FRONT_LEG, 120°)           │
│  → 只移动LEFT_FRONT_LEG: 150° → 120°                    │
│  → 其他三条腿完全不动 ✅                                  │
└─────────────────────────────────────────────────────────┘

循环重复，其他腿始终保持静止！
```

## 代码对比

### 修复前的招手循环

```cpp
for (int i = 0; i < wave_times; i++) {
  // 向前摆
  {
    int target[SERVO_COUNT] = {
      left_sit,        // [0] 想保持坐下
      wave_forward,    // [1] 招手抬高
      neutral,         // [2] 想保持中立  
      right_sit        // [3] 想保持坐下
    };
    MoveServosWithEase(period / 2, target, EASE_IN_OUT);
    //            ↑
    //      问题：移动所有舵机！
  }
  
  // 回来
  {
    int target[SERVO_COUNT] = {
      left_sit,        // [0] 想保持坐下
      wave_back,       // [1] 回来一点
      neutral,         // [2] 想保持中立
      right_sit        // [3] 想保持坐下
    };
    MoveServosWithEase(period / 2, target, EASE_IN_OUT);
    //            ↑
    //      问题：又移动所有舵机！
  }
  
  // 每次循环都读取、计算、移动所有舵机
  // → 累积误差 → 抖动！❌
}
```

### 修复后的招手循环

```cpp
// 先锁定其他三条腿
servo_[LEFT_REAR_LEG].SetPosition(left_sit + servo_trim_[LEFT_REAR_LEG]);
servo_[RIGHT_FRONT_LEG].SetPosition(neutral + servo_trim_[RIGHT_FRONT_LEG]);
servo_[RIGHT_REAR_LEG].SetPosition(right_sit + servo_trim_[RIGHT_REAR_LEG]);
//  ↑
//  直接设置，不平滑移动，锁定位置

for (int i = 0; i < wave_times; i++) {
  // 向前摆
  MoveSingleServoWithEase(LEFT_FRONT_LEG, wave_forward, period / 2, EASE_IN_OUT);
  //                       ↑
  //                 只移动这一个舵机！
  
  // 回来
  MoveSingleServoWithEase(LEFT_FRONT_LEG, wave_back, period / 2, EASE_IN_OUT);
  //                       ↑
  //                 还是只移动这一个！
  
  // 其他三条腿完全不被触碰
  // → 无累积误差 → 不抖动！✅
}
```

## 函数实现对比

### MoveServosWithEase (移动所有舵机)

```cpp
void Dog::MoveServosWithEase(int time, int servo_target[], EaseType ease_type) {
  // 读取所有舵机的起始位置
  int start_position[SERVO_COUNT];
  for (int i = 0; i < SERVO_COUNT; i++) {
    start_position[i] = servo_[i].GetPosition();  // 读取4次
  }
  
  while (未完成) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      // 计算并移动所有舵机
      servo_[i].SetPosition(...);  // 移动4个舵机
    }
  }
  
  // 最终位置
  for (int i = 0; i < SERVO_COUNT; i++) {
    servo_[i].SetPosition(servo_target[i] + servo_trim_[i]);  // 设置4次
  }
}
```

### MoveSingleServoWithEase (只移动一个舵机) ✨ 新增

```cpp
void Dog::MoveSingleServoWithEase(int servo_index, int target_position, 
                                   int time, EaseType ease_type) {
  // 只读取指定舵机的起始位置
  int start_position = servo_[servo_index].GetPosition();  // 读取1次
  //                    ↑
  //              只读取这一个！
  
  while (未完成) {
    // 只计算并移动指定舵机
    servo_[servo_index].SetPosition(...);  // 只移动1个舵机
    //    ↑
    //  只移动这一个！
  }
  
  // 最终位置
  servo_[servo_index].SetPosition(target_position + servo_trim_[servo_index]);
  //    ↑
  //  只设置这一个！
}
```

## 效果对比表

| 对比项 | 修复前 | 修复后 |
|--------|--------|--------|
| **招手时左后腿** | 🔴 抽动（读取→计算→微动） | 🟢 完全静止（不触碰） |
| **招手时右前腿** | 🔴 抽动（读取→计算→微动） | 🟢 完全静止（不触碰） |
| **招手时右后腿** | 🔴 抽动（读取→计算→微动） | 🟢 完全静止（不触碰） |
| **招手时左前腿** | 🟢 正常摆动 | 🟢 正常摆动 |
| **动作流畅度** | 🟡 有瑕疵 | 🟢 流畅自然 |
| **其他动作** | 🟢 正常 | 🟢 正常（不影响） |

## 性能对比

### 修复前（每次招手循环）
```
操作数量：
- 舵机位置读取：4次
- 舵机路径计算：4次
- 舵机位置设置：4次 × 约50帧 = 200次
总计：204次舵机操作

副作用：
- 3个舵机产生不必要的微小移动
- 累积误差导致抖动
```

### 修复后（每次招手循环）
```
操作数量：
- 舵机位置读取：1次 ✅ 减少75%
- 舵机路径计算：1次 ✅ 减少75%
- 舵机位置设置：1次 × 约50帧 = 50次 ✅ 减少75%
总计：51次舵机操作

优势：
- 0个不必要的舵机移动
- 无累积误差，无抖动
- 更高效，更精确
```

## 关键洞察

### 问题根源
**舵机位置读取存在±1-2度的微小误差**

即使目标位置是90度，读取可能返回89.7度或90.3度。当`MoveServosWithEase`被调用时，它会尝试"纠正"这个误差，导致0.3度的微小移动。

人眼对这种0.3度的移动非常敏感，尤其是当它重复发生时（每次招手都触发），就会看起来像"抖动"或"抽动"。

### 解决方案核心
**不要触碰不需要移动的舵机！**

- 不读取 → 无误差
- 不计算 → 无路径
- 不设置 → 无移动
- 结果 → 完全静止 ✅

这是一个"less is more"的典型案例：通过做得更少（只操作需要的舵机），得到了更好的结果（完全消除抖动）。

## 代码质量提升

### 新增函数的优势

1. **单一职责：** `MoveSingleServoWithEase`只负责移动一个舵机
2. **代码复用：** 可用于其他需要单舵机控制的场景
3. **清晰语义：** 函数名明确表达意图
4. **性能优化：** 减少75%的不必要操作
5. **可维护性：** 逻辑清晰，易于理解和调试

### 未来扩展

这个函数可以用于其他类似场景：
- 单腿抬起动作
- 尾巴摆动（如果有尾巴舵机）
- 头部转动（如果有头部舵机）
- 任何只需要移动单个舵机的精细控制

---

**总结：** 通过精确控制只移动需要移动的舵机，完全消除了不必要的抖动，同时提升了代码质量和性能。

