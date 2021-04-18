#ifndef __AUTH__SESSION__H
#define __AUTH__SESSION__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// adbl2 includes
#include <adbl.h>

// module includes
#include "auth_vault.h"

//-----------------------------------------------------------------------------

struct AuthSession_s; typedef struct AuthSession_s* AuthSession;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AuthSession  auth_session_new         (AdblSession adbl_session, AuthVault vault);

__CAPE_LIBEX   void         auth_session_del         (AuthSession*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int          auth_session_add         (AuthSession*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          auth_session_get         (AuthSession*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          auth_session_roles       (AuthSession*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

#endif


