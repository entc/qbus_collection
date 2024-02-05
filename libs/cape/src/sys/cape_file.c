#include "cape_file.h"
#include "cape_log.h"

#include "stc/cape_list.h"
#include "stc/cape_stream.h"
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
#include <stdio.h>
#include <dirent.h>
#include <fts.h>
#include <sys/stat.h>

#define CAPE_FS_FOLDER_SEP   '/'

#elif defined __BSD_OS

#if defined __APPLE__

#include <copyfile.h>

#endif

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

#define CAPE_BUFFER_SIZE 10000

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

void cape_fs_path_rebuild__norm (CapeList parts)
{
  // local objects
  CapeListCursor* cursor = cape_list_cursor_create (parts, CAPE_DIRECTION_FORW);

  while (cape_list_cursor_next (cursor))
  {
    if (cursor->position)
    {
      const CapeString part = cape_list_node_data (cursor->node);

      if (cape_str_empty (part))
      {
        cape_list_cursor_erase (parts, cursor);
      }
    }
  }

  cape_list_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------

int cape_fs_path_rebuild__stack (CapeList parts, CapeList paths, CapeErr err)
{
  int res;

  // local objects
  CapeListCursor* cursor = cape_list_cursor_create (parts, CAPE_DIRECTION_FORW);

  while (cape_list_cursor_next (cursor))
  {
    const CapeString part = cape_list_node_data (cursor->node);

    if (cape_str_equal (part, ".."))
    {
      if (cape_list_size (paths) > 0)
      {
        if (cape_str_empty (cape_list_pop_back (paths)))
        {
          res = cape_err_set (err, CAPE_ERR_OUT_OF_BOUNDS, "path out of bounds");
          goto exit_and_cleanup;
        }
      }
      else
      {
        res = cape_err_set (err, CAPE_ERR_OUT_OF_BOUNDS, "path out of bounds");
        goto exit_and_cleanup;
      }
    }
    else if (cape_str_equal (part, "."))
    {
      // nothing
    }
    else
    {
      cape_list_push_back (paths, (void*)part);
    }
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_list_cursor_destroy (&cursor);
  return res;
}

//-----------------------------------------------------------------------------

CapeString cape_fs_path_rebuild__build (CapeList paths)
{
  // local objects
  CapeListCursor* cursor = cape_list_cursor_create (paths, CAPE_DIRECTION_FORW);
  CapeStream s = cape_stream_new ();

  while (cape_list_cursor_next (cursor))
  {
    if (cursor->position)
    {
      cape_stream_append_c (s, CAPE_FS_FOLDER_SEP);
    }

    cape_stream_append_str (s, cape_list_node_data (cursor->node));
  }

  cape_list_cursor_destroy (&cursor);

  return cape_stream_to_str (&s);
}

//-----------------------------------------------------------------------------

CapeString cape_fs_path_rebuild (const char* filepath, CapeErr err)
{
  CapeString ret = NULL;

  // local objects
  CapeList parts = NULL;
  CapeList paths = NULL;

  if (filepath == NULL)
  {
    goto exit_and_cleanup;
  }

  // split the path into its parts
  parts = cape_tokenizer_buf (filepath, cape_str_size (filepath), CAPE_FS_FOLDER_SEP);
  if (parts)
  {
    paths = cape_list_new (NULL);

    // normalize the parts (no empty parts)
    cape_fs_path_rebuild__norm (parts);

    if (cape_fs_path_rebuild__stack (parts, paths, err))
    {
      goto exit_and_cleanup;
    }

    ret = cape_fs_path_rebuild__build (paths);
  }

exit_and_cleanup:

  cape_list_del (&parts);
  cape_list_del (&paths);
  return ret;
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

CapeString cape_fs_filename (const CapeString source)
{
  CapeString ret = NULL;
  
  if (source)
  {
    const CapeString filename = cape_fs_split (source, NULL);
    
    if (cape_str_not_empty (filename))
    {
      const char* pos = strrchr (filename, '.');
      
      if (pos)
      {
        ret = cape_str_sub (filename, (pos - filename));
      }
      else
      {
        ret = cape_str_cp (filename);
      }
    }
  }
  
  return ret;
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

  // local objects
  CapeString directory = NULL;
  CapeString path_absolute = cape_fs_path_absolute (path);

  // create all path elements
  CapeList tokens = cape_tokenizer_buf (path_absolute, cape_str_size (path_absolute), CAPE_FS_FOLDER_SEP);

  // iterate through those elements
  CapeListCursor* cursor = cape_list_cursor_create (tokens, CAPE_DIRECTION_FORW);
  while (cape_list_cursor_next (cursor))
  {
    const CapeString part = cape_list_node_data (cursor->node);

    if (directory)
    {
      // advance which each part
      CapeString h = cape_fs_path_merge (directory, part);

      // replace directory (memory safe)
      cape_str_replace_mv (&directory, &h);

      //printf ("PART: %s\n", directory);

      res = cape_fs_path_create (directory, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
    else
    {
      directory = cape_str_cp (part);
    }
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_list_cursor_destroy (&cursor);
  cape_list_del (&tokens);

  cape_str_del (&path_absolute);
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

//--------------------------------------------------------------------------------

int cape_fs_path_rm__os (const char* path, CapeErr err)
{
#ifdef __WINDOWS_OS

  return (RemoveDirectory (path) == 0) ? cape_err_lastOSError (err) : CAPE_ERR_NONE;

#elif defined __LINUX_OS || defined __BSD_OS

  return (rmdir (path) == 0) ? CAPE_ERR_NONE : cape_err_lastOSError (err);

#endif
}

//-----------------------------------------------------------------------------

int cape_fs_path_rm (const char* path, int force_on_none_empty, CapeErr err)
{
  int res;

  // local objects
  CapeDirCursor c = NULL;

  if (force_on_none_empty)
  {
    c = cape_dc_new (path, err);
    if (c == NULL)
    {
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "path rm", "can't find directory '%s': %s", path, cape_err_text (err));

      res = cape_err_code (err);
      goto exit_and_cleanup;
    }

    while (cape_dc_next (c))
    {
      if (cape_dc_level (c))
      {
        // determine the node type
        switch (cape_dc_type (c))
        {
          case CAPE_DC_TYPE__FILE:
          case CAPE_DC_TYPE__LINK:
          {
            const CapeString path_child = cape_dc_path (c);

            //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "path rm", "remove file: '%s'", path_child);

            res = cape_fs_file_rm (path_child, err);
            if (res)
            {
              cape_log_fmt (CAPE_LL_ERROR, "CAPE", "path rm", "can't remove file '%s': %s", path_child, cape_err_text (err));
              goto exit_and_cleanup;
            }

            break;
          }
          case CAPE_DC_TYPE__DIR:
          {
            const CapeString path_child = cape_dc_path (c);

            //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "path rm", "remove dir: '%s'", path_child);

            res = cape_fs_path_rm__os (path_child, err);
            if (res)
            {
              cape_log_fmt (CAPE_LL_ERROR, "CAPE", "path rm", "can't remove dir '%s': %s", path_child, cape_err_text (err));
              goto exit_and_cleanup;
            }

            break;
          }
        }
      }
    }
  }

  res = cape_fs_path_rm__os (path, err);

exit_and_cleanup:

  cape_dc_del (&c);
  return res;
}

//-----------------------------------------------------------------------------

int cape_fs_file_rm (const char* path, CapeErr err)
{
#ifdef __WINDOWS_OS

  if (0 != _unlink (path))
  {
    return cape_err_lastOSError (err);
  }

#elif defined __LINUX_OS || defined __BSD_OS

  if (-1 == unlink (path))
  {
    return cape_err_lastOSError (err);
  }

#endif

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_fs_file_mv__cp_rm (const char* source, const char* destination, CapeErr err)
{
  int res;

  // first copy the file
  res = cape_fs_file_cp (source, destination, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = cape_fs_file_rm (source, err);

exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int cape_fs_file_mv (const char* source, const char* destination, CapeErr err)
{
#ifdef __WINDOWS_OS

  if (!MoveFile (source, destination))
  {
    return cape_err_lastOSError (err);
  }

#elif defined __LINUX_OS || defined __BSD_OS

  if (0 != rename (source, destination))
  {
    int err_no = errno;

    // check for a specific error
    // -> the source and target files are not on the same filesystem
    // -> we still can try to copy and remove the file in sequence
    if (err_no == EXDEV)
    {
      return cape_fs_file_mv__cp_rm (source, destination, err);
    }
    else
    {
      return cape_err_lastOSError (err);
    }
  }

#endif

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_fs_file_cp (const char* source, const char* destination, CapeErr err)
{
#ifdef __WINDOWS_OS

  if (!CopyFile (source, destination, TRUE))
  {
    return cape_err_lastOSError (err);
  }

  return CAPE_ERR_NONE;

#elif defined __APPLE_CC__

  if (copyfile (source, destination, 0, COPYFILE_ACL | COPYFILE_XATTR | COPYFILE_DATA) != 0)
  {
    return cape_err_lastOSError (err);
  }

  return CAPE_ERR_NONE;

#else

  int res;

  // local objects
  char* buffer = NULL;
  CapeFileHandle fh_s = cape_fh_new (NULL, source);
  CapeFileHandle fh_d = cape_fh_new (NULL, destination);
  CapeFileAc ac = NULL;
  
  res = cape_fh_open (fh_s, O_RDONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  ac = cape_fs_file_ac (source, err);
  if (ac == NULL)
  {
    goto exit_and_cleanup;
  }
  
  res = cape_fh_open_ac (fh_d, O_WRONLY | O_CREAT, &ac, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  buffer = CAPE_ALLOC (CAPE_BUFFER_SIZE);

  while (TRUE)
  {
    number_t bytes_read = cape_fh_read_buf (fh_s, buffer, CAPE_BUFFER_SIZE);

    if (bytes_read > 0)
    {
      number_t bytes_written = 0;

      while (bytes_written < bytes_read)
      {
        bytes_written = cape_fh_write_buf (fh_d, buffer + bytes_written, bytes_read - bytes_written);

        if (bytes_written <= 0)
        {
          res = cape_err_lastOSError (err);
          goto exit_and_cleanup;
        }
      }
    }
    else
    {
      break;
    }
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_fs_ac_del (&ac);

  cape_fh_del (&fh_d);
  cape_fh_del (&fh_s);

  if (buffer)
  {
    CAPE_FREE (buffer);
  }

  return res;

#endif
}

//-----------------------------------------------------------------------------

off_t cape_fs_file_size (const char* path, CapeErr err)
{
#ifdef __WINDOWS_OS

  off_t ret = 0;
  LARGE_INTEGER lFileSize;

  // local objects
  HANDLE hf = CreateFile (path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);

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

struct CapeFileAc_s
{
#ifdef __WINDOWS_OS
  
  
  
#elif defined __LINUX_OS || defined __BSD_OS
  
  mode_t permissions;
  uid_t uid;
  gid_t gid; 
  
#endif    
};

//-----------------------------------------------------------------------------

void cape_fs_ac_del (CapeFileAc* p_self)
{
  if (*p_self)
  {
    //CapeFileAc self = *p_self;
    
    
    CAPE_DEL (p_self, struct CapeFileAc_s);
  }
}

//-----------------------------------------------------------------------------

CapeFileAc cape_fs_file_ac (const char* path, CapeErr err)
{
#ifdef __WINDOWS_OS
  
  
  
#elif defined __LINUX_OS || defined __BSD_OS
  
  struct stat st;
  
  if (stat (path, &st) == -1)
  {
    cape_err_lastOSError (err);
    return NULL;
  }

  {
    CapeFileAc self = CAPE_NEW (struct CapeFileAc_s);
    
    self->permissions = st.st_mode;
    self->uid = st.st_uid;
    self->gid = st.st_gid;
    
    return self;
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
  char* buffer = NULL;

  // try to open
  res = cape_fh_open (fh, O_RDONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  {
    buffer = CAPE_ALLOC (CAPE_BUFFER_SIZE);

    for (bytes_read = cape_fh_read_buf (fh, buffer, CAPE_BUFFER_SIZE); bytes_read > 0; bytes_read = cape_fh_read_buf (fh, buffer, CAPE_BUFFER_SIZE))
    {
      res = fct (ptr, buffer, bytes_read, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }

exit_and_cleanup:

  if (buffer)
  {
    CAPE_FREE (buffer);
  }

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
  return cape_fh_open_ex (self, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, err);
}

//-----------------------------------------------------------------------------

int cape_fh_open_ex (CapeFileHandle self, int flags, int permissions, CapeErr err)
{
  self->fd = open (self->file, flags, permissions);
  
  if (self->fd == -1)
  {
    return cape_err_lastOSError (err);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_fh_open_ac (CapeFileHandle self, int flags, CapeFileAc* p_ac, CapeErr err)
{
  int res;
  CapeFileAc ac = *p_ac;
  
//  uid_t euid;
//  gid_t egid;
  
  // retrieve the effective uid and gid
//  euid = geteuid ();
//  egid = geteuid ();
  
  // set a new effective UID
/*  if (seteuid (ac->uid) == -1)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "open ac", "can't set user id = %lu", ac->uid);

    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }

  // set a new effective GID
  if (setegid (egid) == -1)
  {
    seteuid (euid);
    
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "open ac", "can't set group id = %lu", ac->gid);
    
    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }
*/  
  res = cape_fh_open_ex (self, flags, ac->permissions, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  /*
  // set the old effective UID
  if (seteuid (euid) == -1)
  {
    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }
  
  // set a new effective GID
  if (setegid (egid) == -1)
  {
    res = cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }
  */
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_fs_ac_del (p_ac);
  return res;
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

  self->fts_path[0] = cape_fs_path_merge (path, "");  // always add a dir seperator at the end
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
    cape_str_del (&(self->current_dir));

    CAPE_DEL(p_self, struct CapeDirCursor_s);
  }
}

//-----------------------------------------------------------------------------

int cape_dc_next (CapeDirCursor self)
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

const CapeString cape_dc_path (CapeDirCursor self)
{
  if (self->node)
  {
    return self->node->fts_path;
  }
  
  return NULL;
}


//-----------------------------------------------------------------------------

off_t cape_dc_size (CapeDirCursor self)
{
  if (self->node)
  {
    if (self->node->fts_info == FTS_F)
    {
      return self->node->fts_statp->st_size;
    }
  }

  return 0;
}

//-----------------------------------------------------------------------------

number_t cape_dc_level (CapeDirCursor self)
{
  if (self->node)
  {
    return self->node->fts_level;
  }

  return 0;
}

//-----------------------------------------------------------------------------

number_t cape_dc_type (CapeDirCursor self)
{
  if (self->node)
  {
    unsigned short fts_info = self->node->fts_info;

//    printf ("%i FTS_DEFAULT=%i, FTS_F=%i, FTS_SL=%i, FTS_DP=%i\n", fts_info, fts_info & FTS_DEFAULT, fts_info & FTS_F, fts_info & FTS_SL, fts_info & FTS_DP);

    switch (self->node->fts_info)
    {
      case FTS_DP:
      {
        return CAPE_DC_TYPE__DIR;
      }
      case FTS_F:
      {
        return CAPE_DC_TYPE__FILE;
      }
      case FTS_SL:
      {
        return CAPE_DC_TYPE__LINK;
      }
    }
  }

  return CAPE_DC_TYPE__NONE;
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
#include <sys/types.h>

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

const CapeString cape_fh_file (CapeFileHandle self)
{
  return self->file;
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

number_t cape_dc_level (CapeDirCursor self)
{
  return 1;
}

//-----------------------------------------------------------------------------

number_t cape_dc_type (CapeDirCursor self)
{
  long attr = self->data.dwFileAttributes;

  if (attr & FILE_ATTRIBUTE_DIRECTORY)
  {
    return CAPE_DC_TYPE__DIR;
  }

  if (attr & FILE_ATTRIBUTE_ARCHIVE)
  {
    return CAPE_DC_TYPE__FILE;
  }

  return CAPE_DC_TYPE__NONE;
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
