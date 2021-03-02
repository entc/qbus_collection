#include "cape_pipe.h"
#include "sys/cape_file.h"

// c includes
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

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

void* cape_pipe_create_or_connect (const char* path, const char* name, CapeErr err)
{
  void* ret = NULL;
  CapeString h = cape_fs_path_merge (path, name);

  mkfifo (h, 0666);

  ret = open (h, O_RDWR);
  
exit_and_cleanup:
  
  cape_str_del (&h);
  return ret;
}

//-----------------------------------------------------------------------------
