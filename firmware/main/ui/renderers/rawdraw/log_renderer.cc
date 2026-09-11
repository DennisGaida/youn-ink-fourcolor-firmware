/**
 * @file log_renderer.cc
 * @brief Log page renderer - displays system events and diagnostics
 *
 * Shows boot events, connection status, memory stats, and recent
 * activity log entries. Scrollable list with UP/DOWN navigation.
 */

#include "log_renderer.h"
#include "rawdraw/rawdraw.h"
#include "rawdraw/style.h"
#include "rawdraw/layout_utils.h"  // FIX: 使用 InkCenteredTextTopYInBox 替代 line_height 居中
#include "rawdraw/theme.h"
#include "i18n.h"
#include <cstring>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <mutex>

// External font references
extern const lv_font_t SourceHanSansSC_Regular_slim;
extern const lv_font_t SourceHanSansSC_Medium_slim;
extern const lv_font_t font_zectrix_16_1;

namespace rawdraw {

// Boot timestamp captured at renderer construction
static time_t s_boot_time = 0;
static bool s_boot_time_set = false;

// Static event log entries (circular buffer).
// CollectLogEntries() (writer, via Init()/BOOT-long-press) can run on a
// different FreeRTOS task than Render() (reader, via the app loop's periodic
// refresh) — button callbacks dispatch on the esp_timer task while page
// refreshes run on the app_main task. Without this lock, a refresh landing
// mid-rewrite reads a partially-updated array, producing garbled/inconsistent
// output across successive e-paper refresh passes.
static std::mutex s_log_mutex;
static constexpr int kMaxLogEntries = 32;
static LogEntry s_log_entries[kMaxLogEntries];
static int s_log_count = 0;
static int s_log_head = 0;  // Next write position

LogRenderer::LogRenderer()
    : selected_index_(0)
    , scroll_offset_(0)
    , font_(&SourceHanSansSC_Regular_slim)
    , title_font_(&SourceHanSansSC_Medium_slim)
    , icon_font_(&font_zectrix_16_1) {
    if (!s_boot_time_set) {
        s_boot_time = time(nullptr);
        s_boot_time_set = true;
    }
}

LogRenderer::~LogRenderer() = default;

void LogRenderer::Init(int width, int height) {
    width_ = width;
    height_ = height;
    needs_full_refresh_ = true;
    CollectLogEntries();
    ClampScrollOffset();
}

void LogRenderer::CollectLogEntries() {
    // Locks for the whole rebuild (including the AddLogEntry() calls below,
    // which assume the lock is already held — see s_log_mutex declaration).
    std::lock_guard<std::mutex> lock(s_log_mutex);
    s_log_count = 0;
    s_log_head = 0;

    // Add boot event
    AddLogEntry("BOOT", i18n::Tr("系统启动", "System booted"));

    // Add memory stats
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    char mem_buf[64];
    snprintf(mem_buf, sizeof(mem_buf), i18n::Tr("可用内存: %zu KB", "Free memory: %zu KB"), free_heap / 1024);
    AddLogEntry("MEM", mem_buf);

    if (free_psram > 0) {
        snprintf(mem_buf, sizeof(mem_buf), "PSRAM: %zu KB", free_psram / 1024);
        AddLogEntry("PSRAM", mem_buf);
    }

    // Add chip info
    AddLogEntry("CHIP", i18n::Tr("芯片: ESP32-S3", "Chip: ESP32-S3"));

    // Add firmware version
    AddLogEntry("FW", "v" PROJECT_VER);

    // Add RTC status
    AddLogEntry("RTC", i18n::Tr("RTC 已初始化", "RTC initialized"));

    // Add WiFi placeholder (could be wired to real WiFi state)
    AddLogEntry("WIFI", i18n::Tr("等待连接...", "Waiting to connect..."));

    // Add LAN placeholder
    AddLogEntry("LAN", i18n::Tr("等待服务器...", "Waiting for server..."));
}

void LogRenderer::AddLogEntry(const char* tag, const char* message) {
    int write_index;
    if (s_log_count >= kMaxLogEntries) {
        // Buffer full: overwrite oldest entry
        s_log_head = (s_log_head + 1) % kMaxLogEntries;
        write_index = s_log_head;
    } else {
        // Still filling: append after the last written entry
        write_index = s_log_count;
        s_log_count++;
    }

    LogEntry& entry = s_log_entries[write_index];
    entry.time = time(nullptr);
    strncpy(entry.tag, tag, sizeof(entry.tag) - 1);
    entry.tag[sizeof(entry.tag) - 1] = '\0';
    strncpy(entry.message, message, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';
}

void LogRenderer::Render(uint8_t* fb, int width, int height) {
    if (!fb) return;

    // Held for the whole render pass so a concurrent CollectLogEntries()
    // (e.g. from a BOOT-long-press refresh on another task) can't be read
    // mid-rewrite — see s_log_mutex declaration.
    std::lock_guard<std::mutex> lock(s_log_mutex);

    // === Title bar ===
    DrawTitleBar(fb, width);

    // === Content area ===
    const int content_top = Style::kStatusBarHeight + kTitleBarH + Style::kSpacingXS;
    const int content_bottom = height - Style::kSpacingSM;
    const int content_height = content_bottom - content_top;
    const int content_left = Style::kSpacingMD;
    const int content_right = width - Style::kSpacingMD;

    // Entries are collected once in Init() (re-run whenever this page is
    // switched into). Re-collecting here on every Render() call re-samples
    // live heap/PSRAM stats each frame; on e-paper's partial refresh, digits
    // that change by a few KB between redraws don't get cleanly erased and
    // visibly ghost/stack on top of each other.
    if (s_log_count == 0) {
        const char* empty_text = i18n::Tr("暂无日志", "No log entries");
        int text_w = MeasureTextWidth(empty_text, font_);
        int text_x = (width - text_w) / 2;
        int text_y = content_top + (content_height / 2);
        DrawText(fb, width, text_x, text_y, empty_text, font_,
                 ThemeManager::Get().ColorFor(ThemeToken::TextSecondary));
        needs_full_refresh_ = false;
        return;
    }

    const int line_h = font_->line_height + Style::kSpacingXS;
    const int tag_w = MeasureTextWidth("WWWWW", font_) + Style::kSpacingSM;
    const int visible_items = content_height / line_h;

    int y = content_top;

    for (int i = scroll_offset_; i < s_log_count; i++) {
        if (y + line_h > content_bottom) break;

        const LogEntry& entry = s_log_entries[i];
        bool selected = (i == selected_index_);
        const auto& theme = ThemeManager::Get();
        const PaintStyle selected_style = theme.Component(ComponentRole::SettingsSelected);

        // Selected: inverted background
        if (selected) {
            DrawStyledRect(fb, width, {content_left, y, content_right - content_left, line_h}, selected_style);
        }

        // Tag (left-aligned, monospace style)
        Color fg = selected ? selected_style.fg : theme.ColorFor(ThemeToken::TextPrimary);
        DrawText(fb, width, content_left, y, entry.tag, font_, fg);

        // Message (right of tag)
        DrawText(fb, width, content_left + tag_w, y, entry.message, font_, fg);

        y += line_h;
    }

    // === Scroll indicator ===
    if (s_log_count > visible_items) {
        const int bar_w = Style::kScrollbarWidth;
        const int bar_x = width - bar_w - Style::kScrollMargin;
        int bar_h = (content_height * visible_items) / s_log_count;
        if (bar_h < Style::kScrollbarMinH) bar_h = Style::kScrollbarMinH;
        int bar_offset = (scroll_offset_ * content_height) / s_log_count;
        int bar_y = content_top + bar_offset;
        if (bar_y + bar_h > content_bottom) bar_h = content_bottom - bar_y;

        const Color thumb = ThemeManager::Get().ColorFor(ThemeToken::Selected);
        DrawRoundRect(fb, width, {bar_x, bar_y, bar_w, bar_h},
                      Style::kBorderRadiusSM, thumb, thumb, 0);
    }

    needs_full_refresh_ = false;
}

void LogRenderer::DrawTitleBar(uint8_t* fb, int width) {
    const auto& theme = ThemeManager::Get();
    const PaintStyle bar_style = theme.Style(ThemeToken::BackgroundSecondary);
    const Color text = theme.ColorFor(ThemeToken::TextPrimary);
    const Color secondary = theme.ColorFor(ThemeToken::TextSecondary);
    const Color border = theme.ColorFor(ThemeToken::Border);
    const int title_y_start = Style::kStatusBarHeight;
    const int title_bar_h = kTitleBarH;
    // Clear title bar area (separate from status bar above)
    DrawStyledRect(fb, width, {0, title_y_start, width, title_bar_h}, bar_style);

    // No top separator needed — status bar bottom border serves as separator

    // Bottom separator (2px)
    const int line_y = title_y_start + title_bar_h - 2;
    DrawHLine(fb, width, line_y, 0, width, border);
    DrawHLine(fb, width, line_y + 1, 0, width, border);

    // FIX: 改用 InkCenteredTextTopYInBox，避免 line_height 居中导致中文偏上
    // 参见 wiki/projects/notellm-baseline-alignment.md
    const char* title_str = i18n::Tr("日志", "Log");
    int title_text_y = InkCenteredTextTopYInBox(font_, title_str, title_y_start, title_bar_h, 1);
    DrawText(fb, width, Style::kSpacingLG, title_text_y, title_str, font_, text);

    // Entry count (right-aligned)
    if (s_log_count > 0) {
        char count_buf[16];
        snprintf(count_buf, sizeof(count_buf), i18n::Tr("%d条", "%d entries"), s_log_count);
        int count_w = MeasureTextWidth(count_buf, font_);
        int count_x = width - count_w - Style::kSpacingLG;
        DrawText(fb, width, count_x, title_text_y, count_buf, font_, secondary);
    }
}

bool LogRenderer::HandleInput(const ButtonEvent& event) {
    // Short-lived lock scopes only (not the whole function) — the
    // kBootLongPress branch below calls CollectLogEntries(), which takes
    // this same non-recursive mutex itself.
    int log_count;
    {
        std::lock_guard<std::mutex> lock(s_log_mutex);
        log_count = s_log_count;
    }
    if (log_count == 0) return false;

    switch (event.type) {
        case ButtonEvent::kUpClick:
            if (selected_index_ > 0) {
                selected_index_--;
                if (selected_index_ < scroll_offset_) {
                    scroll_offset_ = selected_index_;
                }
                needs_full_refresh_ = true;
                return true;
            }
            break;

        case ButtonEvent::kDownClick: {
            if (selected_index_ < log_count - 1) {
                selected_index_++;
                const int content_h = height_ - Style::kStatusBarHeight - Style::kSpacingXXS;
                const int line_h = font_->line_height + Style::kSpacingXS;
                int visible = content_h / line_h;
                if (visible < 1) visible = 1;
                int max_offset = log_count - visible;
                if (max_offset < 0) max_offset = 0;
                if (selected_index_ >= scroll_offset_ + visible) {
                    scroll_offset_ = selected_index_ - visible + 1;
                }
                needs_full_refresh_ = true;
                return true;
            }
            break;
        }

        case ButtonEvent::kBootLongPress:
            // Refresh log data
            CollectLogEntries();
            selected_index_ = 0;
            scroll_offset_ = 0;
            needs_full_refresh_ = true;
            return true;

        default:
            break;
    }

    return false;
}

void LogRenderer::ClampScrollOffset() {
    int log_count;
    {
        std::lock_guard<std::mutex> lock(s_log_mutex);
        log_count = s_log_count;
    }
    if (log_count == 0) {
        scroll_offset_ = 0;
        return;
    }
    const int content_h = height_ - Style::kStatusBarHeight - Style::kSpacingXXS;
    const int line_h = font_->line_height + Style::kSpacingXS;
    const int visible = content_h / line_h;
    int max_offset = log_count - visible;
    if (max_offset < 0) max_offset = 0;
    scroll_offset_ = std::max(0, std::min(scroll_offset_, max_offset));
}

}  // namespace rawdraw
