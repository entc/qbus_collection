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

#define AUTH_STATE__NONE             0
#define AUTH_STATE__FIRST_USE        1
#define AUTH_STATE__NORMAL           2

//-----------------------------------------------------------------------------

struct AuthUI_s; typedef struct AuthUI_s* AuthUI;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AuthUI  auth_ui_new         (QBus qbus, AdblSession adbl_session, AuthTokens tokens, AuthVault vault, CapeUdc options_2factor, CapeUdc options_fp, number_t wpid_default);

__CAPE_LIBEX   void    auth_ui_del         (AuthUI*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int     auth_ui_get         (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_set         (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_add         (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_pp_get      (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_pp_put      (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_login       (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_login_get   (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_login_logs  (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_switch      (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_config_get  (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_config_set  (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_2f_send     (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_fp_send     (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_users       (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int     auth_ui_rm          (AuthUI*, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   int     auth_ui__save_log_entry (AdblTrx adbl_trx, number_t wpid, number_t gpid, number_t usid, CapeUdc rinfo, CapeUdc cdata, number_t status, CapeErr err);

//-----------------------------------------------------------------------------

#endif
