#ifndef __QWEBS__H
#define __QWEBS__H 1

#include "qwebs_structs.h"

//-----------------------------------------------------------------------------

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include <aio/cape_aio_ctx.h>
#include <sys/cape_err.h>
#include <stc/cape_str.h>
#include <stc/cape_list.h>
#include <stc/cape_udc.h>

struct QWebsApi_s; typedef struct QWebsApi_s* QWebsApi;

//-----------------------------------------------------------------------------

__CAPE_LIBEX     QWebs              qwebs_new        (CapeUdc sites, const CapeString host, number_t port, number_t threads, const CapeString pages, CapeUdc route_list);

__CAPE_LIBEX     void               qwebs_del        (QWebs*);

//-----------------------------------------------------------------------------

typedef int     (__STDCALL *fct_qwebs__on_request)   (void* user_ptr, QWebsRequest request, CapeErr err);

__CAPE_LIBEX     int                qwebs_reg        (QWebs, const CapeString name, void* user_ptr, fct_qwebs__on_request, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     int                qwebs_attach     (QWebs, CapeAioContext, CapeErr err);

__CAPE_LIBEX     QWebsApi           qwebs_get_api    (QWebs, const CapeString name);

__CAPE_LIBEX     QWebsFiles         qwebs_files      (QWebs);

__CAPE_LIBEX     const CapeString   qwebs_pages      (QWebs);

__CAPE_LIBEX     int                qwebs_route      (QWebs, const CapeString name);

__CAPE_LIBEX     const CapeString   qwebs_site       (QWebs, const char *bufdat, size_t buflen, CapeString* url);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     int                qwebs_api_call   (QWebsApi, QWebsRequest request, CapeErr err);

//-----------------------------------------------------------------------------

#endif
