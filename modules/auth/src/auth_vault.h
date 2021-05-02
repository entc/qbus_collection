#ifndef __AUTH__VAULT__H
#define __AUTH__VAULT__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"

#include "qbus.h"

//-----------------------------------------------------------------------------

struct AuthVault_s; typedef struct AuthVault_s* AuthVault;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AuthVault    auth_vault_new    (void);

__CAPE_LIBEX   void         auth_vault_del    (AuthVault*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int          auth_vault_set    (AuthVault, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int          auth_vault_get    (AuthVault, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const CapeString   auth_vault__vsec   (AuthVault, number_t wpid);

__CAPE_LIBEX   void               auth_vault__save   (AuthVault, number_t wpid, const CapeString vsec);

//-----------------------------------------------------------------------------

#endif

