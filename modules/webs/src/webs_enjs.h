#ifndef __WEBS__ENJS__H
#define __WEBS__ENJS__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// qwebs includes
#include "qwebs.h"

//-----------------------------------------------------------------------------

struct WebsEnjs_s; typedef struct WebsEnjs_s* WebsEnjs;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   WebsEnjs   webs_enjs_new       (QBus qbus, QWebsRequest request);

__CAPE_LIBEX   void       webs_enjs_del       (WebsEnjs*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int        webs_enjs_run       (WebsEnjs*, CapeErr err);

//-----------------------------------------------------------------------------

#endif
