#ifndef __CAPE_SYS__ERR__H
#define __CAPE_SYS__ERR__H 1

#include "sys/cape_export.h"

//=============================================================================

#define CAPE_ERR_NONE                0
#define CAPE_ERR_CONTINUE            1

#define CAPE_ERR_NOT_FOUND           2
#define CAPE_ERR_NOT_SUPPORTED       3
#define CAPE_ERR_RUNTIME             4
#define CAPE_ERR_EOF                 5
#define CAPE_ERR_OS                  6

#define CAPE_ERR_LIB                 7
#define CAPE_ERR_3RDPARTY_LIB        8

#define CAPE_ERR_NO_OBJECT           9
#define CAPE_ERR_NO_ROLE            10
#define CAPE_ERR_NO_AUTH            11

#define CAPE_ERR_PARSER             12
#define CAPE_ERR_MISSING_PARAM      13
#define CAPE_ERR_PROCESS_ABORT      14
#define CAPE_ERR_PROCESS_FAILED     15
#define CAPE_ERR_WRONG_STATE        16
#define CAPE_ERR_WRONG_VALUE        17

#define CAPE_ERR_NO_CONTENT         20
#define CAPE_ERR_PRESENT            21
#define CAPE_ERR_OUT_OF_BOUNDS      22

//=============================================================================

struct CapeErr_s; typedef struct CapeErr_s* CapeErr;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeErr           cape_err_new           (void);             // allocate memory and initialize the object

__CAPE_LIBEX   void              cape_err_del           (CapeErr*);         // release memory

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              cape_err_clr           (CapeErr);

__CAPE_LIBEX   int               cape_err_set           (CapeErr, unsigned long errCode, const char* error_message);

__CAPE_LIBEX   int               cape_err_set_fmt       (CapeErr, unsigned long errCode, const char* error_message, ...);

__CAPE_LIBEX   const char*       cape_err_text          (CapeErr);

__CAPE_LIBEX   unsigned long     cape_err_code          (CapeErr);

__CAPE_LIBEX   int               cape_err_lastOSError   (CapeErr);

__CAPE_LIBEX   int               cape_err_formatErrorOS (CapeErr, unsigned long errCode);

//-----------------------------------------------------------------------------

#endif

