#include "qbus.h" 

#include "auth_ui.h"
#include "auth_gp.h"
#include "auth_vault.h"
#include "auth_tokens.h"
#include "auth_perm.h"
#include "auth_session.h"
#include "auth_msgs.h"

// adbl includes
#include <adbl.h>

// qjobs includes
#include <qjobs.h>

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-------------------------------------------------------------------------------------

struct AuthContext_s
{
  // module objects
  AuthTokens tokens;
  AuthVault vault;
  
  // adbl objects
  AdblCtx adbl_ctx;
  AdblSession adbl_session;

  // configuration
  CapeUdc options_2factor;
  CapeUdc options_fp;

  QJobs perm_jobs;
  
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
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->options_2factor, ctx->options_fp);
  
  // run the command
  return auth_ui_get (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_login (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->options_2factor, ctx->options_fp);
  
  // run the command
  return auth_ui_login (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_login_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->options_2factor, ctx->options_fp);
  
  // run the command
  return auth_ui_login_get (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_switch (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->options_2factor, ctx->options_fp);
  
  // run the command
  return auth_ui_switch (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->options_2factor, ctx->options_fp);
  
  // run the command
  return auth_ui_set (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_config_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->options_2factor, ctx->options_fp);
  
  // run the command
  return auth_ui_config_get (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_config_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->options_2factor, ctx->options_fp);
  
  // run the command
  return auth_ui_config_set (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_2f_send (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->options_2factor, ctx->options_fp);
  
  // run the command
  return auth_ui_2f_send (&auth_ui, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_ui_fp_send (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthUI auth_ui = auth_ui_new (qbus, ctx->adbl_session, ctx->tokens, ctx->vault, ctx->options_2factor, ctx->options_fp);
  
  // run the command
  return auth_ui_fp_send (&auth_ui, qin, qout, err);
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

static int __STDCALL qbus_auth_perm_add (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthPerm auth_perm = auth_perm_new (qbus, ctx->adbl_session, ctx->vault, ctx->perm_jobs);
  
  // run the command
  return auth_perm_add (&auth_perm, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_perm_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthPerm auth_perm = auth_perm_new (qbus, ctx->adbl_session, ctx->vault, ctx->perm_jobs);
  
  // run the command
  return auth_perm_set (&auth_perm, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_perm_get (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthPerm auth_perm = auth_perm_new (qbus, ctx->adbl_session, ctx->vault, ctx->perm_jobs);
  
  // run the command
  return auth_perm_get (&auth_perm, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_perm_code_set (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthPerm auth_perm = auth_perm_new (qbus, ctx->adbl_session, ctx->vault, ctx->perm_jobs);
  
  // run the command
  return auth_perm_code_set (&auth_perm, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_perm_code_rm (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  AuthContext ctx = ptr;
  
  // create a temporary object
  AuthPerm auth_perm = auth_perm_new (qbus, ctx->adbl_session, ctx->vault, ctx->perm_jobs);
  
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

int __STDCALL auth_context__on_perm_event (QJobs jobs, QJobsEvent event, void* user_ptr, CapeErr err)
{
  AuthContext self = user_ptr;
  
  if (event->params)
  {
    const CapeString token = cape_udc_get_s (event->params, "token", NULL);
    if (token)
    {
      // create a temporary object
      AuthPerm auth_perm = auth_perm_new (NULL, self->adbl_session, self->vault, self->perm_jobs);
      
      if (auth_perm_remove (auth_perm, token, err))
      {
        
      }
      
      auth_perm_del (&auth_perm);
    }
  }
  
  // run it once
  return CAPE_ERR_EOF;
}

//-------------------------------------------------------------------------------------

AuthContext auth_context_new (void)
{
  AuthContext self = CAPE_NEW (struct AuthContext_s);
  
  self->adbl_ctx = NULL;
  self->adbl_session = NULL;

  self->vault = NULL;
  self->tokens = NULL;
  
  self->perm_jobs = NULL;

  return self;
}

//-------------------------------------------------------------------------------------

void auth_context_del (AuthContext* p_self)
{
  if (*p_self)
  {
    AuthContext self = *p_self;
    
    qjobs_del (&(self->perm_jobs));
    
    auth_vault_del (&(self->vault));
    auth_tokens_del (&(self->tokens));
    
    adbl_session_close (&(self->adbl_session));
    adbl_ctx_del (&(self->adbl_ctx));
    
    CAPE_DEL (p_self, struct AuthContext_s);
  }
}

//-------------------------------------------------------------------------------------

int auth_context_init (AuthContext self, QBus qbus, CapeErr err)
{
  int res;
  
  self->adbl_ctx = adbl_ctx_new ("adbl", "adbl2_mysql", err);
  if (self->adbl_ctx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  self->adbl_session = adbl_session_open_file (self->adbl_ctx, "adbl_default.json", err);
  if (self->adbl_session == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  // start the subsystems
  self->vault = auth_vault_new ();
  self->tokens = auth_tokens_new (self->adbl_session, self->vault);
  
  self->perm_jobs = qjobs_new (self->adbl_session, "auth_tokens_jobs");
  
  // init the jobs with 1000 milliseconds checking frequency
  res = qjobs_init (self->perm_jobs, qbus_aio (qbus), 1000, self, auth_context__on_perm_event, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  self->options_2factor = qbus_config_node (qbus, "options_2factor");
  self->options_fp = qbus_config_node (qbus, "options_fp");
  
exit_and_cleanup:
  
  return res;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  int res;
  
  // local objects
  AuthContext ctx = auth_context_new ();
  
  // initialize the context
  res = auth_context_init (ctx, qbus, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
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

  // send the forget password auth messages
  //   args:
  qbus_register (qbus, "ui_fp_send"           , ctx, qbus_auth_ui_fp_send, NULL, err);

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

  // create a new token
  //   args: [code], {roles} : pdata
  qbus_register (qbus, "token_add"            , ctx, qbus_auth_perm_add, NULL, err);

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
  //   args: token | apid
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
  
  auth_context_del (&ctx);
  return cape_err_code(err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_auth_done (QBus qbus, void* ptr, CapeErr err)
{
  AuthContext ctx = ptr;
  
  auth_context_del (&ctx);
  
  return CAPE_ERR_NONE;
}

//-------------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("AUTH", NULL, qbus_auth_init, qbus_auth_done, argc, argv);
  return 0;
}

//-------------------------------------------------------------------------------------
