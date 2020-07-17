#ifndef __WEBS__JSON__H
#define __WEBS__JSON__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// qwebs includes
#include "qwebs.h"

//-----------------------------------------------------------------------------

struct WebsJson_s; typedef struct WebsJson_s* WebsJson;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   WebsJson   webs_json_new       (QBus qbus, QWebsRequest request);

__CAPE_LIBEX   void       webs_json_del       (WebsJson*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int        webs_json_run       (WebsJson*, CapeErr err);

__CAPE_LIBEX   int        webs_json_run_gen   (WebsJson*, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void       webs_json_set       (WebsJson, const CapeString module, const CapeString method, CapeUdc* p_clist, CapeString* p_token);

//-----------------------------------------------------------------------------

#endif
