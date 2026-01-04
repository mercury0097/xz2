#ifndef _SPEECH_WRAPPER_H_
#define _SPEECH_WRAPPER_H_

#include <string>
#include <vector>
#include <map>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_random.h>
#include "action_arbiter.h"

/**
 * @brief Speech Wrapper - Ensures speech-action consistency
 * 
 * Implements PATCH 3: Delayed Confirmation approach
 * - Classifies AI responses into 3 types (A/B/C)
 * - Caches commitment sentences (B-type) until action completes
 * - Outputs short ACK before action, simple confirmation after DONE
 * - Eliminates "personality split" between cautious speech and decisive actions
 */
class SpeechWrapper {
public:
    enum SentenceType {
        TYPE_A_NORMAL_CHAT,       // "今天天气好" - pass through
        TYPE_B_ACTION_COMMITMENT, // "我在走" - cache & delay
        TYPE_C_ACTION_INTENTION   // "我想试试" - pass through
    };

    struct CachedSentence {
        std::string original;
        std::string action_name;
        bool waiting_for_done;
        int64_t timestamp_ms;
    };

    static SpeechWrapper& GetInstance() {
        static SpeechWrapper instance;
        return instance;
    }

    // Delete copy constructor and assignment
    SpeechWrapper(const SpeechWrapper&) = delete;
    SpeechWrapper& operator=(const SpeechWrapper&) = delete;

    /**
     * @brief Main processing entry - intercepts AI responses
     * @param original Original AI response text
     * @return Processed text (may be original, ACK, or empty if cached)
     */
    std::string ProcessResponse(const std::string& original);

    /**
     * @brief Get delayed confirmation after action completes
     * @return Confirmation text (empty if none ready)
     */
    std::string GetDelayedConfirmation();

    /**
     * @brief Notify that an action has started
     * @param action_name Action identifier
     */
    void OnActionStart(const std::string& action_name);

    /**
     * @brief Notify that an action has completed
     * @param action_name Action identifier
     * @param result Action result (DONE or FAILED)
     * @param reason Failure reason (if applicable)
     */
    void OnActionComplete(const std::string& action_name, 
                          ActionResult result, 
                          const std::string& reason = "");

private:
    SpeechWrapper();
    ~SpeechWrapper();

    // Classification
    SentenceType ClassifySentence(const std::string& text);
    std::string ExtractActionName(const std::string& text);

    // Handlers
    std::string HandleNormalChat(const std::string& text);
    std::string HandleActionCommitment(const std::string& text);
    std::string HandleActionIntention(const std::string& text);

    // Output generation
    std::string GenerateShortAck();
    std::string GenerateCompletionPhrase(const std::string& action_name);
    std::string GenerateRefusal(const std::string& reason);

    // State tracking
    std::map<std::string, ActionResult> action_results_;
    std::map<std::string, std::string> action_fail_reasons_;
    std::vector<CachedSentence> cached_sentences_;
    std::string delayed_confirmation_;

    // Get current time in milliseconds
    inline int64_t GetTimeMs() const {
        return esp_timer_get_time() / 1000;
    }
};

#endif // _SPEECH_WRAPPER_H_



