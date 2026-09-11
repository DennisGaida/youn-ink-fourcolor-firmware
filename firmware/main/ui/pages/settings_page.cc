#include "settings_page.h"
#include <esp_log.h>

extern const lv_font_t SourceHanSansSC_Regular_slim;

namespace ui {

constexpr char kTag[] = "SettingsPage";

SettingsPage::SettingsPage(lv_obj_t* parent) {
    // Create the lv_list widget
    list_ = lv_list_create(parent);
    lv_obj_set_size(list_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(list_, lv_color_white(), 0);
    lv_obj_set_style_border_width(list_, 0, 0);

    ESP_LOGI(kTag, "Settings page created with lv_list");
}

SettingsPage::~SettingsPage() {
    // Free callback memory
    for (auto* callback : callbacks_) {
        delete callback;
    }
    callbacks_.clear();
    // Child widgets are deleted along with the list
}

const char* SettingsPage::GetItemSymbol(const SettingsItem& item) const {
    // Return the symbol based on type and state
    switch (item.type) {
        case SettingsItemType::Checkbox:
            return item.checked ? "[x]" : "[ ]";
        case SettingsItemType::Normal:
        case SettingsItemType::Action:
            return ">";
        default:
            return ">";
    }
}

void SettingsPage::SetItems(const std::vector<SettingsItem>& items) {
    // Clear existing items and callback memory
    for (auto* callback : callbacks_) {
        delete callback;
    }
    callbacks_.clear();
    for (lv_obj_t* item : items_) {
        lv_obj_delete(item);
    }
    items_.clear();
    item_data_ = items;

    // Create new list items
    for (const SettingsItem& item : items) {
        const char* symbol = GetItemSymbol(item);

        // Create list button
        lv_obj_t* btn = lv_list_add_button(list_, symbol, item.label.c_str());
        lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_text_font(btn, &SourceHanSansSC_Regular_slim, 0);

        // Show the current value (appended to the right side of the button)
        if (!item.value.empty()) {
            lv_obj_t* value_label = lv_label_create(btn);
            lv_obj_set_style_text_font(value_label, &SourceHanSansSC_Regular_slim, 0);
            lv_label_set_text(value_label, item.value.c_str());
            lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, -8, 0);
        }

        // Set the click callback
        if (item.on_click) {
            auto* callback_ptr = new std::function<void()>(item.on_click);
            callbacks_.push_back(callback_ptr);
            lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                auto* callback = static_cast<std::function<void()>*>(lv_event_get_user_data(e));
                if (callback && *callback) {
                    (*callback)();
                }
            }, LV_EVENT_CLICKED, callback_ptr);
        }

        items_.push_back(btn);
    }
}

void SettingsPage::UpdateItem(int index, const std::string& value) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        lv_obj_t* btn = items_[index];

        // Update data
        item_data_[index].value = value;

        // Find the value label (right-side child widget)
        lv_obj_t* value_label = lv_obj_get_child(btn, 1);
        if (value_label) {
            lv_label_set_text(value_label, value.c_str());
        } else if (!value.empty()) {
            // If there was no value label before, create a new one
            value_label = lv_label_create(btn);
            lv_obj_set_style_text_font(value_label, &SourceHanSansSC_Regular_slim, 0);
            lv_label_set_text(value_label, value.c_str());
            lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, -8, 0);
        }
    }
}

void SettingsPage::UpdateChecked(int index, bool checked) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        // Update data
        item_data_[index].checked = checked;

        // Update the displayed symbol
        const char* symbol = GetItemSymbol(item_data_[index]);
        lv_obj_t* btn = items_[index];

        // Update the button symbol (the first child widget is usually the icon)
        // The icon added by lv_list_add_button lives in the label inside the button
        // Simplified handling: recreate the button or just update the text directly
        // Here we use the simplified approach: update the whole button
        lv_obj_t* icon_label = lv_obj_get_child(btn, 0);
        if (icon_label && lv_obj_check_type(icon_label, &lv_label_class)) {
            lv_label_set_text(icon_label, symbol);
        }
    }
}

void SettingsPage::Refresh() {
    // LVGL handles the refresh automatically
}

}  // namespace ui