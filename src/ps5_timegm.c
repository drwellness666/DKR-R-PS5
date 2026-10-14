/* SPDX-License-Identifier: GPL-3.0-or-later
 * UTC calendar conversion for RT64/ImPlot. The public PS5 stubs do not export
 * timegm. No process-wide timezone changes or local-time assumptions.
 */
#include <errno.h>
#include <stdint.h>
#include <time.h>

__attribute__((visibility("hidden"))) time_t timegm(struct tm *value) {
    if(!value) { errno=EINVAL;return (time_t)-1; }
    int64_t year=(int64_t)value->tm_year+1900;
    int64_t month=value->tm_mon;
    int64_t years=month/12;
    if(month<0 && month%12)years--;
    year+=years;month=month-years*12+1;
    // Gregorian eras start in March, placing the leap day at the era's end.
    year-=month<=2;
    int64_t era=(year>=0?year:year-399)/400;
    int64_t year_in_era=year-era*400;
    int64_t day_in_year=(153*(month+(month>2?-3:9))+2)/5+value->tm_mday-1;
    int64_t day_in_era=year_in_era*365+year_in_era/4-year_in_era/100+day_in_year;
    int64_t days=era*146097+day_in_era-719468;
    int64_t seconds=days*86400+(int64_t)value->tm_hour*3600+(int64_t)value->tm_min*60+value->tm_sec;
    time_t result=(time_t)seconds;
    if((int64_t)result!=seconds) { errno=EOVERFLOW;return (time_t)-1; }
    struct tm normalized;
    if(!gmtime_r(&result,&normalized)) { errno=EOVERFLOW;return (time_t)-1; }
    *value=normalized;
    return result;
}
