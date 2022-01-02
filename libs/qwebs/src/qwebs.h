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
#include <stc/cape_map.h>
#include <stc/cape_udc.h>

#define QWEBS_RAISE_TYPE__MINOR      1
#define QWEBS_RAISE_TYPE__CRITICAL  10

//-----------------------------------------------------------------------------

__CAPE_LIBEX     QWebs              qwebs_new           (CapeUdc sites, const CapeString host, number_t port, number_t threads, const CapeString pages, CapeUdc route_list, const CapeString identifier, const CapeString provider);

__CAPE_LIBEX     void               qwebs_del           (QWebs*);

//-----------------------------------------------------------------------------

typedef int     (__STDCALL *fct_qwebs__on_request)      (void* user_ptr, QWebsRequest request, CapeErr err);

__CAPE_LIBEX     int                qwebs_reg           (QWebs, const CapeString name, void* user_ptr, fct_qwebs__on_request, CapeErr err);

__CAPE_LIBEX     int                qwebs_reg_page      (QWebs, const CapeString page, void* user_ptr, fct_qwebs__on_request, CapeErr err);

//-----------------------------------------------------------------------------

typedef int     (__STDCALL *fct_qwebs__on_raise)        (void* user_ptr, number_t type, QWebsRequest);

__CAPE_LIBEX     void               qwebs_set_raise     (QWebs, void* user_ptr, fct_qwebs__on_raise);

//-----------------------------------------------------------------------------

typedef void*   (__STDCALL *fct_qwebs__on_upgrade)      (void* user_ptr, QWebsRequest, CapeMap return_header, CapeErr err);

__CAPE_LIBEX     int                qwebs_on_upgrade    (QWebs, const CapeString name, void* user_ptr, fct_qwebs__on_upgrade, fct_qwebs__on_recv, fct_qwebs__on_del, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     int                qwebs_attach        (QWebs, CapeAioContext, CapeErr err);

__CAPE_LIBEX     QWebsApi           qwebs_get_api       (QWebs, const CapeString name);

__CAPE_LIBEX     QWebsApi           qwebs_get_page      (QWebs, const CapeString page);

__CAPE_LIBEX     QWebsUpgrade       qwebs_get_upgrade   (QWebs, const CapeString name);

__CAPE_LIBEX     QWebsFiles         qwebs_files         (QWebs);

__CAPE_LIBEX     const CapeString   qwebs_pages         (QWebs);

__CAPE_LIBEX     int                qwebs_route         (QWebs, const CapeString name);

__CAPE_LIBEX     const CapeString   qwebs_site          (QWebs, const char *bufdat, size_t buflen, CapeString* url);

__CAPE_LIBEX     const CapeString   qwebs_identifier    (QWebs);

__CAPE_LIBEX     const CapeString   qwebs_provider      (QWebs);

                                    /* returns TRUE if the file might be critical */
__CAPE_LIBEX     int                qwebs_raise_file    (QWebs, const CapeString file, QWebsRequest);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     CapeString         qwebs_url_encode    (QWebs, const CapeString url);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     int                qwebs_api_call      (QWebsApi, QWebsRequest request, CapeErr err);

__CAPE_LIBEX     int                qwebs_upgrade_call  (QWebsUpgrade, QWebsRequest request, CapeMap return_headers, CapeErr err);

__CAPE_LIBEX     void               qwebs_upgrade_conn  (QWebsUpgrade, QWebsConnection, void* user_ptr);

//-----------------------------------------------------------------------------

#endif
