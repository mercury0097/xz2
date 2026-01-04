#include "action_arbiter.h"

#define TAG "ActionArbiter"

ActionArbiter::ActionArbiter() {
    ESP_LOGI(TAG, "ActionArbiter initialized");
}

ActionArbiter::~ActionArbiter() {
    if (current_action_) {
        delete current_action_;
        current_action_ = nullptr;
    }
}

void ActionArbiter::Initialize(QueueHandle_t action_queue) {
    action_queue_ = action_queue;
    ESP_LOGI(TAG, "Arbiter initialized with queue handle");
}

ActionResult ActionArbiter::RequestAction(const ActionRequest& req) {
    if (!action_queue_) {
        ESP_LOGE(TAG, "Action queue not initialized!");
        return ACK_FAILED;
    }

    ESP_LOGD(TAG, "Request: %s (priority=%d, source=%s)", 
             req.action_name.c_str(), req.priority, req.source.c_str());

    // Check if we can accept this action based on current state
    if (state_ == STATE_BUSY) {
        if (!CanInterrupt(req.priority)) {
            ESP_LOGD(TAG, "Rejected: busy with higher/equal priority action");
            
            // Record failure
            ActionRecord record;
            record.result = ACK_FAILED;
            record.fail_reason = "busy";
            record.timestamp_ms = GetTimeMs();
            action_history_[req.action_name] = record;
            
            return ACK_FAILED;
        }
        // If we can interrupt, continue...
    }

    // Priority-specific checks
    switch (req.priority) {
        case PRIORITY_USER_COMMAND:
            // User commands: check capability only, no willingness check
            // (Users expect commands to work if physically possible)
            if (state_ == STATE_BLOCKED) {
                ESP_LOGD(TAG, "Rejected user command: state blocked (safety/hardware)");
                
                ActionRecord record;
                record.result = ACK_FAILED;
                record.fail_reason = "blocked";
                record.timestamp_ms = GetTimeMs();
                action_history_[req.action_name] = record;
                
                return ACK_FAILED;
            }
            break;

        case PRIORITY_DIALOG_GESTURE:
            // Dialog gestures: check willingness (energy, attention)
            {
                std::string refuse_reason;
                if (!CheckWillingness(req, refuse_reason)) {
                    ESP_LOGD(TAG, "Rejected dialog gesture: %s", refuse_reason.c_str());
                    
                    ActionRecord record;
                    record.result = ACK_FAILED;
                    record.fail_reason = refuse_reason;
                    record.timestamp_ms = GetTimeMs();
                    action_history_[req.action_name] = record;
                    
                    return ACK_FAILED;
                }
            }
            break;

        case PRIORITY_AUTO_PET:
            // Auto pet: only when idle
            if (state_ != STATE_IDLE) {
                ESP_LOGD(TAG, "Rejected auto-pet: not idle");
                return ACK_FAILED;  // Don't track auto-pet failures
            }
            break;

        case PRIORITY_SAFETY:
            // Safety actions always proceed
            break;
    }

    // Accept the action - send to queue
    if (current_action_) {
        delete current_action_;
    }
    current_action_ = new ActionRequest(req);
    
    // Convert to queue structure (assuming palqiqi uses a simple params struct)
    struct {
        int action_type;
        int steps;
        int speed;
        int direction;
        int amount;
    } queue_params;
    
    queue_params.action_type = req.action_type;
    queue_params.steps = req.steps;
    queue_params.speed = req.speed;
    queue_params.direction = req.direction;
    queue_params.amount = req.amount;

    if (xQueueSend(action_queue_, &queue_params, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to send to queue (full?)");
        delete current_action_;
        current_action_ = nullptr;
        
        ActionRecord record;
        record.result = ACK_FAILED;
        record.fail_reason = "queue_full";
        record.timestamp_ms = GetTimeMs();
        action_history_[req.action_name] = record;
        
        return ACK_FAILED;
    }

    state_ = STATE_BUSY;
    
    ESP_LOGI(TAG, "ACCEPTED: %s", req.action_name.c_str());
    
    // Record acceptance
    ActionRecord record;
    record.result = ACK_ACCEPTED;
    record.fail_reason = "";
    record.timestamp_ms = GetTimeMs();
    action_history_[req.action_name] = record;
    
    return ACK_ACCEPTED;
}

void ActionArbiter::OnActionComplete(bool success, const std::string& reason) {
    if (!current_action_) {
        ESP_LOGW(TAG, "OnActionComplete called but no current action");
        return;
    }

    std::string action_name = current_action_->action_name;
    
    if (success) {
        ESP_LOGI(TAG, "DONE: %s", action_name.c_str());
        
        // Record completion
        ActionRecord record;
        record.result = ACK_DONE;
        record.fail_reason = "";
        record.timestamp_ms = GetTimeMs();
        action_history_[action_name] = record;
        
        // Notify speech wrapper if needed
        // (Speech wrapper will poll GetLastResult)
    } else {
        ESP_LOGW(TAG, "FAILED: %s (reason: %s)", action_name.c_str(), reason.c_str());
        
        // Record failure
        ActionRecord record;
        record.result = ACK_FAILED;
        record.fail_reason = reason;
        record.timestamp_ms = GetTimeMs();
        action_history_[action_name] = record;
    }

    delete current_action_;
    current_action_ = nullptr;
    state_ = STATE_IDLE;
}

bool ActionArbiter::CanInterrupt(ActionPriority new_priority) const {
    if (!current_action_) {
        return true;  // No current action
    }

    // Higher priority (lower number) can interrupt lower priority
    return new_priority < current_action_->priority;
}

bool ActionArbiter::CheckWillingness(const ActionRequest& req, std::string& refuse_reason) {
    auto& life = LifeLoop::GetInstance();
    int energy = life.GetEnergy();
    int attention = life.GetAttention();
    
    // Very low energy: high chance of refusal
    if (energy < 20) {
        if ((esp_random() % 100) < 70) {  // 70% refuse
            refuse_reason = "refused";
            return false;
        }
    }
    
    // Low attention: moderate chance of refusal
    if (attention < 30) {
        if ((esp_random() % 100) < 50) {  // 50% refuse
            refuse_reason = "refused";
            return false;
        }
    }
    
    return true;  // Willing to do it
}

ActionResult ActionArbiter::GetLastResult(const std::string& action_name) const {
    auto it = action_history_.find(action_name);
    if (it != action_history_.end()) {
        return it->second.result;
    }
    return ACK_FAILED;  // Unknown action
}

std::string ActionArbiter::GetLastFailReason(const std::string& action_name) const {
    auto it = action_history_.find(action_name);
    if (it != action_history_.end()) {
        return it->second.fail_reason;
    }
    return "unknown";
}



