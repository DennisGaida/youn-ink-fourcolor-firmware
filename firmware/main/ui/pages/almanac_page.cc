#include "almanac_page.h"
#include <esp_log.h>

// External font declaration (supports Chinese characters)
extern const lv_font_t SourceHanSansSC_Regular_slim;

namespace ui {

constexpr char kTag[] = "AlmanacPage";

AlmanacPage::AlmanacPage(lv_obj_t* parent) {
    // Create container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container_, lv_color_white(), 0);
    lv_obj_set_style_pad_all(container_, 8, 0);

    // Flex layout: vertical
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Date (solar calendar)
    date_label_ = lv_label_create(container_);
    lv_label_set_text(date_label_, "Date: --");

    // Lunar calendar date
    lunar_label_ = lv_label_create(container_);
    lv_label_set_text(lunar_label_, "Lunar: --");

    // Suitable activities
    suit_label_ = lv_label_create(container_);
    lv_label_set_text(suit_label_, "Do: --");

    // Activities to avoid
    avoid_label_ = lv_label_create(container_);
    lv_label_set_text(avoid_label_, "Avoid: --");

    // Auspicious hours
    auspicious_label_ = lv_label_create(container_);
    lv_label_set_text(auspicious_label_, "Auspicious hours: --");

    ESP_LOGI(kTag, "Almanac page created");
}

AlmanacPage::~AlmanacPage() {
    // Child widgets are deleted along with the container
}

void AlmanacPage::UpdateData(const AlmanacData& data) {
    if (date_label_) lv_label_set_text_fmt(date_label_, "Date: %s", data.date.c_str());
    if (lunar_label_) lv_label_set_text_fmt(lunar_label_, "Lunar: %s", data.lunar_date.c_str());
    if (suit_label_) lv_label_set_text_fmt(suit_label_, "Do: %s", data.suit.c_str());
    if (avoid_label_) lv_label_set_text_fmt(avoid_label_, "Avoid: %s", data.avoid.c_str());
    if (auspicious_label_) lv_label_set_text_fmt(auspicious_label_, "Auspicious hours: %s", data.auspicious.c_str());
}

void AlmanacPage::Refresh() {
    // LVGL handles the refresh automatically
}

}  // namespace ui