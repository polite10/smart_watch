#include "models/watch_model.h"
uint8_t watch_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if(year < 2000 || year > 2099 || month < 1 || month > 12) return 0;
    return days[month - 1] + (month == 2 && year % 4 == 0);
}

bool watch_datetime_valid(uint16_t year, uint8_t month, uint8_t day,
                          uint8_t hour, uint8_t minute, uint8_t second)
{
    return day >= 1 && day <= watch_days_in_month(year, month) &&
           hour < 24 && minute < 60 && second < 60;
}

uint32_t watch_day_key(uint16_t year, uint8_t month, uint8_t day)
{ return (uint32_t)year * 10000 + (uint32_t)month * 100 + day; }

uint32_t watch_epoch(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    static const uint16_t before[] = {0,31,59,90,120,151,181,212,243,273,304,334};
    uint32_t days = 0;
    for(unsigned y = 2000; y < year; ++y) days += 365 + (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
    days += before[month - 1] + day - 1;
    if(month > 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ++days;
    return days * 86400 + (uint32_t)hour * 3600 + (uint32_t)minute * 60 + second;
}
