/**
 * @file lifebar_renderer.cc
 * @brief Life progress page renderer implementation
 *
 * Large circular gauge showing life percentage, age, days elapsed/remaining,
 * weekends remaining, and a motivational quote.
 * Default: birthdate 1990-01-01, 80-year lifespan.
 */

#include "lifebar_renderer.h"
#include "i18n.h"
#include "rawdraw/rawdraw.h"
#include "rawdraw/layout_utils.h"  // FIX: 使用 InkCenteredTextTopY 替代 line_height 居中
#include "rawdraw/components/progress_bar.h"
#include "rawdraw/theme.h"
#include <algorithm>
#include <cstdio>
#include <ctime>

// External font references
extern const lv_font_t SourceHanSansSC_Regular_slim;
extern const lv_font_t SourceHanSansSC_Medium_slim;
extern const lv_font_t font_zectrix_16_1;

namespace rawdraw {

// ============================================================
// Configurable defaults
// ============================================================

static constexpr int BIRTH_YEAR  = 1990;
static constexpr int BIRTH_MONTH = 1;
static constexpr int BIRTH_DAY   = 1;
static constexpr int EXPECTED_LIFESPAN_YEARS = 80;

// Motivational quotes (rotated by index). Translation is looked up fresh at
// render time (see RenderQuote) rather than cached here, so a language switch
// takes effect immediately.
struct QuoteText {
    const char* zh;
    const char* en;
};
static const QuoteText kQuotes[] = {
    {"时间是最公平的，\n每人每天都只有24小时", "Time is the fairest thing —\neveryone gets 24 hours a day"},
    {"余生很长，何必慌张；\n余生很短，何必平凡", "If life is long, why rush;\nif life is short, why be ordinary"},
    {"把每一天当成\n生命中最后一天来过", "Live each day\nas if it were your last"},
    {"种一棵树最好的时间\n是十年前，其次是现在", "The best time to plant a tree\nwas ten years ago; the next best is now"},
    {"人生没有白走的路，\n每一步都算数", "No step in life is wasted —\nevery step counts"},
};
static constexpr int kNumQuotes = sizeof(kQuotes) / sizeof(kQuotes[0]);

// ============================================================
// Lifecycle
// ============================================================

LifeBarRenderer::LifeBarRenderer()
    : title_font_(&SourceHanSansSC_Medium_slim)
    , body_font_(&SourceHanSansSC_Regular_slim)
    , small_font_(&SourceHanSansSC_Regular_slim)
    , age_years_(0)
    , age_months_(0)
    , days_elapsed_(0)
    , days_remaining_(0)
    , weekends_remaining_(0)
    , life_pct_(0)
    , visible_(true) {
}

LifeBarRenderer::~LifeBarRenderer() {}

// ============================================================
// Data calculation
// ============================================================

static bool is_leap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static int days_in_month(int y, int m) {
    static const int d[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 1 && is_leap(y)) return 29;
    return d[m];
}

void LifeBarRenderer::UpdateStats() {
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    int cur_year  = tm_now.tm_year + 1900;
    int cur_month = tm_now.tm_mon + 1;   // 1-based
    int cur_day   = tm_now.tm_mday;

    // Days elapsed since birth
    int total_days = 0;

    // Full years
    for (int y = BIRTH_YEAR; y < cur_year; y++) {
        total_days += is_leap(y) ? 366 : 365;
    }

    // Full months of current year
    for (int m = 0; m < cur_month - 1; m++) {
        total_days += days_in_month(cur_year, m);
    }

    // Days of current month (minus birth offset for first year)
    if (cur_year == BIRTH_YEAR) {
        total_days += cur_day - BIRTH_DAY;
    } else {
        total_days += cur_day;
    }

    if (total_days < 0) total_days = 0;

    // Total lifespan in days
    int lifespan_days = 0;
    for (int y = BIRTH_YEAR; y < BIRTH_YEAR + EXPECTED_LIFESPAN_YEARS; y++) {
        lifespan_days += is_leap(y) ? 366 : 365;
    }

    // Age
    age_years_ = cur_year - BIRTH_YEAR;
    age_months_ = cur_month - BIRTH_MONTH;
    if (age_months_ < 0) {
        age_years_--;
        age_months_ += 12;
    }
    if (cur_day < BIRTH_DAY) {
        age_months_--;
        if (age_months_ < 0) {
            age_years_--;
            age_months_ += 12;
        }
    }

    days_elapsed_ = total_days;
    days_remaining_ = lifespan_days - total_days;
    if (days_remaining_ < 0) days_remaining_ = 0;

    // Weekends remaining (roughly 2/7 of remaining days)
    weekends_remaining_ = (days_remaining_ * 2) / 7;

    // Life percentage
    if (lifespan_days > 0) {
        life_pct_ = (total_days * 100) / lifespan_days;
    }
    if (life_pct_ > 100) life_pct_ = 100;
}

// ============================================================
// Init
// ============================================================

void LifeBarRenderer::Init(int width, int height) {
    width_ = width;
    height_ = height;
    needs_full_refresh_ = true;
    UpdateStats();
}

// ============================================================
// Render
// ============================================================

void LifeBarRenderer::Render(uint8_t* fb, int width, int height) {
    if (!fb) return;
    const auto& theme = ThemeManager::Get();
    const Color text = theme.ColorFor(ThemeToken::TextPrimary);
    const Color secondary = theme.ColorFor(ThemeToken::TextSecondary);

    if (!visible_) {
        // Show hidden placeholder
        const char* msg = i18n::Tr("人生进度页已隐藏", "Life progress page is hidden");
        int msg_w = MeasureTextWidth(msg, small_font_);
        int msg_x = (width - msg_w) / 2;
        msg_x = (msg_x + 7) & ~7;
        // FIX: 改用 InkCenteredTextTopY，避免 line_height 居中导致中文偏上
        // 参见 wiki/projects/notellm-baseline-alignment.md
        int msg_y = InkCenteredTextTopY(small_font_, msg, height / 2, 0);
        DrawText(fb, width, msg_x, msg_y, msg, small_font_, text);

        const char* hint = i18n::Tr("在设置中重新开启", "Re-enable it in Settings");
        int hint_w = MeasureTextWidth(hint, small_font_);
        int hint_x = (width - hint_w) / 2;
        hint_x = (hint_x + 7) & ~7;
        int hint_y = msg_y + small_font_->line_height + Style::kSpacingSM;
        DrawText(fb, width, hint_x, hint_y, hint, small_font_, secondary);
        needs_full_refresh_ = false;
        return;
    }

    UpdateStats();

    int y = Style::kStatusBarHeight + Style::kSpacingMD;

    // === Header ===
    RenderHeader(fb, width, y);
    y += title_font_->line_height + Style::kSpacingXXS +
         small_font_->line_height + Style::kSpacingSM;

    // === Circular gauge ===
    RenderGauge(fb, width, y, height - y - Style::kSpacingMD);
    // Gauge uses the remaining vertical space

    needs_full_refresh_ = false;
}

void LifeBarRenderer::RenderHeader(uint8_t* fb, int width, int y) const {
    const auto& theme = ThemeManager::Get();
    const Color text = theme.ColorFor(ThemeToken::TextPrimary);
    const Color secondary = theme.ColorFor(ThemeToken::TextSecondary);
    // Title
    const char* title = i18n::Tr("人生进度", "Life Progress");
    int title_w = MeasureTextWidth(title, title_font_);
    int title_x = (width - title_w) / 2;
    title_x = (title_x + 7) & ~7;
    DrawText(fb, width, title_x, y, title, title_font_, text);

    // Subtitle
    const char* sub = i18n::Tr("每一天都值得珍惜", "Every day is worth cherishing");
    int sub_w = MeasureTextWidth(sub, small_font_);
    int sub_x = (width - sub_w) / 2;
    sub_x = (sub_x + 7) & ~7;
    int sub_y = y + title_font_->line_height + Style::kSpacingXXS;
    DrawText(fb, width, sub_x, sub_y, sub, small_font_, secondary);
}

void LifeBarRenderer::RenderGauge(uint8_t* fb, int width, int y_start, int available_h) const {
    const auto& theme = ThemeManager::Get();
    const PaintStyle progress_style = theme.Component(ComponentRole::Progress);
    const Color text = theme.ColorFor(ThemeToken::TextPrimary);
    const Color secondary = theme.ColorFor(ThemeToken::TextSecondary);
    // ColorFor(Accent) returns badge-text white (meant for text on a colored
    // badge bg), which is invisible on the plain page background at the
    // gauge center — use the badge's colored side instead.
    const Color accent = theme.Style(ThemeToken::Accent).bg;
    // Gauge geometry
    const int gauge_thickness = 8;
    const int cx = width / 2;
    const int top_gap = 5;

    // A hardcoded radius left no room below the gauge for the 3rd stat line
    // (weekends remaining) or the quote — they were silently skipped every
    // time on this screen size. Reserve their vertical budget first and size
    // the gauge with whatever's left, so all content actually gets drawn.
    const int stats_reserved_h = 3 * (small_font_->line_height + Style::kSpacingXS) +
                                  Style::kSpacingXS +
                                  (small_font_->line_height * 2 + Style::kSpacingXS);
    int gauge_r = (available_h - top_gap - gauge_thickness - Style::kSpacingMD - stats_reserved_h) / 2;
    gauge_r = std::max(40, std::min(70, gauge_r));

    int cy = y_start + gauge_r + top_gap;

    // Check if gauge fits within available space
    const int gauge_bottom = cy + gauge_r;
    const int content_bottom = height_ - Style::kSpacingSM;

    // If gauge doesn't fit, shift up
    if (gauge_bottom > content_bottom) {
        cy = content_bottom - gauge_r;
    }

    // === Draw circular progress ===
    Point center = {cx, cy};
    // progress_style.fg is badge-text white (meant for ink on a colored
    // badge bg) — passed straight through as the elapsed-arc color it is
    // invisible against the page background. Draw a plain track and fill
    // the elapsed arc with the token's colored side instead.
    const Color track_color = WHITE;
    const Color fill_color = progress_style.bg;
    DrawCircularProgress(fb, width, center, gauge_r, gauge_thickness,
                         life_pct_, track_color, fill_color);

    // === Center text: percentage number only (no overlapping text) ===
    char pct_buf[16];
    snprintf(pct_buf, sizeof(pct_buf), "%d%%", life_pct_);
    int pct_w = MeasureTextWidth(pct_buf, title_font_);
    int pct_x = cx - pct_w / 2;
    pct_x = (pct_x + 7) & ~7;
    // FIX: 改用 InkCenteredTextTopY，避免 line_height 居中导致中文偏上
    // 参见 wiki/projects/notellm-baseline-alignment.md
    int pct_y = InkCenteredTextTopY(title_font_, pct_buf, cy, 0);
    DrawText(fb, width, pct_x, pct_y, pct_buf, title_font_, accent);

    // === Stats below gauge ===
    int stats_y = cy + gauge_r + gauge_thickness + Style::kSpacingMD;
    const int bottom_limit = height_ - Style::kSpacingSM;

    // Calculate how much space stats need (3 lines minimum)
    const int stats_min_h = small_font_->line_height * 3 + Style::kSpacingXS * 2;
    // Quote needs at least 2 lines
    const int quote_min_h = small_font_->line_height * 2 + Style::kSpacingXS;

    char buf[64];

    // Line 1: Age (consistent with gauge percentage)
    if (stats_y + small_font_->line_height <= bottom_limit) {
        snprintf(buf, sizeof(buf), i18n::Tr("%d岁%d月  已过%d%%", "Age %d y %d m  %d%% elapsed"), age_years_, age_months_, life_pct_);
        int w = MeasureTextWidth(buf, small_font_);
        int x = (width - w) / 2;
        x = (x + 7) & ~7;
        DrawText(fb, width, x, stats_y, buf, small_font_, text);
        stats_y += small_font_->line_height + Style::kSpacingXS;
    }

    // Line 2: Days remaining
    if (stats_y + small_font_->line_height <= bottom_limit) {
        snprintf(buf, sizeof(buf), i18n::Tr("剩余天数 %d天", "Days left: %d"), days_remaining_);
        int w = MeasureTextWidth(buf, small_font_);
        int x = (width - w) / 2;
        x = (x + 7) & ~7;
        DrawText(fb, width, x, stats_y, buf, small_font_, secondary);
        stats_y += small_font_->line_height + Style::kSpacingXS;
    }

    // Line 3: Weekends remaining
    if (stats_y + small_font_->line_height <= bottom_limit) {
        snprintf(buf, sizeof(buf), i18n::Tr("剩余周末 %d个", "Weekends left: %d"), weekends_remaining_);
        int w = MeasureTextWidth(buf, small_font_);
        int x = (width - w) / 2;
        x = (x + 7) & ~7;
        DrawText(fb, width, x, stats_y, buf, small_font_, secondary);
        stats_y += small_font_->line_height + Style::kSpacingXS;
    }

    // === Motivational quote at bottom — only if enough room ===
    int quote_start_y = stats_y + Style::kSpacingXS;
    if (quote_start_y + quote_min_h <= bottom_limit) {
        RenderQuote(fb, width, quote_start_y);
    } else {
        // Try squeeze: skip gap, draw directly
        if (stats_y + quote_min_h <= bottom_limit) {
            RenderQuote(fb, width, stats_y);
        }
        // else: not enough space, skip quote entirely
    }
}

void LifeBarRenderer::RenderQuote(uint8_t* fb, int width, int y) const {
    if (y + small_font_->line_height > height_ - Style::kSpacingSM) {
        return;  // Not enough space
    }

    // Pick quote by day (rotates daily)
    time_t now = time(nullptr);
    struct tm tm_buf;
    localtime_r(&now, &tm_buf);
    int idx = (tm_buf.tm_yday) % kNumQuotes;
    const char* quote = i18n::Tr(kQuotes[idx].zh, kQuotes[idx].en);

    // Draw quote lines
    char line[64];
    int line_idx = 0;
    int max_lines = 2;
    const char* p = quote;

    while (*p && line_idx < max_lines) {
        int i = 0;
        while (*p && *p != '\n' && i < 63) {
            line[i++] = *p++;
        }
        line[i] = '\0';
        if (*p == '\n') p++;

        int w = MeasureTextWidth(line, small_font_);
        int x = (width - w) / 2;
        x = (x + 7) & ~7;
        DrawText(fb, width, x, y + line_idx * (small_font_->line_height + Style::kSpacingXS),
                 line, small_font_, ThemeManager::Get().ColorFor(ThemeToken::TextSecondary));
        line_idx++;
    }
}

// ============================================================
// Input handling
// ============================================================

bool LifeBarRenderer::HandleInput(const ButtonEvent& event) {
    // No interactive navigation needed yet — page is informational
    (void)event;
    return false;
}

}  // namespace rawdraw
