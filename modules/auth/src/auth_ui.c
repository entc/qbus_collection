#include "auth_ui.h"
#include "auth_rinfo.h"
#include "auth_perm.h"
#include "auth_gp.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

#define AUTH_LOGIN_STATUS__SUCCESS   1
#define AUTH_LOGIN_STATUS__DENIED    2

//-----------------------------------------------------------------------------

struct AuthUI_s
{
  QBus qbus;                       // reference
  AdblSession adbl_session;        // reference
  AuthTokens tokens;               // reference
  AuthVault vault;                 // reference
  CapeUdc options_2factor;         // reference
  CapeUdc options_fp;              // reference

  number_t userid;
  number_t wpid;
  
  CapeString secret;
  CapeUdc recipients;
};

//-----------------------------------------------------------------------------

AuthUI auth_ui_new (QBus qbus, AdblSession adbl_session, AuthTokens tokens, AuthVault vault, CapeUdc options_2factor, CapeUdc options_fp)
{
  AuthUI self = CAPE_NEW (struct AuthUI_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;
  self->tokens = tokens;
  self->vault = vault;
  self->options_2factor = options_2factor;
  self->options_fp = options_fp;
  
  self->userid = 0;
  self->wpid = 0;
  
  self->secret = NULL;
  self->recipients = NULL;

  return self;
}

//---------------------------------------------------------------------------

void auth_ui_del (AuthUI* p_self)
{
  if (*p_self)
  {
    AuthUI self = *p_self;
    
    cape_str_del (&(self->secret));
    cape_udc_del (&(self->recipients));
    
    CAPE_DEL (p_self, struct AuthUI_s);
  }
}

//-----------------------------------------------------------------------------

int auth_ui__save_log_entry (AdblTrx adbl_trx, number_t wpid, number_t gpid, number_t usid, CapeUdc rinfo, CapeUdc cdata, number_t status, CapeErr err)
{
  number_t id;
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // parameters
  cape_udc_add_n      (values, "wpid"         , wpid);
  cape_udc_add_n      (values, "gpid"         , gpid);
  cape_udc_add_n      (values, "userid"       , usid);
  cape_udc_add_n      (values, "status"       , status);
  
  {
    CapeDatetime dt;
    cape_datetime_utc (&dt);
    
    cape_udc_add_d    (values, "ltime"        , &dt);
  }
  
  {
    CapeString h = cape_json_to_s(rinfo);
    
    printf ("RINFO: %s\n", h);
    
  }
  
  {
    const CapeString remote = cape_udc_get_s (rinfo, "remote", NULL);
    if (remote)
    {
      cape_udc_add_s_cp (values, "ip", remote);
    }
  }
  
  if (cdata)
  {
    CapeUdc info = cape_udc_ext (cdata, "info");
    if (info)
    {
      cape_udc_add (values, &info);
    }
  }
  
  return (0 == adbl_trx_insert (adbl_trx, "auth_logins", &values, err)) ? cape_err_code (err) : CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int auth_ui__internal__save_log_entry (AuthUI self, number_t wpid, number_t gpid, number_t usid, CapeUdc rinfo, CapeUdc cdata, number_t status, CapeErr err)
{
  int res;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  
  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  res = auth_ui__save_log_entry (adbl_trx, wpid, gpid, usid, rinfo, cdata, status, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui__internal__save_denied (AuthUI self, number_t wpid, number_t gpid, number_t usid, CapeUdc rinfo, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc cdata = NULL;
  
  if (cape_err_code (err))
  {
    cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    {
      CapeUdc info = cape_udc_add_node (cdata, "info");
      
      cape_udc_add_n    (info, "err_code", cape_err_code (err));
      cape_udc_add_s_cp (info, "err_text", cape_err_text (err));
    }
  }
  
  res = auth_ui__internal__save_log_entry (self, wpid, gpid, usid, rinfo, cdata, AUTH_LOGIN_STATUS__DENIED, err);

  cape_udc_del (&cdata);
  return res;
}

//---------------------------------------------------------------------------

CapeUdc auth_ui_crypt4__extract_from_content (const CapeString content)
{
  CapeUdc auth_crypt_credentials;
  
  // decode base64
  CapeStream s = qcrypt__decode_base64_s (content);
    
  // de-serialize into an UDC container
  auth_crypt_credentials = cape_json_from_buf (cape_stream_data (s), cape_stream_size (s), NULL);

  cape_stream_del (&s);
  
  return auth_crypt_credentials;
}

//---------------------------------------------------------------------------

int auth_ui_crypt4__compare_password (AuthUI self, const CapeString cha, const CapeString cda, CapeErr err)
{
  int res;
  
  // local objects
  CapeString hex_hash = NULL;
  
  if (self->secret == NULL)
  {
    cape_log_fmt (CAPE_LL_ERROR, "AUTH", "crypt4", "no secret found");
    
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no authentication");
    goto exit_and_cleanup;
  }

  // convert the secret using the digest algorithm
  {
    CapeStream s = cape_stream_new ();
    
    cape_stream_append_str (s, cha);
    cape_stream_append_c (s, ':');
    cape_stream_append_str (s, self->secret);

    hex_hash = qcrypt__hash_sha256__hex_m (s, err);

    cape_stream_del (&s);
  }

  if (!cape_str_equal (hex_hash, cda))
  {
    cape_log_fmt (CAPE_LL_WARN, "AUTH", "crypt4", "password wrong");
    
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no authentication");
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_str_del (&hex_hash);
  return res;
}

//---------------------------------------------------------------------------

int auth_ui__intern__get_workspaces (AuthUI self, number_t userid, CapeUdc* p_workspaces, CapeErr err)
{
  int res;

  // local objects
  CapeUdc query_results = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "userid"       , userid);
    
    cape_udc_add_n      (values, "wpid"         , 0);
    cape_udc_add_s_cp   (values, "workspace"    , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "rbac_users_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  cape_udc_replace_mv (p_workspaces, &query_results);
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  return res;
}

//---------------------------------------------------------------------------

int auth_ui__intern__get_recipients (AuthUI self, number_t wpid, number_t gpid, CapeUdc* p_recipients, const CapeString vsec, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "gpid"         , gpid);
    
    cape_udc_add_n      (values, "id"           , 0);
    cape_udc_add_n      (values, "type"         , 0);
    cape_udc_add_s_cp   (values, "email"        , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "glob_emails", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    res = qcrypt__decrypt_row_text (vsec, cursor->item, "email", err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    // cross out the content
    {
      CapeUdc email_node = cape_udc_get (cursor->item, "email");
      if (email_node)
      {
        CapeString h = cape_udc_s_mv (email_node, NULL);
        if (h)
        {
          cape_str_override (h, 5, 4, 'X');
          cape_udc_set_s_mv (email_node, &h);
        }
      }
    }
    
  }
  
  res = CAPE_ERR_NONE;
  cape_udc_replace_mv (p_recipients, &query_results);
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);
  return res;
}

//---------------------------------------------------------------------------

int auth_ui_crypt4__2f_token (AuthUI self, number_t wpid, number_t gpid, number_t userid, CapeString* p_token, CapeErr err)
{
  int res;
  
  // local objects
  QBusM ap_qin = qbus_message_new (NULL, NULL);
  QBusM ap_qout = qbus_message_new (NULL, NULL);
  
  // we don't need jobs for the following calls
  AuthPerm ap = auth_perm_new (self->qbus, self->adbl_session, self->vault, NULL);
  CapeString token = NULL;
  
  ap_qin->rinfo = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  cape_udc_add_n (ap_qin->rinfo, "wpid", wpid);
  cape_udc_add_n (ap_qin->rinfo, "gpid", gpid);
  cape_udc_add_n (ap_qin->rinfo, "userid", userid);

  ap_qin->pdata = cape_udc_new (CAPE_UDC_NODE, NULL);

  cape_udc_add_n (ap_qin->pdata, "rfid", gpid);
  cape_udc_add_n (ap_qin->pdata, "ttl", 300000);

  {
    CapeUdc roles = cape_udc_new (CAPE_UDC_NODE, "roles");
    
    cape_udc_add_b (roles, "AUTH_2F", TRUE);
    
    cape_udc_add (ap_qin->pdata, &roles);
  }
  
  res = auth_perm_set (&ap, ap_qin, ap_qout, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  token = cape_udc_ext_s (ap_qout->cdata, "token");
  if (token == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't get token");
    goto exit_and_cleanup;
  }

  cape_str_replace_mv (p_token, &token);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  auth_perm_del (&ap);
  
  qbus_message_del (&ap_qout);
  qbus_message_del (&ap_qin);
  
  return res;
}

//---------------------------------------------------------------------------

int auth_ui_crypt4__code_remove (AuthUI self, number_t wpid, number_t gpid, CapeErr err)
{
  int res;
  
  // local objects
  AdblTrx adbl_trx = NULL;

  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n     (params, "wpid"          , wpid);
    cape_udc_add_n     (params, "gpid"          , gpid);
    
    cape_udc_add_z     (values, "code_2factor");
    
    // execute query
    res = adbl_trx_update (adbl_trx, "rbac_users", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  return res;
}

//---------------------------------------------------------------------------

int auth_ui_crypt4__fix_vault (AuthUI self, number_t wpid, number_t gpid, CapeString* p_vault_secret, const CapeString vault_password, CapeErr err)
{
  int res;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  CapeUdc query_results = NULL;
  CapeString vault_key = NULL;
  CapeString h1 = NULL;

  // we can fix the vault if there is only one user defined for that workspace
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n     (params, "wpid"          , wpid);
    
    cape_udc_add_s_cp  (values, "secret"        , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "auth_users_secret_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  if (cape_udc_size (query_results) != 1)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no vault defined");
    goto exit_and_cleanup;
  }

  // create a new vault key
  vault_key = cape_str_uuid ();

  // ecrypt the vault with the vault password
  h1 = qcrypt__encrypt (vault_password, vault_key, err);
  if (h1 == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n     (params, "wpid"          , wpid);
    cape_udc_add_n     (params, "gpid"          , gpid);
    
    cape_udc_add_s_cp  (values, "secret"        , h1);
    
    // execute query
    res = adbl_trx_update (adbl_trx, "rbac_users", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
  cape_str_replace_mv (p_vault_secret, &h1);
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  cape_str_del (&vault_key);
  cape_str_del (&h1);
  
  adbl_trx_rollback (&adbl_trx, err);
  return res;
}

//---------------------------------------------------------------------------

int auth_ha_update (AuthUI self, number_t userid, const CapeString ha_value, CapeErr err)
{
  int res;

  // local objects
  AdblTrx adbl_trx = NULL;

  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n     (params, "id"            , userid);
    cape_udc_add_s_cp  (values, "ha_value"      , ha_value);
    
    // execute query
    res = adbl_trx_update (adbl_trx, "q5_users", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  return res;
}

//---------------------------------------------------------------------------

int auth_ui__2factor (AuthUI self, number_t userid, number_t wpid, number_t gpid, const CapeString vsec, CapeUdc sec_item, CapeUdc auth_crypt_credentials, QBusM qout, CapeErr err)
{
  int res;
  CapeUdc opt_2factor_node;
  const CapeString code_2f_from_db;

  opt_2factor_node = cape_udc_get (sec_item, "opt_2factor");
  if (NULL == opt_2factor_node)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no 2factor");
    goto exit_and_cleanup;
  }
  
  // check the status of the 2factor authentication
  if (1 != cape_udc_n (opt_2factor_node, 0))
  {
    res = CAPE_ERR_NONE;
    goto exit_and_cleanup;
  }
  
  // check if the code was given by the user
  const CapeString code_2f = cape_udc_get_s (auth_crypt_credentials, "code", NULL);
  if (NULL == code_2f)
  {
    qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // retrieve the recipients
    {
      CapeUdc recipients = NULL;
      
      res = auth_ui__intern__get_recipients (self, wpid, gpid, &recipients, vsec, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
      
      cape_udc_add_name (qout->cdata, &recipients, "recipients");
    }
    
    {
      CapeString token = NULL;
      
      res = auth_ui_crypt4__2f_token (self, wpid, gpid, userid, &token, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
      
      cape_udc_add_s_mv (qout->cdata, "token", &token);
    }

    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "2f_code");
    goto exit_and_cleanup;
  }
  
  // get the code from the database
  code_2f_from_db = cape_udc_get_s (sec_item, "code_2factor", NULL);
  
  // NULL is not a valid code
  if (NULL == code_2f_from_db)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "2f_secret");
    goto exit_and_cleanup;
  }
  
  // check if both codes are identical
  if (FALSE == cape_str_equal (code_2f_from_db, code_2f))
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "code missmatch");
    goto exit_and_cleanup;
  }

  // if the codes was used, remove it from the database
  res = auth_ui_crypt4__code_remove (self, wpid, gpid, err);

exit_and_cleanup:

  return res;
}

//---------------------------------------------------------------------------

int auth_ui_crypt4 (AuthUI* p_self, const CapeString content, CapeUdc extras, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  const CapeString cid;
  const CapeString cha;
  const CapeString cda;
  const CapeString vsec;
  CapeUdc first_row;
  number_t wpid;
  number_t gpid;
  number_t userid;
  
  number_t ha_active;
  CapeString ha_last = NULL;
  CapeString ha_current = NULL;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc auth_crypt_credentials = NULL;
  CapeString h1 = NULL;
  CapeString h2 = NULL;
  CapeString vault_secret = NULL;
  
  // convert from raw input to credentials part
  auth_crypt_credentials = auth_ui_crypt4__extract_from_content (content);
  if (auth_crypt_credentials == NULL)
  {
    cape_log_fmt (CAPE_LL_TRACE, "AUTH", "crypt4", "can't parse content: '%s'", content);
    
    res = cape_err_set (err, CAPE_ERR_PARSER, "can't parse crypt4 content");
    goto exit_and_cleanup;
  }
  
  // in case there are several workspaces for the same user
  // a wpid will be added to the authentication credentials
  self->wpid = cape_udc_get_n (auth_crypt_credentials, "wpid", 0);
  
  // extract into parts
  cid = cape_udc_get_s (auth_crypt_credentials, "id", NULL);
  cha = cape_udc_get_s (auth_crypt_credentials, "ha", NULL);
  cda = cape_udc_get_s (auth_crypt_credentials, "da", NULL);
  
  if ((NULL == cid) || (NULL == cha) || (NULL == cda))
  {
    cape_log_fmt (CAPE_LL_ERROR, "AUTH", "crypt4", "content has wrong value");
    
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "content has wrong value");
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // conditions
    cape_udc_add_s_cp   (params, "user512"      , cid);
    cape_udc_add_n      (params, "active"       , 1);
    
    if (self->wpid)
    {
      cape_udc_add_n      (params, "wpid"       , self->wpid);
    }
    
    // return values
    cape_udc_add_n      (values, "wpid"         , 0);
    cape_udc_add_n      (values, "gpid"         , 0);

    cape_udc_add_n      (values, "userid"       , 0);
    cape_udc_add_s_cp   (values, "secret"       , NULL);
    cape_udc_add_s_cp   (values, "secret_vault" , NULL);

    cape_udc_add_s_cp   (values, "ha_value"     , 0);
    cape_udc_add_n      (values, "ha_active"    , 0);

    cape_udc_add_n      (values, "opt_msgs"     , 0);
    cape_udc_add_n      (values, "opt_2factor"  , 0);
    
    cape_udc_add_s_cp   (values, "code_2factor" , NULL);
    
    /*
     auth_users_secret_view
     
     select ru.id usid, gpid, wpid, qu.user512, qu.secret, qu.id userid, qu.active, qu.ha_value, qu.ha_active, ru.opt_msgs, ru.opt_2factor, ru.code_2factor, ru.code_fp, ru.secret secret_vault from rbac_users ru join q5_users qu on qu.id = ru.userid;
     */
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "auth_users_secret_view", &params, &values, err);
    if (query_results == NULL)
    {
      cape_log_msg (CAPE_LL_ERROR, "AUTH", "crypt4", cape_err_text (err));
      
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_get_first (query_results);
  if (first_row == NULL)
  {
    cape_log_fmt (CAPE_LL_WARN, "AUTH", "crypt4", "user not found");
    
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no authentication");
    goto exit_and_cleanup;
  }
  
  userid = cape_udc_get_n (first_row, "userid", 0);
  if (userid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no userid assigned");
    goto exit_and_cleanup;
  }
  
  if (cape_udc_size (query_results) > 1)
  {
    res = auth_ui__intern__get_workspaces (self, userid, &(qout->cdata), err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "too many results for rbac");
    goto exit_and_cleanup;
  }

  wpid = cape_udc_get_n (first_row, "wpid", 0);
  if (wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no wpid assigned");
    goto exit_and_cleanup;
  }
  
  gpid = cape_udc_get_n (first_row, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no gpid assigned");
    goto exit_and_cleanup;
  }
  
  // extract secret
  self->secret = cape_udc_ext_s (first_row, "secret");
  if (self->secret == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no secret defined");
    goto exit_and_cleanup;
  }
  
  // always check the password first
  res = auth_ui_crypt4__compare_password (self, cha, cda, err);
  if (res)
  {
    auth_ui__internal__save_denied (self, wpid, gpid, userid, qin->cdata, err);
    goto exit_and_cleanup;
  }
  
  vsec = auth_vault__vsec (self->vault, wpid);
  if (vsec == NULL)
  {
    const CapeString vault_password = cape_udc_get_s (auth_crypt_credentials, "vault", NULL);
    vault_secret = cape_udc_ext_s (first_row, "secret_vault");
    
    if (vault_password == NULL)
    {
      cape_log_fmt (CAPE_LL_TRACE, "AUTH", "ui crypt4", "no vault found -> missing parameter");

      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "vault");
      goto exit_and_cleanup;
    }

    cape_log_fmt (CAPE_LL_TRACE, "AUTH", "ui crypt4", "no vault found -> continue with parameter");

    h1 = qcrypt__decrypt (self->secret, vault_password, err);
    if (h1 == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "wrong vault");
      goto exit_and_cleanup;
    }
    
    if (vault_secret == NULL)
    {
      res = auth_ui_crypt4__fix_vault (self, wpid, gpid, &vault_secret, h1, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }

    h2 = qcrypt__decrypt (h1, vault_secret, err);
    if (h2 == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "can't get vault");
      goto exit_and_cleanup;
    }
    
    // set the vault 
    auth_vault__save (self->vault, wpid, h2);
    
    vsec = auth_vault__vsec (self->vault, wpid);
    if (vsec == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "error in vault");
      goto exit_and_cleanup;
    }
  }
  
  res = auth_ui__2factor (self, userid, wpid, gpid, vsec, first_row, auth_crypt_credentials, qout, err);
  if (res)
  {
    auth_ui__internal__save_denied (self, wpid, gpid, userid, qin->cdata, err);
    goto exit_and_cleanup;
  }
  
  //cape_log_fmt (CAPE_LL_TRACE, "AUTH", "ui crypt4", "password match");

  ha_active = cape_udc_get_n (first_row, "ha_active", 0);
  if (ha_active > 0)
  {
    // fetch the last known ha value
    ha_last = cape_str_ln_normalize (cape_udc_get_s (first_row, "ha_value", NULL));
    ha_current = cape_str_ln_normalize (cha);
    
    cape_log_fmt (CAPE_LL_TRACE, "AUTH", "ui crypt4", "compare ha: current = %s, last = %s", ha_current, ha_last);
    
    if (cape_str_ln_cmp (ha_current, ha_last) <= 0)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.ALREADY_GRANTED");
      goto exit_and_cleanup;
    }
    
    res = auth_ha_update (self, userid, cha, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  {
    // use the rinfo classes
    AuthRInfo rinfo = auth_rinfo_new (self->adbl_session, wpid, gpid);
    
    // fetch all rinfo from database
    res = auth_rinfo_get (&rinfo, qout, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_str_del (&vault_secret);
  cape_str_del (&h1);
  cape_str_del (&h2);

  cape_udc_del (&query_results);
  cape_udc_del (&auth_crypt_credentials);
  return res;
}

//---------------------------------------------------------------------------

int auth_ui_token (AuthUI self, CapeUdc extras, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  const CapeString token;
  
  token = cape_udc_get_s (extras, "__T", NULL);
  if (token == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "extras has no token");
  }

  res = auth_tokens_fetch (self->tokens, token, qin, qout, err);
  if (res)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "token not found");
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//---------------------------------------------------------------------------

number_t auth_convert_type (const CapeString type)
{
  if (cape_str_equal (type, "Basic"))
  {
    return 1;
  }
  else if (cape_str_equal (type, "Digest"))
  {
    return 2;
  }
  else if (cape_str_equal (type, "Crypt4"))
  {
    return 3;
  }
  else if (cape_str_equal (type, "Token"))
  {
    return 4;
  }
  
  return 0;
}

//-----------------------------------------------------------------------------

void auth_ui__intern__write_log (AuthUI self, const CapeString text, CapeUdc cdata)
{
  int res;
  const CapeString remote = "unknown";
  
  /*
  // local objects
  CapeErr err = cape_err_new ();
  CapeFileHandle fh = NULL;
  CapeStream s = cape_stream_new ();
  
  {
    CapeDatetime dt; cape_datetime_utc (&dt);
    cape_stream_append_d (s, &dt);
  }

  cape_stream_append_c (s, ' ');
   */
   
  if (cdata)
  {
    remote = cape_udc_get_s (cdata, "remote", NULL);
  }

  /*
  cape_stream_append_str (s, remote);
  cape_stream_append_c (s, ' ');

  cape_stream_append_str (s, text);

  cape_stream_append_c (s, '\n');

  fh = cape_fh_new (NULL, self->err_log_file);
      
  res = cape_fh_open (fh, O_RDWR | O_CREAT | O_APPEND, err);
  if (res)
  {

  }
  else
  {
    cape_fh_write_buf (fh, cape_stream_data (s), cape_stream_size (s));
  }

  cape_stream_del (&s);
  cape_fh_del (&fh);
  cape_err_del (&err);
  */
  
  // using the new qbus interface
  qbus_log_msg (self->qbus, remote, text);
}

//-----------------------------------------------------------------------------

int auth_ui_get (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  const CapeString auth_type;
  const CapeString auth_content;
  CapeUdc auth_extras;

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "'cdata' is missing");
    goto exit_and_cleanup;
  }
  
  auth_type = cape_udc_get_s (qin->cdata, "type", NULL);
  if (auth_type == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'type' is empty or missing");
    goto exit_and_cleanup;
  }

  // optional
  auth_content = cape_udc_get_s (qin->cdata, "content", NULL);
  
  // optional
  auth_extras = cape_udc_get (qin->cdata, "extras");
  
  switch (auth_convert_type (auth_type))
  {
    case 1:  break;
    case 2:  break;
    case 3:
    {
      if (auth_content == NULL)
      {
        res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'content' is empty or missing");
        goto exit_and_cleanup;
      }
      
      res = auth_ui_crypt4 (p_self, auth_content, auth_extras, qin, qout, err);
      goto exit_and_cleanup;
    }
    case 4:
    {
      res = auth_ui_token (self, auth_extras, qin, qout, err);
      goto exit_and_cleanup;
    }
  }

  res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "authentication type not supported");

exit_and_cleanup:
  
  if (*p_self && res == CAPE_ERR_NO_AUTH)
  {
    auth_ui__intern__write_log (self, "access denied", qin->cdata);
  }
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_login (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  // local objects
  AdblTrx adbl_trx = NULL;
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;
  
  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
    goto exit_and_cleanup;
  }
  
  self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing wpid");
    goto exit_and_cleanup;
  }
  
  number_t usid = cape_udc_get_n (qin->rinfo, "userid", 0);
  if (usid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing userid");
    goto exit_and_cleanup;
  }
  
  number_t gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing gpid");
    goto exit_and_cleanup;
  }
  
  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  // select
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "wpid"             , self->wpid);
    cape_udc_add_n      (params, "userid"           , usid);
    cape_udc_add_d      (values, "ltime"            , NULL);
    cape_udc_add_node   (values, "info"             );

    // execute the query
    query_results = adbl_trx_query (adbl_trx, "auth_logins_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  // extract the first row
  first_row = cape_udc_ext_first (query_results);

  res = auth_ui__save_log_entry (adbl_trx, self->wpid, gpid, usid, qin->rinfo, qin->cdata, AUTH_LOGIN_STATUS__SUCCESS, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // add first row
  cape_udc_replace_mv (&(qout->cdata), &first_row);
  
  // add rinfo
  if (qout->cdata)
  {
    cape_udc_merge_cp (qout->cdata, qin->rinfo);
  }
  else
  {
    qout->cdata = cape_udc_cp (qin->rinfo);
  }
  
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);

  cape_udc_del (&query_results);
  cape_udc_del (&first_row);

  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_set__rbac_users__update (AuthUI self, AdblTrx adbl_trx, number_t usid, number_t wpid, const CapeString secret, CapeErr err)
{
  int res;
  const CapeString vsec;

  // local objects
  CapeString h2 = NULL;
  
  vsec = auth_vault__vsec (self->vault, wpid);
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing vault");
    goto exit_and_cleanup;
  }

  //cape_log_fmt (CAPE_LL_TRACE, "AUTH", "ui set", "update rbac users with usid = %i, vsec = '%s'", usid, vsec);
  
  // encrypt the vault with the user password
  h2 = qcrypt__encrypt (secret, vsec, err);
  if (h2 == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n     (params, "id"        , usid);
    
    cape_udc_add_s_mv  (values, "secret"    , &h2);
    cape_udc_add_n     (values, "state"     , AUTH_STATE__NORMAL);
    
    // execute query
    res = adbl_trx_update (adbl_trx, "rbac_users", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_set__rbac_users (AuthUI self, AdblTrx adbl_trx, number_t userid, const CapeString secret, CapeErr err)
{
  int res;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;
  
  cape_log_fmt (CAPE_LL_TRACE, "AUTH", "ui set", "update rbac users with userid = %i", userid);

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // conditions
    cape_udc_add_n      (params, "userid"       , userid);
    cape_udc_add_n      (values, "id"           , 0);
    cape_udc_add_n      (values, "wpid"         , 0);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "rbac_users", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    number_t usid = cape_udc_get_n (cursor->item, "id", 0);
    number_t wpid = cape_udc_get_n (cursor->item, "wpid", 0);

    res = auth_ui_set__rbac_users__update (self, adbl_trx, usid, wpid, secret, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

CapeString auth_ui__intern__construct_password (const CapeString q5_user, const CapeString secret, CapeErr err)
{
  CapeString ret = NULL;
  
  // local objects
  CapeStream s = cape_stream_new ();
  
  cape_stream_append_str (s, q5_user);
  cape_stream_append_c (s, ':');
  cape_stream_append_str (s, secret);
  
  ret = qcrypt__hash_sha256__hex_m (s, err);
  
  cape_stream_del (&s);

  return ret;
}

//-----------------------------------------------------------------------------

int auth_ui_set__q5_users__update_password (AuthUI self, AdblTrx adbl_trx, number_t userid, const CapeString q5_user, const CapeString secret, CapeErr err)
{
  int res;

  // local objects
  CapeString h1 = NULL;
  
  h1 = auth_ui__intern__construct_password (q5_user, secret, err);
  if (h1 == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n     (params, "id"        , userid);
    cape_udc_add_s_mv  (values, "secret"    , &h1);

    // execute query
    res = adbl_trx_update (adbl_trx, "q5_users", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_str_del (&h1);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_set__q5_users__update_active (AuthUI self, AdblTrx adbl_trx, number_t userid, int active, CapeErr err)
{
  int res;

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n     (params, "id"        , userid);
    cape_udc_add_b     (values, "active"    , active);
    
    // execute query
    res = adbl_trx_update (adbl_trx, "q5_users", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_set__no_authentication (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  number_t userid;

  const CapeString q5_pass;
  const CapeString q5_user;
  const CapeString code;
  CapeUdc first_row;

  // local objects
  AdblTrx adbl_trx = NULL;
  CapeUdc query_results = NULL;
  CapeString h0 = NULL;
  CapeString h1 = NULL;

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "'cdata' is missing");
    goto exit_and_cleanup;
  }

  q5_pass = cape_udc_get_s (qin->cdata, "pass", NULL);
  if (q5_pass == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'pass' is empty or missing");
    goto exit_and_cleanup;
  }
  
  q5_user = cape_udc_get_s (qin->cdata, "user", NULL);
  if (q5_user == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'user' is empty or missing");
    goto exit_and_cleanup;
  }

  // if no authentication the code is mandatory
  code = cape_udc_get_s (qin->cdata, "code", NULL);
  if (code == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'code' is empty or missing");
    goto exit_and_cleanup;
  }

  // create the hash version of the user
  h0 = qcrypt__hash_sha256__hex_o (q5_user, cape_str_size (q5_user), err);
  if (h0 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't hash h0");
    goto exit_and_cleanup;
  }

  // create the hash version of the code
  h1 = qcrypt__hash_sha256__hex_o (code, cape_str_size (code), err);
  if (h1 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't hash h1");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // conditions
    cape_udc_add_n      (params, "active"       , 1);
    cape_udc_add_s_mv   (params, "user512"      , &h0);
    cape_udc_add_s_mv   (params, "code_fp"      , &h1);

    cape_udc_add_n      (values, "usid"         , 0);
    cape_udc_add_n      (values, "userid"       , 0);
    cape_udc_add_s_cp   (values, "secret"       , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "auth_users_secret_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_get_first (query_results);
  if (first_row == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "account not found or active");
    goto exit_and_cleanup;
  }

  userid = cape_udc_get_n (first_row, "userid", 0);
  if (userid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "userid not found");
    goto exit_and_cleanup;
  }

  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  res = auth_ui_set__q5_users__update_password (self, adbl_trx, userid, q5_user, q5_pass, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = auth_ui_set__rbac_users (self, adbl_trx, userid, q5_pass, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;

exit_and_cleanup:

  adbl_trx_rollback (&adbl_trx, err);
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "AUTH", "ui set", cape_err_text (err));
  }

  cape_str_del (&h0);
  cape_str_del (&h1);
  cape_udc_del (&query_results);
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

struct AuthPasswordPolicyContext
{
  number_t length;
  number_t special_chars;
  number_t lower_case;
  number_t upper_case;
  number_t numbers;
  number_t invalid;
};

//-----------------------------------------------------------------------------

void auth_ui__password_policy__fetch (struct AuthPasswordPolicyContext* ctx)
{
  // initialize the context
  ctx->length = 8;
  ctx->lower_case = 1;
  ctx->upper_case = 1;
  ctx->special_chars = 1;
  ctx->numbers = 2;
  ctx->invalid = 0;
}

//-----------------------------------------------------------------------------

void auth_ui__password_policy__from_password (struct AuthPasswordPolicyContext* ctx, const CapeString password)
{
  const char* pos = password;

  // initialize the context
  ctx->length = 0;
  ctx->lower_case = 0;
  ctx->upper_case = 0;
  ctx->special_chars = 0;
  ctx->numbers = 0;
  ctx->invalid = 0;

  while (*pos)
  {
    // retrieve the length of the UTF8 character
    number_t cs = cape_str_utf8__len (*pos);
    
    if (cs == 1)  // normal ASCII
    {
      ctx->length++;
      
      if (*pos < 33)
      {
        ctx->invalid++;
      }
      else if (*pos > 32 && *pos < 48)
      {
        ctx->special_chars++;
      }
      else if (*pos > 47 && *pos < 58)
      {
        ctx->numbers++;
      }
      else if (*pos > 57 && *pos < 65)
      {
        ctx->special_chars++;
      }
      else if (*pos > 64 && *pos < 91)
      {
        ctx->upper_case++;
      }
      else if (*pos > 90 && *pos < 97)
      {
        ctx->special_chars++;
      }
      else if (*pos > 96 && *pos < 123)
      {
        ctx->lower_case++;
      }
      else
      {
        ctx->special_chars++;
      }
      
      pos++;
    }
    else
    {
      number_t i;
      
      ctx->length++;
      
      // increase the position regarding the utf8
      for (i = 0; (i < cs) && *pos; i++, pos++);
    }
  }
  
  cape_log_fmt (CAPE_LL_TRACE, "AUTH", "password policy", "found length = %i, lower_case = %i, upper_case = %i, special = %i, numbers = %i, invalid = %i", ctx->length, ctx->lower_case, ctx->upper_case, ctx->special_chars, ctx->numbers, ctx->invalid);
}

//-----------------------------------------------------------------------------

int auth_ui__password_policy__check (AuthUI self, const CapeString password, number_t* p_strength, struct AuthPasswordPolicyContext* ppctx_password, CapeErr err)
{
  struct AuthPasswordPolicyContext ppctx_rules;

  // get the rules and apply them to the context
  auth_ui__password_policy__fetch (&ppctx_rules);
  
  // set the context by parsing the password
  auth_ui__password_policy__from_password (ppctx_password, password);

  // try to calculate the password strength using a trivial algorithm
  if (p_strength)
  {
    number_t strength = 0;
    
    if (ppctx_password->length > 8)
    {
      strength++;
    }

    if (ppctx_password->length > 10)
    {
      strength++;
    }

    if (ppctx_password->length > 12)
    {
      strength++;
    }
    
    *p_strength = strength;
  }

  if (ppctx_password->invalid > ppctx_rules.invalid)
  {
    return cape_err_set (err, CAPE_ERR_WRONG_VALUE, "ERR.INVALID_CHARS");
  }
  
  if (ppctx_password->length < ppctx_rules.length)
  {
    return cape_err_set (err, CAPE_ERR_WRONG_VALUE, "ERR.INVALID_LENGTH");
  }
  
  if (ppctx_password->numbers < ppctx_rules.numbers)
  {
    return cape_err_set (err, CAPE_ERR_WRONG_VALUE, "ERR.TOOFEW_NUMBERS");
  }

  if (ppctx_password->special_chars < ppctx_rules.special_chars)
  {
    return cape_err_set (err, CAPE_ERR_WRONG_VALUE, "ERR.TOOFEW_SPECIAL_CHARS");
  }

  if (ppctx_password->upper_case < ppctx_rules.upper_case)
  {
    return cape_err_set (err, CAPE_ERR_WRONG_VALUE, "ERR.TOOFEW_UPPERCASE_CHARS");
  }

  if (ppctx_password->lower_case < ppctx_rules.lower_case)
  {
    return cape_err_set (err, CAPE_ERR_WRONG_VALUE, "ERR.TOOFEW_LOWERCASE_CHARS");
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int auth_ui_set__rinfo (AuthUI* p_self, QBusM qin, QBusM qout, number_t wpid, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  number_t gpid;
  number_t userid;
  const CapeString q5_user;
  const CapeString q5_pass;
  const CapeString secret;
  CapeUdc first_row;
  struct AuthPasswordPolicyContext ppctx_password;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  CapeUdc query_results = NULL;
  
  CapeString h0 = NULL;
  CapeString h1 = NULL;
  
  gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing gpid");
    goto exit_and_cleanup;
  }
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "'cdata' is missing");
    goto exit_and_cleanup;
  }
  
  secret = cape_udc_get_s (qin->cdata, "secret", NULL);
  if (secret == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'secret' is empty or missing");
    goto exit_and_cleanup;
  }
  
  q5_user = cape_udc_get_s (qin->cdata, "user", NULL);
  if (q5_user == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'user' is empty or missing");
    goto exit_and_cleanup;
  }
  
  q5_pass = cape_udc_get_s (qin->cdata, "pass", NULL);
  if (q5_pass == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'pass' is empty or missing");
    goto exit_and_cleanup;
  }
  
  res = auth_ui__password_policy__check (self, secret, NULL, &ppctx_password, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // create the hash version of the user
  h0 = qcrypt__hash_sha256__hex_o (q5_user, cape_str_size (q5_user), err);
  if (h0 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't hash h0");
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // conditions
    cape_udc_add_n      (params, "wpid"         , wpid);
    cape_udc_add_n      (params, "gpid"         , gpid);
    cape_udc_add_n      (params, "active"       , 1);
    cape_udc_add_s_mv   (params, "user512"      , &h0);
    
    cape_udc_add_n      (values, "usid"         , 0);
    cape_udc_add_n      (values, "userid"       , 0);
    cape_udc_add_s_cp   (values, "secret"       , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "auth_users_secret_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_get_first (query_results);
  if (first_row == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "account not found or active");
    goto exit_and_cleanup;
  }
  
  userid = cape_udc_get_n (first_row, "userid", 0);
  if (userid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "userid not found");
    goto exit_and_cleanup;
  }
  
  {
    const CapeString old_password_db = cape_udc_get_s (first_row, "secret", NULL);
    
    h1 = auth_ui__intern__construct_password (q5_user, q5_pass, err);
    if (h1 == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
    
    if (!cape_str_equal (old_password_db, h1))
    {
      res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "password missmatch");
      goto exit_and_cleanup;
    }
  }
  
  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  res = auth_ui_set__q5_users__update_password (self, adbl_trx, userid, q5_user, secret, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = auth_ui_set__rbac_users (self, adbl_trx, userid, secret, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "AUTH", "ui set", cape_err_text (err));
  }
  
  cape_str_del (&h0);
  cape_str_del (&h1);
  cape_udc_del (&query_results);
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_set__admin_active (AuthUI* p_self, number_t userid, number_t wpid, number_t gpid, int active, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  // local objects
  AdblTrx adbl_trx = NULL;

  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  res = auth_ui_set__q5_users__update_active (self, adbl_trx, userid, active, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_set__admin_password (AuthUI* p_self, number_t userid, number_t wpid, number_t gpid, const CapeString q5_user, const CapeString q5_pass, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  CapeUdc first_row;
  
  // local objects
  CapeUdc query_results = NULL;
  AdblTrx adbl_trx = NULL;
  CapeString h0 = NULL;

  // create the hash version of the user
  h0 = qcrypt__hash_sha256__hex_o (q5_user, cape_str_size (q5_user), err);
  if (h0 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't hash h0");
    goto exit_and_cleanup;
  }

  // check if the account realy exists
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // conditions
    cape_udc_add_n      (params, "id"           , userid);
    cape_udc_add_s_mv   (params, "user512"      , &h0);
    
    cape_udc_add_s_cp   (values, "secret"       , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "q5_users", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_get_first (query_results);
  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "ERR.NO_ACCOUNT");
    goto exit_and_cleanup;
  }

  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  res = auth_ui_set__q5_users__update_password (self, adbl_trx, userid, q5_user, q5_pass, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);

  cape_str_del (&h0);
  cape_udc_del (&query_results);

  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_set (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  number_t wpid;
  
  if (qbus_message_role_has (qin, "admin"))
  {
    CapeUdc active_node;
    
    number_t gpid;
    number_t userid;
    
    if (NULL == qin->cdata)
    {
      res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NO_CDATA");
      goto exit_and_cleanup;
    }
    
    userid = cape_udc_get_n (qin->cdata, "userid", 0);
    if (0 == userid)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_USERID");
      goto exit_and_cleanup;
    }

    wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
    if (0 == wpid)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_WPID");
      goto exit_and_cleanup;
    }
    
    gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
    if (0 == gpid)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_GPID");
      goto exit_and_cleanup;
    }
    
    active_node = cape_udc_get (qin->cdata, "active");
    if (active_node)
    {
      res = auth_ui_set__admin_active (p_self, userid, wpid, gpid, cape_udc_b (active_node, FALSE), err);
    }
    else
    {
      const CapeString user;
      const CapeString pass;

      user = cape_udc_get_s (qin->cdata, "user", NULL);
      if (NULL == user)
      {
        res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_USER");
        goto exit_and_cleanup;
      }
      
      pass = cape_udc_get_s (qin->cdata, "pass", NULL);
      if (NULL == pass)
      {
        res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_PASS");
        goto exit_and_cleanup;
      }
      
      res = auth_ui_set__admin_password (p_self, userid, wpid, gpid, user, pass, err);
    }
  }
  else
  {
    // do some security checks
    if (qin->rinfo == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "{ui set} missing rinfo");
      goto exit_and_cleanup;
    }
    
    wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
    if (wpid == 0)
    {
      res = auth_ui_set__no_authentication (p_self, qin, qout, err);
    }
    else
    {
      res = auth_ui_set__rinfo (p_self, qin, qout, wpid, err);
    }
  }

exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui__intern__check_workspace (AuthUI self, CapeString* p_domain, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;
  CapeString domain = NULL;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"               , self->wpid);
    cape_udc_add_s_cp   (values, "domain"           , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "rbac_workspaces", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_get_first (query_results);
  if (query_results == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "workspace not found");
    goto exit_and_cleanup;
  }
  
  domain = cape_udc_ext_s (first_row, "domain");
  if (domain == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "domain not found");
    goto exit_and_cleanup;
  }
  
  cape_str_replace_mv (p_domain, &domain);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui__intern__encode (const CapeString domain, const CapeString vsec, const CapeString name, const CapeString pass, CapeString* p_name_encoded, CapeString* p_pass_encoded, CapeString* p_vsec_encoded, CapeString* p_login, CapeErr err)
{
  int res;
  
  // local objects
  CapeStream login = cape_stream_new ();
  
  CapeString name_encoded = NULL;
  CapeString pass_encoded = NULL;
  CapeString vsec_encoded = NULL;
  CapeString login_text = NULL;
  
  cape_stream_append_str   (login, name);
  cape_stream_append_c     (login, '@');
  cape_stream_append_str   (login, domain);
  
  name_encoded = qcrypt__hash_sha256__hex_m (login, err);
  if (NULL == name_encoded)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  login_text = cape_stream_to_s (login);
  cape_log_fmt (CAPE_LL_TRACE, "WSPC", "users add", "create a new login '%s'", login_text);
  
  cape_stream_clr (login);
  
  cape_stream_append_str   (login, name);
  cape_stream_append_c     (login, '@');
  cape_stream_append_str   (login, domain);
  cape_stream_append_c     (login, ':');
  cape_stream_append_str   (login, pass);
  
  pass_encoded = qcrypt__hash_sha256__hex_m (login, err);
  if (pass_encoded == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  vsec_encoded = qcrypt__encrypt (pass, vsec, err);
  if (vsec_encoded == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
  cape_str_replace_mv (p_name_encoded, &name_encoded);
  cape_str_replace_mv (p_pass_encoded, &pass_encoded);
  cape_str_replace_mv (p_vsec_encoded, &vsec_encoded);
  
  if (p_login)
  {
    cape_str_replace_mv (p_login, &login_text);
  }
  
exit_and_cleanup:
  
  cape_str_del (&login_text);
  cape_str_del (&name_encoded);
  cape_str_del (&pass_encoded);
  cape_str_del (&vsec_encoded);
  
  cape_stream_del (&login);
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui__intern__users_add (AdblTrx adbl_trx, number_t userid, number_t wpid, number_t gpid, const CapeString vsec_encrypted, number_t* p_ruid, CapeErr err)
{
  int res;
  number_t ruid;
  
  // prepare the insert query
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // insert values
  cape_udc_add_n      (values, "id"           , ADBL_AUTO_SEQUENCE_ID);
  cape_udc_add_n      (values, "wpid"         , wpid);
  cape_udc_add_n      (values, "userid"       , userid);
  cape_udc_add_n      (values, "gpid"         , gpid);
  cape_udc_add_s_cp   (values, "secret"       , vsec_encrypted);
  
  // execute query
  ruid = adbl_trx_insert (adbl_trx, "rbac_users", &values, err);
  if (0 == ruid)
  {
    res = cape_err_code (err);
  }
  else
  {
    *p_ruid = ruid;
    res = CAPE_ERR_NONE;
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_add (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  const CapeString vsec;

  // local objects
  AdblTrx trx = NULL;
  CapeString user = NULL;
  CapeString pass = NULL;
  CapeString domain = NULL;
  CapeString user_encoded = NULL;
  CapeString pass_encoded = NULL;
  CapeString vsec_encoded = NULL;
  CapeString login = NULL;
  number_t gpid;
  number_t ruid;
  CapeUdc gpdata = NULL;

  if (NULL == qin->cdata)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NO_CDATA");
    goto exit_and_cleanup;
  }

  if (qbus_message_role_has (qin, "admin"))
  {
    self->wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
  }
  else if (qbus_message_role_has (qin, "auth_wacc"))
  {
    self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  }

  if (0 == self->wpid)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_WPID");
    goto exit_and_cleanup;
  }

  user = cape_udc_ext_s (qin->cdata, "username");
  if (NULL == user)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_USERNAME");
    goto exit_and_cleanup;
  }

  pass = cape_udc_ext_s (qin->cdata, "password");
  if (NULL == pass)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_PASSWORD");
    goto exit_and_cleanup;
  }

  gpdata = cape_udc_ext (qin->cdata, "gpdata");
  if (NULL == gpdata)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_GPDATA");
    goto exit_and_cleanup;
  }
  
  res = auth_ui__intern__check_workspace (self, &domain, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  vsec = auth_vault__vsec (self->vault, self->wpid);
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "no vault");
    goto exit_and_cleanup;
  }
  
  res = auth_ui__intern__encode (domain, vsec, user, pass, &user_encoded, &pass_encoded, &vsec_encoded, &login, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // start a new transaction
  trx = adbl_trx_new (self->adbl_session, err);
  if (trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    // prepare the insert query
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // insert values
    cape_udc_add_n      (values, "id"           , ADBL_AUTO_SEQUENCE_ID);
    cape_udc_add_s_cp   (values, "secret"       , pass_encoded);
    cape_udc_add_s_cp   (values, "user512"      , user_encoded);
    
    // execute query
    self->userid = adbl_trx_insert (trx, "q5_users", &values, err);
    if (self->userid == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  res = auth_wp_add (trx, vsec, self->wpid, gpdata, &gpid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = auth_ui__intern__users_add (trx, self->userid, self->wpid, gpid, vsec_encoded, &ruid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  adbl_trx_commit (&trx, err);
  
  // add output
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);

  cape_udc_add_n (qout->cdata, "userid", self->userid);
  cape_udc_add_n (qout->cdata, "ruid", ruid);
  cape_udc_add_n (qout->cdata, "gpid", gpid);

exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);

  cape_str_del (&domain);
  cape_str_del (&pass);
  cape_str_del (&user);

  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_pp_get (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  struct AuthPasswordPolicyContext policy_rules;
  
  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_AUTH");
    goto exit_and_cleanup;
  }

  // this will fetch the current policy rules
  auth_ui__password_policy__fetch (&policy_rules);
  
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_n (qout->cdata, "invalid", policy_rules.invalid);
  cape_udc_add_n (qout->cdata, "length", policy_rules.length);
  cape_udc_add_n (qout->cdata, "lower_case", policy_rules.lower_case);
  cape_udc_add_n (qout->cdata, "numbers", policy_rules.numbers);
  cape_udc_add_n (qout->cdata, "special_chars", policy_rules.special_chars);
  cape_udc_add_n (qout->cdata, "upper_case", policy_rules.upper_case);

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_pp_put (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  const CapeString secret;
  number_t strength;
  struct AuthPasswordPolicyContext ppctx_password;
  
  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_AUTH");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_CDATA");
    goto exit_and_cleanup;
  }
  
  secret = cape_udc_get_s (qin->cdata, "secret", NULL);
  if (secret == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.EMPTY_SECRET");
    goto exit_and_cleanup;
  }

  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);

  res = auth_ui__password_policy__check (self, secret, &strength, &ppctx_password, err);
  if (res)
  {
    cape_udc_add_s_cp (qout->cdata, "err_text", cape_err_text (err));
    cape_udc_add_n (qout->cdata, "err_code", cape_err_code (err));
    cape_udc_add_n (qout->cdata, "strength", 0);
    
    cape_err_clr (err);
  }
  else
  {
    cape_udc_add_n (qout->cdata, "strength", strength);
  }
  
  cape_udc_add_n (qout->cdata, "invalid", ppctx_password.invalid);
  cape_udc_add_n (qout->cdata, "length", ppctx_password.length);
  cape_udc_add_n (qout->cdata, "lower_case", ppctx_password.lower_case);
  cape_udc_add_n (qout->cdata, "numbers", ppctx_password.numbers);
  cape_udc_add_n (qout->cdata, "special_chars", ppctx_password.special_chars);
  cape_udc_add_n (qout->cdata, "upper_case", ppctx_password.upper_case);

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_login_get (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeDatetime* sd = NULL;
  CapeDatetime* td = NULL;
  
  if (!qbus_message_role_has (qin, "admin"))
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing roles");
    goto exit_and_cleanup;
  }
  
  // lp
  {
    const CapeString lp = cape_udc_get_s (qin->cdata, "lp", NULL);
    if (lp)
    {
      td = cape_datetime_new ();
      
      cape_datetime_utc (td);
      cape_datetime__remove_s (td, lp, td);
    }
    else
    {
      // optional
      sd = cape_datetime_cp (cape_udc_get_d (qin->cdata, "sd", NULL));
      td = cape_datetime_cp (cape_udc_get_d (qin->cdata, "td", NULL));
    }
  }
  
  if (sd)
  {
    CapeString h = cape_datetime_s__std (sd);
    cape_log_fmt (CAPE_LL_TRACE, "AUTH", "login get", "use start date = %s", h);
    cape_str_del (&h);
  }
  
  if (td)
  {
    CapeString h = cape_datetime_s__std (td);
    cape_log_fmt (CAPE_LL_TRACE, "AUTH", "login get", "use stop date = %s", h);
    cape_str_del (&h);
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    if (sd && td)
    {
      adbl_param_add__between_d (params, "ltime", sd, td);
    }
    else
    {
      if (sd)
      {
        adbl_param_add__less_than_d (params, "ltime", sd);
      }
      
      if (td)
      {
        adbl_param_add__greater_than_d (params, "ltime", td);
      }
    }

    cape_udc_add_n      (values, "wpid"        , 0);
    cape_udc_add_n      (values, "gpid"        , 0);
    cape_udc_add_n      (values, "userid"      , 0);
    cape_udc_add_d      (values, "ltime"       , NULL);
    cape_udc_add_s_cp   (values, "ip"          , NULL);
    cape_udc_add_node   (values, "info"        );
    cape_udc_add_s_cp   (values, "workspace"   , NULL);

    // delete the params if its empty
    if (cape_udc_size (params) == 0)
    {
      cape_udc_del (&params);
    }
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "auth_logins_get_view", &params, &values, err);
    if (query_results == NULL)
    {
      return cape_err_code (err);
    }
  }
  
  cape_udc_replace_mv (&(qout->cdata), &query_results);
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "AUTH", "logins get", cape_err_text (err));
  }
  
  cape_datetime_del (&sd);
  cape_datetime_del (&td);
  
  cape_udc_del (&query_results);
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_login_logs (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  number_t gpid;
  number_t wpid;
  
  // local objects
  CapeUdc query_results = NULL;

  if (qbus_message_role_has (qin, "admin"))
  {
    if (qin->cdata == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_CDATA");
      goto exit_and_cleanup;
    }
    
    wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
    gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
  }
  else
  {
    // do some security checks
    if (qin->rinfo == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_AUTH");
      goto exit_and_cleanup;
    }
    
    wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
    gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  }
  
  if (0 == wpid)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_WPID");
    goto exit_and_cleanup;
  }
  
  if (0 == gpid)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_GPID");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "wpid"           , wpid);
    cape_udc_add_n      (params, "gpid"           , gpid);
    cape_udc_add_n      (values, "id"             , 0);
    cape_udc_add_d      (values, "ltime"          , NULL);
    cape_udc_add_s_cp   (values, "ip"             , NULL);
    cape_udc_add_node   (values, "info"           );
    cape_udc_add_n      (values, "status"         , 0);

    // execute the query
    query_results = adbl_session_query_ex (self->adbl_session, "auth_logins", &params, &values, 20, 0, NULL, "-ltime", err);
    if (query_results == NULL)
    {
      return cape_err_code (err);
    }
  }

  cape_udc_replace_mv (&(qout->cdata), &query_results);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_switch (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  number_t gpid;
  number_t wpid;
  
  // local objects
  CapeUdc rinfo = NULL;
  CapeUdc cdata = NULL;

  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "{ui switch} missing rinfo");
    goto exit_and_cleanup;
  }
  
  // check role
  {
    CapeUdc role_admin;
    CapeUdc roles = cape_udc_get (qin->rinfo, "roles");
    
    if (roles == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_ROLE, "{ui switch} missing roles");
      goto exit_and_cleanup;
    }
    
    role_admin = cape_udc_get (roles, "auth_ui_su_w");
    if (role_admin == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_ROLE, "{ui switch} missing role");
      goto exit_and_cleanup;
    }
  }

  // for security reasons the switch is only possible from inside
  if (qin->pdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui switch} 'pdata' is missing");
    goto exit_and_cleanup;
  }

  gpid = cape_udc_get_n (qin->pdata, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui switch} 'gpid' is missing");
    goto exit_and_cleanup;
  }
  
  // optional
  wpid = cape_udc_get_n (qin->pdata, "wpid", 0);
  
  {
    // use the rinfo classes
    AuthRInfo auth_rinfo = auth_rinfo_new (self->adbl_session, wpid, gpid);
    
    // fetch all rinfo from database
    res = auth_rinfo_get_gpid (&auth_rinfo, gpid, &rinfo, &cdata, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  // replace the existing rinfo and cdata
  cape_udc_replace_mv (&(qout->rinfo), &rinfo);
  cape_udc_replace_mv (&(qout->cdata), &cdata);

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&rinfo);
  cape_udc_del (&cdata);

  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_config_get (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  number_t gpid;
  number_t wpid;

  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;

  if (qbus_message_role_has (qin, "admin"))
  {
    if (qin->cdata == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_CDATA");
      goto exit_and_cleanup;
    }
    
    wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
    gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
  }
  else
  {
    // do some security checks
    if (qin->rinfo == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_AUTH");
      goto exit_and_cleanup;
    }
    
    wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
    gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  }
  
  if (0 == wpid)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_WPID");
    goto exit_and_cleanup;
  }
  
  if (0 == gpid)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_GPID");
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "wpid"           , wpid);
    cape_udc_add_n      (params, "gpid"           , gpid);
    cape_udc_add_n      (values, "id"             , 0);
    cape_udc_add_n      (values, "opt_msgs"       , 0);
    cape_udc_add_n      (values, "opt_2factor"    , 0);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "rbac_users", &params, &values, err);
    if (query_results == NULL)
    {
      return cape_err_code (err);
    }
  }

  first_row = cape_udc_ext_first (query_results);
  if (first_row == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "{ui config get} entry not found");
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  cape_udc_replace_mv (&(qout->cdata), &first_row);
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  cape_udc_del (&first_row);

  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_config_set (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  number_t gpid;
  number_t wpid;
  CapeUdc opt_msgs_node;
  CapeUdc opt_2factor_node;
  CapeUdc opt_ttl_node;
  int is_admin = FALSE;

  // local objects
  AdblTrx adbl_trx = NULL;
  
  if (qbus_message_role_has (qin, "admin"))
  {
    if (qin->cdata == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_CDATA");
      goto exit_and_cleanup;
    }
    
    wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
    gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
    
    is_admin = TRUE;
  }
  else
  {
    // do some security checks
    if (qin->rinfo == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_AUTH");
      goto exit_and_cleanup;
    }
    
    wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
    gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  }
  
  if (0 == wpid)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_WPID");
    goto exit_and_cleanup;
  }
  
  if (0 == gpid)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_GPID");
    goto exit_and_cleanup;
  }

  // do some security checks
  if (NULL == qin->cdata)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NO_CDATA");
    goto exit_and_cleanup;
  }
  
  // optional
  opt_msgs_node = cape_udc_get (qin->cdata, "opt_msgs");

  // optional
  opt_2factor_node = cape_udc_get (qin->cdata, "opt_2factor");

  // optional
  opt_ttl_node = cape_udc_get (qin->cdata, "opt_tll");
  
  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n     (params, "wpid"        , wpid);
    cape_udc_add_n     (params, "gpid"        , gpid);

    if (opt_msgs_node)
    {
      cape_udc_add_n     (values, "opt_msgs"    , cape_udc_n (opt_msgs_node, 0));
    }
    
    if (opt_2factor_node)
    {
      cape_udc_add_n     (values, "opt_2factor" , cape_udc_n (opt_2factor_node, 0));
    }

    if (opt_ttl_node && is_admin)
    {
      cape_udc_add_n     (values, "opt_ttl"    , cape_udc_n (opt_ttl_node, 0));
    }
    
    // execute query
    res = adbl_trx_update (adbl_trx, "rbac_users", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);

  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_2f_send__has (CapeUdc list, number_t id)
{
  int ret = FALSE;
  
  // local objects
  CapeUdcCursor* cursor = cape_udc_cursor_new (list, CAPE_DIRECTION_FORW);

  while (cape_udc_cursor_next (cursor))
  {
    if (cape_udc_n (cursor->item, 0) == id)
    {
      ret = TRUE;
      goto exit_and_cleanup;
    }
  }
  
exit_and_cleanup:

  cape_udc_cursor_del (&cursor);
  return ret;
}

//-----------------------------------------------------------------------------

CapeUdc auth_ui_2f_send__find_mappin (CapeUdc mappin, number_t mappin_id)
{
  CapeUdc ret = NULL;
  
  // local objects
  CapeUdcCursor* cursor = NULL;
  
  if (mappin == NULL)
  {
    goto exit_and_cleanup;
  }
  
  if (cape_udc_type (mappin) != CAPE_UDC_LIST)
  {
    goto exit_and_cleanup;
  }

  cursor = cape_udc_cursor_new (mappin, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    if (cape_udc_get_n (cursor->item, "key", 0) == mappin_id)
    {
      ret = cape_udc_get (cursor->item, "val");
      goto exit_and_cleanup;
    }
  }
  
exit_and_cleanup:

  cape_udc_cursor_del (&cursor);
  return ret;
}

//-----------------------------------------------------------------------------

int auth_ui__intern__send_message (AuthUI* p_self, CapeUdc item, CapeUdc options, QBusM qin, QBusM qout, fct_qbus_onMessage fct, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  const CapeString module = NULL;
  const CapeString method = NULL;
  CapeUdc params = NULL;
  CapeUdc merges = NULL;
  CapeUdc mappin = NULL;
  
  if (options)
  {
    module = cape_udc_get_s (options, "module", NULL);
    method = cape_udc_get_s (options, "method", NULL);
    
    // params can contain additional values used in the message body
    params = cape_udc_get (options, "params");
    
    // this will extend the cdata used in the call
    merges = cape_udc_get (options, "merges");
    
    // convert the type value into cdata value
    mappin = cape_udc_get (options, "mappin");
  }
  
  if (module && method)
  {
    // clean up
    qbus_message_clr (qin, CAPE_UDC_NODE);
    
    // recipients
    {
      CapeUdc recipients = cape_udc_new (CAPE_UDC_LIST, "recipients");
      
      cape_udc_add_s_cp (recipients, NULL, cape_udc_get_s (item, "email", NULL));
      
      cape_udc_add (qin->cdata, &recipients);
    }
    
    // params
    {
      CapeUdc h = cape_udc_new (CAPE_UDC_NODE, "params");
      
      cape_udc_add_s_cp (h, "code", self->secret);
      
      if (params)
      {
        cape_udc_merge_cp (h, params);
      }
      
      cape_udc_add (qin->cdata, &h);
    }
    
    // mappin
    {
      CapeUdc h = auth_ui_2f_send__find_mappin (mappin, cape_udc_get_n (item, "type", 0));
      
      if (h)
      {
        cape_udc_merge_cp (qin->cdata, h);
      }
    }
    
    // merges
    if (merges)
    {
      cape_udc_merge_cp (qin->cdata, merges);
    }
    
    // send message to qbus
    res = qbus_continue (self->qbus, module, method, qin, (void**)p_self, fct, err);
  }
  else
  {
    *p_self = NULL;
    
    // do nothing just call the callback
    res = fct (self->qbus, self, qin, qout, err);
  }
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_2f_send__next (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

static int __STDCALL auth_ui_2f_send__on_call (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = ptr;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  res = auth_ui_2f_send__next (&self, qin, qout, err);
  
exit_and_cleanup:
  
  auth_ui_del (&self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_2f_send__next (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  // local objects
  CapeUdc item = cape_udc_ext_first (self->recipients);
  if (item)
  {
    res = auth_ui__intern__send_message (p_self, item, self->options_2factor, qin, qout, auth_ui_2f_send__on_call, err);
  }
  else
  {
    res = CAPE_ERR_NONE;
  }
  
exit_and_cleanup:
  
  cape_udc_del (&item);
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_2f_send (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  number_t wpid;
  number_t gpid;
  const CapeString vsec;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;
  
  CapeString h1 = NULL;
  
  if (!qbus_message_role_has (qin, "AUTH_2F"))
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "{ui 2f send} missing role");
    goto exit_and_cleanup;
  }
  
  wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui 2f send} 'wpid' is missing");
    goto exit_and_cleanup;
  }
  
  gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui 2f send} 'gpid' is missing");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui 2f send} missing cdata");
    goto exit_and_cleanup;
  }

  if (cape_udc_type (qin->cdata) != CAPE_UDC_LIST)
  {
    res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "{ui 2f send} cdata is not a list");
    goto exit_and_cleanup;
  }
  
  vsec = auth_vault__vsec (self->vault, wpid);
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "{ui 2f send} no vault");
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "gpid"         , gpid);
    
    cape_udc_add_n      (values, "id"           , 0);
    cape_udc_add_n      (values, "type"         , 0);
    cape_udc_add_s_cp   (values, "email"        , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "glob_emails", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    if (auth_ui_2f_send__has (qin->cdata, cape_udc_get_n (cursor->item, "id", 0)))
    {
      res = qcrypt__decrypt_row_text (vsec, cursor->item, "email", err);
      if (res)
      {
        goto exit_and_cleanup;
      }
      
      cape_log_fmt (CAPE_LL_TRACE, "AUTH", "ui 2f send", "send message to '%s'", cape_udc_get_s (cursor->item, "email", ""));
    }
    else
    {
      cape_udc_cursor_rm (query_results, cursor);
    }
  }

  // transfer ownership
  cape_udc_replace_mv (&(self->recipients), &query_results);
  
  // create code
  self->secret = cape_str_random_n (6);

  h1 = qcrypt__hash_sha256__hex_o (self->secret, 6, err);
  if (h1 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "{ui 2f send} can't hash code");
    goto exit_and_cleanup;
  }
  
  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n     (params, "wpid"          , wpid);
    cape_udc_add_n     (params, "gpid"          , gpid);
    
    cape_udc_add_s_mv  (values, "code_2factor"  , &h1);
    
    // execute query
    res = adbl_trx_update (adbl_trx, "rbac_users", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  printf ("CODE: %s\n", self->secret);
  
  adbl_trx_commit (&adbl_trx, err);
  
  res = auth_ui_2f_send__next (p_self, qin, qout, err);

exit_and_cleanup:
  
  cape_str_del (&h1);
  
  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);
  
  adbl_trx_rollback (&adbl_trx, err);
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui__internal__save_fp_code (AdblTrx adbl_trx, number_t wpid, number_t gpid, const CapeString code, CapeErr err)
{
  int res;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n     (params, "wpid"          , wpid);
    cape_udc_add_n     (params, "gpid"          , gpid);
    
    cape_udc_add_s_cp  (values, "code_fp"       , code);
    
    // execute query
    res = adbl_trx_update (adbl_trx, "rbac_users", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_fp_send__next (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err);

//-----------------------------------------------------------------------------

static int __STDCALL auth_ui_fp_send__on_call (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = ptr;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  res = auth_ui_fp_send__next (&self, qin, qout, err);
  
exit_and_cleanup:
  
  auth_ui_del (&self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_fp_send__next (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  // local objects
  CapeUdc item = cape_udc_ext_first (self->recipients);
  if (item)
  {
    number_t wpid = cape_udc_get_n (item, "wpid", 0);
    number_t gpid = cape_udc_get_n (item, "gpid", 0);

    cape_udc_del (&(qin->rinfo));
    
    qin->rinfo = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n (qin->rinfo, "wpid", wpid);
    cape_udc_add_n (qin->rinfo, "gpid", gpid);

    res = auth_ui__intern__send_message (p_self, item, self->options_fp, qin, qout, auth_ui_fp_send__on_call, err);
  }
  else
  {
    res = CAPE_ERR_NONE;
  }
  
exit_and_cleanup:
  
  cape_udc_del (&item);
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_fp_send__update_code (AuthUI self, CapeErr err)
{
  int res;
  
  // local objects
  AdblTrx adbl_trx = NULL;
  CapeUdcCursor* cursor = NULL;
  CapeString h1 = NULL;

  // hash the code for the database
  h1 = qcrypt__hash_sha256__hex_o (self->secret, 6, err);
  if (h1 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "{ui fp send} can't hash code");
    goto exit_and_cleanup;
  }

  // start transaction
  adbl_trx = adbl_trx_new (self->adbl_session, err);
  if (adbl_trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  cursor = cape_udc_cursor_new (self->recipients, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    number_t wpid;
    number_t gpid;
    const CapeString vsec;
    
    wpid = cape_udc_get_n (cursor->item, "wpid", 0);
    if (wpid == 0)
    {
      res = cape_err_set (err, CAPE_ERR_RUNTIME, "{ui fp send} no valid wpid");
      goto exit_and_cleanup;
    }
    
    gpid = cape_udc_get_n (cursor->item, "gpid", 0);
    if (gpid == 0)
    {
      res = cape_err_set (err, CAPE_ERR_RUNTIME, "{ui fp send} no valid gpid");
      goto exit_and_cleanup;
    }
    
    vsec = auth_vault__vsec (self->vault, wpid);
    if (vsec == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_RUNTIME, "{ui fp send} no vault");
      goto exit_and_cleanup;
    }
    
    res = qcrypt__decrypt_row_text (vsec, cursor->item, "email", err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    cape_log_fmt (CAPE_LL_TRACE, "AUTH", "ui fp send", "send message to '%s'", cape_udc_get_s (cursor->item, "email", ""));
    
    res = auth_ui__internal__save_fp_code (adbl_trx, wpid, gpid, h1, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  adbl_trx_rollback (&adbl_trx, err);
  
  cape_str_del (&h1);
  cape_udc_cursor_del (&cursor);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_fp_send (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  const CapeString user;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeString h1 = NULL;
  
  // don't check the rinfo, because there is none
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui fp send} missing cdata");
    goto exit_and_cleanup;
  }

  user = cape_udc_get_s (qin->cdata, "user", NULL);
  if (user == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui fp send} missing cdata");
    goto exit_and_cleanup;
  }
  
  h1 = qcrypt__hash_sha256__hex_o (user, cape_str_size (user), err);
  if (h1 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "{ui 2f send} can't hash code");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_s_mv   (params, "user512"      , &h1);
    
    cape_udc_add_n      (values, "wpid"         , 0);
    cape_udc_add_n      (values, "gpid"         , 0);

    cape_udc_add_s_cp   (values, "email"        , NULL);
    cape_udc_add_n      (values, "type"         , 0);

    /*
     auth_users_email_view
     
     select ru.gpid, ru.wpid, qu.user512, ge.type, ge.email from rbac_users ru join q5_users qu on qu.id = ru.userid join glob_emails ge on ge.gpid = ru.gpid;
     */

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "auth_users_email_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  if (cape_udc_size (query_results) == 0)
  {
    auth_ui__intern__write_log (self, "wrong user", qin->cdata);

    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "{ui fp send} wrong user");
    goto exit_and_cleanup;
  }
  
  // transfer ownership
  cape_udc_replace_mv (&(self->recipients), &query_results);
  
  // create code
  self->secret = cape_str_random_n (6);

  res = auth_ui_fp_send__update_code (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = auth_ui_fp_send__next (p_self, qin, qout, err);
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);

  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_users (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  const CapeString vsec;
  
  // local objects
  number_t wpid = 0;
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;
  
  if (qbus_message_role_has (qin, "admin"))
  {
    if (qin->cdata == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_CDATA");
      goto exit_and_cleanup;
    }

    wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
  }
  else if (qbus_message_role_or2 (qin, "auth_wp_lu", "auth_wacc"))
  {
    wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  }
  
  if (0 == wpid)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_ROLE");
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "wpid"         , wpid);
    
    cape_udc_add_n      (values, "userid"       , 0);
    cape_udc_add_n      (values, "gpid"         , 0);
    cape_udc_add_n      (values, "active"       , 0);
    cape_udc_add_n      (values, "state"        , 0);
    cape_udc_add_n      (values, "opt_msgs"     , 0);
    cape_udc_add_n      (values, "opt_2factor"  , 0);
    cape_udc_add_s_cp   (values, "opt_locale"   , NULL);
    cape_udc_add_n      (values, "opt_ttl"      , 0);
    cape_udc_add_s_cp   (values, "firstname"    , NULL);
    cape_udc_add_s_cp   (values, "lastname"     , NULL);
    cape_udc_add_d      (values, "last"         , NULL);
    cape_udc_add_s_cp   (values, "ip"           , NULL);
    cape_udc_add_n      (values, "logins"       , 0);

    /*
     auth_logins_last_view
     
     select wpid, gpid, userid, max(ltime) last, ip from auth_logins group by wpid, gpid;
     */
    
    /*
     auth_logins_cnt_view
     
     select wpid, gpid, userid, count(*) logins from auth_logins group by wpid, gpid;
     */
    /*
     rbac_users_view
     
     select ru.id, ru.wpid, ru.userid, ru.gpid, wp.name workspace, wp.token, ru.secret, gp.title, gp.firstname, gp.lastname, qu.active, ru.state, ru.opt_msgs, ru.opt_2factor, ru.opt_locale, ru.opt_ttl from rbac_users ru left join rbac_workspaces wp on wp.id = ru.wpid left join q5_users qu on qu.id = ru.userid join glob_persons gp on gp.id = ru.gpid;
     */
    /*
     rbac_users_logins_view
     
     select uv.wpid, uv.userid, uv.gpid, uv.active, uv.state, uv.opt_msgs, uv.opt_2factor, uv.opt_locale, uv.opt_ttl, uv.firstname, uv.lastname, lw.last, lw.ip, cv.logins from rbac_users_view uv left join auth_logins_last_view lw on lw.wpid = uv.wpid and lw.gpid = uv.gpid and lw.userid = uv.userid left join auth_logins_cnt_view cv on cv.wpid = uv.wpid and cv.gpid = uv.gpid and cv.userid = uv.userid
     */

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "rbac_users_logins_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  cursor = cape_udc_cursor_new (query_results, CAPE_DIRECTION_FORW);
  
  vsec = auth_vault__vsec (self->vault, wpid);
  if (vsec)
  {
    while (cape_udc_cursor_next (cursor))
    {
      res = qcrypt__decrypt_row_text (vsec, cursor->item, "firstname", err);
      if (res)
      {
        goto exit_and_cleanup;
      }

      res = qcrypt__decrypt_row_text (vsec, cursor->item, "lastname", err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  else
  {
    while (cape_udc_cursor_next (cursor))
    {
      cape_udc_put_s_cp (cursor->item, "firstname", "[encrypted]");
      cape_udc_put_s_cp (cursor->item, "lastname", "[encrypted]");
    }
  }

  cape_udc_replace_mv (&(qout->cdata), &query_results);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);

  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_ui_rm (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;

  number_t wpid;
  number_t gpid;
  number_t userid;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_CDATA");
    goto exit_and_cleanup;
  }

  // run security check
  if (qbus_message_role_has (qin, "admin"))
  {
    wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
  }
  else if (qbus_message_role_or2 (qin, "auth_wp_lu", "auth_wacc"))
  {
    wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  }
  
  if (0 == wpid)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_ROLE");
    goto exit_and_cleanup;
  }
  
  userid = cape_udc_get_n (qin->cdata, "userid", 0);
  if (0 == userid)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_USERID");
    goto exit_and_cleanup;
  }

  gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
  if (0 == gpid)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_GPID");
    goto exit_and_cleanup;
  }
  
  // check if account exists and userid and gpid belong together
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "wpid"         , wpid);
    cape_udc_add_n      (params, "userid"       , userid);
    cape_udc_add_n      (params, "gpid"         , gpid);

    cape_udc_add_n      (values, "id"           , 0);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "rbac_users_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  first_row = cape_udc_ext_first (query_results);
  if (NULL == first_row)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "ERR.NOT_FOUND");
    goto exit_and_cleanup;
  }

  
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&first_row);
  cape_udc_del (&query_results);
  
  auth_ui_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
