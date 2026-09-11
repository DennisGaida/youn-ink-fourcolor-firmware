#ifndef CHAT_PAGE_H
#define CHAT_PAGE_H

#include <lvgl.h>
#include <string>
#include <vector>
#include <memory>

#include "renderers/common/ui_components.h"

namespace ui {

// Chat message role
enum class ChatRole {
    User,   // User message (right side, black background with white text)
    AI,     // AI reply (left side, white background with black border)
    System  // System hint (centered, transparent)
};

// Chat message
struct ChatMessage {
    std::string text;
    ChatRole role;
};

// AI chat page - fixed version (Spec §3)
// Fixes: text not showing, streaming append, auto line wrap
class ChatPage {
public:
    ChatPage(lv_obj_t* parent);
    ~ChatPage();

    // Clear the message list
    void Clear();

    // Add a message (auto-scrolls to the bottom)
    void AddMessage(const std::string& text, ChatRole role);

    // Show a temporary status hint (recording/recognizing/thinking)
    void ShowStatus(const std::string& status, ChatRole role);

    // Hide the status hint
    void HideStatus();

    // Stream-append text to the last bubble (for LLM streaming)
    void AppendText(const std::string& chunk);

    // Start a new streaming response (creates a new AI bubble)
    void BeginStream();

    // End the streaming response
    void EndStream();

    // Get the last bubble for stream-appending
    Bubble* GetLastBubble() const;

    // Get the container object (for use by the renderer)
    lv_obj_t* container() const { return container_; }

    // Refresh display
    void Refresh();

private:
    // Bubble container management
    struct BubbleEntry {
        std::unique_ptr<Bubble> bubble;
        ChatRole role;
    };

    lv_obj_t* container_ = nullptr;       // Flex scroll container
    lv_obj_t* status_bubble_ = nullptr;   // Temporary status bubble
    Bubble* status_bubble_wrapper_ = nullptr;
    std::vector<BubbleEntry> bubbles_;    // Message bubble list
    bool is_streaming_ = false;           // Whether currently streaming
};

}  // namespace ui

#endif  // CHAT_PAGE_H
