#ifndef __AUTH__GP__H
#define __AUTH__GP__H 1

#include "auth_vault.h"

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// adbl2 includes
#include <adbl.h>

//-----------------------------------------------------------------------------

struct AuthGP_s; typedef struct AuthGP_s* AuthGP;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AuthGP   auth_gp_new       (AdblSession adbl_session, AuthVault);

__CAPE_LIBEX   void     auth_gp_del       (AuthGP*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int      auth_gp_get       (AuthGP*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int      auth_gp_account   (AuthGP*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int      auth_gp_set       (AuthGP*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int      auth_wp_get       (AuthGP*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   int      auth_wp_add       (AdblTrx trx, const CapeString vsec, number_t wpid, CapeUdc gpdata, number_t* p_gpid, CapeErr err);

//-----------------------------------------------------------------------------

#endif
