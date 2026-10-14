/* SPDX-License-Identifier: GPL-3.0-or-later */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
time_t dkrr_timegm(struct tm *);
static void compare(struct tm value) {
    struct tm expected=value,actual=value;
    time_t a=timegm(&expected),b=dkrr_timegm(&actual);
    assert(a==b);
    assert(expected.tm_year==actual.tm_year && expected.tm_mon==actual.tm_mon);
    assert(expected.tm_mday==actual.tm_mday && expected.tm_hour==actual.tm_hour);
    assert(expected.tm_min==actual.tm_min && expected.tm_sec==actual.tm_sec);
    assert(expected.tm_yday==actual.tm_yday && expected.tm_wday==actual.tm_wday);
}
int main(void) {
    const char *zones[]={"UTC","Europe/Berlin","America/New_York"};
    unsigned count=0;
    for(unsigned z=0;z<3;z++) {
        setenv("TZ",zones[z],1);tzset();
        for(int year=1600;year<=2400;year+=4) for(int month=-2;month<=14;month+=2) {
            struct tm t={0};t.tm_year=year-1900;t.tm_mon=month;
            t.tm_mday=year%37-3;t.tm_hour=27;t.tm_min=-73;t.tm_sec=121;
            compare(t);count++;
        }
        struct tm minus_one={.tm_year=69,.tm_mon=11,.tm_mday=31,.tm_hour=23,.tm_min=59,.tm_sec=59};
        compare(minus_one);count++;
    }
    errno=0;assert(dkrr_timegm(NULL)==(time_t)-1 && errno==EINVAL);
    printf("PASS timegm: %u normalized UTC comparisons across three timezones\n",count);
}
