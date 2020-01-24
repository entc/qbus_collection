#ifndef __CAPE_STC__BUF__H
#define __CAPE_STC__BUF__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"

//=============================================================================

struct CapeBuffer_s; typedef struct CapeBuffer_s* CapeBuffer;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeBuffer        cape_buf_new               (number_t size);

__CAPE_LIBEX   void              cape_buf_del               (CapeBuffer*);

//-----------------------------------------------------------------------------

/* use the given memory fragment to initialize the object and data within this fragment
                */
__CAPE_LIBEX   CapeBuffer        cape_buf_reg               (char* data, number_t size);  

//-----------------------------------------------------------------------------

__CAPE_LIBEX   number_t          cape_buf_size              (CapeBuffer);

__CAPE_LIBEX   const char*       cape_buf_data              (CapeBuffer);

//-----------------------------------------------------------------------------

#endif

