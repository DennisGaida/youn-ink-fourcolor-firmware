#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <lvgl.h>
#include <functional>
#include <string>
#include <memory>

#include "pages/chat_page.h"
#include "pages/todo_page.h"
#include "pages/log_page.h"
#include "pages/weather_page.h"
#include "pages/lifebar_page.h"
#include "pages/almanac_page.h"
#include "pages/settings_page.h"
#include "widgets/status_bar.h"

namespace ui {

// Page index (7 pages, in spec_v2 order)
enum class PageId {
    Chat = 0,      // AI chat
    Todo = 1,      // Todo task list
    Log = 2,       // System log
    LifeBar = 3,   // Life progress
    Almanac = 4,   // Almanac
    Weather = 5,   // Weather dashboard
    Settings = 6,  // Settings
    Count = 7,     // Total number of pages
};

// UI manager - TabView container + status bar
class UiManager {
public:
    UiManager();
    ~UiManager();

    // Initialize the UI framework (pass in the LVGL display)
    void Init(lv_display_t* display);

    // Switch page
    void SwitchPage(PageId page);

    // Get the current page
    PageId GetCurrentPage() const { return current_page_; }

    // Refresh the current page (static refresh, for e-paper)
    void RefreshNow();

    // Trigger a full refresh (clears ghosting)
    void RequestFullRefresh();

    // Clear the content area (preserve the status bar, spec_v3 §5)
    void ClearContentArea();

    // Update the status bar
    void UpdateStatusBar(const StatusBarData& data);

    // Page object access
    lv_obj_t* GetTabView() const { return tabview_; }
    lv_obj_t* GetPage(PageId page) const;

    // Page instance access
    ChatPage* GetChatPage() { return chat_page_.get(); }
    TodoPage* GetTodoPage() { return todo_page_.get(); }
    LogPage* GetLogPage() { return log_page_.get(); }
    WeatherPage* GetWeatherPage() { return weather_page_.get(); }
    LifeBarPage* GetLifeBarPage() { return lifebar_page_.get(); }
    AlmanacPage* GetAlmanacPage() { return almanac_page_.get(); }
    SettingsPage* GetSettingsPage() { return settings_page_.get(); }

private:
    lv_display_t* display_ = nullptr;
    lv_obj_t* tabview_ = nullptr;
    lv_obj_t* tabs_[7] = {nullptr};  // 7 page objects
    PageId current_page_ = PageId::Chat;
    int refresh_count_ = 0;
    bool full_refresh_pending_ = false;

    // Status bar (fixed at the top)
    std::unique_ptr<StatusBar> status_bar_;

    // Page instances
    std::unique_ptr<ChatPage> chat_page_;
    std::unique_ptr<TodoPage> todo_page_;
    std::unique_ptr<LogPage> log_page_;
    std::unique_ptr<WeatherPage> weather_page_;
    std::unique_ptr<LifeBarPage> lifebar_page_;
    std::unique_ptr<AlmanacPage> almanac_page_;
    std::unique_ptr<SettingsPage> settings_page_;

    void CreatePages();
};

}  // namespace ui

#endif  // UI_MANAGER_H