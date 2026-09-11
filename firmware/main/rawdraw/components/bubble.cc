/**
 * @file bubble.cc
 * @brief Chat bubble implementation - v3.8.0 visual fix
 *
 * F2 FIXES:
 * - Drop the complex rounded border, use simple rectangular color blocks instead
 * - Draw order: background first -> then text
 * - User message: black rectangle + white text
 * - AI message: no background (default white) + black text
 * - Remove rounded-rect drawing to reduce rendering issues
 */

#include "bubble.h"
#include "rawdraw/theme.h"
#include <algorithm>

namespace rawdraw {

Bubble::Bubble(BubbleAlign align, int margin, int max_width, int radius)
    : align_(align)
    , margin_(margin)
    , max_width_(max_width)
    , radius_(radius)
    , font_(nullptr)
    , line_spacing_(0)
    , padding_(4)
    , text_()
    , y_(0)
    , fill_color_(WHITE)
    , text_color_(BLACK)
    , border_color_(BLACK)
    , border_width_(1)
    , custom_colors_(false) {
    ApplyDefaultStyle();
}

Bubble::~Bubble() {}

void Bubble::ApplyDefaultStyle() {
    if (custom_colors_) return;

    const auto& theme = ThemeManager::Get();
    switch (align_) {
        case BubbleAlign::Left:
            // AI/system messages: themed surface with readable border
            fill_color_ = theme.ColorFor(ThemeToken::BackgroundPrimary);
            text_color_ = theme.ColorFor(ThemeToken::TextPrimary);
            border_color_ = theme.ColorFor(ThemeToken::Border);
            border_width_ = theme.Component(ComponentRole::CardDefault).border_width;
            break;
        case BubbleAlign::Right:
            // User messages: accent/selected block with inverse text where needed
            fill_color_ = theme.ColorFor(ThemeToken::Selected);
            text_color_ = theme.Component(ComponentRole::ButtonSelected).fg;
            border_color_ = theme.Component(ComponentRole::ButtonSelected).border;
            border_width_ = 0;
            break;
        case BubbleAlign::Center:
            // System notifications: minimal styling
            fill_color_ = theme.ColorFor(ThemeToken::BackgroundPrimary);
            text_color_ = theme.ColorFor(ThemeToken::TextSecondary);
            border_color_ = fill_color_;  // No visible border
            border_width_ = 0;
            break;
    }
}

void Bubble::SetAlign(BubbleAlign align) {
    align_ = align;
    ApplyDefaultStyle();
}

void Bubble::SetMargin(int margin) { margin_ = margin; }
void Bubble::SetMaxWidth(int max_width) { max_width_ = max_width; }
void Bubble::SetRadius(int radius) { radius_ = radius; }
void Bubble::SetFont(const lv_font_t* font) { font_ = font; }
void Bubble::SetLineSpacing(int spacing) { line_spacing_ = spacing; }
void Bubble::SetPadding(int padding) { padding_ = padding; }

void Bubble::SetText(const char* text) {
    text_ = text ? text : "";
}

void Bubble::SetText(const std::string& text) {
    text_ = text;
}

void Bubble::AppendText(const char* chunk) {
    if (chunk) {
        text_ += chunk;
    }
}

void Bubble::AppendText(const std::string& chunk) {
    text_ += chunk;
}

void Bubble::Clear() {
    text_.clear();
}

const std::string& Bubble::GetText() const {
    return text_;
}

bool Bubble::HasContent() const {
    return !text_.empty();
}

void Bubble::SetY(int y) {
    y_ = y;
}

Rect Bubble::CalculateTextBounds() const {
    if (!font_ || text_.empty()) {
        return {0, 0, 0, 0};
    }
    return MeasureTextBounds(text_.c_str(), font_, max_width_ - 2 * padding_);
}

int Bubble::CalculateHeight() const {
    Rect text_bounds = CalculateTextBounds();
    // Count lines to apply line spacing properly
    int line_count = 1;
    if (text_.empty()) {
        line_count = 0;
    } else {
        for (char c : text_) {
            if (c == '\n') line_count++;
        }
    }
    // Also account for word-wrapping lines from MeasureTextBounds
    if (text_bounds.h > 0 && font_) {
        int measured_lines = text_bounds.h / font_->line_height;
        if (measured_lines > line_count) line_count = measured_lines;
    }

    // Line step must be >= 24px for 1bpp readability
    int line_step = font_->line_height + line_spacing_;
    if (line_step < 24) line_step = 24;

    int height = (line_count > 0) ? line_step * line_count + 2 * padding_
                                  : 2 * padding_ + (font_ ? font_->line_height : 16);

    // Minimum height check
    if (font_) {
        int min_height = font_->line_height + 2 * padding_;
        if (height < min_height) height = min_height;
    } else {
        int min_height = 2 * padding_ + 16;
        if (height < min_height) height = min_height;
    }

    return height;
}

int Bubble::CalculateWidth() const {
    Rect text_bounds = CalculateTextBounds();
    int width = text_bounds.w + 2 * padding_;

    // Minimum width check: ensure bubble has reasonable width
    int min_width = 2 * padding_ + 20;  // At least 20px text width
    if (width < min_width) {
        width = min_width;
    }

    // Respect max_width if set
    if (max_width_ > 0 && width > max_width_) {
        width = max_width_;
    }

    return width;
}

Rect Bubble::GetBounds(int screen_width) const {
    int w = CalculateWidth();
    int h = CalculateHeight();

    int x;
    switch (align_) {
        case BubbleAlign::Left:
            x = margin_;
            break;
        case BubbleAlign::Right:
            x = screen_width - margin_ - w;
            break;
        case BubbleAlign::Center:
            x = (screen_width - w) / 2;
            break;
        default:
            x = margin_;
    }

    // Boundary clamping: ensure bubble stays within screen bounds
    // For right-aligned bubbles, clamp to not overflow right edge
    if (align_ == BubbleAlign::Right) {
        if (x + w > screen_width - margin_) {
            x = screen_width - margin_ - w;
        }
    }
    // For left-aligned bubbles, clamp to not overflow left edge
    if (align_ == BubbleAlign::Left) {
        if (x < margin_) {
            x = margin_;
        }
    }
    // For centered bubbles, ensure both edges are within bounds
    if (align_ == BubbleAlign::Center) {
        if (x + w > screen_width - margin_) {
            x = screen_width - margin_ - w;
        }
        if (x < margin_) {
            x = margin_;
        }
    }

    return {x, y_, w, h};
}

void Bubble::SetColors(Color fill, Color text, Color border, int border_width) {
    fill_color_ = fill;
    text_color_ = text;
    border_color_ = border;
    border_width_ = border_width;
    custom_colors_ = true;
}

void Bubble::Draw(uint8_t* fb, int width, int height) {
    if (!fb || !HasContent()) return;
    if (!custom_colors_) {
        ApplyDefaultStyle();
    }

    Rect bounds = GetBounds(width);

    // Clamp to screen
    bounds = clamp_rect(bounds, width, height);
    if (rect_area(bounds) <= 0) return;

    // F2: Draw order = background -> text
    // Rounded-rect background, using the radius_ parameter

    // === F2: Background (draw the background color block first) ===
    if (align_ == BubbleAlign::Right) {
        // User message: theme selection/highlight background
        if (radius_ > 0) {
            DrawRoundRect(fb, width, bounds, radius_, fill_color_, border_color_, 0);
        } else {
            DrawRect(fb, width, bounds, fill_color_);
        }
    } else if (border_width_ > 0) {
        // AI/System message with border: draw theme surface + border
        if (radius_ > 0) {
            DrawRoundRect(fb, width, bounds, radius_, fill_color_, border_color_, border_width_);
        } else {
            DrawRect(fb, width, bounds, fill_color_);
            DrawRectBorder(fb, width, bounds, border_width_, border_color_);
        }
    } else {
        // AI/System message without border: don't draw a background (default white), just draw text
    }

    // === Text (render line-by-line with explicit line spacing >=24px) ===
    if (font_) {
        int text_x = bounds.x + padding_;
        int text_y = bounds.y + padding_;

        // Line step must be >=24px for 1bpp readability
        int line_step = font_->line_height + line_spacing_;
        if (line_step < 24) line_step = 24;

        Color draw_color = text_color_;
        const char* p = text_.c_str();
        int current_y = text_y;
        char line_buf[256];

        while (*p && current_y < bounds.y + bounds.h) {
            int i = 0;
            while (*p && *p != '\n' && i < 255) {
                line_buf[i++] = *p++;
            }
            line_buf[i] = '\0';
            if (*p == '\n') p++;

            if (line_buf[0] != '\0') {
                DrawText(fb, width, text_x, current_y, line_buf, font_, draw_color, height);
            }
            current_y += line_step;
        }
    }
}

void Bubble::Draw(Framebuffer* fb, int screen_width, int screen_height) {
    if (!fb) return;
    fb->SafeDraw([this, screen_width, screen_height](uint8_t* buffer) {
        this->Draw(buffer, screen_width, screen_height);
    });
}

}  // namespace rawdraw
