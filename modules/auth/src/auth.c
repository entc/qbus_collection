#include "qbus.h" 

#include "auth_ui.h"
#include "auth_gp.h"
#include "auth_vault.h"
#include "auth_tokens.h"
#include "auth_perm.h"
#include "auth_session.h"
#include "auth_msgs.h"

#include <adbl.h>

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-------------------------------------------------------------------------------------

struct AuthContext_s
{
  AuthTokens tokens;
  
  AuthVault vault;
  
  AdblCtx adbl_ctx;
  
  AdblSession adbl_session;
  
  CapeString err_log_file;
  
  CapeUdc options_2factor;
  
}; typedef struct AuthContext_s* AuthContext;

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_rinfo_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  if (qin->rinfo)
  {
    // transfer ownership
    qout->cdata = qin->rinfo;
    qin->rinfo = NULL;
    
    // add type
    cape_udc_add_s_cp (qout->cdata, "type", "rinfo");
    
    return CAPE_ERR_NONE;
  }
  
  return cape_err_set (err, CAPE_ERR_NO_AUTH, "no authentication");
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;

  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->err_log_file, ctx->options_2factor);
  
  // run the command
  return auth_ui_get (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_login (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->err_log_file, ctx->options_2factor);
  
  // run the command
  return auth_ui_login (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_login_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->err_log_file, ctx->options_2factor);
  
  // run the command
  return auth_ui_login_get (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_switch (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->err_log_file, ctx->options_2factor);
  
  // run the command
  return auth_ui_switch (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->err_log_file, ctx->options_2factor);
  
  // run the command
  return auth_ui_set (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_config_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->err_log_file, ctx->options_2factor);
  
  // run the command
  return auth_ui_config_get (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_config_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->err_log_file, ctx->options_2factor);
  
  // run the command
  return auth_ui_config_set (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_2f_send (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->err_log_file, ctx->options_2factor);
  
  // run the command
  return auth_ui_2f_send (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth__gp_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthGP auth_gp = auth_gp_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_gp_get (&auth_gp, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth__account_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthGP auth_gp = auth_gp_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_gp_account (&auth_gp, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth__gp_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthGP auth_gp = auth_gp_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_gp_set (&auth_gp, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth__msgs_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthMsgs auth_msgs = auth_msgs_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_msgs_get (&auth_msgs, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth__msgs_add (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthMsgs auth_msgs = auth_msgs_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_msgs_add (&auth_msgs, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth__msgs_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthMsgs auth_msgs = auth_msgs_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_msgs_set (&auth_msgs, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth__msgs_rm (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthMsgs auth_msgs = auth_msgs_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_msgs_rm (&auth_msgs, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_token_add (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;

  return auth_tokens_add (ctx->tokens, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_token_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;

  return auth_tokens_get (ctx->tokens, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_token_del (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;

  return auth_tokens_rm (ctx->tokens, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_perm_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthPerm auth_perm = auth_perm_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_perm_set (&auth_perm, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_perm_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthPerm auth_perm = auth_perm_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_perm_get (&auth_perm, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_perm_code_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthPerm auth_perm = auth_perm_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_perm_code_set (&auth_perm, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_perm_code_rm (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthPerm auth_perm = auth_perm_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_perm_rm (&auth_perm, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_session_add (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthSession auth_session = auth_session_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_session_add (&auth_session, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_session_roles (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthSession auth_session = auth_session_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_session_roles (&auth_session, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_session_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthSession auth_session = auth_session_new (ctx->adbl_session, ctx->vault);
  
  // run the command
  return auth_session_get (&auth_session, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_vault_open (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;

  return auth_vault_set (ctx->vault, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_vault_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  return auth_vault_get (ctx->vault, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  AdblCtx adbl_ctx = NULL;
  AdblSession adbl_session = NULL;
  
  AuthContext ctx;

  adbl_ctx = adbl_ctx_new ("adbl", "adbl2_mysql", err);
  if (adbl_ctx == NULL)
  {
    goto exit_and_cleanup;
  }

  adbl_session = adbl_session_open_file (adbl_ctx, "adbl_default.json", err);
  if (adbl_session == NULL)
  {
    goto exit_and_cleanup;
  }

  // create the context
  ctx = CAPE_NEW (struct AuthContext_s);
  
  ctx->adbl_ctx = adbl_ctx;
  ctx->adbl_session = adbl_session;
  
  ctx->vault = auth_vault_new ();
  ctx->tokens = auth_tokens_new (ctx->adbl_session, ctx->vault);
  
  ctx->err_log_file = cape_str_cp (qbus_config_s (qbus, "access_denied_log", "access_denied.log"));
  ctx->options_2factor = qbus_config_node (qbus, "options_2factor");
  
  *p_ptr = ctx;

  // -------- callback methods --------------------------------------------

  // REST interface
  qbus_register (qbus, "GET"                  , ctx, qbus_auth_rinfo_get, NULL, err);

  // -------- callback methods --------------------------------------------

  // a fresh login
  //   args: wpid
  qbus_register (qbus, "ui_login"             , ctx, qbus_auth_ui_login, NULL, err);

  // a fresh login
  //   args: wpid
  qbus_register (qbus, "ui_login_get"         , ctx, qbus_auth_ui_login_get, NULL, err);

  // all user credential functions
  qbus_register (qbus, "getUI"                , ctx, qbus_auth_ui_get, NULL, err);

  // change a user (needs admin roles)
  //   args: gpid
  qbus_register (qbus, "ui_switch"            , ctx, qbus_auth_ui_switch, NULL, err);

  // change password for an user account
  //   args: usid, password
  qbus_register (qbus, "ui_set"               , ctx, qbus_auth_ui_set, NULL, err);

  // retrieve the config of the ui system
  //   args:
  qbus_register (qbus, "ui_config_get"        , ctx, qbus_auth_ui_config_get, NULL, err);

  // set the config
  //   args: opt_msgs_active, opt_2factor
  qbus_register (qbus, "ui_config_set"        , ctx, qbus_auth_ui_config_set, NULL, err);

  // send the 2factor auth messages
  //   args:
  qbus_register (qbus, "ui_2f_send"           , ctx, qbus_auth_ui_2f_send, NULL, err);

  // -------- callback methods --------------------------------------------

  // all global person functions
  qbus_register (qbus, "globperson_get"       , ctx, qbus_auth__gp_get, NULL, err);

  // get the user account info
  //   -> global person info
  //   -> account info (workspace)
  //   -> secret (encrypted)
  qbus_register (qbus, "account_get"          , ctx, qbus_auth__account_get, NULL, err);

  // set global user
  qbus_register (qbus, "globperson_set"       , ctx, qbus_auth__gp_set, NULL, err);

  // -------- callback methods --------------------------------------------

  // retrieve all recipients related to a person
  //   args: gpid
  qbus_register (qbus, "msgs_get"             , ctx, qbus_auth__msgs_get, NULL, err);

  // add a recipient to a person
  //   args: gpid
  qbus_register (qbus, "msgs_add"             , ctx, qbus_auth__msgs_add, NULL, err);

  // update a recipient to a person
  //   args: gpid
  qbus_register (qbus, "msgs_set"             , ctx, qbus_auth__msgs_set, NULL, err);

  // removes a recipient to a person
  //   args: gpid
  qbus_register (qbus, "msgs_rm"              , ctx, qbus_auth__msgs_rm, NULL, err);

  // -------- callback methods --------------------------------------------

  // all token functions
  qbus_register (qbus, "newToken"             , ctx, qbus_auth_token_add, NULL, err);
  qbus_register (qbus, "getTokenContent"      , ctx, qbus_auth_token_get, NULL, err);
  qbus_register (qbus, "invalidateToken"      , ctx, qbus_auth_token_del, NULL, err);

  // -------- callback methods --------------------------------------------

  // renew a permanent token
  //   args: [code], {roles} : pdata
  qbus_register (qbus, "token_perm_set"       , ctx, qbus_auth_perm_set, NULL, err);

  // get a permanent token
  //   args: token
  qbus_register (qbus, "token_perm_get"       , ctx, qbus_auth_perm_get, NULL, err);

  // get a permanent token
  //   args: token, [code], [active]
  qbus_register (qbus, "perm_code_set"        , ctx, qbus_auth_perm_code_set, NULL, err);

  // remove permanent token
  //   args: token
  qbus_register (qbus, "token_perm_rm"        , ctx, qbus_auth_perm_code_rm, NULL, err);

  // -------- callback methods --------------------------------------------

  // create a new session if the credentials were correct
  //   args:
  qbus_register (qbus, "session_add"          , ctx, qbus_session_add, NULL, err);

  // returns rinfo with a valid session token
  //   args: token
  qbus_register (qbus, "session_get"          , ctx, qbus_session_get, NULL, err);

  // returns roles
  //   args:
  qbus_register (qbus, "session_roles"        , ctx, qbus_session_roles, NULL, err);

  // -------- callback methods --------------------------------------------

  // all vault functions
  qbus_register (qbus, "vault_set"            , ctx, qbus_auth_vault_open, NULL, err);
  qbus_register (qbus, "getVaultSecret"       , ctx, qbus_auth_vault_get, NULL, err);

  // -------- callback methods --------------------------------------------

  return CAPE_ERR_NONE;

exit_and_cleanup:
  
  cape_log_msg (CAPE_LL_ERROR, "AUTH", "init", cape_err_text(err));
  
  if (adbl_session)
  {
    adbl_session_close (&adbl_session);
  }

  if (adbl_ctx)
  {
    adbl_ctx_del (&adbl_ctx);
  }
  
  return cape_err_code(err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_done (QBus qbus, void* ptr, CapeErr err)
{
  AuthContext ctx = ptr;
  
  if (ctx)
  {
    auth_vault_del (&(ctx->vault));
    auth_tokens_del (&(ctx->tokens));
    
    adbl_session_close (&(ctx->adbl_session));
    adbl_ctx_del (&(ctx->adbl_ctx));
    
    CAPE_DEL (&ctx, struct AuthContext_s);
  }
  
  return CAPE_ERR_NONE;
}

//-------------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("AUTH", NULL, qbus_auth_init, qbus_auth_done, argc, argv);
  return 0;
}

//-------------------------------------------------------------------------------------
