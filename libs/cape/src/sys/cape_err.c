#include "cape_err.h"

// cape includes
#include "stc/cape_str.h"
#include "sys/cape_file.h"

#ifdef __WINDOWS_OS
#include <windows.h>
#else
#include <errno.h>
#endif

// c includes
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

//-----------------------------------------------------------------------------

struct CapeErr_s
{
    char* text;
    int code;
};

//-----------------------------------------------------------------------------

CapeErr cape_err_new (void)
{
  CapeErr self = CAPE_NEW (struct CapeErr_s);

  self->text = NULL;
  self->code = CAPE_ERR_NONE;
  
  return self;    
}

//-----------------------------------------------------------------------------

void cape_err_del (CapeErr* p_self)
{
  if (*p_self)
  {
    CapeErr self = *p_self;
    
    cape_err_clr (self);
    
    CAPE_DEL (p_self, struct CapeErr_s);
  }  
}

//-----------------------------------------------------------------------------

void cape_err_clr (CapeErr self)
{
  cape_str_del (&(self->text));
  self->code = CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

const char* cape_err_text (CapeErr self)
{
  return (NULL == self->text) ? "" : self->text;    
}

//-----------------------------------------------------------------------------

int cape_err_code (CapeErr self)
{
  return self->code;
}

//-----------------------------------------------------------------------------

int cape_err_set__i (CapeErr self, int line_number, const char* file, unsigned long code, const char* error_message)
{
  self->code = code;
  
  // returns only the file
  const CapeString file_relative = cape_fs_split (file, NULL);
  
  // print to STDERR
  fprintf (stderr, "{%s:%i} | %s\n", file_relative, line_number, error_message);
  
  cape_str_replace_cp (&(self->text), error_message);
  
  return code;
}

//-----------------------------------------------------------------------------

int cape_err_set_fmt__i (CapeErr self, int line_number, const char* file, unsigned long code, const char* error_message, ...)
{
  char buffer [1002];
  
  // variables
  va_list ptr;
  
  if (self == NULL)
  {
    return code;
  }
  
  va_start(ptr, error_message);
  
#ifdef __WINDOWS_OS
  vsnprintf_s (buffer, 1001, 1000, error_message, ptr);
#else
  vsnprintf (buffer, 1000, error_message, ptr);
#endif
  
  cape_err_set__i (self, line_number, file, code, buffer);
  
  va_end(ptr);
  
  return code;
}

//-----------------------------------------------------------------------------

int cape_err_formatErrorOS_i (CapeErr self, int line_number, const char* file, unsigned long errCode)
{
#ifdef __WINDOWS_OS

  if (errCode)
  {
    LPTSTR buffer = NULL;
    DWORD res = FormatMessageA (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buffer, 0, NULL);
    
    if (buffer)
    {
      if (res > 0)
      {
        cape_err_set__i (self, line_number, file, CAPE_ERR_OS, buffer);
      }
      // release buffer
      LocalFree (buffer);
    }
  }

  return CAPE_ERR_OS;

#else
  return cape_err_set__i (self, line_number, file, CAPE_ERR_OS, strerror(errCode));
#endif
}

//-----------------------------------------------------------------------------

int cape_err_lastOSError_i (CapeErr self, int line_number, const char* file)
{
  if (self)
  {
    #ifdef __WINDOWS_OS
      return cape_err_formatErrorOS_i (self, line_number, file, GetLastError ());
    #else
      return cape_err_formatErrorOS_i (self, line_number, file, errno);
    #endif
  }
  
  return CAPE_ERR_OS;
}

//-----------------------------------------------------------------------------

