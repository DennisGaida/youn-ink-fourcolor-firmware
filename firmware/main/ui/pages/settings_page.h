#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

#include <lvgl.h>
#include <functional>
#include <string>
#include <vector>

namespace ui {

// Settings item type
enum class SettingsItemType {
    Normal,    // Normal item, shows >
    Checkbox,  // Checkable item, shows [x] or [ ]
    Action,    // Action item, shows >
};

// Settings item structure
struct SettingsItem {
    std::string label;                   // Display text
    std::string value;                   // Current value (optional)
    SettingsItemType type = SettingsItemType::Normal;
    bool checked = false;                // Checked state for Checkbox type
    std::function<void()> on_click;      // Click callback
};

// Settings page - lv_list component (spec_v2 flattened list menu)
class SettingsPage {
public:
    SettingsPage(lv_obj_t* parent);
    ~SettingsPage();

    // Set the list of settings items
    void SetItems(const std::vector<SettingsItem>& items);

    // Update the displayed value of a single item
    void UpdateItem(int index, const std::string& value);

    // Update the checked state of a Checkbox item
    void UpdateChecked(int index, bool checked);

    // Refresh display
    void Refresh();

private:
    lv_obj_t* list_ = nullptr;           // lv_list widget
    std::vector<lv_obj_t*> items_;       // List item widgets
    std::vector<SettingsItem> item_data_; // Saved item data
    std::vector<std::function<void()>*> callbacks_; // Callback pointers, for freeing memory

    // Get the prefix symbol for an item
    const char* GetItemSymbol(const SettingsItem& item) const;
};

}  // namespace ui

#endif  // SETTINGS_PAGE_H