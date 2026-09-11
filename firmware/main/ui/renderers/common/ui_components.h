#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include <lvgl.h>
#include <string>

namespace ui {

// ============================================================
// Common UI component library (Spec §2)
// Shared by all pages to ensure visual consistency
// ============================================================

// Font declarations (extern C to prevent C++ name mangling)
extern "C" {
    extern const lv_font_t font_zectrix_16_1;
    extern const lv_font_t SourceHanSansSC_Regular_slim;
}

// Color constants
namespace Colors {
    inline lv_color_t White() { return lv_color_white(); }
    inline lv_color_t Black() { return lv_color_black(); }
    inline lv_color_t Gray() { return lv_color_hex(0x888888); }
    inline lv_color_t LightGray() { return lv_color_hex(0xCCCCCC); }
    inline lv_color_t DarkGray() { return lv_color_hex(0x333333); }
}

// ============================================================
// Panel: an area container with border, rounded corners, and a title bar
// ============================================================
class Panel {
public:
    Panel();
    ~Panel();

    // Create the panel
    void Create(lv_obj_t* parent, int x, int y, int w, int h);
    void Create(lv_obj_t* parent, lv_coord_t x_pct, lv_coord_t y_pct,
                lv_coord_t w_pct, lv_coord_t h_pct);

    // Set the title
    void SetTitle(const char* title);

    // Get the container object (for placing child controls)
    lv_obj_t* content() const { return content_; }
    lv_obj_t* root() const { return panel_; }

    // Show/hide
    void Show();
    void Hide();

    // Set style
    void SetBorder(bool enabled, lv_color_t color = lv_color_black(), lv_coord_t width = 1);
    void SetBackground(lv_color_t color, lv_opa_t opa = LV_OPA_COVER);
    void SetRadius(lv_coord_t radius);

private:
    lv_obj_t* panel_ = nullptr;
    lv_obj_t* title_bar_ = nullptr;
    lv_obj_t* content_ = nullptr;
};

// ============================================================
// ScrollView: a scrollable content area with a scrollbar indicator
// ============================================================
class ScrollView {
public:
    ScrollView();
    ~ScrollView();

    // Create the scroll container
    void Create(lv_obj_t* parent, int x, int y, int w, int h);

    // Get the content object
    lv_obj_t* content() const { return scroll_; }
    lv_obj_t* root() const { return scroll_; }

    // Scroll to the bottom
    void ScrollToEnd(bool anim = false);

    // Show/hide the scrollbar
    void ShowScrollbar(bool show);

    // Show/hide
    void Show();
    void Hide();

private:
    lv_obj_t* scroll_ = nullptr;
};

// ============================================================
// IconButton: a button with an icon font
// ============================================================
class IconButton {
public:
    IconButton();
    ~IconButton();

    // Create the icon button
    void Create(lv_obj_t* parent, const char* icon_text, int w = 40, int h = 40);

    // Set the click callback
    void SetClickCallback(void (*callback)(void*), void* user_data);

    // Set style
    void SetIconColor(lv_color_t color);
    void SetBgColor(lv_color_t color, lv_opa_t opa = LV_OPA_COVER);
    void SetBorder(bool enabled, lv_color_t color = lv_color_black(), lv_coord_t width = 1);

    // Get the object
    lv_obj_t* root() const { return btn_; }

    // Show/hide
    void Show();
    void Hide();

private:
    lv_obj_t* btn_ = nullptr;
    lv_obj_t* icon_label_ = nullptr;
};

// ============================================================
// Bubble: a chat bubble (left/right aligned, rounded corners, supports streaming text append)
// ============================================================
class Bubble {
public:
    enum class Align {
        Left,   // AI reply (left side, white background, black border)
        Right,  // User message (right side, black background, white text)
        Center  // System hint (centered, transparent background)
    };

    Bubble();
    ~Bubble();

    // Create the bubble
    void Create(lv_obj_t* parent, Align align = Align::Left);

    // Set the text
    void SetText(const char* text);

    // Append streaming text (Spec §3 key feature)
    void AppendText(const char* chunk);

    // Get the current text
    std::string GetText() const;

    // Get the object
    lv_obj_t* root() const { return bubble_; }
    lv_obj_t* label() const { return label_; }

    // Show/hide
    void Show();
    void Hide();

    // Scroll into view
    void ScrollToView(bool anim = false);

private:
    void ApplyStyle(Align align);

    lv_obj_t* bubble_ = nullptr;
    lv_obj_t* label_ = nullptr;
    std::string text_;
    Align align_ = Align::Left;
};

// ============================================================
// ProgressBar: a progress bar
// ============================================================
class ProgressBar {
public:
    ProgressBar();
    ~ProgressBar();

    // Create the progress bar
    void Create(lv_obj_t* parent, int x, int y, int w, int h = 8);

    // Set the progress (0-100)
    void SetValue(int value);

    // Get the object
    lv_obj_t* root() const { return bar_; }

    // Show/hide
    void Show();
    void Hide();

    // Set style
    void SetBgColor(lv_color_t color);
    void SetIndicColor(lv_color_t color);

private:
    lv_obj_t* bar_ = nullptr;
};

// ============================================================
// Utility functions
// ============================================================

// Safely set the label's long mode (auto wrap)
inline void SetLabelWrap(lv_obj_t* label) {
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
}

// Get the font height
inline int GetFontHeight(const lv_font_t* font) {
    return font ? font->line_height : 16;
}

// Measure the text width
inline int GetTextWidth(const char* text, const lv_font_t* font) {
    if (!text || !font) return 0;
    lv_point_t size = {0, 0};
    lv_text_get_size(&size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return (int)size.x;
}

}  // namespace ui

#endif  // UI_COMPONENTS_H
