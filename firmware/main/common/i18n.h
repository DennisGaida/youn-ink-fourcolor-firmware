/**
 * @file i18n.h
 * @brief Minimal runtime language switch for the on-device UI.
 *
 * Every UI string literal is written inline as Tr("中文", "English") at its
 * call site instead of living in a separate keyed table. With ~700+ strings
 * spread across ~40 renderer files, a table keyed by a shared enum would
 * require every call site to agree on a key name; keeping the two strings
 * next to each other at the point of use is what actually stays correct
 * during a mechanical, file-by-file migration.
 */

#ifndef I18N_H
#define I18N_H

#include <cstdint>

namespace i18n {

enum class Language : uint8_t {
    kZhCN = 0,
    kEnUS = 1,
};

// Current UI language. Lazily loads the persisted NVS setting on first call.
Language GetLanguage();

// Persists the choice to NVS (key "ui_lang") and updates the in-memory value.
void SetLanguage(Language lang);

// Returns `zh` when the current language is Chinese, `en` otherwise.
const char* Tr(const char* zh, const char* en);

}  // namespace i18n

#endif  // I18N_H
