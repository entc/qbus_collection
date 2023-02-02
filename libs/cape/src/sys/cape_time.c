#include "cape_time.h"

#include "sys/cape_types.h"
#include "fmt/cape_tokenizer.h"
#include "sys/cape_log.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

#define _BSD_SOURCE
#define cape_sscanf sscanf

#include <sys/time.h>

#elif defined __WINDOWS_OS

#include <windows.h>
#define cape_sscanf sscanf_s

#define timeradd(s,t,a) (void) ( (a)->tv_sec = (s)->tv_sec + (t)->tv_sec, \
	((a)->tv_usec = (s)->tv_usec + (t)->tv_usec) >= 1000000 && \
	((a)->tv_usec -= 1000000, (a)->tv_sec++) )

#define timersub(s,t,a) (void) ( (a)->tv_sec = (s)->tv_sec - (t)->tv_sec, \
	((a)->tv_usec = (s)->tv_usec - (t)->tv_usec) < 0 && \
	((a)->tv_usec += 1000000, (a)->tv_sec--) )

#endif

//-----------------------------------------------------------------------------

CapeDatetime* cape_datetime_new (void)
{
  CapeDatetime* self = CAPE_NEW(CapeDatetime);
  
  memset (self, 0x0, sizeof(CapeDatetime));
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_datetime_del (CapeDatetime** p_self)
{
  if (*p_self)
  {
    CAPE_DEL (p_self, CapeDatetime);
  }
}

//-----------------------------------------------------------------------------

CapeDatetime* cape_datetime_cp (const CapeDatetime* self)
{
  CapeDatetime* ret = NULL;
  
  if (self)
  {
    ret = CAPE_NEW (CapeDatetime);
    
    memcpy (ret, self, sizeof(CapeDatetime));
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeDatetime* cape_datetime_mv (CapeDatetime** p_self)
{
  CapeDatetime* ret = NULL;
  
  if (p_self)
  {
    ret = *p_self;
    *p_self = NULL;
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void cape_datetime_set (CapeDatetime* self, const CapeDatetime* rhs)
{
  memcpy (self, rhs, sizeof(CapeDatetime));
}

//-----------------------------------------------------------------------------

void cape_datetime__convert_timeinfo (CapeDatetime* dt, const struct tm* timeinfo)
{
  // fill the timeinfo
  dt->sec = timeinfo->tm_sec;
  dt->msec = 0;
  dt->usec = 0;
  
  dt->minute = timeinfo->tm_min;
  dt->hour = timeinfo->tm_hour;
  
  dt->is_dst = timeinfo->tm_isdst;
  
  dt->day = timeinfo->tm_mday;
  dt->month = timeinfo->tm_mon + 1;
  dt->year = timeinfo->tm_year + 1900; 
}

//-----------------------------------------------------------------------------

void cape_datetime__convert_cape (struct tm* timeinfo, const CapeDatetime* dt)
{
  // fill the timeinfo
  timeinfo->tm_sec   = dt->sec;
  timeinfo->tm_hour  = dt->hour;
  timeinfo->tm_min   = dt->minute;
  
  timeinfo->tm_mday  = dt->day;
  timeinfo->tm_mon   = dt->month - 1;
  timeinfo->tm_year  = dt->year - 1900;  
  
  timeinfo->tm_isdst = dt->is_dst;
}

//-----------------------------------------------------------------------------

void cape_datetime_utc__s (CapeDatetime* dt, time_t unix_time_since_1970)
{
#if defined __WINDOWS_OS

#else

  struct tm* l01;
  time_t h = unix_time_since_1970;

  l01 = gmtime (&h);
  
  cape_datetime__convert_timeinfo (dt, l01);
  
  dt->msec = 0;
  dt->usec = 0;
  
  dt->is_utc = TRUE;
  
#endif
}

//-----------------------------------------------------------------------------

void cape_datetime_utc__ms (CapeDatetime* dt, time_t unix_time_since_1970)
{
#if defined __WINDOWS_OS
  
#else
  
  struct tm* l01;
  time_t h;
  double d = (double)unix_time_since_1970 / 1000;
  
  // fast floor
  h = (time_t)d; if (h > d) h--;
  
  l01 = gmtime (&h);
  
  cape_datetime__convert_timeinfo (dt, l01);
  
  dt->msec = unix_time_since_1970 - h * 1000;
  dt->usec = 0;
  
  dt->is_utc = TRUE;
  
#endif
}

//-----------------------------------------------------------------------------

void cape_datetime_utc (CapeDatetime* dt)
{
#if defined __WINDOWS_OS

  SYSTEMTIME time;
  GetSystemTime(&time);
  
  dt->sec = time.wSecond;
  dt->msec = time.wMilliseconds;
  dt->usec = 0;
  
  dt->minute = time.wMinute;
  dt->hour = time.wHour;
  
  dt->day = time.wDay;
  dt->month = time.wMonth;
  dt->year = time.wYear;
  
  dt->is_dst = FALSE;
  dt->is_utc = TRUE;

#else

  struct timeval time;  
  struct tm* l01;
  
  gettimeofday (&time, NULL);
  l01 = gmtime (&(time.tv_sec));
  
  cape_datetime__convert_timeinfo (dt, l01); 
  
  dt->msec = time.tv_usec / 1000;
  dt->usec = time.tv_usec;
  
  dt->is_utc = TRUE;

#endif
}

//-----------------------------------------------------------------------------

void cape_datetime_local (CapeDatetime* dt)
{
#if defined __WINDOWS_OS

  SYSTEMTIME time;
  GetLocalTime(&time);
  
  dt->sec = time.wSecond;
  dt->msec = time.wMilliseconds;
  dt->usec = 0;
  
  dt->minute = time.wMinute;
  dt->hour = time.wHour;
  
  dt->day = time.wDay;
  dt->month = time.wMonth;
  dt->year = time.wYear; 
  
  dt->is_dst = FALSE;
  dt->is_utc = FALSE;

#else

  struct timeval time;  
  struct tm* l01;
  
  gettimeofday (&time, NULL);
  l01 = localtime (&(time.tv_sec));
  
  cape_datetime__convert_timeinfo (dt, l01);
  
  dt->msec = time.tv_usec / 1000;
  dt->usec = time.tv_usec;

  dt->is_utc = FALSE;

#endif
}

//-----------------------------------------------------------------------------

void cape_datetime_to_local (CapeDatetime* dt)
{
#if defined __WINDOWS_OS


#else

  if (dt->is_utc && dt->month)
  {
    struct tm timeinfo;
    struct tm* l01;
    time_t t_of_day;
    unsigned int msec;
    
    cape_datetime__convert_cape (&timeinfo, dt);
    
    // turn on automated DST, depends on the timezone
    timeinfo.tm_isdst = -1;
    
    // the function takes a local time and calculate extra values
    // in our case it is UTC, so we will get the (localtime) UTC value
    // because the function also applies timezone conversion
    t_of_day = mktime (&timeinfo);
    
    // correct (localtime) UTC to (current) UTC :-)
    t_of_day = t_of_day + timeinfo.tm_gmtoff;
    
    // now get the localtime from our UTC
    l01 = localtime (&t_of_day);
    
    // save the msec
    msec = dt->msec;
    
    cape_datetime__convert_timeinfo (dt, l01);
    
    dt->msec = msec;
    
    dt->is_utc = FALSE;
  }

#endif
}

//-----------------------------------------------------------------------------

void cape_datetime__convert_int_cape (CapeDatetime* self, struct timeval* time_timeval, struct tm* time_tm)
{
  // convert to cape time representation
  cape_datetime__convert_timeinfo (self, time_tm);

  // check if the result was within defined boundaries
  // c functions somtimes don't return usec within 0 - 999999 as defined
  if (time_timeval->tv_usec < 999999)
  {
    // calculate milliseconds and microseconds
    self->msec = time_timeval->tv_usec / 1000;
    self->usec = time_timeval->tv_usec;
  }
  else
  {
    cape_log_fmt (CAPE_LL_WARN, "CAPE", "datetime class", "faulty USEC value in time_timeval.tv_usec (out of range): %li", time_timeval->tv_usec);

    self->msec = 0;
    self->usec = 0;
  }
  
  self->is_utc = TRUE;
}

//-----------------------------------------------------------------------------

void cape_datetime__intern_sub_delta (struct timeval* time_timeval, const CapeString delta)
{
  CapeList tokens = cape_tokenizer_buf (delta, cape_str_size (delta), ':');
  
  {
    CapeListCursor* cursor = cape_list_cursor_create (tokens, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      const CapeString delta_part = cape_list_node_data (cursor->node);
      
      switch (delta_part[0])
      {
        case 'D':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1) * 3600 * 24;
          h.tv_usec = 0;

          timersub (time_timeval, &h, time_timeval);
          break;
        }
        case 'h':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1) * 3600;
          h.tv_usec = 0;

          timersub (time_timeval, &h, time_timeval);
          break;
        }
        case 'm':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1) * 60;
          h.tv_usec = 0;

          timersub (time_timeval, &h, time_timeval);
          break;
        }
        case 's':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1);
          h.tv_usec = 0;
          
          timersub (time_timeval, &h, time_timeval);
          break;
        }
        case 'u':
        {
          struct timeval h;
          
          h.tv_sec = 0;
          h.tv_usec = atol (delta_part + 1) * 1000;

          timersub (time_timeval, &h, time_timeval);
          break;
        }
      }
    }
    
    cape_list_cursor_destroy (&cursor);
  }

  cape_list_del (&tokens);
}

//-----------------------------------------------------------------------------

void cape_datetime__intern_add_delta (struct timeval* time_timeval, const CapeString delta)
{
  CapeList tokens = cape_tokenizer_buf (delta, cape_str_size (delta), ':');
  
  {
    CapeListCursor* cursor = cape_list_cursor_create (tokens, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      const CapeString delta_part = cape_list_node_data (cursor->node);
      
      switch (delta_part[0])
      {
        case 'D':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1) * 3600 * 24;
          h.tv_usec = 0;

          timeradd (time_timeval, &h, time_timeval);
          break;
        }
        case 'h':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1) * 3600;
          h.tv_usec = 0;

          timeradd (time_timeval, &h, time_timeval);
          break;
        }
        case 'm':
        {
          struct timeval h;

          h.tv_sec = atol (delta_part + 1) * 60;
          h.tv_usec = 0;

          timeradd (time_timeval, &h, time_timeval);
          break;
        }
        case 's':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1);
          h.tv_usec = 0;
          
          timeradd (time_timeval, &h, time_timeval);
          break;
        }
        case 'u':
        {
          struct timeval h;
          
          h.tv_sec = 0;
          h.tv_usec = atol (delta_part + 1) * 1000;

          timeradd (time_timeval, &h, time_timeval);
          break;
        }
      }
    }
    
    cape_list_cursor_destroy (&cursor);
  }

  cape_list_del (&tokens);
}

//-----------------------------------------------------------------------------

void cape_datetime_add_s (CapeDatetime* self, const CapeString delta)
{
#if defined __WINDOWS_OS
  
  
#else
  
  struct timeval time_timeval;
  struct tm* time_tm;
  
  // convert cape datetime into c time struct
  time_timeval.tv_sec = cape_datetime_n__unix (self);
  time_timeval.tv_usec = (self->msec * 1000) + self->usec;
  
  // substract delta from time_timeval
  cape_datetime__intern_add_delta (&time_timeval, delta);
  
  // convert into time struct
  time_tm = gmtime (&(time_timeval.tv_sec));

  // convert into result
  cape_datetime__convert_int_cape (self, &time_timeval, time_tm);
  
#endif
}

//-----------------------------------------------------------------------------

void cape_datetime_sub_s (CapeDatetime* self, const CapeString delta)
{
#if defined __WINDOWS_OS
  
  
#else
  
  struct timeval time_timeval;
  struct tm* time_tm;
  
  // convert cape datetime into c time struct
  time_timeval.tv_sec = cape_datetime_n__unix (self);
  time_timeval.tv_usec = (self->msec * 1000) + self->usec;
  
  // substract delta from time_timeval
  cape_datetime__intern_sub_delta (&time_timeval, delta);
  
  // convert into time struct
  time_tm = gmtime (&(time_timeval.tv_sec));

  // convert into result
  cape_datetime__convert_int_cape (self, &time_timeval, time_tm);
  
#endif
}

//-----------------------------------------------------------------------------

void cape_datetime_utc__sub_s (CapeDatetime* self, const CapeString delta)
{
#if defined __WINDOWS_OS
  
  
#else

  struct timeval time_timeval;
  struct tm* time_tm;

  // get current datetime
  gettimeofday (&time_timeval, NULL);

  // substract delta from time_timeval
  cape_datetime__intern_sub_delta (&time_timeval, delta);

  // convert into time struct
  time_tm = gmtime (&(time_timeval.tv_sec));

  // convert into result
  cape_datetime__convert_int_cape (self, &time_timeval, time_tm);

#endif
}

//-----------------------------------------------------------------------------

void cape_datetime_utc__add_s (CapeDatetime* dt, const CapeString delta)
{
#if defined __WINDOWS_OS
  
  
#else

  struct timeval time_timeval;
  struct tm* time_tm;

  // get current datetime
  gettimeofday (&time_timeval, NULL);

  // accumulate delta to time_timeval
  cape_datetime__intern_add_delta (&time_timeval, delta);
  
  // convert into time struct
  time_tm = gmtime (&(time_timeval.tv_sec));

  // convert into result
  cape_datetime__convert_int_cape (dt, &time_timeval, time_tm);

#endif
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// reduce input datetine from a certain time period defined in delta string format ( D2:h2:m2:s2:u2 ) and save it in result datetime 
// Function doesn't change the time zone, so take care to use same time zone for input and result (e.g. both UTC)
void cape_datetime__remove_s (const CapeDatetime* self, const CapeString delta, CapeDatetime* result)
{
#if defined __WINDOWS_OS
  
    
#else

  struct timeval time_timeval;
  struct tm* time_tm;
  
  // convert cape datetime into c time struct
  time_timeval.tv_sec = cape_datetime_n__unix (self);
  time_timeval.tv_usec = (self->msec * 1000) + self->usec;

  // substract delta from time_timeval
  cape_datetime__intern_sub_delta (&time_timeval, delta);

  // convert into time struct
  time_tm = gmtime (&(time_timeval.tv_sec));

  // convert into result
  cape_datetime__convert_int_cape (result, &time_timeval, time_tm);

#endif
}

//-----------------------------------------------------------------------------
// append a certain time period defined in delta string format ( D2:h2:m2:s2:u2 ) and save it in result datetime
// Function doesn't change the time zone, so take care to use same time zone for input and result (e.g. both UTC)
void cape_datetime__add_s (const CapeDatetime* input, const CapeString delta, CapeDatetime* result)
{
#if defined __WINDOWS_OS
  
    
#else

  struct timeval time_timeval;
  struct tm* time_tm;
  
  // convert cape datetime into c time struct
  time_timeval.tv_sec = cape_datetime_n__unix (input);
  time_timeval.tv_usec = (input->msec * 1000) + input->usec;

  // accumulate delta to time_timeval
  cape_datetime__intern_add_delta (&time_timeval, delta);

  // convert into time struct
  time_tm = gmtime (&(time_timeval.tv_sec));

  // convert into result
  cape_datetime__convert_int_cape (result, &time_timeval, time_tm);

#endif
}

//-----------------------------------------------------------------------------

void cape_datetime__sub_s (const CapeDatetime* input, const CapeString delta, CapeDatetime* result)
{
#if defined __WINDOWS_OS
  
  
#else
  
  struct timeval time_timeval;
  struct tm* time_tm;
  
  // convert cape datetime into c time struct
  time_timeval.tv_sec = cape_datetime_n__unix (input);
  time_timeval.tv_usec = (input->msec * 1000) + input->usec;
  
  // accumulate delta to time_timeval
  cape_datetime__intern_sub_delta (&time_timeval, delta);
  
  // convert into time struct
  time_tm = gmtime (&(time_timeval.tv_sec));

  // convert into result
  cape_datetime__convert_int_cape (result, &time_timeval, time_tm);
  
#endif
}

//-----------------------------------------------------------------------------

void cape_datetime__add_n (const CapeDatetime* self, number_t period, CapeDatetime* result)
{
#if defined __WINDOWS_OS
  
  
#else
  
  struct timeval time_timeval;
  struct tm* time_tm;
  
  // convert cape datetime into c time struct
  time_timeval.tv_sec = cape_datetime_n__unix (self);
  time_timeval.tv_usec = (self->msec * 1000) + self->usec;

  // append the period
  time_timeval.tv_sec += period;
  
  // convert into time struct
  time_tm = gmtime (&(time_timeval.tv_sec));
  
  // convert into result
  cape_datetime__convert_int_cape (result, &time_timeval, time_tm);

#endif
}

//-----------------------------------------------------------------------------

void cape_datetime__sub_n (const CapeDatetime* self, number_t period, CapeDatetime* result)
{
#if defined __WINDOWS_OS
  
  
#else
  
  struct timeval time_timeval;
  struct tm* time_tm;
  
  // convert cape datetime into c time struct
  time_timeval.tv_sec = cape_datetime_n__unix (self);
  time_timeval.tv_usec = (self->msec * 1000) + self->usec;
  
  // append the period
  time_timeval.tv_sec -= period;
  
  // convert into time struct
  time_tm = gmtime (&(time_timeval.tv_sec));
  
  // convert into result
  cape_datetime__convert_int_cape (result, &time_timeval, time_tm);
  
#endif
}

//-----------------------------------------------------------------------------

int cape_datetime_cmp (const CapeDatetime* dt1, const CapeDatetime* dt2)
{
  if (dt1 == NULL)
  {
    // null is always the smallest
    return -1;
  }

  if (dt2 == NULL)
  {
    // null is always the smallest
    return 1;
  }
  
  if (dt1->year < dt2->year)
  {
    return -1;
  }
  
  if (dt1->year > dt2->year)
  {
    return 1;
  }

  if (dt1->month < dt2->month)
  {
    return -1;
  }

  if (dt1->month > dt2->month)
  {
    return 1;
  }

  if (dt1->day < dt2->day)
  {
    return -1;
  }
  
  if (dt1->day > dt2->day)
  {
    return 1;
  }

  if (dt1->hour < dt2->hour)
  {
    return -1;
  }
  
  if (dt1->hour > dt2->hour)
  {
    return 1;
  }

  if (dt1->minute < dt2->minute)
  {
    return -1;
  }
  
  if (dt1->minute > dt2->minute)
  {
    return 1;
  }
  
  if (dt1->sec < dt2->sec)
  {
    return -1;
  }
  
  if (dt1->sec > dt2->sec)
  {
    return 1;
  }

  if (dt1->msec < dt2->msec)
  {
    return -1;
  }
  
  if (dt1->msec > dt2->msec)
  {
    return 1;
  }
  
  if (dt1->usec < dt2->usec)
  {
    return -1;
  }
  
  if (dt1->usec > dt2->usec)
  {
    return 1;
  }

  return 0;
}

//-----------------------------------------------------------------------------

void cape_datetime_cross__set (CapeDatetime* self)
{
  self->month = 0;
}

//-----------------------------------------------------------------------------

int cape_datetime_cross__is (const CapeDatetime* self)
{
  return (self->day > 0) && (self->month == 0);
}

//-----------------------------------------------------------------------------

void cape_datetime__internal__cross_out_format (CapeString* p_format)
{
  // replace complex placeholders
  cape_str_replace (p_format, "%X", "%H:%M:%S");
  cape_str_replace (p_format, "%x", "%m/%d/%y");
  
  // replace date placeholders
  cape_str_replace (p_format, "%Y", "XXXX");
  cape_str_replace (p_format, "%y", "XX");
  cape_str_replace (p_format, "%m", "XX");

  // replace time placeholders
  cape_str_replace (p_format, "%H", "XX");
  cape_str_replace (p_format, "%M", "XX");
}

//-----------------------------------------------------------------------------

CapeString cape_datetime__internal__fmt_lcl (const CapeDatetime* self, const CapeString format)
{
  struct tm timeinfo;
  CapeString ret = (CapeString)CAPE_ALLOC (100);
  
  cape_datetime__convert_cape (&timeinfo, self);
  
  // convert into local time
  // using local timezone settings
  mktime (&timeinfo);
  
  // create buffer with timeinfo as string
  strftime (ret, 99, format, &timeinfo);
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__fmt_lcl (const CapeDatetime* self, const CapeString format)
{
  CapeString ret = NULL;
  
  if (self)
  {
    if (cape_datetime_cross__is (self))  // special case to cross out the date
    {
      // copy of self and format
      CapeDatetime* copy_self = cape_datetime_cp (self);
      CapeString copy_format = cape_str_cp (format);
      
      // correct day and month to the first
      copy_self->month = 1;
      
      // cross out content in the format
      cape_datetime__internal__cross_out_format (&copy_format);
      
      ret = cape_datetime__internal__fmt_lcl (copy_self, copy_format);
      
      cape_str_del (&copy_format);
      cape_datetime_del (&copy_self);
    }
    else
    {
      ret = cape_datetime__internal__fmt_lcl (self, format);
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_datetime__internal__fmt_utc (const CapeDatetime* self, const CapeString format)
{
  struct tm timeinfo;
  CapeString ret = (CapeString)CAPE_ALLOC (100);
  
  cape_datetime__convert_cape (&timeinfo, self);
  
  // create buffer with timeinfo as string
  strftime (ret, 99, format, &timeinfo);
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__fmt_utc (const CapeDatetime* self, const CapeString format)
{
  CapeString ret = NULL;
  
  if (self)
  {
    if (cape_datetime_cross__is (self))  // special case to cross out the date
    {
      // copy of self and format
      CapeDatetime* copy_self = cape_datetime_cp (self);
      CapeString copy_format = cape_str_cp (format);
      
      // correct day and month to the first
      copy_self->month = 1;
      
      // cross out content in the format
      cape_datetime__internal__cross_out_format (&copy_format);
      
      ret = cape_datetime__internal__fmt_utc (copy_self, copy_format);
      
      cape_str_del (&copy_format);
      cape_datetime_del (&copy_self);
    }
    else
    {
      ret = cape_datetime__internal__fmt_utc (self, format);
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__std_msec (const CapeDatetime* dt)
{
  return cape_str_fmt ("%04i-%02i-%02iT%02i:%02i:%02i.%03iZ", dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->sec, dt->msec);
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__std (const CapeDatetime* dt)
{
  return cape_str_fmt ("%04i-%02i-%02iT%02i:%02i:%02iZ", dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->sec);
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__str (const CapeDatetime* dt)
{
  return cape_str_fmt ("%i-%02i-%02i %02i:%02i:%02i", dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->sec);
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__log (const CapeDatetime* dt)
{
  return cape_str_fmt ("%04i%02i%02i-%02i:%02i:%02i.%03i", dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->sec, dt->msec);
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__gmt (const CapeDatetime* dt)
{
  // TODO: use the same method as cape_datetime_s__str
  return cape_datetime_s__fmt_lcl (dt, "%a, %d %b %Y %H:%M:%S GMT");
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__aph (const CapeDatetime* dt)
{
  return cape_datetime_s__fmt_lcl (dt, "%a, %e %b %Y %H:%M:%S %z");
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__pre (const CapeDatetime* dt)
{
  // TODO: use the same method as cape_datetime_s__str
  return cape_datetime_s__fmt_lcl (dt, "%Y_%m_%d__%H_%M_%S__");
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__ISO8601 (const CapeDatetime* dt)
{
  // TODO: use the same method as cape_datetime_s__str
  return cape_datetime_s__fmt_lcl (dt, "%Y%m%dT%H%M%SZ");
}

//-----------------------------------------------------------------------------

time_t cape_datetime_n__unix (const CapeDatetime* dt)
{
  struct tm timeinfo;
  
  cape_datetime__convert_cape (&timeinfo, dt);
  
  // this function uses the local timezone info
  //return mktime (&timeinfo);
  
#if defined __WINDOWS_OS

  return _mkgmtime (&timeinfo);

#else

  // this function only exsists on BSD / Linux
  return timegm (&timeinfo);

#endif
}


//-----------------------------------------------------------------------------

int cape_datetime__date_de (CapeDatetime* dt, const CapeString datetime_in_text)
{
  dt->hour = 0;
  dt->minute = 0;
  dt->sec = 0;
  dt->msec = 0;
  dt->usec = 0;

  return cape_sscanf (datetime_in_text, "%u.%u.%u", &(dt->day), &(dt->month), &(dt->year)) == 3;
}

//-----------------------------------------------------------------------------

int cape_datetime__date_iso (CapeDatetime* dt, const CapeString datetime_in_text)
{
  dt->hour = 0;
  dt->minute = 0;
  dt->sec = 0;
  dt->msec = 0;
  dt->usec = 0;
  
  return cape_sscanf (datetime_in_text, "%u-%u-%u", &(dt->year), &(dt->month), &(dt->day)) == 3;
}

//-----------------------------------------------------------------------------

int cape_datetime__std_msec (CapeDatetime* dt, const CapeString datetime_in_text)
{
  return cape_sscanf (datetime_in_text, "%u-%u-%uT%u:%u:%u.%uZ", &(dt->year), &(dt->month), &(dt->day), &(dt->hour), &(dt->minute), &(dt->sec), &(dt->msec)) == 7;
}

//-----------------------------------------------------------------------------

int cape_datetime__str_msec (CapeDatetime* dt, const CapeString datetime_in_text)
{
  if (cape_str_begins (datetime_in_text, "XXXX-XX-"))
  {
    dt->year = 0;
    dt->month = 0;
    dt->hour = 0;
    dt->minute = 0;
    
    return cape_sscanf (datetime_in_text, "XXXX-XX-%u XX:XX:%u.%u", &(dt->day), &(dt->sec), &(dt->msec)) == 3;
  }
  else
  {
    return cape_sscanf (datetime_in_text, "%u-%u-%u %u:%u:%u.%u", &(dt->year), &(dt->month), &(dt->day), &(dt->hour), &(dt->minute), &(dt->sec), &(dt->msec)) == 7;
  }
}

//-----------------------------------------------------------------------------

int cape_datetime__str (CapeDatetime* dt, const CapeString datetime_in_text)
{
  dt->msec = 0;
  
  if (cape_str_begins (datetime_in_text, "XXXX-XX-"))
  {
    dt->year = 0;
    dt->month = 0;
    dt->hour = 0;
    dt->minute = 0;

    return cape_sscanf (datetime_in_text, "XXXX-XX-%u XX:XX:%u", &(dt->day), &(dt->sec)) == 2;
  }
  else
  {
    return cape_sscanf (datetime_in_text, "%u-%u-%u %u:%u:%u", &(dt->year), &(dt->month), &(dt->day), &(dt->hour), &(dt->minute), &(dt->sec)) == 6;
  }
}

//-----------------------------------------------------------------------------

struct CapeStopTimer_s
{
  double time_passed;

#if defined __WINDOWS_OS

  DWORD time_start;

#else

  struct timeval time_start;  

#endif
};

//-----------------------------------------------------------------------------

CapeStopTimer cape_stoptimer_new ()
{
  CapeStopTimer self = CAPE_NEW (struct CapeStopTimer_s);

  self->time_passed = .0;

  memset (&(self->time_start), 0, sizeof(struct timeval));
  
  return self;  
}

//-----------------------------------------------------------------------------

void cape_stoptimer_del (CapeStopTimer* p_self)
{
  if (*p_self)
  {  
    CAPE_DEL (p_self, struct CapeStopTimer_s);
  }
}

//-----------------------------------------------------------------------------

void cape_stoptimer_start (CapeStopTimer self)
{
#if defined __WINDOWS_OS

  self->time_start = GetTickCount();

#else

  gettimeofday (&(self->time_start), NULL);

#endif
}

//-----------------------------------------------------------------------------

void cape_stoptimer_stop (CapeStopTimer self)
{
#if defined __WINDOWS_OS

  DWORD end = GetTickCount();

  self->time_passed = end - self->time_start;

#else

  struct timeval time_res;
  struct timeval time_end;
  
  gettimeofday (&time_end, NULL);
  
  timersub (&time_end, &(self->time_start), &time_res);
  
  self->time_passed += ((double)time_res.tv_sec * 1000) + ((double)time_res.tv_usec / 1000);

#endif
}

//-----------------------------------------------------------------------------

void cape_stoptimer_set (CapeStopTimer self, double val_in_ms)
{
  self->time_passed = val_in_ms;
}

//-----------------------------------------------------------------------------

double cape_stoptimer_get (CapeStopTimer self)
{
  return self->time_passed;
}

//-----------------------------------------------------------------------------
