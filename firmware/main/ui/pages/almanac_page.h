#ifndef ALMANAC_PAGE_H
#define ALMANAC_PAGE_H

#include <lvgl.h>
#include <string>

namespace ui {

// Chinese almanac data
struct AlmanacData {
    std::string date;          // Date
    std::string lunar_date;    // Lunar calendar date
    std::string suit;          // Suitable activities
    std::string avoid;         // Activities to avoid
    std::string auspicious;    // Auspicious hours
};

// Chinese almanac page - simplified layout
class AlmanacPage {
public:
    AlmanacPage(lv_obj_t* parent);
    ~AlmanacPage();

    // Update data
    void UpdateData(const AlmanacData& data);

    // Refresh display
    void Refresh();

private:
    lv_obj_t* container_ = nullptr;
    lv_obj_t* date_label_ = nullptr;
    lv_obj_t* lunar_label_ = nullptr;
    lv_obj_t* suit_label_ = nullptr;
    lv_obj_t* avoid_label_ = nullptr;
    lv_obj_t* auspicious_label_ = nullptr;
};

}  // namespace ui

#endif  // ALMANAC_PAGE_H