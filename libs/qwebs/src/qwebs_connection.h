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

__CAPE_LIBEX     QWebsConnection    qwebs_connection_new      (void* handle, CapeQueue, QWebs);

__CAPE_LIBEX     void               qwebs_connection_del      (QWebsConnection*);

__CAPE_LIBEX     void               qwebs_connection_attach   (QWebsConnection, CapeAioContext aio_context);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     void               qwebs_request_send_json   (QWebsRequest*, CapeUdc content, CapeErr);

__CAPE_LIBEX     void               qwebs_request_send_file   (QWebsRequest*, CapeUdc file_node, CapeErr);

__CAPE_LIBEX     CapeList           qwebs_request_clist       (QWebsRequest);

__CAPE_LIBEX     CapeMap            qwebs_request_headers     (QWebsRequest);

__CAPE_LIBEX     CapeStream         qwebs_request_body        (QWebsRequest);

__CAPE_LIBEX     const CapeString   qwebs_request_method      (QWebsRequest);

//-----------------------------------------------------------------------------

#endif
