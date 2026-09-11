#ifndef CHAT_RENDERER_H
#define CHAT_RENDERER_H

#include "renderers/page_renderer.h"
#include "pages/chat_page.h"
#include <memory>

namespace ui {

// ChatRenderer: chat page renderer
// Wraps ChatPage and implements the PageRenderer interface
class ChatRenderer : public PageRenderer {
public:
    ChatRenderer();
    ~ChatRenderer() override;

    // PageRenderer interface implementation
    void Create(lv_obj_t* parent) override;
    void Destroy() override;
    void Update() override;
    bool HandleInput(const ButtonEvent& event) override;
    lv_obj_t* root() const override;

    // Streaming text support
    bool AppendText(const char* chunk) override;
    void BeginStream() override;
    void EndStream() override;

    // Data interface
    void AddMessage(const std::string& text, ChatRole role);
    void ShowStatus(const std::string& status, ChatRole role);
    void HideStatus();
    void Clear();

private:
    std::unique_ptr<ChatPage> chat_page_;
    lv_obj_t* parent_ = nullptr;
};

}  // namespace ui

#endif  // CHAT_RENDERER_H
