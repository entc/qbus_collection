#include "cape_time.h"

#include "sys/cape_types.h"
#include "fmt/cape_tokenizer.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

#define _BSD_SOURCE

#include <sys/time.h>

#elif defined __WINDOWS_OS

#include <windows.h>

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

  if (dt->is_utc)
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

void cape_datetime_utc__add_s (CapeDatetime* dt, const CapeString delta)
{
#if defined __WINDOWS_OS
  
  
#else

  struct timeval time;

  struct tm* l01;
  
  gettimeofday (&time, NULL);

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

          timeradd (&time, &h, &time);
          break;
        }
        case 'h':
        {
          struct timeval h;
          
          printf ("append hour\n");
          
          h.tv_sec = atol (delta_part + 1) * 3600;
          h.tv_usec = 0;

          timeradd (&time, &h, &time);
          break;
        }
        case 'm':
        {
          struct timeval h;

          h.tv_sec = atol (delta_part + 1) * 60;
          h.tv_usec = 0;

          timeradd (&time, &h, &time);
          break;
        }
        case 's':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1);
          h.tv_usec = 0;
          
          timeradd (&time, &h, &time);
          break;
        }
        case 'u':
        {
          struct timeval h;
          
          h.tv_sec = 0;
          h.tv_usec = atol (delta_part + 1) * 1000;

          timeradd (&time, &h, &time);
          break;
        }
      }
    }
    
    cape_list_cursor_destroy (&cursor);
  }

  cape_list_del (&tokens);
  
  l01 = gmtime (&(time.tv_sec));
  
  cape_datetime__convert_timeinfo (dt, l01);
  
  dt->msec = time.tv_usec / 1000;
  dt->usec = time.tv_usec;
  
  dt->is_utc = TRUE;

#endif
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// reduce input datetine from a certain time period defined in delta string format ( D2:h2:m2:s2:u2 ) and save it in result datetime 
// Function doesn't change the time zone, so take care to use same time zone for input and result (e.g. both UTC)
void  cape_datetime__remove_s (CapeDatetime* input,  const CapeString delta, CapeDatetime* result)
{
#if defined __WINDOWS_OS
  
    
#else

  struct timeval time_timeval;
  struct tm* time_tm;
  
  time_timeval.tv_sec = cape_datetime_n__unix (input);
  time_timeval.tv_usec = (input->msec * 1000) + input->usec;

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

          timersub (&time_timeval, &h, &time_timeval);
          break;
        }
        case 'h':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1) * 3600;
          h.tv_usec = 0;

          timersub (&time_timeval, &h, &time_timeval);
          break;
        }
        case 'm':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1) * 60;
          h.tv_usec = 0;

          timersub (&time_timeval, &h, &time_timeval);
          break;
        }
        case 's':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1);
          h.tv_usec = 0;
          
         timersub (&time_timeval, &h, &time_timeval);
          break;
        }
        case 'u':
        {
          struct timeval h;
          
          h.tv_sec = 0;
          h.tv_usec = atol (delta_part + 1) * 1000;

          timersub (&time_timeval, &h, &time_timeval);
          break;
        }
      }
    }
    
    cape_list_cursor_destroy (&cursor);
  }

  cape_list_del (&tokens);
  
  time_tm = localtime(&(time_timeval.tv_sec));
  
  cape_datetime__convert_timeinfo (result, time_tm);
  
  result->msec = time_timeval.tv_usec / 1000;
  result->usec = time_timeval.tv_usec;
  
  result->is_utc = TRUE;

#endif
}

//-----------------------------------------------------------------------------
// append a certain time period defined in delta string format ( D2:h2:m2:s2:u2 ) and save it in result datetime
// Function doesn't change the time zone, so take care to use same time zone for input and result (e.g. both UTC)
void  cape_datetime__add_s (CapeDatetime* input,  const CapeString delta, CapeDatetime* result)
{
#if defined __WINDOWS_OS
  
    
#else

  struct timeval time_timeval;
  struct tm* time_tm;
  
  time_timeval.tv_sec = cape_datetime_n__unix (input);
  time_timeval.tv_usec = (input->msec * 1000) + input->usec;

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

          timeradd (&time_timeval, &h, &time_timeval);
          break;
        }
        case 'h':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1) * 3600;
          h.tv_usec = 0;

          timeradd (&time_timeval, &h, &time_timeval);
          break;
        }
        case 'm':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1) * 60;
          h.tv_usec = 0;

          timeradd (&time_timeval, &h, &time_timeval);
          break;
        }
        case 's':
        {
          struct timeval h;
          
          h.tv_sec = atol (delta_part + 1);
          h.tv_usec = 0;
          
         timeradd (&time_timeval, &h, &time_timeval);
          break;
        }
        case 'u':
        {
          struct timeval h;
          
          h.tv_sec = 0;
          h.tv_usec = atol (delta_part + 1) * 1000;

          timeradd (&time_timeval, &h, &time_timeval);
          break;
        }
      }
    }
    
    cape_list_cursor_destroy (&cursor);
  }

  cape_list_del (&tokens);
  
  time_tm = localtime(&(time_timeval.tv_sec));
  
  cape_datetime__convert_timeinfo (result, time_tm);
  
  result->msec = time_timeval.tv_usec / 1000;
  result->usec = time_timeval.tv_usec;
  
  result->is_utc = TRUE;

#endif
}

//-----------------------------------------------------------------------------

int cape_datetime_cmp (const CapeDatetime* dt1, const CapeDatetime* dt2)
{
  /*
  CapeString h1 = cape_datetime_s__std (dt1);
  CapeString h2 = cape_datetime_s__std (dt2);
  
  printf ("%s <--> %s\n", h1, h2);
   */
  
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

CapeString cape_datetime_s__fmt (const CapeDatetime* dt, const CapeString format)
{
  CapeString ret = (CapeString)CAPE_ALLOC (100);
  
  {
    struct tm timeinfo;
    
    cape_datetime__convert_cape (&timeinfo, dt);
    
    mktime (&timeinfo);
    
    // create buffer with timeinfo as string
    strftime (ret, 99, format, &timeinfo);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__std (const CapeDatetime* dt)
{
  return cape_str_fmt ("%04i-%02i-%02iT%02i:%02i:%02i.%03iZ", dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->sec, dt->msec);
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
  return cape_datetime_s__fmt (dt, "%a, %d %b %Y %H:%M:%S GMT");
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__aph (const CapeDatetime* dt)
{
  return cape_datetime_s__fmt (dt, "%a, %e %b %Y %H:%M:%S %z");
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__pre (const CapeDatetime* dt)
{
  // TODO: use the same method as cape_datetime_s__str
  return cape_datetime_s__fmt (dt, "%Y_%m_%d__%H_%M_%S__");
}

//-----------------------------------------------------------------------------

CapeString cape_datetime_s__ISO8601 (const CapeDatetime* dt)
{
  // TODO: use the same method as cape_datetime_s__str
  return cape_datetime_s__fmt (dt, "%Y%m%dT%H%M%SZ");
}

//-----------------------------------------------------------------------------

time_t cape_datetime_n__unix (const CapeDatetime* dt)
{
  struct tm timeinfo;
  
  cape_datetime__convert_cape (&timeinfo, dt);
  
  return mktime (&timeinfo);
}

//-----------------------------------------------------------------------------

int cape_datetime__std (CapeDatetime* dt, const CapeString datetime_in_text)
{
  return sscanf (datetime_in_text, "%u-%u-%uT%u:%u:%u.%uZ", &(dt->year), &(dt->month), &(dt->day), &(dt->hour), &(dt->minute), &(dt->sec), &(dt->msec)) == 7;
}

//-----------------------------------------------------------------------------

int cape_datetime__str (CapeDatetime* dt, const CapeString datetime_in_text)
{
  return sscanf (datetime_in_text, "%u-%u-%u %u:%u:%u.%u", &(dt->year), &(dt->month), &(dt->day), &(dt->hour), &(dt->minute), &(dt->sec), &(dt->msec)) == 7;
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
