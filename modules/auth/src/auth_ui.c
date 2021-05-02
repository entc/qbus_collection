#include "auth_ui.h"
#include "auth_rinfo.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

struct AuthUI_s
{
  AdblSession adbl_session;        // reference
  AuthTokens tokens;               // reference
  AuthVault vault;                 // reference
  const CapeString err_log_file;   // reference
  
  number_t userid;
  number_t wpid;
  
  CapeString secret;
};

//-----------------------------------------------------------------------------

AuthUI auth_ui_new (AdblSession adbl_session, AuthTokens tokens, AuthVault vault, const CapeString err_log_file)
{
  AuthUI self = CAPE_NEW (struct AuthUI_s);
  
  self->adbl_session = adbl_session;
  self->tokens = tokens;
  self->vault = vault;
  self->err_log_file = err_log_file;
  
  self->userid = 0;
  self->wpid = 0;
  
  self->secret = NULL;

  return self;
}

//---------------------------------------------------------------------------

void auth_ui_del (AuthUI* p_self)
{
  if (*p_self)
  {
    AuthUI self = *p_self;
    
    cape_str_del (&(self->secret));
    
    CAPE_DEL (p_self, struct AuthUI_s);
  }
}

//---------------------------------------------------------------------------

CapeUdc auth_ui_crypt4__extract_from_content (const CapeString content)
{
  CapeUdc auth_crypt_credentials;
  
  // decode base64
  CapeStream s = qcrypt__decode_base64_s (content);
    
  // de-serialize into an UDC container
  auth_crypt_credentials = cape_json_from_buf (cape_stream_data (s), cape_stream_size (s));

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

int auth_ui__intern__save_2f_code (AuthUI self, CapeErr err)
{

  
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
  }
  
  res = CAPE_ERR_NONE;
  cape_udc_replace_mv (p_recipients, &query_results);
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);
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
  CapeUdc opt_2factor_node;
  number_t wpid;
  number_t gpid;
  number_t userid;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc auth_crypt_credentials = NULL;
  CapeString h1 = NULL;
  CapeString h2 = NULL;
  
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

    cape_udc_add_n      (values, "opt_msgs"     , 0);
    cape_udc_add_n      (values, "opt_2factor"  , 0);
    
    cape_udc_add_s_cp   (values, "code_2factor" , NULL);
    
    /*
     auth_users_secret_view
     
     select ru.id usid, gpid, wpid, qu.user512, qu.secret, qu.id userid, qu.active, ru.opt_msgs, ru.opt_2factor, ru.code_2factor, ru.secret secret_vault from rbac_users ru join q5_users qu on qu.id = ru.userid;
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
  
  // extract secret
  self->secret = cape_udc_ext_s (first_row, "secret");
  if (self->secret == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no secret defined");
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

  vsec = auth_vault__vsec (self->vault, wpid);
  if (vsec == NULL)
  {
    const CapeString vault_password = cape_udc_get_s (auth_crypt_credentials, "vault", NULL);
    const CapeString vault_secret = cape_udc_get_s (first_row, "secret_vault", NULL);
    
    if (vault_password == NULL)
    {
      cape_log_fmt (CAPE_LL_TRACE, "AUTH", "ui crypt4", "no vault found -> missing parameter");

      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "vault");
      goto exit_and_cleanup;
    }

    cape_log_fmt (CAPE_LL_TRACE, "AUTH", "ui crypt4", "no vault found -> continue with parameter");

    if (vault_secret == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no vault defined");
      goto exit_and_cleanup;
    }

    h1 = qcrypt__decrypt (self->secret, vault_password, err);
    if (h1 == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "wrong vault");
      goto exit_and_cleanup;
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
  
  opt_2factor_node = cape_udc_get (first_row, "opt_2factor");
  if (opt_2factor_node == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no 2factor");
    goto exit_and_cleanup;
  }
  
  // check the status of the 2factor authentication
  if (cape_udc_n (opt_2factor_node, 0) == 1)
  {
    const CapeString code_2f_from_db = cape_udc_get_s (first_row, "code_2factor", NULL);
    
    if (code_2f_from_db)
    {
      // check if the code was given by the user
      
    }
    else
    {
      qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      {
        CapeUdc recipients = NULL;
        
        res = auth_ui__intern__get_recipients (self, wpid, gpid, &recipients, vsec, err);
        if (res)
        {
          goto exit_and_cleanup;
        }
        
        cape_udc_add_name (qout->cdata, &recipients, "recipients");
      }
      
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "2f_code");
      goto exit_and_cleanup;
    }
  }
  
  // get userid
  self->userid = cape_udc_get_n (first_row, "userid", 0);
  
  res = auth_ui_crypt4__compare_password (self, cha, cda, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_log_fmt (CAPE_LL_TRACE, "AUTH", "ui crypt4", "password match");

  {
    // use the rinfo classes
    AuthRInfo rinfo = auth_rinfo_new (self->adbl_session, self->userid, self->wpid);
    
    // fetch all rinfo from database
    res = auth_rinfo_get (&rinfo, qout, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  cape_udc_del (&auth_crypt_credentials);
  return res;
}

//---------------------------------------------------------------------------

int auth_ui_token (AuthUI self, CapeUdc extras, QBusM qout, CapeErr err)
{
  int res;
  
  const CapeString token;
  
  token = cape_udc_get_s (extras, "__T", NULL);
  if (token == NULL)
  {
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "extras has no token");
  }

  res = auth_tokens_fetch (self->tokens, token, qout, err);
  if (res)
  {
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "token not found");
  }
  
  return CAPE_ERR_NONE;
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

void auth_ui__intern__write_log (AuthUI self, CapeUdc cdata)
{
  int res;
  const CapeString remote = "unknown";
  
  // local objects
  CapeErr err = cape_err_new ();
  CapeFileHandle fh = NULL;
  CapeStream s = cape_stream_new ();
  
  {
    CapeDatetime dt; cape_datetime_utc (&dt);
    cape_stream_append_d (s, &dt);
  }

  cape_stream_append_c (s, ' ');

  if (cdata)
  {
    remote = cape_udc_get_s (cdata, "remote", NULL);
  }

  cape_stream_append_str (s, remote);
  cape_stream_append_c (s, ' ');

  cape_stream_append_str (s, "access denied");

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
      res = auth_ui_token (self, auth_extras, qout, err);
      goto exit_and_cleanup;
    }
  }

  res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "authentication type not supported");

exit_and_cleanup:
  
  if (*p_self && res == CAPE_ERR_NO_AUTH)
  {
    auth_ui__intern__write_log (self, qin->cdata);
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

  // insert into logins
  {
    number_t id;
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // parameters
    cape_udc_add_n      (values, "wpid"         , self->wpid);
    cape_udc_add_n      (values, "gpid"         , gpid);
    cape_udc_add_n      (values, "userid"       , usid);

    {
      CapeDatetime dt;
      cape_datetime_utc (&dt);
      
      cape_udc_add_d    (values, "ltime"        , &dt);
    }
    
    {
      const CapeString remote = cape_udc_get_s (qin->rinfo, "remote", NULL);
      if (remote)
      {
        cape_udc_add_s_cp (values, "ip", remote);
      }
    }
    
    if (qin->cdata)
    {
      CapeUdc info = cape_udc_ext (qin->cdata, "info");
      if (info)
      {
        cape_udc_add (values, &info);
      }
    }
    
    // execute query
    id = adbl_trx_insert (adbl_trx, "auth_logins", &values, err);
    if (id == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  adbl_trx_commit (&adbl_trx, err);
  res = CAPE_ERR_NONE;
  
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
    cape_udc_add_n     (values, "state"     , 2);
    
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

int auth_ui_set__q5_users__update (AuthUI self, AdblTrx adbl_trx, number_t userid, const CapeString q5_user, const CapeString secret, CapeErr err)
{
  int res;

  // local objects
  CapeString h1 = auth_ui__intern__construct_password (q5_user, secret, err);
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

int auth_ui_set (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  number_t wpid;
  number_t gpid;
  number_t userid;
  const CapeString q5_user;
  const CapeString q5_pass;
  const CapeString secret;
  CapeUdc first_row;

  // local objects
  AdblTrx adbl_trx = NULL;
  CapeUdc query_results = NULL;
  
  CapeString h0 = NULL;
  CapeString h1 = NULL;

  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
    goto exit_and_cleanup;
  }

  wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing wpid");
    goto exit_and_cleanup;
  }
  
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
  
  // optional (if given we need to check)
  q5_pass = cape_udc_get_s (qin->cdata, "pass", NULL);

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
  
  if (q5_pass)
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

  res = auth_ui_set__q5_users__update (self, adbl_trx, userid, q5_user, secret, err);
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
    AuthRInfo auth_rinfo = auth_rinfo_new (self->adbl_session, 0, wpid);
    
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

  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "{ui config get} missing rinfo");
    goto exit_and_cleanup;
  }

  wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui config get} 'wpid' is missing");
    goto exit_and_cleanup;
  }

  gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui config get} 'gpid' is missing");
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

  // local objects
  AdblTrx adbl_trx = NULL;
  
  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "{ui config set} missing rinfo");
    goto exit_and_cleanup;
  }
  
  wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui config set} 'wpid' is missing");
    goto exit_and_cleanup;
  }
  
  gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui config set} 'gpid' is missing");
    goto exit_and_cleanup;
  }

  // do some security checks
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "{ui config set} missing cdata");
    goto exit_and_cleanup;
  }
  
  opt_msgs_node = cape_udc_get (qin->cdata, "opt_msgs");
  if (opt_msgs_node == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui config set} 'opt_msgs' is missing");
    goto exit_and_cleanup;
  }

  opt_2factor_node = cape_udc_get (qin->cdata, "opt_2factor");
  if (opt_2factor_node == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "{ui config set} 'opt_2factor' is missing");
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
    cape_udc_add_n     (params, "wpid"        , wpid);
    cape_udc_add_n     (params, "gpid"        , gpid);

    cape_udc_add_n     (values, "opt_msgs"    , cape_udc_n (opt_msgs_node, 0));
    cape_udc_add_n     (values, "opt_2factor" , cape_udc_n (opt_2factor_node, 0));

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
