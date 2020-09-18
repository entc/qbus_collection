#ifndef __WEBS__IMCA__H
#define __WEBS__IMCA__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// qwebs includes
#include "qwebs.h"

//-----------------------------------------------------------------------------

struct WebsStream_s; typedef struct WebsStream_s* WebsStream;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   WebsStream   webs_stream_new     ();

__CAPE_LIBEX   void         webs_stream_del     (WebsStream*);

__CAPE_LIBEX   void         webs_stream_reset   (WebsStream);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int          webs_stream_add     (WebsStream, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          webs_stream_set     (WebsStream, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          webs_stream_get     (WebsStream, QWebsRequest request, CapeErr err);

//-----------------------------------------------------------------------------

struct WebsImca_s; typedef struct WebsImca_s* WebsImca;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   WebsImca     webs_imca_new       (WebsStream, QWebsRequest request);

__CAPE_LIBEX   void         webs_imca_del       (WebsImca*);

__CAPE_LIBEX   int          webs_imca_run       (WebsImca*, CapeErr err);

//-----------------------------------------------------------------------------

#endif
