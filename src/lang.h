#ifndef LANG_H_
#define LANG_H_

#include <Arduino.h>
#include "fonts.h"

/*
 * Multilingual support.
 *
 * The active language is selected by the Language variable in setup_data.cpp,
 * using one of the LANG_xx values defined below.
 * All on-screen text lives in the string tables inside lang.cpp, so adding a
 * language or rewording an entry only touches that single file.
 *
 * NOTE: Chinese strings may only use characters that the bundled bitmap font
 * actually covers. The current font table FontCN_zh (see fontCN_zh.c) contains
 * just the 13 glyphs used by the UI:
 *     未来天气星期一二三四五六日
 * To display any other Han character, add it to CHARS in tools/gen_fontcn.py
 * and regenerate the table, otherwise the glyph will render with missing
 * strokes or will not appear at all.
 */

#define LANG_CN "cn"   // Chinese
#define LANG_EN "en"   // English
#define LANG_ES "es"   // Spanish

// Returns true when the currently selected language is Chinese
bool Lang_IsChinese();

// Title text: "未来天气" / "FUTURE TIME" / "TIEMPO FUTURO"
const char* Lang_GetTitle();

// Weekday names. wday follows the C standard library tm_wday convention:
// 0 = Sunday, 1 = Monday ... 6 = Saturday
const char* Lang_GetWeekday(int wday);

#endif /* LANG_H_ */
