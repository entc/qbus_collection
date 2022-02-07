#ifndef __AUTH__ROLES__H
#define __AUTH__ROLES__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// adbl2 includes
#include <adbl.h>

//-----------------------------------------------------------------------------

struct AuthRoles_s; typedef struct AuthRoles_s* AuthRoles;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AuthRoles  auth_roles_new         (QBus qbus, AdblSession adbl_session);

__CAPE_LIBEX   void       auth_roles_del         (AuthRoles*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int        auth_roles_get         (AuthRoles*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int        auth_roles_ui_get      (AuthRoles*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int        auth_roles_wp_get      (AuthRoles*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

#endif
