#include "chat_page.h"
#include <esp_log.h>
#include <esp_lvgl_port.h>

// External font declaration (supports Chinese characters)
extern "C" {
    extern const lv_font_t font_zectrix_16_1;
    extern const lv_font_t SourceHanSansSC_Regular_slim;
}

namespace ui {

static constexpr char kTag[] = "ChatPage";

ChatPage::ChatPage(lv_obj_t* parent) {
    if (!lvgl_port_lock(0)) {
        ESP_LOGE(kTag, "Failed to acquire LVGL lock during creation");
        return;
    }

    // Create Flex scroll container (vertical layout)
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container_, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(container_, 8, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_radius(container_, 0, 0);

    // Flex layout: vertical, starting from the top
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Enable scrolling
    lv_obj_set_scroll_dir(container_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(container_, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(container_, lv_color_hex(0x888888), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(container_, LV_OPA_40, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(container_, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(container_, 2, LV_PART_SCROLLBAR);

    ESP_LOGI(kTag, "Chat page created with scrollable Flex layout");

    lvgl_port_unlock();
}

ChatPage::~ChatPage() {
    // Bubbles are automatically deleted along with the container
    bubbles_.clear();
}

void ChatPage::Clear() {
    if (!lvgl_port_lock(0)) return;

    // Delete all bubble objects
    for (auto& entry : bubbles_) {
        if (entry.bubble && entry.bubble->root()) {
            lv_obj_delete(entry.bubble->root());
        }
    }
    bubbles_.clear();

    // Delete status bubble
    if (status_bubble_) {
        lv_obj_delete(status_bubble_);
        status_bubble_ = nullptr;
        status_bubble_wrapper_ = nullptr;
    }

    lvgl_port_unlock();
}

void ChatPage::AddMessage(const std::string& text, ChatRole role) {
    // Hide the status hint first
    HideStatus();

    Bubble::Align align;
    switch (role) {
        case ChatRole::User:   align = Bubble::Align::Right; break;
        case ChatRole::AI:     align = Bubble::Align::Left; break;
        case ChatRole::System: align = Bubble::Align::Center; break;
        default:               align = Bubble::Align::Left; break;
    }

    auto bubble = std::make_unique<Bubble>();

    if (!lvgl_port_lock(0)) return;
    bubble->Create(container_, align);
    bubble->SetText(text.c_str());
    lv_obj_t* root = bubble->root();

    // Set bubble width to 80% (Spec §3)
    lv_obj_set_width(root, LV_PCT(80));
    lv_obj_set_style_min_width(root, 60, 0);

    // Add to the list
    bubbles_.push_back({std::move(bubble), role});

    // Scroll to the bottom to show the new message
    if (!bubbles_.empty()) {
        lv_obj_t* last = bubbles_.back().bubble->root();
        if (last) {
            lv_obj_scroll_to_view(last, LV_ANIM_OFF);
        }
    }

    lvgl_port_unlock();

    ESP_LOGD(kTag, "Added message (role=%d, len=%zu)", static_cast<int>(role), text.size());
}

void ChatPage::ShowStatus(const std::string& status, ChatRole role) {
    if (!lvgl_port_lock(0)) return;

    // Hide the old status
    if (status_bubble_) {
        lv_obj_remove_flag(status_bubble_, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Create a new status bubble
        Bubble::Align align;
        switch (role) {
            case ChatRole::User:   align = Bubble::Align::Right; break;
            case ChatRole::AI:     align = Bubble::Align::Left; break;
            case ChatRole::System: align = Bubble::Align::Center; break;
            default:               align = Bubble::Align::Center; break;
        }

        status_bubble_ = lv_obj_create(container_);
        lv_obj_set_width(status_bubble_, LV_PCT(80));
        lv_obj_set_style_min_width(status_bubble_, 60, 0);
        lv_obj_set_style_radius(status_bubble_, 8, 0);
        lv_obj_set_style_pad_all(status_bubble_, 8, 0);
        lv_obj_set_style_border_width(status_bubble_, 0, 0);

        // Status text
        lv_obj_t* label = lv_label_create(status_bubble_);
        lv_label_set_text(label, status.c_str());
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(label, &SourceHanSansSC_Regular_slim, 0);
        lv_obj_set_width(label, LV_PCT(100));

        // Apply styles
        switch (role) {
            case ChatRole::User:
                lv_obj_set_style_bg_color(status_bubble_, lv_color_black(), 0);
                lv_obj_set_style_bg_opa(status_bubble_, LV_OPA_COVER, 0);
                lv_obj_set_style_text_color(label, lv_color_white(), 0);
                lv_obj_set_style_align(status_bubble_, LV_ALIGN_RIGHT_MID, 0);
                break;
            case ChatRole::AI:
                lv_obj_set_style_bg_color(status_bubble_, lv_color_white(), 0);
                lv_obj_set_style_bg_opa(status_bubble_, LV_OPA_COVER, 0);
                lv_obj_set_style_border_color(status_bubble_, lv_color_black(), 0);
                lv_obj_set_style_border_width(status_bubble_, 1, 0);
                lv_obj_set_style_text_color(label, lv_color_black(), 0);
                lv_obj_set_style_align(status_bubble_, LV_ALIGN_LEFT_MID, 0);
                break;
            case ChatRole::System:
                lv_obj_set_style_bg_color(status_bubble_, lv_color_white(), 0);
                lv_obj_set_style_bg_opa(status_bubble_, LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_width(status_bubble_, 0, 0);
                lv_obj_set_style_text_color(label, lv_color_hex(0x666666), 0);
                lv_obj_set_style_align(status_bubble_, LV_ALIGN_CENTER, 0);
                break;
        }
    }

    // Update status text
    lv_obj_t* label = lv_obj_get_child(status_bubble_, 0);
    if (label && lv_obj_check_type(label, &lv_label_class)) {
        lv_label_set_text(label, status.c_str());
    }

    lvgl_port_unlock();
}

void ChatPage::HideStatus() {
    if (!lvgl_port_lock(0)) return;

    if (status_bubble_) {
        lv_obj_add_flag(status_bubble_, LV_OBJ_FLAG_HIDDEN);
    }

    lvgl_port_unlock();
}

void ChatPage::AppendText(const std::string& chunk) {
    if (chunk.empty() || bubbles_.empty()) return;

    if (!lvgl_port_lock(0)) return;

    // Append to the last bubble
    auto& last = bubbles_.back();
    if (last.bubble) {
        last.bubble->AppendText(chunk.c_str());
    }

    lvgl_port_unlock();
}

void ChatPage::BeginStream() {
    is_streaming_ = true;
    // Create a new AI bubble for streaming reception
    auto bubble = std::make_unique<Bubble>();

    if (!lvgl_port_lock(0)) return;

    bubble->Create(container_, Bubble::Align::Left);
    bubble->SetText("");  // Start empty

    lv_obj_t* root = bubble->root();
    lv_obj_set_width(root, LV_PCT(80));
    lv_obj_set_style_min_width(root, 60, 0);

    bubbles_.push_back({std::move(bubble), ChatRole::AI});

    // Hide the status hint
    if (status_bubble_) {
        lv_obj_add_flag(status_bubble_, LV_OBJ_FLAG_HIDDEN);
    }

    lvgl_port_unlock();

    ESP_LOGD(kTag, "BeginStream: new AI bubble created");
}

void ChatPage::EndStream() {
    is_streaming_ = false;
    ESP_LOGD(kTag, "EndStream");
}

Bubble* ChatPage::GetLastBubble() const {
    if (bubbles_.empty()) return nullptr;
    return bubbles_.back().bubble.get();
}

void ChatPage::Refresh() {
    // LVGL handles the refresh automatically; e-paper partial refresh happens via lv_obj_invalidate()
    if (container_ && lvgl_port_lock(0)) {
        lv_obj_invalidate(container_);
        lvgl_port_unlock();
    }
}

}  // namespace ui
