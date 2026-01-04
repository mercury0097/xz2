#ifndef _LIFE_LOOP_H_
#define _LIFE_LOOP_H_

#include <esp_timer.h>
#include <esp_log.h>
#include <esp_random.h>
#include <algorithm>

/**
 * @brief Life Loop - Internal state system for pet-like behavior
 * 
 * Maintains 3 continuous state variables that drive natural behavior:
 * - attention: How focused on external stimuli (high during interaction)
 * - urge: Internal drive to move (builds up when idle, cleared on action)
 * - energy: Physical vitality (depletes on action, recovers when resting)
 * 
 * Updates every 100ms with rate-limited, continuous changes.
 */
class LifeLoop {
public:
    static LifeLoop& GetInstance() {
        static LifeLoop instance;
        return instance;
    }

    // Delete copy constructor and assignment
    LifeLoop(const LifeLoop&) = delete;
    LifeLoop& operator=(const LifeLoop&) = delete;

    /**
     * @brief Start the life loop timer (100ms tick)
     */
    void Start();

    /**
     * @brief Stop the life loop timer
     */
    void Stop();

    /**
     * @brief Notify of user interaction (voice, touch, etc.)
     * Raises attention, resets interaction timer
     */
    void NotifyInteraction();

    /**
     * @brief Notify that an action was executed
     * @param energy_cost Energy consumed by the action (1-10)
     */
    void NotifyActionExecuted(int energy_cost = 3);

    /**
     * @brief Partially reduce urge (for hold-back scenarios)
     * @param amount Amount to reduce (default 20)
     */
    void ReduceUrge(int amount = 20);

    // State getters
    inline int GetAttention() const { return static_cast<int>(attention_); }
    inline int GetUrge() const { return static_cast<int>(urge_); }
    inline int GetEnergy() const { return static_cast<int>(energy_); }

private:
    LifeLoop();
    ~LifeLoop();

    // Timer callback
    static void TimerCallback(void* arg);
    void OnTick();

    // State variables (0-100 float for smooth updates)
    float attention_ = 50.0f;
    float urge_ = 0.0f;
    float energy_ = 70.0f;

    // Rate limits (per 100ms tick)
    static constexpr float ATTENTION_DECAY_RATE = 0.5f;    // How fast focus fades
    static constexpr float URGE_RISE_RATE = 0.6f;          // 🔧 加快urge增长（原0.3）
    static constexpr float ENERGY_RECOVER_RATE = 0.3f;     // 🔧 加快能量恢复（原0.2）

    // Timing
    int64_t last_interaction_time_ms_ = 0;
    static constexpr int64_t INTERACTION_WINDOW_MS = 10000;  // 10 seconds

    // Timer handle
    esp_timer_handle_t timer_ = nullptr;
    bool running_ = false;

    // Helper to clamp values to 0-100
    inline float Clamp(float value) const {
        return std::min(100.0f, std::max(0.0f, value));
    }

    // Get current time in milliseconds
    inline int64_t GetTimeMs() const {
        return esp_timer_get_time() / 1000;
    }
};

#endif // _LIFE_LOOP_H_


