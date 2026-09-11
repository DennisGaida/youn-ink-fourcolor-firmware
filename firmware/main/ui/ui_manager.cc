#include "ui_manager.h"
#include <esp_log.h>

namespace ui {

constexpr char kTag[] = "UiManager";

UiManager::UiManager() {}

UiManager::~UiManager() {
    if (tabview_) {
        lv_obj_delete(tabview_);
    }
}

void UiManager::Init(lv_display_t* display) {
    display_ = display;

    // Set this display as default so lv_screen_active() works
    lv_display_set_default(display);

    // Create the main screen
    lv_obj_t* scr = lv_screen_active();

    // Create the status bar (fixed at the top)
    status_bar_ = std::make_unique<StatusBar>(scr);

    // Create the TabView container (below the status bar)
    tabview_ = lv_tabview_create(scr);
    lv_tabview_set_tab_bar_position(tabview_, LV_DIR_NONE);  // Hide tab buttons; switch via physical keys
    lv_obj_set_size(tabview_, LV_PCT(100), 300 - StatusBar::GetHeight());
    lv_obj_set_pos(tabview_, 0, StatusBar::GetHeight());

    // Create each page
    CreatePages();

    // Initialize the status bar to show the current page title
    StatusBarData init_data;
    init_data.page_title = "Chat";
    init_data.wifi_connected = false;
    init_data.server_connected = false;
    init_data.battery_level = -1;
    init_data.battery_charging = false;
    UpdateStatusBar(init_data);

    ESP_LOGI(kTag, "UI Manager initialized with status bar and 7 pages");

    // Force an immediate refresh to ensure UI is visible on e-paper
    if (display_) {
        lv_refr_now(display_);
        ESP_LOGI(kTag, "Forced initial LVGL refresh");
    }
}

void UiManager::CreatePages() {
    // 7 pages (in spec_v2 order)
    const char* page_names[] = {
        "Chat",    // 0 - Chat
        "Todo",    // 1 - Todo
        "Log",     // 2 - Log
        "LifeBar", // 3 - Life progress
        "Almanac", // 4 - Almanac
        "Weather", // 5 - Weather
        "Settings" // 6 - Settings
    };

    for (int i = 0; i < 7; ++i) {
        tabs_[i] = lv_tabview_add_tab(tabview_, page_names[i]);
        lv_obj_set_style_bg_color(tabs_[i], lv_color_white(), 0);
    }

    // Create page instances
    chat_page_ = std::make_unique<ChatPage>(tabs_[0]);
    todo_page_ = std::make_unique<TodoPage>(tabs_[1]);
    log_page_ = std::make_unique<LogPage>(tabs_[2]);
    lifebar_page_ = std::make_unique<LifeBarPage>(tabs_[3]);
    almanac_page_ = std::make_unique<AlmanacPage>(tabs_[4]);
    weather_page_ = std::make_unique<WeatherPage>(tabs_[5]);
    settings_page_ = std::make_unique<SettingsPage>(tabs_[6]);
}

void UiManager::SwitchPage(PageId page) {
    if (!tabview_) return;

    int index = static_cast<int>(page);

    // [Spec v3 §5] Screen-clear mechanism: clear the content area before switching pages to prevent ghosting
    // Preserve the status bar (Y: 0-30), clear the content area (Y: 30-300)
    ClearContentArea();

    // Switch to the new page
    lv_tabview_set_active(tabview_, index, LV_ANIM_OFF);
    current_page_ = page;

    // Update the page title in the status bar
    const char* titles[] = {
        "Chat",     // 0
        "Todo",     // 1
        "Log",      // 2
        "Life Progress", // 3
        "Almanac",  // 4
        "Weather",  // 5
        "Settings"  // 6
    };
    StatusBarData data;
    data.page_title = titles[index];
    UpdateStatusBar(data);

    // E-paper: refresh immediately to show the switch
    RefreshNow();

    // Force a full refresh to clear ghosting (spec_v2 §5)
    RequestFullRefresh();

    ESP_LOGI(kTag, "Switched to page %d (%s)", index, titles[index]);
}

void UiManager::ClearContentArea() {
    // Clear the content area (preserve the status bar at the top)
    // Clear using LVGL's invalidate + fill approach
    lv_obj_t* scr = lv_screen_active();
    if (!scr) return;

    // Draw a white rectangle over the TabView area to clear the content
    // TabView position: Y starts at StatusBar::GetHeight() (30)
    int content_y = StatusBar::GetHeight();
    int content_height = 300 - content_y;

    // Create a temporary white overlay layer to clear the content
    lv_obj_t* clear_layer = lv_obj_create(scr);
    lv_obj_set_pos(clear_layer, 0, content_y);
    lv_obj_set_size(clear_layer, 400, content_height);
    lv_obj_set_style_bg_color(clear_layer, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(clear_layer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(clear_layer, 0, 0);
    lv_obj_set_style_radius(clear_layer, 0, 0);

    // Immediately refresh this clear layer
    lv_refr_now(display_);

    // Delete the clear layer
    lv_obj_delete(clear_layer);

    ESP_LOGI(kTag, "Content area cleared (Y: %d-%d)", content_y, content_y + content_height);
}

lv_obj_t* UiManager::GetPage(PageId page) const {
    int index = static_cast<int>(page);
    if (index >= 0 && index < 7) {
        return tabs_[index];
    }
    return nullptr;
}

void UiManager::RefreshNow() {
    if (!display_) return;

    // LVGL static refresh: force immediate rendering
    lv_refr_now(display_);

    refresh_count_++;

    // After every 10 partial refreshes, trigger a full refresh to clear ghosting
    if (refresh_count_ >= 10) {
        full_refresh_pending_ = true;
        refresh_count_ = 0;
    }
}

void UiManager::RequestFullRefresh() {
    full_refresh_pending_ = true;
}

void UiManager::UpdateStatusBar(const StatusBarData& data) {
    if (status_bar_) {
        status_bar_->Update(data);
    }
}

}  // namespace ui