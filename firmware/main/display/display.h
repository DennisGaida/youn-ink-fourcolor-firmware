#ifndef DISPLAY_H
#define DISPLAY_H

#ifndef CONFIG_USE_EMOTE_MESSAGE_STYLE
#define HAVE_LVGL 1
#include <lvgl.h>
#endif

#include <esp_timer.h>
#include <esp_log.h>
#include <esp_pm.h>

#include <string>
#include <vector>
#include <chrono>

class Theme {
public:
    Theme(const std::string& name) : name_(name) {}
    virtual ~Theme() = default;

    inline std::string name() const { return name_; }
private:
    std::string name_;
};

class Display {
public:
    Display();
    virtual ~Display();

    virtual void SetStatus(const char* status);
    virtual void ShowNotification(const char* notification, int duration_ms = 3000);
    virtual void ShowNotification(const std::string &notification, int duration_ms = 3000);
    virtual void SetEmotion(const char* emotion);
    virtual void SetChatMessage(const char* role, const char* content);
    virtual void SetTheme(Theme* theme);
    virtual Theme* GetTheme() { return current_theme_; }
    virtual void UpdateStatusBar(bool update_all = false);
    virtual void SetPowerSaveMode(bool on);
    virtual void RequestUrgentRefresh() {}
    virtual void RequestUrgentFullRefresh() {}

    // Get the LVGL display object (used by the LVGL UI module)
#ifdef HAVE_LVGL
    virtual lv_display_t* GetLvDisplay() { return nullptr; }
#endif

    // Write raw 1bpp bitmap data to the frame buffer (implemented by subclass)
    // In data, bit=1 means black pixel, bit=0 means white pixel
    virtual void WriteRaw1bpp(int x, int y, int w, int h, const uint8_t* data, size_t len) { (void)x; (void)y; (void)w; (void)h; (void)data; (void)len; }

    // Invert (XOR) the specified region of the frame buffer
    // bit=1 becomes bit=0 (black to white), bit=0 becomes bit=1 (white to black)
    virtual void InvertRegion(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }

    // Text render item
    struct TextItem {
        std::string content;
        int x = 0;
        int y = 0;
        int size = 24;  // 16 or 24
    };

    // Render text directly on-device to the frame buffer (implemented by subclass)
    virtual void DrawTexts(const std::vector<TextItem>& texts, bool clear) { (void)texts; (void)clear; }

    // Update the picture page cache (no-op by default)
    virtual void UpdatePicRegion(int x, int y, int w, int h, const uint8_t* data, size_t len) {
        (void)x;
        (void)y;
        (void)w;
        (void)h;
        (void)data;
        (void)len;
    }

    // Whether the picture page has valid content (none by default)
    virtual bool HasPicContent() const { return false; }

    // Direct raw 4-color EPD frame display. Data is packed 2bpp, four pixels per byte.
    virtual bool DisplayRaw4ColorImage(const uint8_t* data, size_t len, int width, int height) {
        (void)data;
        (void)len;
        (void)width;
        (void)height;
        return false;
    }

    inline int width() const { return width_; }
    inline int height() const { return height_; }

protected:
    int width_ = 0;
    int height_ = 0;

    Theme* current_theme_ = nullptr;

    friend class DisplayLockGuard;
    virtual bool Lock(int timeout_ms = 0) = 0;
    virtual void Unlock() = 0;
};


class DisplayLockGuard {
public:
    DisplayLockGuard(Display *display) : display_(display), locked_(false) {
        locked_ = display_->Lock(30000);
        if (!locked_) {
            ESP_LOGE("Display", "Failed to lock display");
        }
    }
    ~DisplayLockGuard() {
        if (locked_) {
            display_->Unlock();
        }
    }

private:
    Display *display_;
    bool locked_;
};

class NoDisplay : public Display {
private:
    virtual bool Lock(int timeout_ms = 0) override {
        return true;
    }
    virtual void Unlock() override {}
};

#endif
