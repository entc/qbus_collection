#ifndef __QWEBS_CONNECTION__H
#define __QWEBS_CONNECTION__H 1

#include "qwebs_structs.h"

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include <aio/cape_aio_ctx.h>
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include <sys/cape_queue.h>
#include <stc/cape_stream.h>
#include <stc/cape_list.h>
#include <stc/cape_map.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

__CAPE_LIBEX     QWebsConnection    qwebs_connection_new        (void* handle, CapeQueue, QWebs, const CapeString remote);

__CAPE_LIBEX     void               qwebs_connection_del        (QWebsConnection*);

__CAPE_LIBEX     void               qwebs_connection_attach     (QWebsConnection, CapeAioContext aio_context);

__CAPE_LIBEX     void               qwebs_connection_upgrade    (QWebsConnection, void* user_ptr, fct_qwebs__on_recv, fct_qwebs__on_del);

__CAPE_LIBEX     QWebsRequest       qwebs_connection_factory    (QWebsConnection);

__CAPE_LIBEX     void               qwebs_connection_close      (QWebsConnection);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     void               qwebs_request_send_json     (QWebsRequest*, CapeUdc content, number_t ttl, CapeErr);

__CAPE_LIBEX     void               qwebs_request_send_file     (QWebsRequest*, CapeUdc file_node, CapeErr);

__CAPE_LIBEX     void               qwebs_request_send_image    (QWebsRequest*, const CapeString image, CapeErr);

__CAPE_LIBEX     void               qwebs_request_send_buf      (QWebsRequest*, const CapeString buf, const CapeString mime_type, number_t ttl, CapeErr);

__CAPE_LIBEX     void               qwebs_request_redirect      (QWebsRequest*, const CapeString url);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     void               qwebs_request_stream_init   (QWebsRequest, const CapeString boundry, const CapeString mime_type, CapeErr);

__CAPE_LIBEX     void               qwebs_request_stream_buf    (QWebsRequest, const CapeString boundry, const char* bufdat, number_t buflen, const CapeString mime_type, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     CapeList           qwebs_request_clist         (QWebsRequest);

__CAPE_LIBEX     CapeMap            qwebs_request_headers       (QWebsRequest);

__CAPE_LIBEX     CapeMap            qwebs_request_query         (QWebsRequest);

__CAPE_LIBEX     CapeStream         qwebs_request_body          (QWebsRequest);

__CAPE_LIBEX     const CapeString   qwebs_request_method        (QWebsRequest);

__CAPE_LIBEX     CapeString         qwebs_request_remote        (QWebsRequest);

//-----------------------------------------------------------------------------

#endif
