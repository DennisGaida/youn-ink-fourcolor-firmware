#include "weather_page.h"
#include <esp_log.h>

// External font declaration (supports Chinese characters)
extern const lv_font_t SourceHanSansSC_Regular_slim;
extern const lv_font_t weather_icons_48;

namespace ui {

constexpr char kTag[] = "WeatherPage";

WeatherPage::WeatherPage(lv_obj_t* parent) {
    // Create Grid container
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container_, lv_color_white(), 0);
    lv_obj_set_style_pad_all(container_, 8, 0);

    SetupGrid();

    ESP_LOGI(kTag, "Weather page created with Grid layout");
}

WeatherPage::~WeatherPage() {
    // Child widgets are deleted along with the container
}

void WeatherPage::SetupGrid() {
    // Grid layout: 4 columns, 3 rows
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(container_, col_dsc, row_dsc);

    // City name (row 0, spans 4 columns)
    city_label_ = lv_label_create(container_);
    lv_label_set_text(city_label_, "Beijing");
    lv_obj_set_grid_cell(city_label_, LV_GRID_ALIGN_CENTER, 0, 4, LV_GRID_ALIGN_CENTER, 0, 1);

    // Temperature (row 1, spans 2 columns)
    temp_label_ = lv_label_create(container_);
    lv_label_set_text(temp_label_, "25°C");
    lv_obj_set_grid_cell(temp_label_, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_CENTER, 1, 1);

    // Weather condition (row 1, spans 2 columns)
    condition_label_ = lv_label_create(container_);
    lv_label_set_text(condition_label_, "Sunny");
    lv_obj_set_grid_cell(condition_label_, LV_GRID_ALIGN_END, 2, 2, LV_GRID_ALIGN_CENTER, 1, 1);

    // Humidity (row 2, spans 2 columns)
    humidity_label_ = lv_label_create(container_);
    lv_label_set_text(humidity_label_, "Humidity: 45%");
    lv_obj_set_grid_cell(humidity_label_, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_CENTER, 2, 1);

    // Wind speed (row 2, spans 2 columns)
    wind_label_ = lv_label_create(container_);
    lv_label_set_text(wind_label_, "Wind: 3m/s");
    lv_obj_set_grid_cell(wind_label_, LV_GRID_ALIGN_END, 2, 2, LV_GRID_ALIGN_CENTER, 2, 1);

    // Update time (bottom)
    time_label_ = lv_label_create(container_);
    lv_label_set_text(time_label_, "Updated: --:--");
    lv_obj_set_grid_cell(time_label_, LV_GRID_ALIGN_CENTER, 0, 4, LV_GRID_ALIGN_END, 2, 1);
}

void WeatherPage::UpdateWeather(const WeatherData& data) {
    if (city_label_) lv_label_set_text(city_label_, data.city.c_str());
    if (temp_label_) lv_label_set_text(temp_label_, data.temp.c_str());
    if (condition_label_) lv_label_set_text(condition_label_, data.condition.c_str());
    if (humidity_label_) lv_label_set_text(humidity_label_, data.humidity.c_str());
    if (wind_label_) lv_label_set_text(wind_label_, data.wind.c_str());
    if (time_label_) lv_label_set_text(time_label_, data.update_time.c_str());
}

void WeatherPage::Refresh() {
    // LVGL handles the refresh automatically
}

}  // namespace ui