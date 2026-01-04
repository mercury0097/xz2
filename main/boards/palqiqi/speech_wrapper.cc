#include "speech_wrapper.h"

#define TAG "SpeechWrapper"

SpeechWrapper::SpeechWrapper() {
    ESP_LOGI(TAG, "SpeechWrapper initialized (Delayed Confirmation mode)");
}

SpeechWrapper::~SpeechWrapper() {
}

std::string SpeechWrapper::ProcessResponse(const std::string& original) {
    if (original.empty()) {
        return original;
    }

    SentenceType type = ClassifySentence(original);
    
    switch (type) {
        case TYPE_A_NORMAL_CHAT:
            return HandleNormalChat(original);
        case TYPE_B_ACTION_COMMITMENT:
            return HandleActionCommitment(original);
        case TYPE_C_ACTION_INTENTION:
            return HandleActionIntention(original);
        default:
            return original;
    }
}

SpeechWrapper::SentenceType SpeechWrapper::ClassifySentence(const std::string& text) {
    // B类: High-risk commitment patterns (must cache)
    const std::vector<std::string> b_patterns = {
        "我在", "我正在", "我走了", "我跳了", "我转了",
        "我已经", "我做了", "我动了", "我抬了", "我摇了",
        "我完成了"
    };
    
    for (const auto& pattern : b_patterns) {
        if (text.find(pattern) != std::string::npos) {
            ESP_LOGD(TAG, "Classified as B (commitment): %s", text.c_str());
            return TYPE_B_ACTION_COMMITMENT;
        }
    }
    
    // C类: Low-risk intention patterns (can pass through)
    const std::vector<std::string> c_patterns = {
        "我想", "让我", "我试试", "我可以", "我去", "我来"
    };
    
    for (const auto& pattern : c_patterns) {
        if (text.find(pattern) != std::string::npos) {
            ESP_LOGD(TAG, "Classified as C (intention): %s", text.c_str());
            return TYPE_C_ACTION_INTENTION;
        }
    }
    
    // A类: Default to normal chat
    ESP_LOGD(TAG, "Classified as A (normal): %s", text.c_str());
    return TYPE_A_NORMAL_CHAT;
}

std::string SpeechWrapper::HandleNormalChat(const std::string& text) {
    // Pass through immediately - no action involved
    return text;
}

std::string SpeechWrapper::HandleActionCommitment(const std::string& text) {
    std::string action = ExtractActionName(text);
    
    // Check if action already completed
    if (action_results_.count(action) && action_results_[action] == ACK_DONE) {
        // Action finished before AI spoke - allow confirmation
        ESP_LOGD(TAG, "Action %s already done, allowing confirmation", action.c_str());
        return text;
    }
    
    // Check if action failed
    if (action_results_.count(action) && action_results_[action] == ACK_FAILED) {
        ESP_LOGD(TAG, "Action %s failed, injecting refusal", action.c_str());
        return GenerateRefusal(action_fail_reasons_[action]);
    }
    
    // Action not done yet: CACHE original, output short ACK
    CachedSentence cached;
    cached.original = text;
    cached.action_name = action;
    cached.waiting_for_done = true;
    cached.timestamp_ms = GetTimeMs();
    cached_sentences_.push_back(cached);
    
    ESP_LOGI(TAG, "B-type cached for action '%s': %s", action.c_str(), text.c_str());
    
    return GenerateShortAck();
}

std::string SpeechWrapper::HandleActionIntention(const std::string& text) {
    // C-type: pass through, let action result speak
    std::string action = ExtractActionName(text);
    ESP_LOGD(TAG, "C-type intention detected: %s", action.c_str());
    return text;
}

std::string SpeechWrapper::GenerateShortAck() {
    const std::vector<std::string> acks = {
        "好，我试试。",
        "我准备一下。",
        "让我动一下看看。",
        "嗯，试试吧。",
        "稍等，让我想想...",
        "好的，等我调整一下",
        "嗯...我动动看"
    };
    std::string ack = acks[esp_random() % acks.size()];
    ESP_LOGI(TAG, "🗣️  [动作前承诺] %s", ack.c_str());  // 增强日志可见度
    return ack;
}

std::string SpeechWrapper::GenerateCompletionPhrase(const std::string& action) {
    const std::vector<std::string> completions = {
        "做到了！",
        "完成啦。",
        "好了。",
        "嗯！",
        "搞定！",
        "怎么样？",
        "嘿嘿，做完了"
    };
    std::string completion = completions[esp_random() % completions.size()];
    ESP_LOGI(TAG, "✅ [动作完成确认] %s (action: %s)", completion.c_str(), action.c_str());
    return completion;
}

std::string SpeechWrapper::GenerateRefusal(const std::string& reason) {
    if (reason == "refused") {
        const std::vector<std::string> refusals = {
            "我现在有点累，不太想动...",
            "唔...让我休息一下嘛",
            "我现在不想动，等会儿好吗？",
            "有点懒...不想动耶"
        };
        return refusals[esp_random() % refusals.size()];
    } else if (reason == "busy") {
        return "等等，我还在忙...";
    } else if (reason == "blocked" || reason == "safety") {
        return "我现在动不了，需要先调整一下";
    }
    return "我做不到...";
}

void SpeechWrapper::OnActionStart(const std::string& action_name) {
    ESP_LOGD(TAG, "Action started: %s", action_name.c_str());
    // Mark as started (could be useful for timeout detection)
}

void SpeechWrapper::OnActionComplete(const std::string& action_name, 
                                      ActionResult result, 
                                      const std::string& reason) {
    action_results_[action_name] = result;
    if (result == ACK_FAILED) {
        action_fail_reasons_[action_name] = reason;
    }
    
    // Check cached sentences waiting for this action
    for (auto& cached : cached_sentences_) {
        if (cached.action_name == action_name && cached.waiting_for_done) {
            if (result == ACK_DONE) {
                // Success: prepare simple confirmation
                delayed_confirmation_ = GenerateCompletionPhrase(action_name);
                ESP_LOGI(TAG, "Action %s DONE, prepared: %s", 
                         action_name.c_str(), delayed_confirmation_.c_str());
            } else {
                // Failed: prepare refusal
                delayed_confirmation_ = GenerateRefusal(reason);
                ESP_LOGI(TAG, "Action %s FAILED (%s), prepared refusal", 
                         action_name.c_str(), reason.c_str());
            }
            cached.waiting_for_done = false;
            break;
        }
    }
}

std::string SpeechWrapper::GetDelayedConfirmation() {
    std::string result = delayed_confirmation_;
    delayed_confirmation_.clear();
    return result;
}

std::string SpeechWrapper::ExtractActionName(const std::string& text) {
    // Simple keyword extraction
    if (text.find("走") != std::string::npos || text.find("行走") != std::string::npos) return "walk";
    if (text.find("跳") != std::string::npos) return "jump";
    if (text.find("转") != std::string::npos || text.find("旋转") != std::string::npos) return "turn";
    if (text.find("看") != std::string::npos || text.find("左右") != std::string::npos) return "look";
    if (text.find("抬") != std::string::npos || text.find("举") != std::string::npos) return "raise";
    if (text.find("摇") != std::string::npos || text.find("晃") != std::string::npos) return "shake";
    if (text.find("前进") != std::string::npos) return "walk";
    if (text.find("后退") != std::string::npos) return "walk";
    if (text.find("左转") != std::string::npos || text.find("右转") != std::string::npos) return "turn";
    
    return "unknown_action";
}


