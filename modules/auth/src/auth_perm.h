#ifndef __AUTH__PERM__H
#define __AUTH__PERM__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// adbl2 includes
#include <adbl.h>

// module includes
#include "auth_vault.h"

//-----------------------------------------------------------------------------

struct AuthPerm_s; typedef struct AuthPerm_s* AuthPerm;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AuthPerm     auth_perm_new           (AdblSession adbl_session, AuthVault vault);

__CAPE_LIBEX   void         auth_perm_del           (AuthPerm*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int          auth_perm_set           (AuthPerm*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          auth_perm_get           (AuthPerm*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          auth_perm_code_set      (AuthPerm*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          auth_perm_rm            (AuthPerm*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   int          auth_perm__helper__get  (AdblSession adbl_session, AuthVault vault, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

#endif


