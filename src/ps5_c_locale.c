/*
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Backport of PS5_PayloadSDK 6b63a2a platform/src/xlocale.c's C locale.
 * https://github.com/mihawk-99/PS5_PayloadSDK/blob/6b63a2a/platform/src/xlocale.c
 * The pinned SDK predates this function. The system library can report an
 * empty decimal point in a native title; nlohmann then truncates fractions.
 * This title supports only the C locale, matching the SDK's locale contract.
 */
#include <limits.h>
#include <locale.h>

struct lconv *dkrr_ps5_localeconv(void) {
    static char empty[] = "";
    static char point[] = ".";
    static struct lconv conventions = {
        .decimal_point=point, .thousands_sep=empty, .grouping=empty,
        .int_curr_symbol=empty, .currency_symbol=empty,
        .mon_decimal_point=empty, .mon_thousands_sep=empty, .mon_grouping=empty,
        .positive_sign=empty, .negative_sign=empty,
        .int_frac_digits=CHAR_MAX, .frac_digits=CHAR_MAX,
        .p_cs_precedes=CHAR_MAX, .p_sep_by_space=CHAR_MAX,
        .n_cs_precedes=CHAR_MAX, .n_sep_by_space=CHAR_MAX,
        .p_sign_posn=CHAR_MAX, .n_sign_posn=CHAR_MAX,
        .int_p_cs_precedes=CHAR_MAX, .int_n_cs_precedes=CHAR_MAX,
        .int_p_sep_by_space=CHAR_MAX, .int_n_sep_by_space=CHAR_MAX,
        .int_p_sign_posn=CHAR_MAX, .int_n_sign_posn=CHAR_MAX,
    };
    return &conventions;
}
