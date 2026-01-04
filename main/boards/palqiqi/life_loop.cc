#include "life_loop.h"

#define TAG "LifeLoop"

LifeLoop::LifeLoop() {
    ESP_LOGI(TAG, "LifeLoop initialized");
}

LifeLoop::~LifeLoop() {
    Stop();
}

void LifeLoop::Start() {
    if (running_) {
        ESP_LOGW(TAG, "LifeLoop already running");
        return;
    }

    // Create timer for 100ms tick
    esp_timer_create_args_t timer_args = {
        .callback = &LifeLoop::TimerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "life_loop",
        .skip_unhandled_events = true
    };

    esp_err_t err = esp_timer_create(&timer_args, &timer_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer: %s", esp_err_to_name(err));
        return;
    }

    // Start periodic timer (100ms = 100000 microseconds)
    err = esp_timer_start_periodic(timer_, 100000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start timer: %s", esp_err_to_name(err));
        esp_timer_delete(timer_);
        timer_ = nullptr;
        return;
    }

    running_ = true;
    last_interaction_time_ms_ = GetTimeMs();
    
    ESP_LOGI(TAG, "LifeLoop started (tick=100ms)");
    ESP_LOGI(TAG, "Initial state - Attention:%d Urge:%d Energy:%d", 
             GetAttention(), GetUrge(), GetEnergy());
}

void LifeLoop::Stop() {
    if (!running_) {
        return;
    }

    if (timer_) {
        esp_timer_stop(timer_);
        esp_timer_delete(timer_);
        timer_ = nullptr;
    }

    running_ = false;
    ESP_LOGI(TAG, "LifeLoop stopped");
}

void LifeLoop::TimerCallback(void* arg) {
    LifeLoop* loop = static_cast<LifeLoop*>(arg);
    loop->OnTick();
}

void LifeLoop::OnTick() {
    int64_t now_ms = GetTimeMs();
    bool is_interacting = (now_ms - last_interaction_time_ms_) < INTERACTION_WINDOW_MS;

    if (!is_interacting) {
        // IDLE STATE: No recent interaction

        // 🔧 PATCH 1: Urge grows with random variation (±10%)
        // This breaks time-periodic patterns
        float random_factor = 0.9f + (esp_random() % 21) / 100.0f;  // 0.9 - 1.1
        float urge_increment = URGE_RISE_RATE * random_factor;
        urge_ = Clamp(urge_ + urge_increment);

        // Attention naturally decays
        attention_ = Clamp(attention_ - ATTENTION_DECAY_RATE);

        // Energy recovers when idle
        energy_ = Clamp(energy_ + ENERGY_RECOVER_RATE);
    } else {
        // INTERACTION STATE: Recent interaction detected
        
        // Attention decays more slowly during interaction
        attention_ = Clamp(attention_ - (ATTENTION_DECAY_RATE * 0.5f));
        
        // Urge still builds but slower
        float random_factor = 0.9f + (esp_random() % 21) / 100.0f;
        float urge_increment = (URGE_RISE_RATE * 0.3f) * random_factor;
        urge_ = Clamp(urge_ + urge_increment);
        
        // Energy still recovers
        energy_ = Clamp(energy_ + ENERGY_RECOVER_RATE);
    }

    // Periodic debug log (every 5 seconds = 50 ticks)
    static int tick_count = 0;
    tick_count++;
    if (tick_count >= 50) {
        tick_count = 0;
        ESP_LOGD(TAG, "State: A=%d U=%d E=%d %s", 
                 GetAttention(), GetUrge(), GetEnergy(),
                 is_interacting ? "[INTERACT]" : "[IDLE]");
    }
}

void LifeLoop::NotifyInteraction() {
    last_interaction_time_ms_ = GetTimeMs();
    
    // Boost attention (capped at 100)
    attention_ = Clamp(attention_ + 30.0f);
    
    ESP_LOGD(TAG, "Interaction! Attention boosted to %d", GetAttention());
}

void LifeLoop::NotifyActionExecuted(int energy_cost) {
    // Clear urge completely
    urge_ = 0.0f;
    
    // Reduce energy by cost
    energy_ = Clamp(energy_ - static_cast<float>(energy_cost));
    
    // Slightly reduce attention (action consumes some focus)
    attention_ = Clamp(attention_ - 5.0f);
    
    ESP_LOGD(TAG, "Action executed (cost=%d). State: A=%d U=%d E=%d", 
             energy_cost, GetAttention(), GetUrge(), GetEnergy());
}

void LifeLoop::ReduceUrge(int amount) {
    urge_ = Clamp(urge_ - static_cast<float>(amount));
    ESP_LOGD(TAG, "Urge reduced by %d, now at %d", amount, GetUrge());
}



