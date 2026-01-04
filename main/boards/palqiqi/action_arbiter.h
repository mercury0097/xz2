#ifndef _ACTION_ARBITER_H_
#define _ACTION_ARBITER_H_

#include <string>
#include <map>
#include <esp_log.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "life_loop.h"

/**
 * @brief Action priority levels (higher priority can interrupt lower)
 */
enum ActionPriority {
    PRIORITY_SAFETY = 0,        // Highest: Safety/recovery actions
    PRIORITY_USER_COMMAND = 1,  // User explicit commands (walk, turn, etc.)
    PRIORITY_DIALOG_GESTURE = 2, // Conversation gestures (nod, wave)
    PRIORITY_AUTO_PET = 3       // Lowest: Autonomous pet behaviors
};

/**
 * @brief Action execution result
 */
enum ActionResult {
    ACK_ACCEPTED,   // Action accepted and queued
    ACK_DONE,       // Action completed successfully
    ACK_FAILED      // Action failed (with reason)
};

/**
 * @brief Action request structure
 */
struct ActionRequest {
    std::string action_name;     // Human-readable name (for logging/tracking)
    ActionPriority priority;
    int action_type;             // Internal action type ID
    int steps;
    int speed;
    int direction;
    int amount;
    std::string source;          // Source of request (for debugging)
    
    ActionRequest() : 
        action_name(""),
        priority(PRIORITY_AUTO_PET),
        action_type(0),
        steps(0),
        speed(0),
        direction(0),
        amount(0),
        source("") {}
};

/**
 * @brief Action Arbiter - Priority-based action execution manager
 * 
 * All robot actions MUST go through this arbiter to prevent conflicts.
 * Enforces priority rules and provides ACK feedback for speech consistency.
 */
class ActionArbiter {
public:
    static ActionArbiter& GetInstance() {
        static ActionArbiter instance;
        return instance;
    }

    // Delete copy constructor and assignment
    ActionArbiter(const ActionArbiter&) = delete;
    ActionArbiter& operator=(const ActionArbiter&) = delete;

    /**
     * @brief Initialize the arbiter with action queue
     * @param action_queue Existing FreeRTOS queue for actions
     */
    void Initialize(QueueHandle_t action_queue);

    /**
     * @brief Main entry point - all actions must go through this
     * @param req Action request with priority and parameters
     * @return ActionResult (ACCEPTED, DONE, or FAILED with reason)
     */
    ActionResult RequestAction(const ActionRequest& req);

    /**
     * @brief Called by MotionPlayer when action completes
     * @param success Whether action completed successfully
     * @param reason Failure reason if not successful
     */
    void OnActionComplete(bool success, const std::string& reason = "");

    /**
     * @brief Check if arbiter is currently executing an action
     */
    inline bool IsBusy() const { return state_ == STATE_BUSY; }

    /**
     * @brief Get the last action result (for speech wrapper)
     */
    ActionResult GetLastResult(const std::string& action_name) const;

    /**
     * @brief Get the last failure reason (for speech wrapper)
     */
    std::string GetLastFailReason(const std::string& action_name) const;

private:
    ActionArbiter();
    ~ActionArbiter();

    enum State {
        STATE_IDLE,     // Ready to accept actions
        STATE_BUSY,     // Currently executing an action
        STATE_BLOCKED   // Temporarily blocked (safety/hardware)
    };

    State state_ = STATE_IDLE;
    ActionRequest* current_action_ = nullptr;
    QueueHandle_t action_queue_ = nullptr;

    // Action tracking for speech wrapper
    struct ActionRecord {
        ActionResult result;
        std::string fail_reason;
        int64_t timestamp_ms;
    };
    std::map<std::string, ActionRecord> action_history_;

    // Check if new action can interrupt current action
    bool CanInterrupt(ActionPriority new_priority) const;

    // Check "willingness" based on internal state (for dialog gestures)
    bool CheckWillingness(const ActionRequest& req, std::string& refuse_reason);

    // Get current time in milliseconds
    inline int64_t GetTimeMs() const {
        return esp_timer_get_time() / 1000;
    }
};

#endif // _ACTION_ARBITER_H_

