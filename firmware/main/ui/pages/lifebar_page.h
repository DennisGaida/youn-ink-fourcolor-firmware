#ifndef LIFEBAR_PAGE_H
#define LIFEBAR_PAGE_H

#include <lvgl.h>
#include <string>

namespace ui {

// Life progress data
struct LifeBarData {
    std::string age;         // Age
    std::string goal;        // Goal
    std::string progress;    // Progress percentage
};

// Life progress page - simplified layout
class LifeBarPage {
public:
    LifeBarPage(lv_obj_t* parent);
    ~LifeBarPage();

    // Update data
    void UpdateData(const LifeBarData& data);

    // Refresh display
    void Refresh();

private:
    lv_obj_t* container_ = nullptr;
    lv_obj_t* age_label_ = nullptr;
    lv_obj_t* goal_label_ = nullptr;
    lv_obj_t* progress_bar_ = nullptr;
    lv_obj_t* progress_label_ = nullptr;
};

}  // namespace ui

#endif  // LIFEBAR_PAGE_H