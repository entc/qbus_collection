#include "cape_file.h"
#include "cape_log.h"

#include "stc/cape_list.h"
#include "fmt/cape_tokenizer.h"

//-----------------------------------------------------------------------------

#ifdef __WINDOWS_OS

#include <windows.h>
#include <shlwapi.h>
#define CAPE_FS_FOLDER_SEP   '\\'

#elif defined __LINUX_OS 

#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <fts.h>

#define CAPE_FS_FOLDER_SEP   '/'

#elif defined __BSD_OS

#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fts.h>

#define CAPE_FS_FOLDER_SEP   '/'

#endif

//-----------------------------------------------------------------------------

CapeString cape_fs_path_merge (const char* path1, const char* path2)
{
  // use the platform dependant separator
  return cape_str_catenate_c (path1, CAPE_FS_FOLDER_SEP, path2);
}

//-----------------------------------------------------------------------------

CapeString cape_fs_path_merge_3 (const char* path1, const char* path2, const char* path3)
{
  CapeString h1 = cape_fs_path_merge (path1, path2);
  CapeString h2 = cape_fs_path_merge (h1, path3);
  
  cape_str_del (&h1);
  
  return h2;
}

//-----------------------------------------------------------------------------

int cape_fs_is_relative (const char* filepath)
{
  if (filepath == NULL)
  {
    return TRUE;
  }
  
#ifdef __WINDOWS_OS

  // use windows API
  return PathIsRelative (filepath);

#elif defined __LINUX_OS || defined __BSD_OS

  // trivial approach: check if first character is the separator
  return (*filepath != CAPE_FS_FOLDER_SEP);

#endif
}

//-----------------------------------------------------------------------------

CapeString cape_fs_path_current (const char* filepath)
{
#ifdef __WINDOWS_OS
  
  char* ret = CAPE_ALLOC (MAX_PATH + 1);
  DWORD dwRet;
  
  // use windows API
  dwRet = GetCurrentDirectory (MAX_PATH, ret);

  if ((dwRet == 0) || (dwRet > MAX_PATH))
  {
    CAPE_FREE (ret);
    return NULL;
  }
  
#elif defined __LINUX_OS || defined __BSD_OS

  char* ret = CAPE_ALLOC (PATH_MAX + 1);
  
  getcwd (ret, PATH_MAX);
  
#endif

  if (filepath)
  {
    CapeString h = cape_fs_path_merge (ret, filepath);
    
    CAPE_FREE(ret);
    
    return h;
  }
  else
  {
    return ret;
  }  
}

//-----------------------------------------------------------------------------

CapeString cape_fs_path_absolute (const char* filepath)
{
  if (filepath == NULL)
  {
    return cape_fs_path_current (NULL);
  }
  
  if (cape_fs_is_relative (filepath))
  {
    return cape_fs_path_current (filepath);
  }
  else
  {
    return cape_str_cp (filepath);
  }
}

//-----------------------------------------------------------------------------

CapeString cape_fs_path_resolve (const char* filepath, CapeErr err)
{
  if (filepath == NULL)
  {
    return cape_fs_path_current (NULL);
  }
  
#ifdef __WINDOWS_OS

  {
    char* ret = CAPE_ALLOC (MAX_PATH + 1);
    
    if (!PathCanonicalize (ret, filepath))
    {
      CAPE_FREE (ret);
      return NULL;
    }
    
    return ret;
  }

#elif defined __LINUX_OS || defined __BSD_OS

  {
    char* ret = CAPE_ALLOC (PATH_MAX + 1);

    if (!realpath (filepath, ret))
    {
      cape_err_lastOSError (err);
      
      cape_log_msg (CAPE_LL_ERROR, "CAPE", "path absolute", cape_err_text(err));
      
      CAPE_FREE (ret);
      return NULL;
    }
    
    return ret;
  }

#endif
}

//-----------------------------------------------------------------------------

const CapeString cape_fs_split (const char* filepath, CapeString* p_path)
{
  const char* last_sep_position = NULL;
  const char* ret = NULL;
  
  if (filepath == NULL)
  {
    return NULL;
  }
  
  // try to find the last position of the separator
  last_sep_position = strrchr (filepath, CAPE_FS_FOLDER_SEP);
  
  if (last_sep_position)
  {
    // we do have a path and file
    ret = last_sep_position + 1;

    if (p_path)
    {
      // extract the path from the origin filepath
      CapeString path = cape_str_sub (filepath, last_sep_position - filepath);

      // correct the path
      CapeString h = cape_fs_path_absolute (path);

      // override path
      cape_str_replace_mv (&path, &h);
      
      *p_path = path;
    }
  }
  else
  {
    // we do have only a file
    ret = filepath;
    
    if (p_path)
    {
      *p_path = cape_fs_path_current (NULL);
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------

const CapeString cape_fs_extension (const CapeString source)
{
  if (source)
  {
    const char* pos = strrchr (source, '.');
    
    if (pos)
    {
      return pos + 1;
    }
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

int cape_fs_path_create (const char* path, CapeErr err)
{
#ifdef __WINDOWS_OS
  
  if (CreateDirectory (path, NULL) == 0)
  {
    // get current system error code
    DWORD error_code = GetLastError();
    
    if (error_code != ERROR_ALREADY_EXISTS)  // ignore this error
    {
      return cape_err_formatErrorOS (err, error_code);;
    }
  }
  
  return CAPE_ERR_NONE;

#elif defined __LINUX_OS || defined __BSD_OS

  int res = mkdir (path, 0770);
  if (res)
  {
    int err_no = errno;
    if (err_no != EEXIST)
    {
      res = cape_err_formatErrorOS (err, err_no);
      
      //cape_log_msg (CAPE_LL_ERROR, "CAPE", "path create", cape_err_text(err));
      
      return res;
    }
  }

  return CAPE_ERR_NONE;
  
#endif  
}

//-----------------------------------------------------------------------------

int cape_fs_path_create_x (const char* path, CapeErr err)
{
  int res;
  CapeString directory = NULL;
  
  // create all path elements
  CapeList tokens = cape_tokenizer_buf (path, cape_str_size (path), CAPE_FS_FOLDER_SEP);
  
  // iterate through those elements
  CapeListCursor* cursor = cape_list_cursor_create (tokens, CAPE_DIRECTION_FORW);
  while (cape_list_cursor_next (cursor))
  {
    const CapeString part = cape_list_node_data (cursor->node);
    
    {
      if (directory)
      {
        // For other deeps, check each subdir and create it if not existing
        CapeString h = cape_fs_path_merge (directory, part);
        
        // replace directory (memory safe)
        cape_str_replace_mv (&directory, &h);
      }
      else
      {
        directory = cape_str_cp (part);
      }
    }
    
    res = cape_fs_path_create (directory, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    
//    printf ("PART: %s\n", directory);

    /*
     */

    /*
    // Root shall exist
    switch (cursor->position)
    {
      case 0:
      {
        // Nothing to do
        break;
      }
      case 1:
      {
        {
          // First subdir under root shall exist
          CapeString h = cape_str_cp ((const char*)cape_list_node_data (cursor->node));

          // replace directory (memory safe)
          cape_str_replace_mv (&directory, &h);
        }

        break;
      }
      default:
      {
        {
          // For other deeps, check each subdir and create it if not existing
          CapeString h = cape_fs_path_merge (directory, (const char*)cape_list_node_data (cursor->node));
          
          // replace directory (memory safe)
          cape_str_replace_mv (&directory, &h);
        }
        
        //if (!cape_fs_path_resolve (directory, err))
        {
          //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "directory create x", "Subdirectory %s doesn't exist, create it!", directory);
    
          // we can try to create it
          res = cape_fs_path_create (directory, err);
          if (res)
          {
            goto exit_and_cleanup;
          }
        } // End if cape_fs_path_resolve

        break;
      }
    }
     */
  } // End while
         
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_list_cursor_destroy (&cursor);
  cape_list_del (&tokens);
  
  cape_str_del (&directory);
  
  return res;
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_fs_path_size__on_del (void* ptr)
{
  CapeString h = ptr; cape_str_del (&h);
}

//-----------------------------------------------------------------------------

/*
number_t cape_fs_path_size__process_path (DIR* dir, CapeList folders, const char* path, CapeErr err)
{
  number_t total_size = 0;
  struct dirent* dentry;
  
  CapeString file = NULL;
  
  for (dentry = readdir (dir); dentry; dentry = readdir (dir))
  {
    struct stat st;
    
    // create the new filename
    {
      CapeString h = cape_fs_path_merge (path, dentry->d_name);
      
      cape_str_replace_mv (&file, &h);
    }
    
    // excluse special folders
    if (cape_str_equal (dentry->d_name, ".") || cape_str_equal (dentry->d_name, ".."))
    {
      continue;
    }
    
    // get detailed info about the file
    // not all filesystems return the info with readdir
    if (stat (file, &st) == 0)
    {
      // directory
      if (S_ISDIR (st.st_mode))
      {
        cape_list_push_back (folders, file);
        file = NULL;
        
        continue;
      }
      
      // regular file
      if (S_ISREG (st.st_mode))
      {
        total_size += st.st_size;
      }
    }
  }
  
  cape_str_del (&file);
  
  return total_size;
}
*/

//-----------------------------------------------------------------------------

int cape_fs_file_del (const char* path, CapeErr err)
{
  if (-1 == unlink (path))
  {
    return cape_err_lastOSError (err);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

off_t cape_fs_file_size (const char* path, CapeErr err)
{
#ifdef __WINDOWS_OS

  off_t ret = 0;
  LARGE_INTEGER lFileSize;
  
  // local objects
  HANDLE hf = CreateFile (path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
  
  if (hf == INVALID_HANDLE_VALUE)
  {
    cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }
  
  // retrieve the file size
  if (GetFileSizeEx (hf, &lFileSize) == INVALID_FILE_SIZE)
  {
    cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }

  // convert from large integer
  ret = lFileSize.QuadPart;
  
exit_and_cleanup:
  
  CloseHandle (hf);
  return ret;
    
#elif defined __LINUX_OS || defined __BSD_OS

  struct stat st;
  
  if (stat (path, &st) == -1)
  {
    cape_err_lastOSError (err);
    return 0;
  }
  else
  {
    return st.st_size;
  }
  
#endif
}

//-----------------------------------------------------------------------------

int cape_fs_file_load (const CapeString path, const CapeString file, void* ptr, fct_cape_fs_file_load fct, CapeErr err)
{
  int res;
  
  // local objects
  CapeFileHandle fh = cape_fh_new (path, file);
  number_t bytes_read;
  
  // try to open
  res = cape_fh_open (fh, O_RDONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  {
    char buffer [1024];
    
    for (bytes_read = cape_fh_read_buf (fh, buffer, 1024); bytes_read > 0; bytes_read = cape_fh_read_buf (fh, buffer, 1024))
    {
      res = fct (ptr, buffer, bytes_read, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  
exit_and_cleanup:
  
  cape_fh_del (&fh);
  return res;
}

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

off_t cape_fs_path_size (const char* path, CapeErr err)
{
  off_t total_size = 0;
  
  FTSENT *node;
  FTS* tree;
  
  char * fts_path[2] = {cape_str_cp(path), NULL};
  
  tree = fts_open (fts_path, FTS_PHYSICAL, NULL);
  
  if (NULL == tree)
  {
    cape_err_lastOSError (err);
    return 0;
  }
  
  while ((node = fts_read(tree)))
  {
    if (node->fts_info & FTS_F)
    {
      total_size += node->fts_statp->st_size;
    }
  }
  
  // cleanup
  fts_close (tree);
  cape_str_del (&(fts_path[0]));
  
  return total_size;
}

//-----------------------------------------------------------------------------

int cape_fs_path_exists (const char* path)
{
  struct stat st;
  
  if (cape_str_empty (path))
  {
    return FALSE;
  }
  
  return stat (path, &st) == 0;
}

//-----------------------------------------------------------------------------

struct CapeFileHandle_s
{
  CapeString file;
  
  number_t fd;
};

//-----------------------------------------------------------------------------

CapeFileHandle cape_fh_new (const CapeString path, const CapeString file)
{
  CapeFileHandle self = CAPE_NEW (struct CapeFileHandle_s);
  
  if (path)
  {
    self->file = cape_fs_path_merge (path, file);
  }
  else
  {
    self->file = cape_str_cp (file);
  }
  
  self->fd = -1;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_fh_del (CapeFileHandle* p_self)
{
  if (*p_self)
  {
    CapeFileHandle self = *p_self;
    
    cape_str_del (&(self->file));
    
    if (self->fd >= 0)
    {
      close (self->fd);
    }
    
    CAPE_DEL (p_self, struct CapeFileHandle_s);
  }  
}

//-----------------------------------------------------------------------------

const CapeString cape_fh_file (CapeFileHandle self)
{
  return self->file;
}

//-----------------------------------------------------------------------------

int cape_fh_open (CapeFileHandle self, int flags, CapeErr err)
{
  self->fd = open (self->file, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  
  if (self->fd == -1)
  {
    return cape_err_lastOSError (err);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void* cape_fh_fd (CapeFileHandle self)
{
  return (void*)self->fd;
}

//-----------------------------------------------------------------------------

number_t cape_fh_read_buf (CapeFileHandle self, char* bufdat, number_t buflen)
{
  int res = read (self->fd, bufdat, buflen);
  if (res < 0)
  {
    return 0;
  }
  
  return res;
}

//-----------------------------------------------------------------------------

number_t cape_fh_write_buf (CapeFileHandle self, const char* bufdat, number_t buflen)
{
  return write (self->fd, bufdat, buflen);
}

//-----------------------------------------------------------------------------

struct CapeDirCursor_s
{
  FTS* tree;
  FTSENT* node;
  
  char* fts_path[2];
  CapeString current_dir;
};

//-----------------------------------------------------------------------------

CapeDirCursor cape_dc_new (const CapeString path, CapeErr err)
{
  CapeDirCursor self = CAPE_NEW(struct CapeDirCursor_s);
  
  self->fts_path[0] = cape_str_cp (path);
  self->fts_path[1] = NULL;

  // save the current directory
  self->current_dir = cape_fs_path_current (NULL);

  self->tree = fts_open (self->fts_path, FTS_PHYSICAL | FTS_XDEV, NULL);
  self->node = NULL;
  
  if (self->tree == NULL)
  {
    cape_err_lastOSError (err);
    cape_dc_del (&self);
  }
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_dc_del (CapeDirCursor* p_self)
{
  if (*p_self)
  {
    CapeDirCursor self = *p_self;
    
    cape_str_del (&(self->fts_path[0]));
    
    if (self->tree)
    {
      fts_close (self->tree);
    }
    
    // somehow the treesearch changed the current path
    // set it back
    chdir (self->current_dir);
    
    CAPE_DEL(p_self, struct CapeDirCursor_s);
  }
}

//-----------------------------------------------------------------------------

int  cape_dc_next (CapeDirCursor self)
{
  self->node = fts_read (self->tree);
  
  return (self->node != NULL);
}

//-----------------------------------------------------------------------------

const CapeString cape_dc_name (CapeDirCursor self)
{
  if (self->node)
  {
    if (self->node->fts_info)
    {
      return self->node->fts_name;
    }
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

off_t cape_dc_size (CapeDirCursor self)
{
  if (self->node)
  {
    if (self->node->fts_info & FTS_F)
    {
      return self->node->fts_statp->st_size;
    }
  }
  
  return 0;
}

//-----------------------------------------------------------------------------

const void* cape_fs_stdout (void)
{
  return stdout;
}

//-----------------------------------------------------------------------------

void cape_fs_write_msg (const void* handle, const char* buf, number_t len)
{
  if (handle)
  {
    write ((number_t)handle, buf, len);
  }
}

//-----------------------------------------------------------------------------

void cape_fs_writeln_msg (const void* handle, const char* buf, number_t len)
{
  if (handle)
  {
    write ((number_t)handle, buf, len);
    write ((number_t)handle, "\n", 1);
  }
}

//-----------------------------------------------------------------------------

void cape_fs_write_fmt (const void* handle, const char* format, ...)
{
  
}

//-----------------------------------------------------------------------------

#elif defined __WINDOWS_OS

#include <stdio.h>
#include <share.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include <windows.h>

//-----------------------------------------------------------------------------

off_t cape_fs_path_size (const char* path, CapeErr err)
{
  return 0;
}

//-----------------------------------------------------------------------------

struct CapeFileHandle_s
{
  CapeString file;
  
  number_t fd;
};

//-----------------------------------------------------------------------------

CapeFileHandle cape_fh_new (const CapeString path, const CapeString file)
{
  CapeFileHandle self = CAPE_NEW (struct CapeFileHandle_s);
  
  if (path)
  {
    self->file = cape_fs_path_merge (path, file);
  }
  else
  {
    self->file = cape_str_cp (file);
  }
  
  self->fd = -1;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_fh_del (CapeFileHandle* p_self)
{
  if (*p_self)
  {
    CapeFileHandle self = *p_self;
    
    cape_str_del (&(self->file));
    
    if (self->fd >= 0)
    {
      _close (self->fd);
    }
    
    CAPE_DEL (p_self, struct CapeFileHandle_s);
  }  
}

//-----------------------------------------------------------------------------

int cape_fh_open (CapeFileHandle self, int flags, CapeErr err)
{
  // always open in binary mode
  if (_sopen_s(&(self->fd), self->file, flags | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE) != 0)
  {
    return cape_err_lastOSError (err);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void* cape_fh_fd (CapeFileHandle self)
{
  return (void*)self->fd;
}

//-----------------------------------------------------------------------------

number_t cape_fh_read_buf (CapeFileHandle self, char* bufdat, number_t buflen)
{
  int res = _read (self->fd, bufdat, buflen);
  if (res < 0)
  {
    return 0;
  }
  
  return res;
}

//-----------------------------------------------------------------------------

number_t cape_fh_write_buf (CapeFileHandle self, const char* bufdat, number_t buflen)
{
  return _write (self->fd, bufdat, buflen);
}

//-----------------------------------------------------------------------------

struct CapeDirCursor_s
{
  /* the handle */
  HANDLE dhandle;
  
  /* windows structure to store file informations */
  WIN32_FIND_DATA data;
  
  int initial;
};

//-----------------------------------------------------------------------------

CapeDirCursor cape_dc_new (const CapeString path, CapeErr err)
{
  CapeDirCursor self = CAPE_NEW (struct CapeDirCursor_s);
  
  {
    CapeString search_pattern = cape_fs_path_merge (path, "*");
    
    self->dhandle = FindFirstFile (search_pattern, &(self->data));
    
    cape_str_del (&search_pattern);
  }
  
  if (self->dhandle == NULL)
  {
    cape_err_lastOSError (err);
    
    cape_dc_del (&self);
  }
  
  self->initial = TRUE;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_dc_del (CapeDirCursor* p_self)
{
  if (*p_self)
  {
    CapeDirCursor self = *p_self;
    
    if (self->dhandle)
    {
      FindClose (self->dhandle);
    }
    
    CAPE_DEL(p_self, struct CapeDirCursor_s);
  }
}

//-----------------------------------------------------------------------------

int cape_dc_next (CapeDirCursor self)
{
  if (self->dhandle == NULL)
  {
    return FALSE;
  }
  
  if (self->initial)
  {
    self->initial = FALSE;
    return TRUE;
  }
  
  return FindNextFile (self->dhandle, &(self->data));
}

//-----------------------------------------------------------------------------

const CapeString cape_dc_name (CapeDirCursor self)
{
  if (self->dhandle)
  {
    return self->data.cFileName;
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------
