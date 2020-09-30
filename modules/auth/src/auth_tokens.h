#ifndef __AUTH__TOKENS__H
#define __AUTH__TOKENS__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// adbl2 includes
#include <adbl.h>

//-----------------------------------------------------------------------------

struct AuthTokens_s; typedef struct AuthTokens_s* AuthTokens;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AuthTokens   auth_tokens_new    (AdblSession adbl_session);

__CAPE_LIBEX   void         auth_tokens_del    (AuthTokens*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int          auth_tokens_add    (AuthTokens, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          auth_tokens_get    (AuthTokens, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          auth_tokens_rm     (AuthTokens, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int          auth_tokens_fetch  (AuthTokens, const CapeString token, QBusM qin, CapeErr err);

//-----------------------------------------------------------------------------

#endif


