#include "lang.h"
#include "setup_data.h"

/*
 * UI string tables. Every piece of translatable text is collected here.
 *
 * How to add a new language:
 *   1. Add a LANG_xx macro in lang.h;
 *   2. Add a matching entry to each table below;
 *   3. Set Language to that value in setup_data.cpp.
 */

// ---------------------------------------------------------------- Title

static const char* TITLE_CN = "未来天气";
static const char* TITLE_EN = "FUTURE TIME";
static const char* TITLE_ES = "TIEMPO FUTURO";

// ---------------------------------------------------------------- Weekday names
// Indices follow the C standard library tm_wday convention:
// 0 = Sunday, 1 = Monday ... 6 = Saturday

static const char* WEEKDAY_CN[] = {
    "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"
};
static const char* WEEKDAY_EN[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
static const char* WEEKDAY_ES[] = {
    "Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado"
};

// ---------------------------------------------------------------- Accessors

bool Lang_IsChinese() {
    return Language == LANG_CN;
}

const char* Lang_GetTitle() {
    if (Language == LANG_CN) return TITLE_CN;
    if (Language == LANG_ES) return TITLE_ES;
    return TITLE_EN;   // English fallback
}

const char* Lang_GetWeekday(int wday) {
    if (wday < 0 || wday > 6) wday = 0;
    if (Language == LANG_CN) return WEEKDAY_CN[wday];
    if (Language == LANG_ES) return WEEKDAY_ES[wday];
    return WEEKDAY_EN[wday];   // English fallback
}
