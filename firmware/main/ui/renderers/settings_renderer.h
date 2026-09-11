#ifndef SETTINGS_RENDERER_H
#define SETTINGS_RENDERER_H

#include <lvgl.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace ui {

// Settings item type
enum class SettingsItemType {
    Normal,    // Normal item, shows >
    Checkbox,  // Checkable item
    Action,    // Action item
};

// Settings item structure
struct SettingsItemDef {
    std::string label;
    std::string value;
    const char* icon;                   // font_zectrix icon
    SettingsItemType type = SettingsItemType::Normal;
    bool checked = false;
    std::function<void()> on_click;
};

// Settings page renderer (Spec §6)
// All menu items use font_zectrix icons for a modern UI style
class SettingsRenderer {
public:
    SettingsRenderer();
    ~SettingsRenderer();

    // Create the settings page
    void Create(lv_obj_t* parent);

    // Set the list of settings items
    void SetItems(const std::vector<SettingsItemDef>& items);

    // Update a single item's displayed value
    void UpdateItem(int index, const std::string& value);

    // Update a Checkbox item's checked state
    void UpdateChecked(int index, bool checked);

    // Get the root object
    lv_obj_t* root() const { return container_; }

    // Show/hide
    void Show();
    void Hide();

private:
    // Create a single settings item
    lv_obj_t* CreateItem(lv_obj_t* parent, const SettingsItemDef& def, int index);

    // Get the checkbox icon
    const char* GetCheckboxIcon(bool checked) const;

    lv_obj_t* container_ = nullptr;
    std::vector<lv_obj_t*> item_buttons_;
    std::vector<SettingsItemDef> item_data_;
    std::vector<std::function<void()>*> callbacks_;
};

}  // namespace ui

#endif  // SETTINGS_RENDERER_H
