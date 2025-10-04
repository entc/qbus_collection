#include "cape_log.h"

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_file.h"
#include "sys/cape_time.h"
#include "stc/cape_str.h"

const void* CAPE_LOG_OUT_FD = NULL;
static CapeLogLevel g_log_level = CAPE_LL_TRACE;

//-----------------------------------------------------------------------------

static const char* msg_matrix[7] = { "___", "FAT", "ERR", "WRN", "INF", "DBG", "TRA" };

#if defined _WIN64 || defined _WIN32

#include <windows.h>

  static WORD clr_matrix[11] = {
  0, 
  FOREGROUND_RED | FOREGROUND_INTENSITY,
  FOREGROUND_RED | FOREGROUND_INTENSITY,
  FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
  FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY,
  FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED,
  FOREGROUND_GREEN | FOREGROUND_BLUE
  };

#else

#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
  
  static const char* clr_matrix[7] = { "0", "0;31", "1;31", "1;33", "0;32", "0;34", "0;30" };

#endif

//-----------------------------------------------------------------------------

void cape_log_set_level (CapeLogLevel log_level)
{
  g_log_level = log_level;
}

//-----------------------------------------------------------------------------

CapeLogLevel cape_log_level_from_s (const char* log_level_as_text, CapeLogLevel alt)
{
  CapeLogLevel ret = alt;
  
  if (log_level_as_text)
  {
    if (cape_str_compare (log_level_as_text, "fatal"))
    {
      ret = CAPE_LL_FATAL;
    }
    else if (cape_str_compare (log_level_as_text, "error"))
    {
      ret = CAPE_LL_ERROR;
    }
    else if (cape_str_compare (log_level_as_text, "warn"))
    {
      ret = CAPE_LL_WARN;
    }
    else if (cape_str_compare (log_level_as_text, "info"))
    {
      ret = CAPE_LL_INFO;
    }
    else if (cape_str_compare (log_level_as_text, "debug"))
    {
      ret = CAPE_LL_DEBUG;
    }
    else if (cape_str_compare (log_level_as_text, "trace"))
    {
      ret = CAPE_LL_TRACE;
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void cape_log_msg (CapeLogLevel lvl, const char* unit, const char* method, const char* msg)
{
  char buffer [2050];

  if ((lvl > g_log_level) || (lvl > CAPE_LL_TRACE))
  {
    return;
  }
  
#if defined _WIN64 || defined _WIN32 
  
  _snprintf_s (buffer, 2048, _TRUNCATE, "%-12s %s|%-8s] %s", method, msg_matrix[lvl], unit, msg);
  {
    CONSOLE_SCREEN_BUFFER_INFO info;
    // get the console handle
    HANDLE hStdout = GetStdHandle (STD_OUTPUT_HANDLE);      
    // remember the original background color
    GetConsoleScreenBufferInfo (hStdout, &info);
    // do some fancy stuff
    SetConsoleTextAttribute (hStdout, clr_matrix[lvl]);
    
    printf("%s\n", buffer);
    
    SetConsoleTextAttribute (hStdout, info.wAttributes);
  }

#else

  CapeDatetime dt;
  number_t len;
  
  cape_datetime_utc (&dt);
  
  if (msg)
  {
    len = snprintf (buffer, 2048, "%04i%02i%02i-%02i:%02i:%02i.%03i|%s|%8s|%-20s| %s", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.sec, dt.msec, msg_matrix[lvl], unit, method, msg);
  }
  else
  {
    len = snprintf (buffer, 2048, "%-12s %s|%-8s]", method, msg_matrix[lvl], unit);
  }
  
  if (CAPE_LOG_OUT_FD)
  {
    cape_fs_writeln_msg (CAPE_LOG_OUT_FD, buffer, len);
  }
  else
  {
    printf("\033[%sm%s\033[0m\n", clr_matrix[lvl], buffer);
  }
  
#endif 
}

//-----------------------------------------------------------------------------

void cape_log_fmt (CapeLogLevel lvl, const char* unit, const char* method, const char* format, ...)
{
  char buffer [1002];
  va_list ptr;
  
  if ((lvl > g_log_level) || (lvl > CAPE_LL_TRACE))
  {
    return;
  }
  
  va_start(ptr, format);
  
#ifdef _WIN32
  vsnprintf_s (buffer, 1001, 1000, format, ptr);
#else
  vsnprintf (buffer, 1000, format, ptr);
#endif 
  
  cape_log_msg (lvl, unit, method, buffer);
  
  va_end(ptr);
}

//-----------------------------------------------------------------------------

struct CapeFileLog_s
{
  CapeFileHandle fh;
};

//-----------------------------------------------------------------------------

int cape_log_get_file (CapeFileLog self, const char* filepath, CapeErr err)
{
  int res;
  CapeString path = NULL;
  
  const CapeString log_file = cape_fs_split (filepath, &path);
  
  if (path)
  {
    res = cape_fs_path_create (path, NULL, err);

    // finally merge the path
    self->fh = cape_fh_new (path, log_file);

    cape_str_del (&path);
  }
  else
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't split filepath");
  }
      
  return res;
}

//-----------------------------------------------------------------------------

CapeFileLog cape_log_new (const char* filepath)
{
  int res = CAPE_ERR_NONE;
  
  CapeErr err = cape_err_new ();
  
  CapeFileLog self = CAPE_NEW (struct CapeFileLog_s);
  
  self->fh = NULL;
  
  res = cape_log_get_file (self, filepath, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = cape_fh_open (self->fh, O_CREAT | O_APPEND | O_WRONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  CAPE_LOG_OUT_FD = cape_fh_fd (self->fh);
  
  //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "log new", "use log file: %s", self->file);
    
exit_and_cleanup:

  cape_err_del (&err);

  if (res)
  {
    cape_log_msg (CAPE_LL_ERROR, "CAPE", "log new", cape_err_text(err));
    
    cape_log_del (&self);
  }
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_log_del (CapeFileLog* p_self)
{
  if (*p_self)
  {
    CapeFileLog self = *p_self;
    
    cape_fh_del (&(self->fh));
    
    CAPE_DEL (p_self, struct CapeFileLog_s);
  }
}

//-----------------------------------------------------------------------------
