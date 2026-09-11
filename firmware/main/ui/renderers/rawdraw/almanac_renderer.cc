/**
 * @file almanac_renderer.cc
 * @brief Almanac page renderer - displays lunar date, solar terms, and almanac info
 *
 * Uses Calendar::ToLunarDate() for lunar calendar conversion (2000-2050).
 * Shows: today's Gregorian date, lunar date, solar term, weekday, and
 * traditional almanac info (yiji - auspicious/inauspicious activities).
 */

#include "almanac_renderer.h"
#include "rawdraw/rawdraw.h"
#include "rawdraw/style.h"
#include "rawdraw/layout_utils.h"  // FIX: 使用 InkCenteredTextTopYInBox 替代 line_height 居中
#include "rawdraw/components/calendar.h"
#include "rawdraw/theme.h"
#include "i18n.h"
#include <cstring>
#include <ctime>
#include <cstdio>

// External font references
extern const lv_font_t SourceHanSansSC_Regular_slim;
extern const lv_font_t SourceHanSansSC_Medium_slim;
extern const lv_font_t weather_icons_48;

// Weekday characters (matches calendar.cc)
static const char* kWeekdayFullZh[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
static const char* kWeekdayFullEn[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char* WeekdayFullName(int i) {
    return i18n::GetLanguage() == i18n::Language::kEnUS ? kWeekdayFullEn[i] : kWeekdayFullZh[i];
}

// Lunar month/day names (same as calendar.cc)
static const char* kLunarMonthsZh[] = {
    "正月", "二月", "三月", "四月", "五月", "六月",
    "七月", "八月", "九月", "十月", "十一月", "腊月"
};
static const char* kLunarMonthsEn[] = {
    "1st Lunar Month", "2nd Lunar Month", "3rd Lunar Month", "4th Lunar Month",
    "5th Lunar Month", "6th Lunar Month", "7th Lunar Month", "8th Lunar Month",
    "9th Lunar Month", "10th Lunar Month", "11th Lunar Month", "12th Lunar Month"
};
static const char* kLunarDaysZh[] = {
    "初一", "初二", "初三", "初四", "初五", "初六", "初七", "初八", "初九", "初十",
    "十一", "十二", "十三", "十四", "十五", "十六", "十七", "十八", "十九", "二十",
    "廿一", "廿二", "廿三", "廿四", "廿五", "廿六", "廿七", "廿八", "廿九", "三十"
};
static const char* kLunarDaysEn[] = {
    "1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "9th", "10th",
    "11th", "12th", "13th", "14th", "15th", "16th", "17th", "18th", "19th", "20th",
    "21st", "22nd", "23rd", "24th", "25th", "26th", "27th", "28th", "29th", "30th"
};

// Tian Gan / Di Zhi (used via Calendar::GetLunarYearName)
// static const char* kTianGan[] = ...;  // Not used directly, Calendar handles it
// static const char* kDiZhi[] = ...;

// Solar terms (same as calendar.cc)
struct SolarTermEntry {
    int month;
    int day;
    const char* name_zh;
    const char* name_en;
};
static const SolarTermEntry kSolarTerms[] = {
    { 1,  5, "小寒", "Minor Cold" }, { 1, 20, "大寒", "Major Cold" },
    { 2,  4, "立春", "Start of Spring" }, { 2, 19, "雨水", "Rain Water" },
    { 3,  5, "惊蛰", "Awakening of Insects" }, { 3, 20, "春分", "Spring Equinox" },
    { 4,  4, "清明", "Clear and Bright (Qingming)" }, { 4, 20, "谷雨", "Grain Rain" },
    { 5,  5, "立夏", "Start of Summer" }, { 5, 21, "小满", "Grain Full" },
    { 6,  5, "芒种", "Grain in Ear" }, { 6, 21, "夏至", "Summer Solstice" },
    { 7,  7, "小暑", "Minor Heat" }, { 7, 23, "大暑", "Major Heat" },
    { 8,  7, "立秋", "Start of Autumn" }, { 8, 23, "处暑", "End of Heat" },
    { 9,  7, "白露", "White Dew" }, { 9, 23, "秋分", "Autumn Equinox" },
    {10,  8, "寒露", "Cold Dew" }, {10, 23, "霜降", "Frost's Descent" },
    {11,  7, "立冬", "Start of Winter" }, {11, 22, "小雪", "Minor Snow" },
    {12,  7, "大雪", "Major Snow" }, {12, 22, "冬至", "Winter Solstice" },
};

static const char* GetSolarTerm(int month, int day) {
    for (size_t i = 0; i < sizeof(kSolarTerms) / sizeof(kSolarTerms[0]); i++) {
        if (kSolarTerms[i].month == month && kSolarTerms[i].day == day) {
            return i18n::Tr(kSolarTerms[i].name_zh, kSolarTerms[i].name_en);
        }
    }
    return nullptr;
}

// Simplified yiji (宜忌) based on lunar day patterns
// This is a traditional approximation, not a full almanac calculation
static const char* kYiTableZh[][4] = {
    {"祭祀", "祈福", "出行", "动土"},
    {"嫁娶", "纳采", "订盟", "出行"},
    {"开市", "交易", "立券", "纳财"},
    {"破土", "启钻", "安葬", "修坟"},
    {"修造", "动土", "起基", "定磉"},
    {"安床", "开市", "交易", "立券"},
    {"祭祀", "沐浴", "扫舍", "修造"},
    {"祈福", "求嗣", "出行", "解除"},
    {"嫁娶", "祭祀", "祈福", "出行"},
    {"开市", "立券", "交易", "纳财"},
};
static const char* kYiTableEn[][4] = {
    {"Worship", "Prayer", "Travel", "Groundbreaking"},
    {"Marriage", "Betrothal Gifts", "Engagement", "Travel"},
    {"Opening Business", "Trading", "Signing Contracts", "Collecting Money"},
    {"Groundbreaking", "Exhuming", "Burial", "Repairing Graves"},
    {"Renovation", "Groundbreaking", "Laying Foundation", "Setting Base Stones"},
    {"Setting Up Bed", "Opening Business", "Trading", "Signing Contracts"},
    {"Worship", "Bathing", "Cleaning House", "Renovation"},
    {"Prayer", "Praying for Offspring", "Travel", "Dispelling Bad Luck"},
    {"Marriage", "Worship", "Prayer", "Travel"},
    {"Opening Business", "Signing Contracts", "Trading", "Collecting Money"},
};
static const char* kJiTableZh[][3] = {
    {"破土", "安葬", "启钻"},
    {"开仓", "出货财", "纳粟"},
    {"词讼", "争执", "诽谤"},
    {"嫁娶", "出行", "祈福"},
    {"安床", "移徙", "入宅"},
    {"祭祀", "修造", "动土"},
    {"开市", "纳财", "交易"},
    {"出行", "解除", "拆卸"},
    {"破土", "启钻", "安葬"},
    {"纳采", "订盟", "嫁娶"},
};
static const char* kJiTableEn[][3] = {
    {"Groundbreaking", "Burial", "Exhuming"},
    {"Opening Warehouse", "Shipping Goods", "Collecting Grain"},
    {"Litigation", "Disputes", "Slander"},
    {"Marriage", "Travel", "Prayer"},
    {"Setting Up Bed", "Moving", "Moving In"},
    {"Worship", "Renovation", "Groundbreaking"},
    {"Opening Business", "Collecting Money", "Trading"},
    {"Travel", "Dispelling Bad Luck", "Demolition"},
    {"Groundbreaking", "Exhuming", "Burial"},
    {"Betrothal Gifts", "Engagement", "Marriage"},
};

namespace rawdraw {

AlmanacRenderer::AlmanacRenderer()
    : font_(&SourceHanSansSC_Regular_slim)
    , title_font_(&SourceHanSansSC_Medium_slim)
    , icon_font_(&weather_icons_48) {
}

AlmanacRenderer::~AlmanacRenderer() = default;

void AlmanacRenderer::Init(int width, int height) {
    width_ = width;
    height_ = height;
    needs_full_refresh_ = true;
    RefreshData();
}

void AlmanacRenderer::RefreshData() {
    time_t now = time(nullptr);
    localtime_r(&now, &tm_);

    year_ = tm_.tm_year + 1900;
    month_ = tm_.tm_mon + 1;
    day_ = tm_.tm_mday;
    weekday_ = tm_.tm_wday;  // 0=Sun

    // Lunar date via Calendar algorithm
    lunar_ = Calendar::ToLunarDate(year_, month_, day_);
    lunar_year_name_ = Calendar::GetLunarYearName(year_);

    // Solar term
    solar_term_ = GetSolarTerm(month_, day_);

    // Yiji (宜忌) - simplified based on lunar day
    yi_idx_ = (lunar_.lunar_day - 1) % 10;
    ji_idx_ = (lunar_.lunar_day) % 10;
}

static const char* GetYiEntry(int row, int col) {
    return i18n::GetLanguage() == i18n::Language::kEnUS
               ? kYiTableEn[row][col]
               : kYiTableZh[row][col];
}

static const char* GetJiEntry(int row, int col) {
    return i18n::GetLanguage() == i18n::Language::kEnUS
               ? kJiTableEn[row][col]
               : kJiTableZh[row][col];
}

void AlmanacRenderer::Render(uint8_t* fb, int width, int height) {
    if (!fb) return;

    const int content_top = Style::kStatusBarHeight + kTitleBarH + Style::kSpacingXS;
    int y = content_top + Style::kSpacingMD;
    const auto& theme = ThemeManager::Get();
    const Color text = theme.ColorFor(ThemeToken::TextPrimary);
    const Color secondary = theme.ColorFor(ThemeToken::TextSecondary);
    // ColorFor(Accent/Danger) returns badge-text white (meant for ink drawn
    // on top of a colored badge bg), which is invisible when used directly
    // on the plain page background — use the badge's colored side instead.
    const Color accent = theme.Style(ThemeToken::Accent).bg;
    const Color danger = theme.Style(ThemeToken::Danger).bg;
    const Color border = theme.ColorFor(ThemeToken::Border);

    // === Title bar ===
    DrawTitleBar(fb, width);

    // === Large lunar year name + date ===
    // e.g. "丙午年 三月初八"
    char lunar_full[64];
    if (lunar_.lunar_month > 0 && lunar_.lunar_day > 0) {
        snprintf(lunar_full, sizeof(lunar_full), i18n::Tr("%s年 %s%s", "%s Year %s %s"),
                 lunar_year_name_, GetLunarMonthName(lunar_.lunar_month),
                 GetLunarDayName(lunar_.lunar_day));
    } else {
        snprintf(lunar_full, sizeof(lunar_full), i18n::Tr("%s年", "%s Year"), lunar_year_name_);
    }

    // Draw centered
    int lunar_w = MeasureTextWidth(lunar_full, title_font_);
    int lunar_x = (width - lunar_w) / 2;
    DrawText(fb, width, lunar_x, y, lunar_full, title_font_, accent);
    y += title_font_->line_height + Style::kSpacingMD;

    // === Gregorian date ===
    char greg_buf[64];
    snprintf(greg_buf, sizeof(greg_buf), i18n::Tr("公历 %d年%d月%d日 %s", "%d-%d-%d %s"),
             year_, month_, day_, WeekdayFullName(weekday_));
    int greg_w = MeasureTextWidth(greg_buf, font_);
    int greg_x = (width - greg_w) / 2;
    DrawText(fb, width, greg_x, y, greg_buf, font_, secondary);
    y += font_->line_height + Style::kSpacingMD;

    // === Solar term (if today) ===
    if (solar_term_) {
        char st_buf[32];
        snprintf(st_buf, sizeof(st_buf), i18n::Tr("【%s】", "[%s]"), solar_term_);
        int st_w = MeasureTextWidth(st_buf, title_font_);
        int st_x = (width - st_w) / 2;
        DrawText(fb, width, st_x, y, st_buf, title_font_, accent);
        y += title_font_->line_height + Style::kSpacingMD;
    }

    // === Divider ===
    DrawHLine(fb, width, y, Style::kSpacingLG, width - Style::kSpacingLG, border);
    y += Style::kSpacingSM;

    // === 宜 (auspicious) section ===
    // Entries are drawn with a running x-cursor (measured per-entry) instead
    // of a fixed column pitch, wrapping to a new row when an entry would run
    // past the right margin — the English translations can be far wider than
    // any fixed pitch would allow.
    const int right_limit = width - Style::kSpacingLG;
    const char* yi_label = i18n::Tr("宜", "Do");
    DrawText(fb, width, Style::kSpacingLG, y, yi_label, title_font_, accent);
    int yi_label_w = MeasureTextWidth(yi_label, title_font_);
    int yi_start = Style::kSpacingLG + yi_label_w + Style::kSpacingSM;
    int yi_rows = 1;
    {
        int x = yi_start;
        int cur_y = y;
        for (int i = 0; i < 4; i++) {
            char buf[24];
            snprintf(buf, sizeof(buf), "%s", GetYiEntry(yi_idx_, i));
            int w = MeasureTextWidth(buf, font_);
            if (x > yi_start && x + w > right_limit) {
                x = yi_start;
                cur_y += font_->line_height + Style::kSpacingXS;
                yi_rows++;
            }
            DrawText(fb, width, x, cur_y, buf, font_, text);
            x += w + Style::kSpacingSM;
        }
    }
    y += yi_rows * font_->line_height + (yi_rows - 1) * Style::kSpacingXS + Style::kSpacingMD;

    // === 忌 (inauspicious) section ===
    const char* ji_label = i18n::Tr("忌", "Avoid");
    DrawText(fb, width, Style::kSpacingLG, y, ji_label, title_font_, danger);
    int ji_label_w = MeasureTextWidth(ji_label, title_font_);
    int ji_start = Style::kSpacingLG + ji_label_w + Style::kSpacingSM;
    {
        int x = ji_start;
        int cur_y = y;
        for (int i = 0; i < 3; i++) {
            char buf[24];
            snprintf(buf, sizeof(buf), "%s", GetJiEntry(ji_idx_, i));
            int w = MeasureTextWidth(buf, font_);
            if (x > ji_start && x + w > right_limit) {
                x = ji_start;
                cur_y += font_->line_height + Style::kSpacingXS;
            }
            DrawText(fb, width, x, cur_y, buf, font_, text);
            x += w + Style::kSpacingSM;
        }
    }

    needs_full_refresh_ = false;
}

void AlmanacRenderer::DrawTitleBar(uint8_t* fb, int width) {
    const auto& theme = ThemeManager::Get();
    const PaintStyle bar_style = theme.Style(ThemeToken::BackgroundSecondary);
    const Color text = theme.ColorFor(ThemeToken::TextPrimary);
    const Color border = theme.ColorFor(ThemeToken::Border);
    const int title_y_start = Style::kStatusBarHeight;
    const int title_bar_h = kTitleBarH;

    // Background
    DrawStyledRect(fb, width, {0, title_y_start, width, title_bar_h}, bar_style);

    // Top divider (2px)
    DrawHLine(fb, width, title_y_start, 0, width, border);
    DrawHLine(fb, width, title_y_start + 1, 0, width, border);

    // Bottom divider (2px)
    const int line_y = title_y_start + title_bar_h - 2;
    DrawHLine(fb, width, line_y, 0, width, border);
    DrawHLine(fb, width, line_y + 1, 0, width, border);

    // FIX: 改用 InkCenteredTextTopYInBox，避免 line_height 居中导致中文偏上
    // 参见 wiki/projects/notellm-baseline-alignment.md
    const char* title_str = i18n::Tr("老黄历", "Almanac");
    int title_text_y = InkCenteredTextTopYInBox(font_, title_str, title_y_start, title_bar_h, 1);
    DrawText(fb, width, Style::kSpacingLG, title_text_y, title_str, font_, text);
}

const char* AlmanacRenderer::GetLunarMonthName(int month) {
    if (month < 1 || month > 12) return "";
    return i18n::GetLanguage() == i18n::Language::kEnUS ? kLunarMonthsEn[month - 1] : kLunarMonthsZh[month - 1];
}

const char* AlmanacRenderer::GetLunarDayName(int day) {
    if (day < 1 || day > 30) return "";
    return i18n::GetLanguage() == i18n::Language::kEnUS ? kLunarDaysEn[day - 1] : kLunarDaysZh[day - 1];
}

bool AlmanacRenderer::HandleInput(const ButtonEvent& event) {
    switch (event.type) {
        case ButtonEvent::kUpClick:
        case ButtonEvent::kDownClick:
            // Navigate months (UP=prev, DOWN=next)
            if (event.type == ButtonEvent::kUpClick) {
                month_--;
                if (month_ < 1) { month_ = 12; year_--; }
            } else {
                month_++;
                if (month_ > 12) { month_ = 1; year_++; }
            }
            lunar_ = Calendar::ToLunarDate(year_, month_, day_);
            solar_term_ = GetSolarTerm(month_, day_);
            needs_full_refresh_ = true;
            return true;

        case ButtonEvent::kBootLongPress:
            // Jump to today
            RefreshData();
            needs_full_refresh_ = true;
            return true;

        default:
            break;
    }
    return false;
}

}  // namespace rawdraw
