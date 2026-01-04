# 🚀 Build and Flash Guide

## Quick Start

### 1. Navigate to Project
```bash
cd /Users/machenyang/Desktop/xz1-main
```

### 2. Build the Firmware
```bash
idf.py build
```

**Expected output**:
- Compilation of all new modules (life_loop, action_arbiter, speech_wrapper)
- Linking of palqiqi_controller with new enhancements
- Final .bin files generated

**If errors occur**:
- Check that you're building for the Palqiqi board
- Verify ESP-IDF environment is properly set up
- Check `IMPLEMENTATION_COMPLETE.md` for troubleshooting

### 3. Flash to Device
```bash
idf.py flash
```

**Alternative with monitoring**:
```bash
idf.py flash monitor
```

### 4. Monitor Logs
```bash
idf.py monitor
```

**To exit monitor**: Press `Ctrl+]`

## What to Look For

### Startup Logs
You should see:
```
[LifeLoop] LifeLoop started (tick=100ms)
[LifeLoop] Initial state - Attention:50 Urge:0 Energy:70
[ActionArbiter] Arbiter initialized with queue handle
[SpeechWrapper] SpeechWrapper initialized (Delayed Confirmation mode)
[PalqiqiController] 🧬 Life-Driven Behavior Task started
[PalqiqiController] 🎯 Enhanced MotionPlayer started (hesitate + settle + randomize)
```

### During Idle
Every 5 seconds:
```
[LifeLoop] State: A=45 U=23 E=72 [IDLE]
```

Watch urge (U) slowly climb towards 70.

### First Auto-Action (after ~2-3 minutes)
```
[PalqiqiController] 🎬 Auto-action triggered! (A=35 U=71 E=68)
[ActionArbiter] ACCEPTED: shift_weight
[PalqiqiController] ⏱️  Hesitate 347ms before action 19
[PalqiqiController] ▶️  Executing action: 19
[PalqiqiController] 💤 Settle 612ms after action
[PalqiqiController] 🏠 Returning to default pose variant 1
[ActionArbiter] DONE: shift_weight
[PalqiqiController] ✅ Action complete (energy cost: 1)
[LifeLoop] Action executed (cost=1). State: A=30 U=0 E=67
```

### During Voice Interaction
```
[LifeLoop] Interaction! Attention boosted to 80
[Application] << 我在走
[SpeechWrapper] Classified as B (commitment): 我在走
[SpeechWrapper] B-type cached for action 'walk': 我在走
[Display] 好，我试试。
... (action executes) ...
[SpeechWrapper] Action walk DONE, prepared: 做到了！
[Display] 做到了！
```

### Hold-Back Events
```
[PalqiqiController] 🤔 Hold-back triggered, cooldown for 35 ticks (3.5s)
[LifeLoop] Urge reduced by 20, now at 51
```

## Tuning During Testing

### If Robot Moves Too Often
```cpp
// In palqiqi_controller.cc line ~36:
#define URGE_TRIGGER_THRESHOLD 80  // Increase from 70
#define HOLD_BACK_PROBABILITY 60   // Increase from 45
```

### If Robot Moves Too Rarely
```cpp
#define URGE_TRIGGER_THRESHOLD 60  // Decrease from 70
#define HOLD_BACK_PROBABILITY 30   // Decrease from 45
```

### If Movements Feel Too Slow
```cpp
// In palqiqi_controller.cc line ~218:
int hesitate_ms = 100 + (esp_random() % 300);  // Reduce from 200-600ms
```

### If Movements Feel Jerky
```cpp
// In palqiqi_controller.cc line ~253:
int settle_ms = 500 + (esp_random() % 500);  // Increase from 300-800ms
```

### To Make Robot Stay Focused Longer
```cpp
// In life_loop.cc line ~51:
static constexpr float ATTENTION_DECAY_RATE = 0.3f;  // Reduce from 0.5
```

## Test Scenarios

### Test 1: Idle Behavior
1. Flash robot, place on flat surface
2. Don't interact for 5 minutes
3. **Expected**: 10-20 small auto-actions (shifts, adjusts, micro-turns)
4. **Check**: Timing is unpredictable (not periodic)

### Test 2: Voice Interaction
1. Wake robot with voice
2. Say "走几步" (walk a few steps)
3. **Expected**: 
   - Hears "好，我试试" first
   - Robot hesitates, then walks
   - Hears "做到了！" after walking
4. **Check**: No "我在走" before actual movement

### Test 3: Touch Interaction
1. Touch robot's touch sensor
2. Wait for response
3. **Expected**: Attention jumps to 80+
4. **Check**: Auto-actions suppressed for next ~20 seconds

### Test 4: Low Energy
1. Trigger many actions to deplete energy below 30
2. Give voice command for gesture
3. **Expected**: 70% chance of refusal ("有点累...不想动耶")
4. **Check**: Refusal message is pet-like, not error message

### Test 5: Hold-Back
1. Observe multiple auto-action triggers
2. **Expected**: ~45% are hold-backs (urge reduces, no motion)
3. **Check**: After hold-back, 2-5 second pause before next check

## Performance Checks

### CPU Load
Monitor task watermarks:
```
[PalqiqiController] Life task stack high water mark: XXX
[PalqiqiController] Action task stack high water mark: XXX
```

Should have >500 bytes free. If low, increase stack sizes in `xTaskCreate` calls.

### Memory
Monitor heap:
```
[Application] Free heap: XXXX KB
```

New system adds ~2KB overhead. Should still have >100KB free.

## Rollback (If Needed)

If you need to revert to original behavior:

```bash
cd /Users/machenyang/Desktop/xz1-main/main/boards/palqiqi
cp palqiqi_controller.cc.bak palqiqi_controller.cc
```

Then rebuild and flash.

## Common Issues

### Issue: "life_loop.h: No such file"
**Solution**: Make sure all 6 new files are in `main/boards/palqiqi/`

### Issue: Undefined reference to `LifeLoop::GetInstance()`
**Solution**: Check that `life_loop.cc` is being compiled (should be automatic)

### Issue: Robot doesn't move at all
**Solution**: 
1. Check device state reaches `kDeviceStateIdle`
2. Verify `life_behaviors_enabled_ = true`
3. Check logs for urge building up

### Issue: Speech wrapper not activating
**Solution**: Verify build is for `BOARD_PALQIQI` (check `sdkconfig`)

## Next Steps After Successful Flash

1. **Observe**: Watch logs for 10 minutes, note behavior patterns
2. **Document**: Record which parameters feel right/wrong
3. **Tune**: Adjust 1-2 parameters at a time
4. **Iterate**: Rebuild, reflash, test again
5. **Enjoy**: Your robot now has a pet-like personality!

---

**Ready to flash?**

```bash
cd /Users/machenyang/Desktop/xz1-main
idf.py build flash monitor
```

**Good luck! Your robot will soon feel alive! 🐾**



