#include "base/sunrise_sunset.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"
#include "base/timer.hpp"
#include "base/timegm.hpp"
#include "base/logging.hpp"

namespace
{

// Sun's zenith for sunrise/sunset
//   offical      = 90 degrees 50'
//   civil        = 96 degrees
//   nautical     = 102 degrees
//   astronomical = 108 degrees
double constexpr kZenith = 90 + 50. / 60.; // 90 degrees 50'

time_t constexpr kOneDaySeconds = 24 * 60 * 60;

inline double NormalizeAngle(double a)
{
  double res = fmod(a, 360.);
  if (res < 0)
    res += 360.;
  return res;
}

inline int DaysOfMonth(int year, int month)
{
  ASSERT_GREATER_OR_EQUAL(month, 1, ());
  ASSERT_LESS_OR_EQUAL(month, 12, ());
  int const february = base::IsLeapYear(year) ? 29 : 28;
  int const daysPerMonth[12] = { 31, february, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  return daysPerMonth[month - 1];
}

enum class DayEventType
{
  Sunrise,
  Sunset,
  PolarDay,
  PolarNight
};

// Main work-horse function which calculates sunrise/sunset in a specified date in a specified location.
// This function was taken from source http://williams.best.vwh.net/sunrise_sunset_algorithm.htm
// and corrected according to original article at http://babel.hathitrust.org/cgi/pt?id=uiug.30112059294311;view=1up;seq=2
// Notation is kept to have source close to source. 
DayEventType CalculateDayEventTime(int year, int month, int day,
                                   double latitude, double longitude,
                                   bool sunrise, time_t & outUtc)
{
  // 1. first calculate the day of the year

  double const N1 = floor(275. * month / 9.);
  double const N2 = floor((month + 9.) / 12.);
  double const N3 = (1. + floor((year - 4. * floor(year / 4.) + 2.) / 3.));
  double const N = N1 - (N2 * N3) + day - 30.;

  // 2. convert the longitude to hour value and calculate an approximate time

  double const lngHour = longitude / 15;

  double t = 0;
  if (sunrise)
    t = N + ((6 - lngHour) / 24);
  else
    t = N + ((18 - lngHour) / 24);

  // 3. calculate the Sun's mean anomaly

  double const M = (0.9856 * t) - 3.289;

  // 4. calculate the Sun's true longitude

  double L = M + (1.916 * sin(my::DegToRad(M))) + (0.020 * sin(2 * my::DegToRad(M))) + 282.634;
  // NOTE: L potentially needs to be adjusted into the range [0,360) by adding/subtracting 360
  L = NormalizeAngle(L);

  // 5a. calculate the Sun's right ascension

  double RA = my::RadToDeg( atan(0.91764 * tan(my::DegToRad(L))) );
  // NOTE: RA potentially needs to be adjusted into the range [0,360) by adding/subtracting 360
  RA = NormalizeAngle(RA);

  // 5b. right ascension value needs to be in the same quadrant as L

  double const Lquadrant = (floor( L / 90)) * 90;
  double const RAquadrant = (floor(RA / 90)) * 90;
  RA = RA + (Lquadrant - RAquadrant);

  // 5c. right ascension value needs to be converted into hours

  RA = RA / 15;

  // 6. calculate the Sun's declination

  double sinDec = 0.39782 * sin(my::DegToRad(L));
  double cosDec = cos(asin(sinDec));

  // 7a. calculate the Sun's local hour angle

  double cosH = (cos(my::DegToRad(kZenith)) - (sinDec * sin(my::DegToRad(latitude)))) / (cosDec * cos(my::DegToRad(latitude)));


  struct tm stm = {};
  stm.tm_year = year - 1900;
  stm.tm_mon = month - 1;
  stm.tm_mday = day;
  // if cosH > 1 then sun is never rises on this location on specified date (polar night)
  // if cosH < -1 then sun is never sets on this location on specified date (polar day)
  // TODO(AlexZ): Correctly handle edge cases - calculate real day for sunrise/sunset, even if it's far in the past/future.
  if (cosH < -1 || cosH > 1)
  {
    stm.tm_hour = sunrise ? 0 : 23;
    stm.tm_min = sunrise ? 0 : 59;
    stm.tm_sec = sunrise ? 0 : 59;
    outUtc = base::TimeGM(stm);
    return (cosH < -1) ? DayEventType::PolarDay : DayEventType::PolarNight;
  }

  // 7b. finish calculating H and convert into hours

  double H = 0;
  if (sunrise)
    H = 360 - my::RadToDeg(acos(cosH));
  else
    H = my::RadToDeg(acos(cosH));

  H = H / 15;

  // 8. calculate local mean time of rising/setting

  double T = H + RA - (0.06571 * t) - 6.622;
  if (T > 24)
    T = fmod(T, 24.);
  else if (T < 0)
    T += 24.;

  // 9. adjust back to UTC

  double UT = T - lngHour;
  // NOTE: UT potentially needs to be adjusted into the range [0,24) by adding/subtracting 24
  // >24 means event occurs on the next day, <0 means on a previous one.
  if (UT > 24)
  {
    ++stm.tm_mday;  // mktime magically handles day overflows.
    UT = fmod(UT, 24.);
  }
  else if (UT < 0)
  {
    --stm.tm_mday; // mktime magically handles day underflows.
    UT += 24.;
  }

  // UT - is a hour with fractional part of date year/month/day, in range of [0;24)
  stm.tm_hour = floor(UT); // [0;24)
  stm.tm_min = floor((UT - stm.tm_hour) * 60); // [0;60)
  stm.tm_sec = fmod(floor(UT * 60 * 60) /* number of seconds from 0:0 to UT */, 60); // [0;60)
  outUtc = base::TimeGM(stm);

  return sunrise ? DayEventType::Sunrise : DayEventType::Sunset;
}

} // namespace

pair<time_t, time_t> SunriseSunsetForUtcDay(int year, int month, int day, double latitude, double longitude)
{
  ASSERT_GREATER_OR_EQUAL(month, 1, ());
  ASSERT_LESS_OR_EQUAL(month, 12, ());
  ASSERT_GREATER_OR_EQUAL(day, 1, ());
  ASSERT_LESS_OR_EQUAL(day, DaysOfMonth(year, month), ());

  time_t sunriseUtc, sunsetUtc;
  DayEventType const sunriseRes = CalculateDayEventTime(year, month, day, latitude, longitude, true /* sunrise */, sunriseUtc);
  DayEventType const sunsetRes = CalculateDayEventTime(year, month, day, latitude, longitude, false /* sunrise */, sunsetUtc);

  ASSERT_LESS_OR_EQUAL(sunriseUtc, sunsetUtc, ("This is impossible."));

  // Edge cases: polar day and polar night
  if (sunriseRes == DayEventType::PolarDay || sunsetRes == DayEventType::PolarDay)
  {
    // Polar day: 24 hours of sun
    sunriseUtc = base::TimeGM(year, month, day, 0, 0, 0);
    sunsetUtc = sunriseUtc + kOneDaySeconds;
  }
  else if (sunriseRes == DayEventType::PolarNight || sunsetRes == DayEventType::PolarNight)
  {
    // Polar night: 0 seconds of sun
    sunriseUtc = sunsetUtc = base::TimeGM(year, month, day, 0, 0, 0);
  }
  ASSERT_LESS_OR_EQUAL(sunriseUtc, sunsetUtc, ("This is impossible."));
  return make_pair(sunriseUtc, sunsetUtc);
}

pair<time_t, time_t> SunriseSunsetForUtc(time_t utcTime, double latitude, double longitude)
{
  // Store copies not pointers for safe usage in multithreading environment.
  struct tm const stm = *gmtime(&utcTime);

  int const year = stm.tm_year + 1900;
  int const mon = stm.tm_mon + 1;
  int const day = stm.tm_mday;

  return SunriseSunsetForUtcDay(year, mon, day, latitude, longitude);
}

pair<DayTimeType, time_t> GetNextUtcSunriseOrSunset(time_t timeUtc, double latitude, double longitude)
{
  auto const sunriseSunsetUtc = SunriseSunsetForUtc(timeUtc, latitude, longitude);

  if (sunriseSunsetUtc.first == my::INVALID_TIME_STAMP ||
      sunriseSunsetUtc.second == my::INVALID_TIME_STAMP)
  {
    return make_pair(DayTimeType::DayTime, my::INVALID_TIME_STAMP);
  }

  // CalculateSunriseSunsetTime returns only 2 possible cases:
  // <time> (sunrise) (sunset) ---> time axis
  // (sunrise) <time> (sunset) ---> time axis
  // it never returns sunrise and sunset before time

  // Edge cases: polar day and polar night
  if (sunriseSunsetUtc.first == sunriseSunsetUtc.second)
  {
    // Polar night: 0 seconds of sun
    return make_pair(DayTimeType::PolarNight, timeUtc + kOneDaySeconds);
  }
  else if (sunriseSunsetUtc.second == (sunriseSunsetUtc.first + kOneDaySeconds))
  {
    // Polar day: 24 hours of sun
    return make_pair(DayTimeType::PolarDay, timeUtc + kOneDaySeconds);
  }

  if (timeUtc < sunriseSunsetUtc.first)
  {
    // 00:00 (time) (sunrise) (sunset) 24:00 ---> time axis
    return make_pair(DayTimeType::NightTime, sunriseSunsetUtc.first);
  }
  else if (timeUtc < sunriseSunsetUtc.second)
  {
    // 00:00 (sunrise) (time) (sunset) 24:00 ---> time axis
    return make_pair(DayTimeType::DayTime, sunriseSunsetUtc.second);
  }
  else
  {
    // 00:00 (sunrise) (sunset) (time) 24:00 ---> time axis
    return make_pair(DayTimeType::NightTime, SunriseSunsetForUtc(timeUtc + kOneDaySeconds, latitude, longitude).first);
  }
}

string DebugPrint(DayTimeType type)
{
  switch (type)
  {
  case DayTimeType::DayTime: return "DayTime";
  case DayTimeType::NightTime: return "NightTime";
  case DayTimeType::PolarDay: return "PolarDay";
  case DayTimeType::PolarNight: return "PolarNight";
  }
  return string();
}
