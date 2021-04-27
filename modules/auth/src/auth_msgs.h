#ifndef __AUTH__MSGS__H
#define __AUTH__MSGS__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// adbl2 includes
#include <adbl.h>

// module includes
#include "auth_vault.h"

//-----------------------------------------------------------------------------

struct AuthMsgs_s; typedef struct AuthMsgs_s* AuthMsgs;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AuthMsgs     auth_msgs_new         (AdblSession adbl_session, AuthVault vault);

__CAPE_LIBEX   void         auth_msgs_del         (AuthMsgs*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int          auth_msgs_get         (AuthMsgs*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          auth_msgs_add         (AuthMsgs*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          auth_msgs_set         (AuthMsgs*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          auth_msgs_rm          (AuthMsgs*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

#endif


