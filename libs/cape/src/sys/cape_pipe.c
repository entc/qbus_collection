#include "cape_pipe.h"
#include "sys/cape_file.h"

#if defined __LINUX_OS || defined __BSD_OS

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
  long fd = 0;
  CapeString h = cape_fs_path_merge (path, name);

  mkfifo (h, 0666);

  fd = open (h, O_RDWR);
  
exit_and_cleanup:
  
  cape_str_del (&h);
  return (void*)fd;
}

//-----------------------------------------------------------------------------

#elif defined __WINDOWS_OS

#include <windows.h>

//-----------------------------------------------------------------------------

void* cape_pipe_create (const char* path, const char* name, CapeErr err)
{

}

//-----------------------------------------------------------------------------

void* cape_pipe_connect (const char* path, const char* name, CapeErr err)
{

}

//-----------------------------------------------------------------------------

void* cape_pipe_create_or_connect (const char* path, const char* name, CapeErr err)
{

}

#endif