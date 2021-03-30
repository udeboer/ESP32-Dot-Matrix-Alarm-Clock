/* Patched and copied-over version of newlib localtime for 64 bit time_t support for unix 32bit
 * time overflow in 2038. This provides patched version of tszet() and localtime(). Tested on
 * ESP32. Newlib version 3.3.0 still had the bug.
 *
 * This library has the function names and variable names changed from the original
 * to prevent clashes. And is tested with multiple dates and times and years > 2038.
 *
 * Library can be used when toolchain has the error and is not easily patched. Several functions
 * that are seperate newlib files have been copied to this file. Malloc_r changed in malloc.
 *
 * Only usefull with toolchain with 64bit time-t enabled. For idf 4.2 this means compiling your
 * own toolchain. Needs 64bit time-t enabled in idf.py menuconfig. It will work with 32bit time-t.
 *
 * Modified by Udo de Boer
 */

// License is the newlib license as provide in the file LICENSE-newlib

/* localtime_r.c
 * Original Author: Adapted from tzcode maintained by Arthur David Olson.
 * Modifications:
 * - Changed to mktm_r and added _patch_tzcalc_limits - 04/10/02, Jeff Johnston
 * - Fixed bug in mday computations - 08/12/04, Alex Mogilnikov <alx@intellectronika.ru>
 * - Fixed bug in _patch_tzcalc_limits - 08/12/04, Alex Mogilnikov <alx@intellectronika.ru>
 * - Implement localtime_r() with gmtime_r() and the conditional code moved
 *   from _mktm_r() - 05/09/14, Freddie Chopin <freddie_chopin@op.pl>
 *
 * Converts the calendar time pointed to by tim_p into a broken-down time
 * expressed as local time. Returns a pointer to a structure containing the
 * broken-down time.
 */

#include "64bitpatch_localtime.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define sscanf siscanf /* avoid to pull in FP functions. */

#define SECSPERMIN 60L
#define MINSPERHOUR 60L
#define HOURSPERDAY 24L
#define SECSPERHOUR (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY (SECSPERHOUR * HOURSPERDAY)
#define DAYSPERWEEK 7
#define MONSPERYEAR 12

#define YEAR_BASE 1900
#define EPOCH_YEAR 1970
#define EPOCH_WDAY 4
#define EPOCH_YEARS_SINCE_LEAP 2
#define EPOCH_YEARS_SINCE_CENTURY 70
#define EPOCH_YEARS_SINCE_LEAP_CENTURY 370

#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

typedef struct _patch_tzrule_struct {
  char ch;
  int m;
  int n;
  int d;
  int s;
  time_t change;
  long offset; /* Match type of _timezone. */
} _patch_tzrule_type;

typedef struct _patch_tzinfo_struct {
  int _patch_tznorth;
  int _patch_tzyear;
  _patch_tzrule_type _patch_tzrule[2];
} _patch_tzinfo_type;

const int _patch_month_lengths[2][MONSPERYEAR] = {{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
                                                  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

/* Shared timezone information for libc/time functions.  */
static _patch_tzinfo_type tzinfo = {
    1, 0, {{'J', 0, 0, 0, 0, (time_t)0, 0L}, {'J', 0, 0, 0, 0, (time_t)0, 0L}}};

_patch_tzinfo_type *_patch_gettzinfo(void) { return &tzinfo; }

int _internaldaylight = 0;

static char _patch_tzname_std[11];
static char _patch_tzname_dst[11];
static char *prev_tzenv = NULL;

// function copied over from individual file in newlib
int _patch_tzcalc_limits(int year) {
  int days, year_days, years;
  int i, j;
  _patch_tzinfo_type *const tz = _patch_gettzinfo();

  if (year < EPOCH_YEAR) return 0;

  tz->_patch_tzyear = year;

  years = (year - EPOCH_YEAR);

  year_days = years * 365 + (years - 1 + EPOCH_YEARS_SINCE_LEAP) / 4 -
              (years - 1 + EPOCH_YEARS_SINCE_CENTURY) / 100 +
              (years - 1 + EPOCH_YEARS_SINCE_LEAP_CENTURY) / 400;

  for (i = 0; i < 2; ++i) {
    if (tz->_patch_tzrule[i].ch == 'J') {
      /* The Julian day n (1 <= n <= 365). */
      days = year_days + tz->_patch_tzrule[i].d + (isleap(year) && tz->_patch_tzrule[i].d >= 60);
      /* Convert to yday */
      --days;
    } else if (tz->_patch_tzrule[i].ch == 'D')
      days = year_days + tz->_patch_tzrule[i].d;
    else {
      const int yleap = isleap(year);
      int m_day, m_wday, wday_diff;
      const int *const ip = _patch_month_lengths[yleap];

      days = year_days;

      for (j = 1; j < tz->_patch_tzrule[i].m; ++j) days += ip[j - 1];

      m_wday = (EPOCH_WDAY + days) % DAYSPERWEEK;

      wday_diff = tz->_patch_tzrule[i].d - m_wday;
      if (wday_diff < 0) wday_diff += DAYSPERWEEK;
      m_day = (tz->_patch_tzrule[i].n - 1) * DAYSPERWEEK + wday_diff;

      while (m_day >= ip[j - 1]) m_day -= DAYSPERWEEK;

      days += m_day;
    }

    /* store the change-over time in GMT form by adding offset */
    tz->_patch_tzrule[i].change =
        // This has been changed to support 64bit time_t
        // days * SECSPERDAY + tz->_patch_tzrule[i].s + tz->_patch_tzrule[i].offset;
        (time_t)days * SECSPERDAY + tz->_patch_tzrule[i].s + tz->_patch_tzrule[i].offset;
  }

  tz->_patch_tznorth = (tz->_patch_tzrule[0].change < tz->_patch_tzrule[1].change);
  return 1;
}

// function copied over from individual file in newlib
void tzset_patch(void) {
  char *tzenv;
  unsigned short hh, mm, ss, m, w, d;
  int sign, n;
  int i, ch;
  _patch_tzinfo_type *tz = _patch_gettzinfo();

  if ((tzenv = getenv("TZ")) == NULL) {
    _timezone = 0;
    _internaldaylight = 0;
    _tzname[0] = "GMT";
    _tzname[1] = "GMT";
    free(prev_tzenv);
    prev_tzenv = NULL;
    return;
  }

  if (prev_tzenv != NULL && strcmp(tzenv, prev_tzenv) == 0) return;

  free(prev_tzenv);
  // prev_tzenv = _malloc_r(reent_ptr, strlen(tzenv) + 1);
  prev_tzenv = malloc(strlen(tzenv) + 1);
  if (prev_tzenv != NULL) strcpy(prev_tzenv, tzenv);

  /* ignore implementation-specific format specifier */
  if (*tzenv == ':') ++tzenv;

  if (sscanf(tzenv, "%10[^0-9,+-]%n", _patch_tzname_std, &n) <= 0) return;

  tzenv += n;

  sign = 1;
  if (*tzenv == '-') {
    sign = -1;
    ++tzenv;
  } else if (*tzenv == '+')
    ++tzenv;

  mm = 0;
  ss = 0;

  if (sscanf(tzenv, "%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n) < 1) return;

  tz->_patch_tzrule[0].offset = sign * (ss + SECSPERMIN * mm + SECSPERHOUR * hh);
  _tzname[0] = _patch_tzname_std;
  tzenv += n;

  if (sscanf(tzenv, "%10[^0-9,+-]%n", _patch_tzname_dst, &n) <= 0) { /* No dst */
    _tzname[1] = _tzname[0];
    _timezone = tz->_patch_tzrule[0].offset;
    _internaldaylight = 0;
    return;
  } else
    _tzname[1] = _patch_tzname_dst;

  tzenv += n;

  /* otherwise we have a dst name, look for the offset */
  sign = 1;
  if (*tzenv == '-') {
    sign = -1;
    ++tzenv;
  } else if (*tzenv == '+')
    ++tzenv;

  hh = 0;
  mm = 0;
  ss = 0;

  n = 0;
  if (sscanf(tzenv, "%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n) <= 0)
    tz->_patch_tzrule[1].offset = tz->_patch_tzrule[0].offset - 3600;
  else
    tz->_patch_tzrule[1].offset = sign * (ss + SECSPERMIN * mm + SECSPERHOUR * hh);

  tzenv += n;

  for (i = 0; i < 2; ++i) {
    if (*tzenv == ',') ++tzenv;

    if (*tzenv == 'M') {
      if (sscanf(tzenv, "M%hu%n.%hu%n.%hu%n", &m, &n, &w, &n, &d, &n) != 3 || m < 1 || m > 12 ||
          w < 1 || w > 5 || d > 6)
        return;

      tz->_patch_tzrule[i].ch = 'M';
      tz->_patch_tzrule[i].m = m;
      tz->_patch_tzrule[i].n = w;
      tz->_patch_tzrule[i].d = d;

      tzenv += n;
    } else {
      char *end;
      if (*tzenv == 'J') {
        ch = 'J';
        ++tzenv;
      } else
        ch = 'D';

      d = strtoul(tzenv, &end, 10);

      /* if unspecified, default to US settings */
      /* From 1987-2006, US was M4.1.0,M10.5.0, but starting in 2007 is
       * M3.2.0,M11.1.0 (2nd Sunday March through 1st Sunday November)  */
      if (end == tzenv) {
        if (i == 0) {
          tz->_patch_tzrule[0].ch = 'M';
          tz->_patch_tzrule[0].m = 3;
          tz->_patch_tzrule[0].n = 2;
          tz->_patch_tzrule[0].d = 0;
        } else {
          tz->_patch_tzrule[1].ch = 'M';
          tz->_patch_tzrule[1].m = 11;
          tz->_patch_tzrule[1].n = 1;
          tz->_patch_tzrule[1].d = 0;
        }
      } else {
        tz->_patch_tzrule[i].ch = ch;
        tz->_patch_tzrule[i].d = d;
      }

      tzenv = end;
    }

    /* default time is 02:00:00 am */
    hh = 2;
    mm = 0;
    ss = 0;
    n = 0;

    if (*tzenv == '/') sscanf(tzenv, "/%hu%n:%hu%n:%hu%n", &hh, &n, &mm, &n, &ss, &n);

    tz->_patch_tzrule[i].s = ss + SECSPERMIN * mm + SECSPERHOUR * hh;

    tzenv += n;
  }

  _patch_tzcalc_limits(tz->_patch_tzyear);
  _timezone = tz->_patch_tzrule[0].offset;
  _internaldaylight = tz->_patch_tzrule[0].offset != tz->_patch_tzrule[1].offset;
}

// function copied over from individual file in newlib
struct tm *localtime_patch(const time_t *__restrict tim_p, struct tm *__restrict res) {
  long offset;
  int hours, mins, secs;
  int year;
  _patch_tzinfo_type *const tz = _patch_gettzinfo();
  const int *ip;

  res = gmtime_r(tim_p, res);

  year = res->tm_year + YEAR_BASE;
  ip = _patch_month_lengths[isleap(year)];

  if (_internaldaylight) {
    if (year == tz->_patch_tzyear || _patch_tzcalc_limits(year))
      res->tm_isdst =
          (tz->_patch_tznorth
               ? (*tim_p >= tz->_patch_tzrule[0].change && *tim_p < tz->_patch_tzrule[1].change)
               : (*tim_p >= tz->_patch_tzrule[0].change || *tim_p < tz->_patch_tzrule[1].change));
    else
      res->tm_isdst = -1;
  } else
    res->tm_isdst = 0;

  offset = (res->tm_isdst == 1 ? tz->_patch_tzrule[1].offset : tz->_patch_tzrule[0].offset);

  hours = (int)(offset / SECSPERHOUR);
  offset = offset % SECSPERHOUR;

  mins = (int)(offset / SECSPERMIN);
  secs = (int)(offset % SECSPERMIN);

  res->tm_sec -= secs;
  res->tm_min -= mins;
  res->tm_hour -= hours;

  if (res->tm_sec >= SECSPERMIN) {
    res->tm_min += 1;
    res->tm_sec -= SECSPERMIN;
  } else if (res->tm_sec < 0) {
    res->tm_min -= 1;
    res->tm_sec += SECSPERMIN;
  }
  if (res->tm_min >= MINSPERHOUR) {
    res->tm_hour += 1;
    res->tm_min -= MINSPERHOUR;
  } else if (res->tm_min < 0) {
    res->tm_hour -= 1;
    res->tm_min += MINSPERHOUR;
  }
  if (res->tm_hour >= HOURSPERDAY) {
    ++res->tm_yday;
    ++res->tm_wday;
    if (res->tm_wday > 6) res->tm_wday = 0;
    ++res->tm_mday;
    res->tm_hour -= HOURSPERDAY;
    if (res->tm_mday > ip[res->tm_mon]) {
      res->tm_mday -= ip[res->tm_mon];
      res->tm_mon += 1;
      if (res->tm_mon == 12) {
        res->tm_mon = 0;
        res->tm_year += 1;
        res->tm_yday = 0;
      }
    }
  } else if (res->tm_hour < 0) {
    res->tm_yday -= 1;
    res->tm_wday -= 1;
    if (res->tm_wday < 0) res->tm_wday = 6;
    res->tm_mday -= 1;
    res->tm_hour += 24;
    if (res->tm_mday == 0) {
      res->tm_mon -= 1;
      if (res->tm_mon < 0) {
        res->tm_mon = 11;
        res->tm_year -= 1;
        res->tm_yday = 364 + isleap(res->tm_year + YEAR_BASE);
      }
      res->tm_mday = ip[res->tm_mon];
    }
  }

  return (res);
}
