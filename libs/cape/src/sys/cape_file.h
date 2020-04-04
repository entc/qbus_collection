#ifndef __CAPE_SYS__FILE__H
#define __CAPE_SYS__FILE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "sys/cape_types.h"

#include <fcntl.h>

//=============================================================================

__CAPE_LIBEX   CapeString         cape_fs_path_merge     (const char* path1, const char* path2);

__CAPE_LIBEX   CapeString         cape_fs_path_merge_3   (const char* path1, const char* path2, const char* path3);

__CAPE_LIBEX   CapeString         cape_fs_path_current   (const char* filepath);

__CAPE_LIBEX   CapeString         cape_fs_path_absolute  (const char* filepath);

__CAPE_LIBEX   CapeString         cape_fs_path_resolve   (const char* filepath, CapeErr);

//-----------------------------------------------------------------------------

// splits a filepath into path and filename, returns the filename, sets the path if not NULL
// returns always absolute paths
__CAPE_LIBEX   const CapeString   cape_fs_split          (const char* filepath, CapeString* p_path);

__CAPE_LIBEX   const CapeString   cape_fs_extension      (const CapeString);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                cape_fs_path_create    (const char* path, CapeErr);

__CAPE_LIBEX   int                cape_fs_path_create_x  (const char* path, CapeErr);

__CAPE_LIBEX   off_t              cape_fs_path_size      (const char* path, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                cape_fs_file_del       (const char* path, CapeErr);

//-----------------------------------------------------------------------------

typedef int  (__STDCALL *fct_cape_fs_file_load)   (void* ptr, const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX   int                cape_fs_file_load      (const CapeString path, const CapeString file, void* ptr, fct_cape_fs_file_load, CapeErr);

//=============================================================================

struct CapeFileHandle_s; typedef struct CapeFileHandle_s* CapeFileHandle;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeFileHandle     cape_fh_new            (const CapeString path, const CapeString file);

__CAPE_LIBEX   void               cape_fh_del            (CapeFileHandle*);

__CAPE_LIBEX   int                cape_fh_open           (CapeFileHandle, int flags, CapeErr);

__CAPE_LIBEX   void*              cape_fh_fd             (CapeFileHandle);

__CAPE_LIBEX   number_t           cape_fh_read_buf       (CapeFileHandle, char* bufdat, number_t buflen);

__CAPE_LIBEX   number_t           cape_fh_write_buf      (CapeFileHandle, const char* bufdat, number_t buflen);

__CAPE_LIBEX   const CapeString   cape_fh_file           (CapeFileHandle);

//-----------------------------------------------------------------------------

struct CapeDirCursor_s; typedef struct CapeDirCursor_s* CapeDirCursor;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeDirCursor      cape_dc_new            (const CapeString path, CapeErr err);    // returns NULL in case of error

__CAPE_LIBEX   void               cape_dc_del            (CapeDirCursor*);

__CAPE_LIBEX   int                cape_dc_next           (CapeDirCursor);

__CAPE_LIBEX   const CapeString   cape_dc_name           (CapeDirCursor);

__CAPE_LIBEX   off_t              cape_dc_size           (CapeDirCursor);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const void*        cape_fs_stdout         (void);

__CAPE_LIBEX   void               cape_fs_write_msg      (const void* handle, const char* buf, number_t len);

__CAPE_LIBEX   void               cape_fs_writeln_msg    (const void* handle, const char* buf, number_t len);

__CAPE_LIBEX   void               cape_fs_write_fmt      (const void* handle, const char* format, ...);

//-----------------------------------------------------------------------------

#endif
