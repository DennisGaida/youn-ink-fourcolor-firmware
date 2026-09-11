#ifndef LOG_PAGE_H
#define LOG_PAGE_H

#include <lvgl.h>
#include <string>
#include <vector>

namespace ui {

// Log entry
struct LogEntry {
    std::string text;
    int level = 0;  // 0=info, 1=warn, 2=error
};

// Log page - system log viewer
class LogPage {
public:
    LogPage(lv_obj_t* parent);
    ~LogPage();

    // Clear the log
    void Clear();

    // Add a log entry
    void AddEntry(const std::string& text, int level = 0);

    // Set the log list
    void SetEntries(const std::vector<LogEntry>& entries);

    // Refresh display
    void Refresh();

private:
    lv_obj_t* container_ = nullptr;     // Container
    lv_obj_t* log_label_ = nullptr;     // Log text label
    std::vector<std::string> entries_;  // Log content
};

}  // namespace ui

#endif  // LOG_PAGE_H