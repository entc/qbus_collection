#include "cape_pipe.h"

#include "sys/cape_file.h"

//-----------------------------------------------------------------------------

void* cape_pipe_create (const char* path, const char* name, CapeErr err)
{
  void* ret = NULL;
  CapeString h = cape_fs_path_merge (path, name);
  
  cape_str_del (&h);

  return ret;
}

//-----------------------------------------------------------------------------

void* cape_pipe_connect (const char* path, const char* name, CapeErr err)
{
  void* ret = NULL;
  CapeString h = cape_fs_path_merge (path, name);
  
  cape_str_del (&h);
  
  return ret;
}

//-----------------------------------------------------------------------------

