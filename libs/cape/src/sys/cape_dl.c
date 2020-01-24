#include "cape_dl.h"

#include "stc/cape_str.h"
#include "sys/cape_file.h"
#include "sys/cape_log.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

#include <dlfcn.h>

//-----------------------------------------------------------------------------

struct CapeDl_s
{
  void* handle;
};

//-----------------------------------------------------------------------------

CapeDl cape_dl_new (void)
{
  CapeDl self = CAPE_NEW(struct CapeDl_s);

  self->handle = NULL;
  
  return self;  
}

//-----------------------------------------------------------------------------

void cape_dl_del (CapeDl* p_self)
{
  if (*p_self)
  {
    CapeDl self = *p_self;
    
    if (self->handle)
    {
      // this somehow happen automatically
      dlclose (self->handle);
    }
    
    CAPE_DEL(p_self, struct CapeDl_s);  
  }
}

//-----------------------------------------------------------------------------

int cape_dl_load (CapeDl self, const char* path, const char* name, CapeErr err)
{
  int res;

#if defined __LINUX_OS
  CapeString fullname = cape_str_catenate_3 ("lib", name, ".so");
#elif defined __BSD_OS
  CapeString fullname = cape_str_catenate_3 ("lib", name, ".dylib");
#endif
  
  CapeString filename = NULL;
  
  if (path)
  {
    filename = cape_fs_path_merge (path, fullname);
  }
  else
  {
    filename = cape_str_cp (fullname);
  }

  // try to aquire a handle loading the shared library
#if defined RTLD_NODELETE  
  self->handle = dlopen (filename, RTLD_NOW | RTLD_NODELETE);
#else
  self->handle = dlopen (filename, RTLD_NOW);
#endif
  
  if (self->handle == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_OS, dlerror());
    goto exit;
  }
  
  res = CAPE_ERR_NONE;

exit:

  cape_str_del (&fullname);
  cape_str_del (&filename);
  
  return res;
}

//-----------------------------------------------------------------------------

void* cape_dl_funct (CapeDl self, const char* name, CapeErr err)
{
  void* function_ptr;
  const char* dlerror_text;
  
  // clear error code  
  dlerror();
  
  function_ptr = dlsym (self->handle, name);
  
  dlerror_text = dlerror ();

  // check if an error ocours
  if (dlerror_text)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "dl_funct", "can't find '%s' function", name);
    
    cape_err_set (err, CAPE_ERR_OS, dlerror_text);
    return NULL;
  }
  
  return function_ptr;
}

//-----------------------------------------------------------------------------

#elif defined __WINDOWS_OS

#include <windows.h>

//-----------------------------------------------------------------------------

struct CapeDl_s
{
  HMODULE ptr;
};

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

CapeDl cape_dl_new (void)
{
  CapeDl self = CAPE_NEW(struct CapeDl_s);
  
  self->ptr = NULL;
  
  return self;  
}

//-----------------------------------------------------------------------------

void cape_dl_del (CapeDl* p_self)
{
  if (*p_self)
  {
    CapeDl self = *p_self;
    
    if (FreeLibrary (self->ptr) == 0)
    {
      // error
    }
    
    CAPE_DEL(p_self, struct CapeDl_s);  
  }
}

//-----------------------------------------------------------------------------

int cape_dl_load (CapeDl self, const char* path, const char* name, CapeErr err)
{
  int res;
  
  CapeString fullname = cape_str_catenate_3 ("lib", name, ".dll");  
  CapeString filename = NULL;
  
  if (path)
  {
    filename = cape_fs_path_merge (path, fullname);
  }
  else
  {
    filename = cape_str_cp (fullname);
  }

  // TODO: maybe we need to use LoadLibrary_ex
  self->ptr = LoadLibrary (filename);  
  if (self->ptr == NULL)
  {
    res = cape_err_lastOSError (err);
    goto exit;
  }
  
  res = CAPE_ERR_NONE;
  
exit:
  
  cape_str_del (&fullname);
  cape_str_del (&filename);
  
  return res;
}

//-----------------------------------------------------------------------------

void* cape_dl_funct (CapeDl self, const char* name, CapeErr err)
{
  void* function_ptr = GetProcAddress(self->ptr, name);
  
  if (function_ptr == NULL)
  {
    cape_err_lastOSError (err);
    return NULL;
  }
  
  return function_ptr;
}

//-----------------------------------------------------------------------------

#endif
