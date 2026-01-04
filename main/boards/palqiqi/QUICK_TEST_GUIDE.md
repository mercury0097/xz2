# 🐾 Palqiqi Pet-like Life System - 快速测试指南

## 📋 烧录前检查清单

```bash
# 1. 确认编译成功
idf.py build

# 2. 烧录固件到设备
idf.py flash

# 3. 查看串口输出
idf.py monitor
```

---

## 🔍 如何确认新系统在运行

### 启动时应该看到的日志：

```
🚀 Initializing Palqiqi with Pet-like Life System
LifeLoop initialized
LifeLoop started (tick=100ms)
Initial state - Attention:50 Urge:0 Energy:70
✅ Life System modules initialized
🧬 Life-driven behavior task started
📊 Thresholds: urge=70, attention_suppress=60, holdback=45%
```

如果看到这些日志 ✅ **新系统已启动**

---

## 🎭 新旧系统的区别对比

| 特征 | 旧系统 | 新系统（Pet-like） |
|------|--------|-------------------|
| **空闲动作** | 定时触发（如每30秒） | 内部驱动（urge累积到70+才可能触发） |
| **动作频率** | 规律、可预测 | 不规律、有"忍住"概率（45%） |
| **动作执行** | 立即开始 | 有200-600ms犹豫延迟 |
| **动作结束** | 突然停止 | 有300-800ms平滑收尾 |
| **每次动作** | 完全一样 | 幅度±10%、时长±20%随机化 |
| **交互响应** | 立即反应 | 有犹豫感 |
| **语音输出** | AI说什么就输出什么 | 动作完成前说"我试试"，完成后才确认 |

---

## 🧪 测试步骤（按顺序）

### 第1步：观察启动（30秒）

连接串口后，观察：
- ✅ 是否看到 "LifeLoop started"
- ✅ 是否看到初始状态 "Attention:50 Urge:0 Energy:70"
- ✅ 是否看到 "Life-driven behavior task started"

**预期行为**：机器人应该保持静止，不会立刻动

---

### 第2步：等待自动动作（2-5分钟）

**不要触碰、不要说话**，只是观察：

```
每10秒左右，串口会输出状态（如果开启DEBUG）：
State: A=48 U=18 E=71 (idle)
State: A=46 U=36 E=72 (idle)
State: A=44 U=54 E=73 (idle)
State: A=42 U=72 E=74 (idle)  ← urge 到达阈值！
```

当 `urge >= 70` 时：
- 🎲 **45%概率**：机器人"忍住"，只是轻微调整
- 🎲 **55%概率**：触发一个小动作（shift_weight/foot_adjust/micro_turn）

**日志示例**：
```
🎬 Auto-action triggered! (A=42 U=72 E=74)
🐾 Shift weight (subtle leg angle change)
✅ Auto-action accepted, cooldown for 150 ticks (15.0s)
```

**预期行为**：
- ❌ 不会像屏保一样定时动
- ✅ 会突然"憋不住"动一下
- ✅ 动作很小（不是大幅度表演）
- ✅ 动完后会安静 8-30 秒

---

### 第3步：测试交互响应（触摸/说话）

**触摸按钮或说"小智"唤醒**：

```
应该看到：
Interaction! Attention boosted to 80
```

**预期行为**：
- `attention` 立刻升高
- `urge` 增长变慢（因为在互动）
- 自动动作会减少（attention > 60 时抑制）

---

### 第4步：测试语音-动作一致性

**说："跳一下" 或 "转个圈"**

**旧系统**：
```
AI: "好的，我跳一下！"  ← 立刻说
（然后才开始跳）
```

**新系统**：
```
AI: "我试试～"  ← 先说这个
（开始执行动作，有200-600ms犹豫）
（动作完成后）
AI: "跳好了！"  ← 完成后才确认
```

**如果机器人拒绝（能量低/不想动）**：
```
AI: "我现在有点累，不想动～"
或
AI: "我在忙，等我一下～"
```

---

### 第5步：观察动作细节

**连续让机器人做同一个动作 3 次**（如"跳一下"）：

**旧系统**：
- 3次完全一样
- 立刻开始，突然结束

**新系统**：
- 每次幅度、速度、时长都略有不同
- 开始前有短暂停顿（犹豫）
- 结束后平滑回到默认姿态

---

## 📊 状态监控（可选）

如果想实时看到内部状态，可以通过 MCP 工具查询：

```json
{
  "tool": "self.palqiqi.get_life_state"
}
```

返回：
```json
{
  "attention": 45,
  "urge": 68,
  "energy": 72,
  "is_busy": false
}
```

---

## 🐛 如果看不到效果

### 检查清单：

1. **确认已烧录新固件**
   ```bash
   idf.py flash
   ```

2. **确认串口日志级别**
   - 在 `menuconfig` 中设置日志级别为 INFO 或 DEBUG
   ```bash
   idf.py menuconfig
   # Component config → Log output → Default log verbosity → Info
   ```

3. **确认 Life System 已启动**
   - 串口输出中搜索 "LifeLoop started"
   - 如果没有，可能编译时条件编译没生效

4. **等待足够时间**
   - 第一次自动动作可能需要 2-3 分钟
   - urge 从 0 增长到 70 需要约 140 秒（70 / 0.3 * 0.1s * 2）

5. **检查参数**
   - 如果觉得太慢，可以在 `palqiqi_controller.cc` 中调整：
   ```cpp
   #define URGE_TRIGGER_THRESHOLD 70    // 改成 50 会更快触发
   #define HOLD_BACK_PROBABILITY 45     // 改成 20 会更容易触发
   ```

---

## 🎯 预期的"活物感"

新系统的目标不是"更花哨"，而是：

- ✅ **不可预测性**：不知道它什么时候会动
- ✅ **主体感**：它自己"想动"，不是被命令
- ✅ **犹豫感**：不是即时反射
- ✅ **自然感**：每次动作都略有不同
- ✅ **一致性**：说到做到（不会"嘴上说做了，实际没做"）

如果您感觉机器人：
- 不再像"定时脚本"
- 有时会"突然想动一下"
- 对命令的响应不是"秒回"
- 每次动作看起来都有点不一样

**那就对了！** 🎉

---

## 📝 调参建议

如果想调整"性格"：

```cpp
// 在 palqiqi_controller.cc 顶部

// 更"活泼"（动得多）
#define URGE_TRIGGER_THRESHOLD 50        // 降低阈值
#define HOLD_BACK_PROBABILITY 20         // 降低忍耐力

// 更"安静"（动得少）
#define URGE_TRIGGER_THRESHOLD 85        // 提高阈值
#define HOLD_BACK_PROBABILITY 70         // 提高忍耐力

// 在 life_loop.cc 中

// 更"急躁"（urge 增长快）
static constexpr float URGE_RISE_RATE = 0.5f;  // 从 0.3 改成 0.5

// 更"慢热"（attention 衰减慢）
static constexpr float ATTENTION_DECAY_RATE = 0.3f;  // 从 0.5 改成 0.3
```

---

## 🆘 需要帮助？

如果遇到问题，请提供：
1. 串口完整日志（从启动到问题发生）
2. 是否看到 "LifeLoop started" 日志
3. 具体的"没区别"是指什么（频率？动作？响应？）

---

**祝测试顺利！🐾**


