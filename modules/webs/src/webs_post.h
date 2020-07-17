#ifndef __WEBS__POST__H
#define __WEBS__POST__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// qwebs includes
#include "qwebs.h"

//-----------------------------------------------------------------------------

struct WebsPost_s; typedef struct WebsPost_s* WebsPost;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   WebsPost   webs_post_new       (QBus qbus, QWebsRequest request);

__CAPE_LIBEX   void       webs_post_del       (WebsPost*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int        webs_post_run       (WebsPost*, CapeErr err);

//-----------------------------------------------------------------------------

#endif
