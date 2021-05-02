#ifndef __AUTH__UI__H
#define __AUTH__UI__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"

// qbus includes
#include "qbus.h"

// adbl2 includes
#include <adbl.h>

// module includes
#include "auth_tokens.h"
#include "auth_vault.h"

//-----------------------------------------------------------------------------

struct AuthUI_s; typedef struct AuthUI_s* AuthUI;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AuthUI  auth_ui_new         (AdblSession adbl_session, AuthTokens tokens, AuthVault vault, const CapeString err_log_file);

__CAPE_LIBEX   void    auth_ui_del         (AuthUI*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int     auth_ui_get         (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_set         (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_login       (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_login_get   (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_switch      (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_config_get  (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_config_set  (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

#endif
