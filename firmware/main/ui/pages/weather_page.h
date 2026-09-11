#ifndef WEATHER_PAGE_H
#define WEATHER_PAGE_H

#include <lvgl.h>
#include <string>

namespace ui {

// Weather data structure
struct WeatherData {
    std::string city;           // City
    std::string temp;           // Temperature
    std::string condition;      // Weather condition
    std::string humidity;       // Humidity
    std::string wind;           // Wind speed/direction
    std::string update_time;    // Update time
};

// Weather dashboard page - Grid layout
class WeatherPage {
public:
    WeatherPage(lv_obj_t* parent);
    ~WeatherPage();

    // Update weather data
    void UpdateWeather(const WeatherData& data);

    // Refresh display
    void Refresh();

private:
    lv_obj_t* container_ = nullptr;   // Grid container
    lv_obj_t* city_label_ = nullptr;
    lv_obj_t* temp_label_ = nullptr;
    lv_obj_t* condition_label_ = nullptr;
    lv_obj_t* humidity_label_ = nullptr;
    lv_obj_t* wind_label_ = nullptr;
    lv_obj_t* time_label_ = nullptr;

    // Initialize the Grid layout
    void SetupGrid();
};

}  // namespace ui

#endif  // WEATHER_PAGE_H