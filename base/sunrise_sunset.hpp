#pragma once

#include "std/ctime.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"

/// Get times of the sunrise and the sunset at a specified local date in a given location.
/// @param year - year, since 0, like 2015.
/// @param month - month, 1-jan...12-dec
/// @param day - day of month 1...31 (in local time at latitude, longitude)
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @returns pair where first is UTC time of sunrise and second is UTC time of sunset. sunrise <= sunset
/// @note date year/month/day is specified for the interesting point latituda/longitude
/// @note for polar day sunrise is set to year/month/day,0:0:0 and sunset is set to sunrise + 24h
/// @note for polar night sunrise and sunset both are set to year/month/day,0:0:0 - 0 sec of day
pair<time_t, time_t> SunriseSunsetForUtcDay(int year, int month, int day, double latitude, double longitude);

/// Get times of the sunrise and the sunset for today in a given location.
/// To get correct result for a distant location please specify it's time zone offset.
/// @param utcTime - utc time at given location, please note locationTimezoneOffsetInSeconds parameter.
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @param locationTimezoneOffsetInSeconds is needed for algorithm to correctly choose yesterday, today or tomorrow.
/// @returns pair where first is UTC time of sunrise and second is UTC time of sunset. sunrise <= sunset
/// @note if error occurs then pair(-1, -1) is returned.
/// @note if sunrise/sunset occur before the specified timeUtc then next sunrise and sunset are returned.
/// @note for polar day sunrise is set to year/month/day,0:0:0 and sunset is set to sunrise + 24h - 24h of sun
/// @note for polar night sunrise and sunset both are set to year/month/day,0:0:0 - 0 sec of sun
pair<time_t, time_t> SunriseSunsetForUtc(time_t utcTime, double latitude, double longitude);

enum class DayTimeType
{
  DayTime,
  NightTime,
  PolarDay,
  PolarNight
};

string DebugPrint(DayTimeType type);

/// Gets current sun position (day or night) and time for the next sunrise/sunset
/// at a specified date and location, assuming that the caller is at the same location
/// (algorithm uses day of the year retrieved from localtime() call).
/// @param timeUtc - utc time
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @returns pair where first is type of the day type and second is UTC time when
/// day time type will be changed. For polar day and polar night second is timeUtc + 24h.
/// @note if error occurs then pair(DayTime, -1) is returned.
pair<DayTimeType, time_t> GetNextUtcSunriseOrSunset(time_t timeUtc, double latitude, double longitude);
