#ifndef __QWEBS_MULTIPART__H
#define __QWEBS_MULTIPART__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "stc/cape_map.h"

//-----------------------------------------------------------------------------

struct QWebsMultipart_s; typedef struct QWebsMultipart_s* QWebsMultipart;

typedef void  (__STDCALL *qwebs_multipart__on_part)  (void* ptr, const char* bufdat, number_t buflen, CapeMap part_values);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QWebsMultipart    qwebs_multipart_new      (const CapeString boundary, void* user_ptr, qwebs_multipart__on_part);

__CAPE_LIBEX   void              qwebs_multipart_del      (QWebsMultipart*);

__CAPE_LIBEX   void              qwebs_multipart_process  (QWebsMultipart, const char* bufdat, number_t buflen);

//-----------------------------------------------------------------------------

#endif
