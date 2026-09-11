#include "i18n.h"

#include "nvs_state.h"

namespace i18n {

namespace {
Language g_language = Language::kZhCN;
bool g_loaded = false;

void EnsureLoaded() {
    if (g_loaded) return;
    g_loaded = true;
    g_language = nvs_state::LoadUiLanguage() == 1 ? Language::kEnUS : Language::kZhCN;
}
}  // namespace

Language GetLanguage() {
    EnsureLoaded();
    return g_language;
}

void SetLanguage(Language lang) {
    g_language = lang;
    g_loaded = true;
    nvs_state::SaveUiLanguage(lang == Language::kEnUS ? 1 : 0);
}

const char* Tr(const char* zh, const char* en) {
    return GetLanguage() == Language::kEnUS ? en : zh;
}

}  // namespace i18n
