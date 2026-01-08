# 桌面小狗机器人使用示例

## 基础动作示例

### 前进动作

```javascript
// 基础前进 - 默认参数
self.dog.walk_forward()

// 慢速前进 - 增加周期时间
self.dog.walk_forward({steps: 4, speed: 1500, amount: 25})

// 快速前进 - 减少周期时间
self.dog.walk_forward({steps: 4, speed: 800, amount: 30})

// 小步前进 - 减少步幅
self.dog.walk_forward({steps: 6, speed: 1000, amount: 20})

// 大步前进 - 增加步幅
self.dog.walk_forward({steps: 3, speed: 1200, amount: 40})
```

### 后退动作

```javascript
// 基础后退
self.dog.walk_backward()

// 慢速后退
self.dog.walk_backward({steps: 4, speed: 1500, amount: 25})

// 快速后退
self.dog.walk_backward({steps: 4, speed: 800, amount: 30})
```

### 停止和复位

```javascript
// 立即停止当前动作
self.dog.stop()

// 回到休息姿态
self.dog.home()
```

## 组合动作示例

### 前进后退循环

```javascript
// 前进3步
self.dog.walk_forward({steps: 3, speed: 1000, amount: 30})

// 等待完成后后退3步
setTimeout(() => {
    self.dog.walk_backward({steps: 3, speed: 1000, amount: 30})
}, 3500)  // 3步 × 1000ms + 500ms缓冲
```

### 探索模式

```javascript
// 前进
self.dog.walk_forward({steps: 5, speed: 1000, amount: 30})

// 停止观察
setTimeout(() => {
    self.dog.home()
}, 5500)

// 再次前进
setTimeout(() => {
    self.dog.walk_forward({steps: 3, speed: 1200, amount: 25})
}, 7000)
```

## 舵机校准示例

### 检查初始姿态

```javascript
// 让小狗站立
self.dog.home()

// 获取当前微调值
self.dog.get_trims()
```

### 校准左后腿

如果左后腿偏高：
```javascript
self.dog.set_trim({servo: "left_rear_leg", value: -5})
```

如果左后腿偏低：
```javascript
self.dog.set_trim({servo: "left_rear_leg", value: 5})
```

### 校准所有舵机

```javascript
// 左后腿
self.dog.set_trim({servo: "left_rear_leg", value: 3})

// 左前腿
self.dog.set_trim({servo: "left_front_leg", value: -2})

// 右前腿
self.dog.set_trim({servo: "right_front_leg", value: 1})

// 右后腿
self.dog.set_trim({servo: "right_rear_leg", value: -4})

// 检查结果
self.dog.home()
```

### 重置所有微调

```javascript
self.dog.set_trim({servo: "left_rear_leg", value: 0})
self.dog.set_trim({servo: "left_front_leg", value: 0})
self.dog.set_trim({servo: "right_front_leg", value: 0})
self.dog.set_trim({servo: "right_rear_leg", value: 0})
```

## 参数调优示例

### 测试不同步幅

```javascript
// 小步幅 - 更稳定但速度慢
self.dog.walk_forward({steps: 5, speed: 1000, amount: 15})

// 中等步幅 - 平衡
self.dog.walk_forward({steps: 5, speed: 1000, amount: 25})

// 大步幅 - 速度快但可能不稳
self.dog.walk_forward({steps: 5, speed: 1000, amount: 35})

// 最大步幅 - 测试极限
self.dog.walk_forward({steps: 3, speed: 1200, amount: 40})
```

### 测试不同速度

```javascript
// 超慢速 - 观察细节
self.dog.walk_forward({steps: 2, speed: 2000, amount: 30})

// 慢速 - 稳定
self.dog.walk_forward({steps: 3, speed: 1500, amount: 30})

// 正常速度
self.dog.walk_forward({steps: 4, speed: 1000, amount: 30})

// 快速
self.dog.walk_forward({steps: 4, speed: 800, amount: 30})

// 超快速 - 可能不稳
self.dog.walk_forward({steps: 5, speed: 600, amount: 30})
```

## 状态监控示例

### 获取状态

```javascript
// 获取小狗状态
self.dog.get_status()
// 返回: {is_resting: true/false, action_running: true/false, version: "1.0.0"}

// 获取电池信息
self.battery.get_level()
// 返回: {level: 85, charging: false, discharging: true}
```

### 状态检查后执行动作

```javascript
// 检查是否有动作在运行
const status = self.dog.get_status()
if (!status.action_running) {
    self.dog.walk_forward({steps: 3})
}
```

## 交互场景示例

### 场景1：打招呼

```javascript
// 前进接近
self.dog.walk_forward({steps: 3, speed: 1000, amount: 30})

// 停止
setTimeout(() => {
    self.dog.home()
}, 3500)
```

### 场景2：探索

```javascript
// 前进
self.dog.walk_forward({steps: 5, speed: 1200, amount: 30})

// 短暂停留
setTimeout(() => {
    self.dog.home()
}, 6500)

// 后退
setTimeout(() => {
    self.dog.walk_backward({steps: 3, speed: 1200, amount: 30})
}, 8000)
```

### 场景3：巡逻

```javascript
// 巡逻循环
function patrol() {
    // 前进
    self.dog.walk_forward({steps: 4, speed: 1000, amount: 30})
    
    setTimeout(() => {
        self.dog.home()
    }, 4500)
    
    setTimeout(() => {
        self.dog.walk_backward({steps: 4, speed: 1000, amount: 30})
    }, 6000)
    
    setTimeout(() => {
        self.dog.home()
    }, 10500)
}

// 执行巡逻
patrol()
```

## 调试技巧

### 测试单个舵机

使用微调功能测试单个舵机是否正常：

```javascript
// 测试左后腿
self.dog.set_trim({servo: "left_rear_leg", value: 20})
self.dog.home()  // 观察左后腿是否移动

// 恢复
self.dog.set_trim({servo: "left_rear_leg", value: 0})
self.dog.home()
```

### 测试对角线配对

```javascript
// 让左前和右后同时动
self.dog.set_trim({servo: "left_front_leg", value: 20})
self.dog.set_trim({servo: "right_rear_leg", value: 20})
self.dog.home()

// 观察是否同步
// 恢复
self.dog.set_trim({servo: "left_front_leg", value: 0})
self.dog.set_trim({servo: "right_rear_leg", value: 0})
```

### 寻找最佳参数

```javascript
// 记录测试结果
const tests = [
    {speed: 1500, amount: 20, result: "太慢"},
    {speed: 1000, amount: 25, result: "还可以"},
    {speed: 1000, amount: 30, result: "最佳"},
    {speed: 800, amount: 35, result: "太快不稳"},
]

// 逐个测试
tests.forEach((test, index) => {
    setTimeout(() => {
        console.log(`测试 ${index + 1}: speed=${test.speed}, amount=${test.amount}`)
        self.dog.walk_forward({steps: 2, speed: test.speed, amount: test.amount})
    }, index * 5000)
})
```

## 错误处理示例

### 动作被打断

```javascript
try {
    self.dog.walk_forward({steps: 10})
    
    // 如果需要紧急停止
    setTimeout(() => {
        self.dog.stop()
    }, 2000)
} catch (e) {
    console.error("动作执行失败:", e)
    self.dog.home()
}
```

### 参数验证

```javascript
function safeWalkForward(steps, speed, amount) {
    // 参数范围检查
    if (steps < 1 || steps > 20) {
        console.error("步数必须在1-20之间")
        return
    }
    if (speed < 500 || speed > 3000) {
        console.error("速度必须在500-3000之间")
        return
    }
    if (amount < 10 || amount > 45) {
        console.error("步幅必须在10-45之间")
        return
    }
    
    self.dog.walk_forward({steps, speed, amount})
}

// 使用
safeWalkForward(5, 1000, 30)
```

## 高级应用示例

### 语音控制

```javascript
// 假设有语音识别
function onVoiceCommand(command) {
    switch(command) {
        case "前进":
            self.dog.walk_forward({steps: 4})
            break
        case "后退":
            self.dog.walk_backward({steps: 4})
            break
        case "停止":
            self.dog.stop()
            break
        case "休息":
            self.dog.home()
            break
    }
}
```

### 表情联动

```javascript
// 前进时显示开心表情
self.emotion.set("happy")
self.dog.walk_forward({steps: 5})

// 后退时显示担心表情
setTimeout(() => {
    self.emotion.set("worried")
    self.dog.walk_backward({steps: 3})
}, 6000)
```

### 自适应步态

```javascript
// 根据电池电量调整速度
function adaptiveWalk(steps) {
    const battery = self.battery.get_level()
    
    let speed = 1000
    if (battery.level < 20) {
        speed = 1500  // 低电量，慢速
    } else if (battery.level > 80) {
        speed = 800   // 高电量，快速
    }
    
    self.dog.walk_forward({steps, speed, amount: 30})
}

adaptiveWalk(5)
```

## 性能优化建议

### 减少命令发送频率

```javascript
// 不好的做法
for (let i = 0; i < 10; i++) {
    self.dog.walk_forward({steps: 1})
}

// 好的做法
self.dog.walk_forward({steps: 10})
```

### 批量设置微调

```javascript
// 先准备好所有值
const trims = {
    left_rear_leg: 3,
    left_front_leg: -2,
    right_front_leg: 1,
    right_rear_leg: -4
}

// 批量设置
Object.keys(trims).forEach(servo => {
    self.dog.set_trim({servo, value: trims[servo]})
})
```

## 维护提示

### 定期校准

```javascript
// 每次开机后检查校准
function checkCalibration() {
    console.log("检查小狗姿态...")
    self.dog.home()
    
    // 获取当前微调
    const trims = self.dog.get_trims()
    console.log("当前微调:", trims)
}

checkCalibration()
```

### 舵机寿命保护

```javascript
// 避免连续长时间运动
function protectedWalk(totalSteps) {
    const maxContinuousSteps = 10
    const restTime = 2000  // 休息2秒
    
    for (let i = 0; i < totalSteps; i += maxContinuousSteps) {
        const steps = Math.min(maxContinuousSteps, totalSteps - i)
        
        setTimeout(() => {
            self.dog.walk_forward({steps})
            
            // 休息
            setTimeout(() => {
                self.dog.home()
            }, steps * 1000 + 500)
        }, i * (1000 + restTime / maxContinuousSteps))
    }
}

protectedWalk(30)  // 30步，分批执行
```

---

**提示**：这些示例可以直接在MCP客户端中使用。记得根据实际硬件情况调整参数！




