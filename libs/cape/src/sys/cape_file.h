#ifndef __CAPE_SYS__FILE__H
#define __CAPE_SYS__FILE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "sys/cape_types.h"

#include <fcntl.h>

//=============================================================================

#ifdef __WINDOWS_OS
#define CAPE_FS_FOLDER_SEP   '\\'
#else
#define CAPE_FS_FOLDER_SEP   '/'
#endif

//=============================================================================

                                  /* constructs a path from 2 paths given using the system separator */
__CAPE_LIBEX   CapeString         cape_fs_path_merge     (const char* path1, const char* path2);

                                  /* constructs a path from 3 paths given using the system separator */
__CAPE_LIBEX   CapeString         cape_fs_path_merge_3   (const char* path1, const char* path2, const char* path3);

                                  /* reduce the path from the end */
__CAPE_LIBEX   CapeString         cape_fs_path_reduce    (const char* path);

                                  /* constructs the current path gathering from the system.
                                   -> if a filepath is given it will be appended to the current path */
__CAPE_LIBEX   CapeString         cape_fs_path_current   (const char* filepath);

                                  /* constructs a path weather it is absolute or relative always to an absolute path.
                                   -> if filepath is NULL the current path will be returned */
__CAPE_LIBEX   CapeString         cape_fs_path_absolute  (const char* filepath);

                                  /* resolves a path using the system functions */
__CAPE_LIBEX   CapeString         cape_fs_path_resolve   (const char* filepath, CapeErr);

                                  /* resolves a path by algorithm */
__CAPE_LIBEX   CapeString         cape_fs_path_rebuild   (const char* filepath, CapeErr);

//-----------------------------------------------------------------------------

                                  /* splits the filepath into path and filename parts,
                                   -> returns the filename, sets the content of p_path if not NULL
                                   -> returns always absolute paths */
__CAPE_LIBEX   const CapeString   cape_fs_split          (const char* filepath, CapeString* p_path);

                                  /* returns the extension of a filename given */
__CAPE_LIBEX   const CapeString   cape_fs_extension      (const CapeString);

                                  /* create a copy of the filename without extension */
__CAPE_LIBEX   CapeString         cape_fs_filename       (const CapeString);

//-----------------------------------------------------------------------------

struct CapeFileAc_s; typedef struct CapeFileAc_s* CapeFileAc;

#ifdef __WINDOWS_OS


#else

__CAPE_LIBEX   CapeFileAc         cape_fs_ac_new         (uid_t uid, gid_t gid, mode_t mod);

#endif

__CAPE_LIBEX   void               cape_fs_ac_del         (CapeFileAc*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                cape_fs_path_create    (const char* path, CapeErr);

__CAPE_LIBEX   int                cape_fs_path_create_x  (const char* path, CapeErr);

                                  /* creates a path like cape_fs_path_create only if it not exists */
__CAPE_LIBEX   int                cape_fs_path_create_e  (const char* path, CapeErr);

                                  /* creates a path like cape_fs_path_create_x only if it not exists */
__CAPE_LIBEX   int                cape_fs_path_create_xe (const char* path, CapeErr);

__CAPE_LIBEX   off_t              cape_fs_path_size      (const char* path, CapeErr);

__CAPE_LIBEX   int                cape_fs_path_exists    (const char* path);

__CAPE_LIBEX   int                cape_fs_path_rm        (const char* path, int force_on_none_empty, CapeErr);

__CAPE_LIBEX   int                cape_fs_path_cp        (const char* source, const char* destination, CapeErr);

__CAPE_LIBEX   int                cape_fs_path_ln        (const char* source, const char* destination, const char* path, CapeErr);

//-----------------------------------------------------------------------------

                                  /*
                                  removes a file from the filesystem, returns a cape error
                                   */
__CAPE_LIBEX   int                cape_fs_file_rm        (const char* path, CapeErr);

                                  /*
                                  moves a file, returns a cape error
                                   */
__CAPE_LIBEX   int                cape_fs_file_mv        (const char* source, const char* destination, CapeErr);

                                  /*
                                  copies a file, returns a cape error
                                   */
__CAPE_LIBEX   int                cape_fs_file_cp        (const char* source, const char* destination, CapeErr);

                                  /*
                                  copies a file with AC definitions, returns a cape error
                                   */
__CAPE_LIBEX   int                cape_fs_file_cp__ac    (const char* source, const char* destination, CapeFileAc*, CapeErr);

                                  /*
                                  returns the size of a file, please check err for errors
                                   */
__CAPE_LIBEX   off_t              cape_fs_file_size      (const char* path, CapeErr);

                                  /*
                                   returns file permissions or ACLs, including UID and GID
                                   */
__CAPE_LIBEX   CapeFileAc         cape_fs_file_ac_get    (const char* path, CapeErr);


__CAPE_LIBEX   int                cape_fs_file_ac_set    (const char* path, CapeFileAc*, CapeErr);

//-----------------------------------------------------------------------------

typedef int  (__STDCALL *fct_cape_fs_file_load)   (void* ptr, const char* bufdat, number_t buflen, CapeErr err);

__CAPE_LIBEX   int                cape_fs_file_load      (const CapeString path, const CapeString file, void* ptr, fct_cape_fs_file_load, CapeErr);

//=============================================================================

struct CapeFileHandle_s; typedef struct CapeFileHandle_s* CapeFileHandle;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeFileHandle     cape_fh_new            (const CapeString path, const CapeString file);

__CAPE_LIBEX   void               cape_fh_del            (CapeFileHandle*);

__CAPE_LIBEX   int                cape_fh_open           (CapeFileHandle, int flags, CapeErr);

__CAPE_LIBEX   int                cape_fh_open_ex        (CapeFileHandle, int flags, int ac, CapeErr);

__CAPE_LIBEX   int                cape_fh_open_ac        (CapeFileHandle, int flags, CapeFileAc*, CapeErr);

__CAPE_LIBEX   void*              cape_fh_fd             (CapeFileHandle);

__CAPE_LIBEX   number_t           cape_fh_read_buf       (CapeFileHandle, char* bufdat, number_t buflen);

__CAPE_LIBEX   number_t           cape_fh_write_buf      (CapeFileHandle, const char* bufdat, number_t buflen);

__CAPE_LIBEX   const CapeString   cape_fh_file           (CapeFileHandle);

//-----------------------------------------------------------------------------

struct CapeDirCursor_s; typedef struct CapeDirCursor_s* CapeDirCursor;

//-----------------------------------------------------------------------------

#define CAPE_DC_TYPE__NONE     0
#define CAPE_DC_TYPE__FILE     1
#define CAPE_DC_TYPE__LINK     2
#define CAPE_DC_TYPE__DIR      3
#define CAPE_DC_TYPE__ERR      4

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeDirCursor      cape_dc_new            (const CapeString path, CapeErr err);    // returns NULL in case of error

__CAPE_LIBEX   void               cape_dc_del            (CapeDirCursor*);

__CAPE_LIBEX   int                cape_dc_next           (CapeDirCursor);

__CAPE_LIBEX   const CapeString   cape_dc_path           (CapeDirCursor);

__CAPE_LIBEX   const CapeString   cape_dc_name           (CapeDirCursor);

__CAPE_LIBEX   off_t              cape_dc_size           (CapeDirCursor);

__CAPE_LIBEX   number_t           cape_dc_type           (CapeDirCursor);

                                  /*
                                   level = 0 is the path itself
                                   */
__CAPE_LIBEX   number_t           cape_dc_level          (CapeDirCursor);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const void*        cape_fs_stdout         (void);

__CAPE_LIBEX   void               cape_fs_write_msg      (const void* handle, const char* buf, number_t len);

__CAPE_LIBEX   void               cape_fs_writeln_msg    (const void* handle, const char* buf, number_t len);

__CAPE_LIBEX   void               cape_fs_write_fmt      (const void* handle, const char* format, ...);

//-----------------------------------------------------------------------------

#endif
