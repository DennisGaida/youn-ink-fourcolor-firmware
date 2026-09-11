#include "log_page.h"
#include <esp_log.h>

extern const lv_font_t SourceHanSansSC_Regular_slim;

namespace ui {

constexpr char kTag[] = "LogPage";

LogPage::LogPage(lv_obj_t* parent) {
    // Create container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container_, lv_color_white(), 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 8, 0);

    // Create log text label (multi-line, scrollable)
    log_label_ = lv_label_create(container_);
    lv_obj_set_size(log_label_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_text_font(log_label_, &SourceHanSansSC_Regular_slim, 0);
    lv_label_set_long_mode(log_label_, LV_LABEL_LONG_WRAP);
    lv_label_set_text(log_label_, "System Log\nWaiting for initialization...");

    ESP_LOGI(kTag, "Log page created");
}

LogPage::~LogPage() {
    // Child widgets are deleted along with the container
}

void LogPage::Clear() {
    entries_.clear();
    lv_label_set_text(log_label_, "");
}

void LogPage::AddEntry(const std::string& text, int level) {
    // Add log prefix
    const char* prefix = "";
    switch (level) {
        case 1: prefix = "[WARN] "; break;
        case 2: prefix = "[ERR] "; break;
        default: prefix = "[INFO] "; break;
    }

    entries_.push_back(prefix + text);

    // Update display
    std::string full_text;
    for (const std::string& entry : entries_) {
        full_text += entry + "\n";
    }
    lv_label_set_text(log_label_, full_text.c_str());
}

void LogPage::SetEntries(const std::vector<LogEntry>& entries) {
    Clear();
    for (const LogEntry& entry : entries) {
        AddEntry(entry.text, entry.level);
    }
}

void LogPage::Refresh() {
    // LVGL refreshes automatically
}

}  // namespace ui