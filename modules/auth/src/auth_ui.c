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
  AdblSession adbl_session;   // reference  
  AuthTokens tokens;          // reference
  AuthVault vault;           // reference

  number_t userid;
  number_t wpid;
  
  CapeString secret;
};

//-----------------------------------------------------------------------------

AuthUI auth_ui_new (AdblSession adbl_session, AuthTokens tokens, AuthVault vault)
{
  AuthUI self = CAPE_NEW (struct AuthUI_s);
  
  self->adbl_session = adbl_session;
  self->tokens = tokens;
  
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

int auth_ui_crypt4__fetch_from_db (AuthUI self, const CapeString username, CapeErr err)
{
  int res;
  
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

  // conditions
  cape_udc_add_s_cp   (params, "user512"      , username);
  cape_udc_add_n      (params, "active"       , 1);

  // return values
  cape_udc_add_n      (values, "id"           , 0);
  cape_udc_add_s_cp   (values, "secret"       , NULL);
  
  // execute the query
  CapeUdc results = adbl_session_query (self->adbl_session, "q5_users", &params, &values, err);
  if (results == NULL)
  {
    cape_log_msg (CAPE_LL_ERROR, "AUTH", "crypt4", cape_err_text (err));

    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  CapeUdc first_row = cape_udc_get_first (results);
  if (first_row == NULL)
  {
    cape_log_fmt (CAPE_LL_WARN, "AUTH", "crypt4", "user not found");
    
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "no authentication");
    goto exit_and_cleanup;
  }

  // get userid
  self->userid = cape_udc_get_n (first_row, "id", 0);

  // extract secret
  self->secret = cape_udc_ext_s (first_row, "secret");
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&results);
  return res;
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

int auth_ui_crypt4__check_password (AuthUI self, CapeUdc auth_crypt_credentials, CapeErr err)
{
  int res;
  
  const CapeString cid;
  const CapeString cha;
  const CapeString cda;

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

  res = auth_ui_crypt4__fetch_from_db (self, cid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = auth_ui_crypt4__compare_password (self, cha, cda, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//---------------------------------------------------------------------------

int auth_ui_crypt4 (AuthUI* p_self, const CapeString content, CapeUdc extras, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  CapeUdc auth_crypt_credentials = NULL;
  
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
  
  // check if given credentials were OK
  res = auth_ui_crypt4__check_password (self, auth_crypt_credentials, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
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
  
  cape_udc_del (&auth_crypt_credentials);

  auth_ui_del (p_self);
  return res;
}

//---------------------------------------------------------------------------

int auth_ui_token (AuthUI* p_self, CapeUdc extras, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
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
  
  auth_ui_del (p_self);

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

int auth_ui_get (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
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
      
      return auth_ui_crypt4 (p_self, auth_content, auth_extras, qout, err);
    }
    case 4:
    {
      return auth_ui_token (p_self, auth_extras, qout, err);
    }
  }

  res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "authentication type not supported");

exit_and_cleanup:
  
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

int auth_ui_set (AuthUI* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthUI self = *p_self;
  
  number_t wpid;
  number_t gpid;
  const CapeString password;
  const CapeString secret;
  CapeUdc first_row;
  const CapeString vsec;

  // local objects
  AdblTrx adbl_trx = NULL;
  CapeUdc query_results = NULL;

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

  // optional (if given we need to check)
  password = cape_udc_get_s (qin->cdata, "password", NULL);

  vsec = auth_vault__vsec (self->vault, wpid);
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing vault");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // conditions
    cape_udc_add_n      (params, "wpid"         , wpid);
    cape_udc_add_n      (params, "gpid"         , gpid);
    cape_udc_add_n      (params, "active"       , 1);
    cape_udc_add_n      (values, "usid"         , 0);
    cape_udc_add_n      (values, "userid"       , 0);
    cape_udc_add_s_cp   (values, "secret"       , NULL);
    
    /*
     auth_users_secret_view
     
     select ru.id usid, gpid, wpid, qu.secret, qu.id userid, qu.active from rbac_users ru join q5_users qu on qu.id = ru.userid;
     */
    
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

  if (password)
  {
    const CapeString old_password_db = cape_udc_get_s (first_row, "secret", NULL);

    if (!cape_str_equal (old_password_db, password))
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

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n     (params, "id"        , cape_udc_get_n (first_row, "userid", 0));
    
    cape_udc_add_s_cp  (values, "secret"    , NULL);

    // execute query
    res = adbl_trx_update (adbl_trx, "q5_users", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n     (params, "id"        , cape_udc_get_n (first_row, "usid", 0));
    
    cape_udc_add_s_cp  (values, "secret"    , NULL);
    cape_udc_add_n     (values, "state"     , 2);
    
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
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "AUTH", "ui set", cape_err_text (err));
  }
  
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
  
  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
    goto exit_and_cleanup;
  }

  // check role
  {
    CapeUdc role_admin;
    CapeUdc roles = cape_udc_get (qin->rinfo, "roles");
    
    if (roles == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing roles");
      goto exit_and_cleanup;
    }
    
    role_admin = cape_udc_get (roles, "tmpl_edit");
    if (role_admin == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing role");
      goto exit_and_cleanup;
    }
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
