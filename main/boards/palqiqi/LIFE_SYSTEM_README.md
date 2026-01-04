# Palqiqi Pet-like Life System

## 🎯 Overview

This system transforms the Palqiqi robot from a command-driven device into a lifelike pet with:
- **Internal state** driving natural behavior (not timer-based)
- **Natural hesitation** before moving (200-600ms)
- **Priority-based action arbitration** preventing conflicts  
- **Speech-action consistency** (no lying about actions)
- **Urge-driven behaviors** (憋不住想动，而非定时触发)

## 📦 Architecture

```
┌─────────────┐
│  LifeLoop   │ ← 100ms tick updates 3 state variables
│ (attention, │
│  urge,      │
│  energy)    │
└──────┬──────┘
       │ triggers when urge > 70
       ▼
┌─────────────────┐     ┌─────────────────┐
│ Auto Behavior   │────►│ ActionArbiter   │
│ (urge-driven)   │     │ (priority queue)│
└─────────────────┘     └────────┬────────┘
                                 │
       ┌─────────────────────────┘
       ▼
┌─────────────────┐     ┌─────────────────┐
│ MotionPlayer    │────►│  SpeechWrapper  │
│ (hesitate +     │     │ (delayed confirm│
│  settle +       │     │  prevents lies) │
│  randomize)     │     └─────────────────┘
└─────────────────┘
```

## 🔧 Implemented Modules

### Module 1: LifeLoop (`life_loop.h/cc`)
**Purpose**: Continuous internal state simulation

**State Variables** (0-100):
- `attention`: Focus on external stimuli (high during interaction)
- `urge`: Internal drive to move (builds up when idle)
- `energy`: Physical vitality (depletes on action, recovers when resting)

**🔧 PATCH 1**: Urge growth has ±10% random variation per tick, breaking time-periodic patterns.

**Update Rules** (per 100ms tick):
```cpp
// Idle (no recent interaction):
urge += URGE_RISE_RATE * random_factor (0.9-1.1)  // 🔧 PATCH 1
attention -= ATTENTION_DECAY_RATE
energy += ENERGY_RECOVER_RATE

// On interaction (voice/touch):
attention += 30

// On action executed:
urge = 0
energy -= action_cost
attention -= 5
```

**Key Parameters** (in `life_loop.cc`):
- `ATTENTION_DECAY_RATE = 0.5` (per 100ms) - **Tune first**
- `URGE_RISE_RATE = 0.3` (per 100ms) - **Tune second**
- `ENERGY_RECOVER_RATE = 0.2` (per 100ms)

### Module 2: ActionArbiter (`action_arbiter.h/cc`)
**Purpose**: Priority-based action conflict prevention

**Priority Levels** (highest → lowest):
1. `PRIORITY_SAFETY` - Safety/recovery actions
2. `PRIORITY_USER_COMMAND` - User explicit commands
3. `PRIORITY_DIALOG_GESTURE` - Conversation gestures
4. `PRIORITY_AUTO_PET` - Autonomous pet behaviors

**ACK System**:
- `ACK_ACCEPTED` - Action queued, will execute
- `ACK_DONE` - Action completed successfully
- `ACK_FAILED` - Action failed (with reason: "busy"|"refused"|"blocked"|"safety")

**Arbitration Rules**:
- User commands: Check capability only, no "willingness"
- Dialog gestures: Check willingness (energy, attention) - may refuse if tired
- Auto pet: Only when idle, never interrupts

### Module 3: Enhanced Motion Player (in `palqiqi_controller.cc`)
**Purpose**: Servo-friendly motion with natural characteristics

**6-Step Execution**:
1. **Pre-hesitation**: 200-600ms random delay
2. **Randomization**: ±10% speed, ±1 step variation
3. **Servo execution**: Actual motion with speed limit (180°/sec)
4. **Post-settle**: 300-800ms smooth deceleration
5. **Default pose**: Return to 1 of 3 comfortable pose variants
6. **Notifications**: Update LifeLoop and ActionArbiter

**Key Parameters** (in `palqiqi_controller.cc`):
- Hesitation range: `200 + (esp_random() % 400)` ms - **Tune for feel**
- Settle range: `300 + (esp_random() % 500)` ms
- Speed variation: 0.9x - 1.1x
- Pose variants: 3 (to avoid robotic repetition)

### Module 4: Life-Driven Behavior (in `palqiqi_controller.cc`)
**Purpose**: Urge-driven auto-actions (not timer-based)

**🔧 PATCH 2**: Hold-back cooldown - after deciding "not to act", enforces 2-5 second quiet period.

**Trigger Logic** (checked every 100ms):
```cpp
Conditions (all must pass):
1. Device is idle
2. Attention < 60 (not currently interacting)
3. Urge ≥ 70 (threshold met)
4. Random hold-back (45% chance): HOLD BACK
   ↓
   Urge -= 20 (not cleared)
   Cooldown 2-5s  // 🔧 PATCH 2
   ↓
   TRIGGER ACTION
   ↓
   Cooldown 8-30s (+ 10s if energy < 30)
```

**Key Parameters** (in `palqiqi_controller.cc`):
- `URGE_TRIGGER_THRESHOLD = 70` - **Tune for frequency**
- `ATTENTION_SUPPRESS_THRESHOLD = 60`
- `HOLD_BACK_PROBABILITY = 45%` - **Tune for willfulness**
- Action cooldown: 8-30s random (+ energy penalty)
- Hold-back cooldown: 2-5s random 🔧 **PATCH 2**

### Module 5: Minimal Auto-Pet Actions
**3 Subtle Actions** (energy-efficient):
1. `shift_weight` (cost: 1) - Tiny leg angle change
2. `foot_adjust` (cost: 1) - Micro foot movement  
3. `micro_turn` (cost: 2) - 5-10° turn

**State-Dependent Selection**:
- `attention < 20`: Only shift_weight, foot_adjust
- `attention < 50`: + look_around
- `attention ≥ 50`: + shake_leg, micro_turn
- `energy < 30`: Force smallest actions only

### Module 6: SpeechWrapper (`speech_wrapper.h/cc`)
**Purpose**: Ensure speech-action consistency (no lying)

**🔧 PATCH 3**: Delayed Confirmation (not direct rewrite)

**Sentence Classification**:
- **Type A** (normal chat): `"今天天气好"` → Pass through
- **Type B** (action commitment): `"我在走"` → **Cache & delay**
  - Output short ACK: `"好，我试试。"`
  - Wait for DONE ACK
  - Then output: `"做到了！"`
  - Original sentence **discarded**
- **Type C** (action intention): `"我想试试"` → Pass through

**B-Type Patterns** (high-risk):
```cpp
"我在", "我正在", "我走了", "我跳了", "我转了",
"我已经", "我做了", "我动了"
```

**C-Type Patterns** (low-risk):
```cpp
"我想", "让我", "我试试", "我可以", "我去", "我来"
```

**Effect**: Unified language-body personality. Language is cautious before action, confident after DONE.

### Module 7: Application.cc Integration
**Changes**:
1. **Includes** (line ~24): Added `life_loop.h` and `speech_wrapper.h` (ifdef BOARD_PALQIQI)
2. **SetDeviceState(Listening)** (line ~920): Added `LifeLoop::NotifyInteraction()`
3. **SendTouchStartSequence** (line ~1240): Added `LifeLoop::NotifyInteraction()`
4. **sentence_start handler** (line ~626): Integrated SpeechWrapper processing

## 🎮 Tuning Guide

### Phase 1: Life Loop (Start Here)
**Goal**: Observe state changes feel natural

1. Flash firmware
2. Monitor logs: `ESP_LOGD("LIFE", "State: A=%d U=%d E=%d")`
3. **Tune `ATTENTION_DECAY_RATE`** (default 0.5):
   - Too high: Robot "forgets" interaction instantly
   - Too low: Robot stays focused too long
4. **Tune `URGE_RISE_RATE`** (default 0.3):
   - Too high: Robot fidgets constantly
   - Too low: Robot never moves on its own

### Phase 2: Auto-Behavior Frequency
**Goal**: Actions feel spontaneous, not periodic

1. Count auto-actions per 10 minutes (should be 15-25)
2. **Tune `URGE_TRIGGER_THRESHOLD`** (default 70):
   - Lower (50-60): More frequent fidgeting
   - Higher (80-90): More stoic behavior
3. **Tune `HOLD_BACK_PROBABILITY`** (default 45%):
   - Lower (20-30%): Less willful, more impulsive
   - Higher (60-70%): More willful, more hesitant

### Phase 3: Motion Feel
**Goal**: Movements look alive, not robotic

1. **Tune hesitation range** (default 200-600ms):
   - Reduce min (100ms) if too slow to respond
   - Increase max (800ms) if looks too eager
2. **Tune settle range** (default 300-800ms):
   - Increase if stops feel abrupt
   - Decrease if too sluggish
3. Observe if same action looks different each time (should vary ±10%)

### Phase 4: Speech Consistency
**Goal**: Zero "AI said it moved but didn't" cases

1. Test various movement commands
2. Check logs for B-type sentence detection
3. Add missing patterns to `speech_wrapper.cc` ClassifySentence()
4. Verify short ACK appears before action, confirmation after

## 📊 Critical Parameters Summary

**Priority Order for Tuning**:
1. **Action cooldown (8-30s)** - Biggest impact on aliveness
2. **🔧 Hold-back cooldown (2-5s)** - PATCH 2, prevents jittery decisions
3. **ATTENTION_DECAY_RATE (0.5)** - Focus duration
4. **Hesitation range (200-600ms)** - Natural vs robotic feel
5. **URGE_TRIGGER_THRESHOLD (70)** - Auto-action frequency
6. **HOLD_BACK_PROBABILITY (45%)** - Willfulness
7. **🔧 Urge randomization (±10%)** - PATCH 1, breaks periodicity
8. **Settle duration (300-800ms)** - Stop quality

## 🔧 Three Critical Patches

### PATCH 1: Urge Randomization (Time Layer)
**Problem**: Fixed urge growth → quasi-periodic behavior → "定时器感"

**Solution**: Apply ±10% random factor per tick on urge increment

**Code** (`life_loop.cc` line ~78):
```cpp
float random_factor = 0.9f + (esp_random() % 21) / 100.0f;  // 0.9 - 1.1
urge_ += URGE_RISE_RATE * random_factor;
```

**Effect**: Breaks time predictability, timing unpredictable like real life

### PATCH 2: Hold-back Cooldown (Decision Layer)
**Problem**: Hold-back → immediate re-check → nervous fidgeting

**Solution**: After "deciding not to act", enforce 2-5 second quiet period

**Code** (`palqiqi_controller.cc` line ~409):
```cpp
if ((esp_random() % 100) < HOLD_BACK_PROBABILITY) {
  life.ReduceUrge(20);  // Partial reduction
  holdback_cooldown_ticks = 20 + (esp_random() % 30);  // 🔧 2-5 sec
  continue;
}
```

**Effect**: Hold-back becomes real decision, not glitch. Robot looks thoughtful.

### PATCH 3: Delayed Confirmation (Cognitive Layer)
**Problem**: Direct rewrite → always hesitant language vs decisive body → personality split

**Solution**: Cache commitment sentences → short ACK → wait DONE → simple confirmation

**Code** (`speech_wrapper.cc` HandleActionCommitment):
```cpp
// Cache original, output short ACK
CachedSentence cached = {original, action_name, true, timestamp};
cached_sentences_.push_back(cached);
return GenerateShortAck();  // "好，我试试。"

// Later, on DONE:
delayed_confirmation_ = GenerateCompletionPhrase();  // "做到了！"
```

**Effect**: Unified language-body personality (cautious→decisive after completion)

## 🐛 Troubleshooting

### Robot doesn't move autonomously
- Check: `life_behaviors_enabled_ = true` in controller
- Check: Urge reaching threshold (add debug logs)
- Check: Device state is `kDeviceStateIdle`
- Try: Lower `URGE_TRIGGER_THRESHOLD` to 50

### Robot moves too often
- Increase `URGE_TRIGGER_THRESHOLD` to 80-90
- Increase `HOLD_BACK_PROBABILITY` to 60-70%
- Increase cooldown range

### Movements feel robotic
- Increase hesitation range to 300-800ms
- Increase settle range to 500-1000ms
- Check servo speed limit is enabled (180°/sec)

### Speech says action happened but didn't
- Check `SpeechWrapper` is processing sentence_start
- Add missing B-type patterns to ClassifySentence()
- Verify `OnActionComplete()` is called in EnhancedActionTask

### Robot still feels periodic
- Verify PATCH 1 is active (urge randomization)
- Try increasing random_factor range to ±15-20%
- Check hold-back cooldown is working (PATCH 2)

## 🎯 Expected Behavior

After full implementation:
- ✅ Robot "thinks" 200-600ms before moving
- ✅ Same action looks slightly different each time
- ✅ Smooth stop into comfortable pose (not abrupt freeze)
- ✅ Auto-actions driven by internal urge (not clock ticks)
- ✅ 🔧 Urge timing unpredictable (no "准点想动")
- ✅ 🔧 Hold-back looks like real decision (not nervous glitch)
- ✅ Sometimes resists actions when tired/distracted
- ✅ User commands work immediately
- ✅ 🔧 Speech and body unified (no personality split)
- ✅ AI never says "I'm walking" before servos move
- ✅ Pet-like refusals when tired
- ✅ Natural 8-30s variable cooldown (not screensaver loop)

**Final Result**: Feels like a living pet with unified consciousness, not a remote-controlled robot.

## 📝 Files Modified/Created

### New Files:
- `main/boards/palqiqi/life_loop.h` (Module 1)
- `main/boards/palqiqi/life_loop.cc` (Module 1)
- `main/boards/palqiqi/action_arbiter.h` (Module 2)
- `main/boards/palqiqi/action_arbiter.cc` (Module 2)
- `main/boards/palqiqi/speech_wrapper.h` (Module 6)
- `main/boards/palqiqi/speech_wrapper.cc` (Module 6)

### Modified Files:
- `main/boards/palqiqi/palqiqi_controller.cc` (Modules 3, 4, 5)
  - Replaced `IdleActionTask` with `LifeDrivenBehaviorTask`
  - Enhanced `ActionTask` → `EnhancedActionTask`
  - Added 3 minimal auto-pet actions
  - Integrated all modules in constructor
- `main/application.cc` (Module 7)
  - Added includes for LifeLoop and SpeechWrapper
  - Modified `SetDeviceState(Listening)` to notify LifeLoop
  - Modified `sentence_start` handler to use SpeechWrapper
  - Modified `SendTouchStartSequence` to notify LifeLoop

### Backup:
- `main/boards/palqiqi/palqiqi_controller.cc.bak` (original backup)

## 🚀 Next Steps

1. **Compile**: `idf.py build` (check for errors)
2. **Flash**: `idf.py flash monitor`
3. **Observe**: Watch logs for state transitions
4. **Tune**: Adjust parameters based on feel
5. **Test**: Voice commands, touch, auto-behaviors
6. **Iterate**: Fine-tune until feels alive

---

**Version**: 1.0  
**Last Updated**: 2026-01-04  
**System**: Pet-like Life System with 3 Patches



