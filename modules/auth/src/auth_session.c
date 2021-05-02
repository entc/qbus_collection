#include "auth_session.h"
#include "auth_rinfo.h"

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_mutex.h>
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

struct AuthSession_s
{
  AdblSession adbl_session;  // reference
  AuthVault vault;           // reference

  number_t wpid;             // rinfo: mandatory
  number_t gpid;             // rinfo: mandatory
  
  CapeString vsec;
};

//-----------------------------------------------------------------------------

AuthSession auth_session_new (AdblSession adbl_session, AuthVault vault)
{
  AuthSession self = CAPE_NEW (struct AuthSession_s);
  
  self->adbl_session = adbl_session;
  self->vault = vault;
  
  self->wpid = 0;
  self->gpid = 0;
  
  self->vsec = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void auth_session_del (AuthSession* p_self)
{
  if (*p_self)
  {
    AuthSession self = *p_self;
    
    cape_str_del (&(self->vsec));

    CAPE_DEL (p_self, struct AuthSession_s);
  }
}

//-----------------------------------------------------------------------------

int auth_session_add (AuthSession* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthSession self = *p_self;
  
  number_t vp = 600;   // 10 minutes
  number_t type;
  
  // local objects
  CapeString session_token = cape_str_uuid ();
  CapeString session_token_hash = NULL;
  
  CapeString h1 = NULL;
  CapeString h2 = NULL;
  CapeString h3 = NULL;

  AdblTrx trx = NULL;
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;

  CapeUdc roles = NULL;

  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
    goto exit_and_cleanup;
  }
  
  self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing wpid");
    goto exit_and_cleanup;
  }
  
  self->gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (self->gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing gpid");
    goto exit_and_cleanup;
  }

  // do some security checks
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing cdata");
    goto exit_and_cleanup;
  }

  type = cape_udc_get_n (qin->cdata, "type", 1);
  if (type == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing type");
    goto exit_and_cleanup;
  }

  self->vsec = cape_str_cp (auth_vault__vsec (self->vault, self->wpid));
  if (self->vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing vault");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "wp_active"   , 1);
    cape_udc_add_n      (params, "wpid"        , self->wpid);
    cape_udc_add_n      (params, "gpid"        , self->gpid);

    cape_udc_add_n      (values, "state"       , 0);
    
    cape_udc_add_s_cp   (values, "firstname"   , NULL);
    cape_udc_add_s_cp   (values, "lastname"    , NULL);
    cape_udc_add_s_cp   (values, "workspace"   , NULL);

    cape_udc_add_d      (values, "lt"          , NULL);
    cape_udc_add_d      (values, "lu"          , NULL);

    cape_udc_add_s_cp   (values, "remote"      , NULL);

    /*
     auth_sessions_wp_view
     
     select wp.id wpid, gp.id gpid, ru.id usid, ru.state state, gp.firstname, gp.lastname, wp.name workspace, au.token, au.lt lt, au.lu lu, au.vp vp, au.remote, au.roles, wp.active wp_active from rbac_workspaces wp join glob_persons gp on gp.wpid = wp.id join rbac_users ru on ru.wpid = wp.id and ru.gpid = gp.id left join auth_sessions au on au.wpid = wp.id and au.gpid = gp.id;
     */
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "auth_sessions_wp_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_ext_first (query_results);
  if (first_row == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't find global person");
    goto exit_and_cleanup;
  }

  roles = cape_udc_ext (qin->rinfo, "roles");
  if (roles == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing roles");
    goto exit_and_cleanup;
  }

  h1 = cape_json_to_s (roles);
  if (h1 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't serialize roles");
    goto exit_and_cleanup;
  }
  
  h2 = qcrypt__encrypt (self->vsec, h1, err);
  if (h1 == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  {
    const CapeString remote = cape_udc_get_s (qin->rinfo, "remote", NULL);
    if (remote)
    {
      h3 = qcrypt__encrypt (self->vsec, remote, err);
      if (h3 == NULL)
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }
  }

  // create the hash of the token
  session_token_hash = qcrypt__hash_sha256__hex_o (session_token, cape_str_size (session_token), err);
  if (session_token_hash == NULL)
  {
    res = cape_err_code (err);
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
    number_t id;
    
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // insert values
    cape_udc_add_n      (values, "id"           , ADBL_AUTO_SEQUENCE_ID);
    
    cape_udc_add_n      (params, "wpid"         , self->wpid);
    cape_udc_add_n      (params, "type"         , type);
    cape_udc_add_n      (params, "gpid"         , self->gpid);

    cape_udc_add_s_mv   (values, "token"         , &session_token_hash);

    {
      CapeDatetime dt; cape_datetime_utc (&dt);
      cape_udc_add_d (values, "lt", &dt);
      cape_udc_add_d (values, "lu", &dt);
    }

    cape_udc_add_n      (values, "vp"            , vp);

    cape_udc_add_s_mv   (values, "roles"         , &h2);

    if (h3)
    {
      cape_udc_add_s_mv   (values, "remote"      , &h3);
    }
    
    // execute query
    id = adbl_trx_inorup (trx, "auth_sessions", &params, &values, err);
    if (id == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  {
    CapeUdc node = cape_udc_ext (first_row, "firstname");
    if (node)
    {
      CapeString h4 = qcrypt__decrypt(self->vsec, cape_udc_s (node, ""), err);
      if (h4 == NULL)
      {
        cape_err_clr (err);
        cape_udc_add (first_row, &node);
      }
      else
      {
        cape_udc_add_s_mv (first_row, "firstname", &h4);
        cape_udc_del (&node);
      }
    }
  }

  {
    CapeUdc node = cape_udc_ext (first_row, "lastname");
    if (node)
    {
      CapeString h4 = qcrypt__decrypt(self->vsec, cape_udc_s (node, ""), err);
      if (h4 == NULL)
      {
        cape_err_clr (err);
        cape_udc_add (first_row, &node);
      }
      else
      {
        cape_udc_add_s_mv (first_row, "lastname", &h4);
        cape_udc_del (&node);
      }
    }
  }

  {
    CapeUdc node = cape_udc_ext (first_row, "remote");
    if (node)
    {
      CapeString h4 = qcrypt__decrypt(self->vsec, cape_udc_s (node, ""), err);
      if (h4 == NULL)
      {
        cape_err_clr (err);
        cape_udc_add (first_row, &node);
      }
      else
      {
        cape_udc_add_s_mv (first_row, "remote", &h4);
        cape_udc_del (&node);
      }
    }
  }

  // set the wpid and gpid
  cape_udc_add_n      (first_row, "wpid"        , self->wpid);
  cape_udc_add_n      (first_row, "gpid"        , self->gpid);
  
  // set the vp
  cape_udc_add_n (first_row, "vp", vp);
  
  res = CAPE_ERR_NONE;
  adbl_trx_commit (&trx, err);

  // create the output
  qout->cdata = cape_udc_mv (&first_row);
  
  // add token
  cape_udc_add_s_mv (qout->cdata, "token", &session_token);
  
  // add roles
  cape_udc_add (qout->cdata, &roles);
  
exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  
  cape_udc_del (&roles);
  
  cape_str_del (&session_token);
  cape_str_del (&session_token_hash);
  
  cape_str_del (&h1);
  cape_str_del (&h2);

  auth_session_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_session_get__update (AuthSession self, number_t session_id, CapeErr err)
{
  int res;

  // local objects
  AdblTrx trx = NULL;

  // start a new transaction
  trx = adbl_trx_new (self->adbl_session, err);
  if (trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n   (params, "id"           , session_id);
    
    {
      CapeDatetime dt; cape_datetime_utc (&dt);
      cape_udc_add_d (values, "lu", &dt);
    }

    // execute query
    res = adbl_trx_update (trx, "auth_sessions", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;
  adbl_trx_commit (&trx, err);

exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int auth_session_get (AuthSession* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthSession self = *p_self;
  
  const CapeString session_token;
  const CapeString roles_ecrypted;
  CapeUdc first_row;
  number_t vp;
  
  // local objects
  CapeString session_token_hash = NULL;
  CapeUdc query_results = NULL;
  CapeString h1 = NULL;
  CapeUdc roles = NULL;
  
  // do some security checks
  if (qin->pdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing pdata");
    goto exit_and_cleanup;
  }

  session_token = cape_udc_get_s (qin->pdata, "token", NULL);
  if (session_token == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing token");
    goto exit_and_cleanup;
  }
  
  // create the hash of the token
  session_token_hash = qcrypt__hash_sha256__hex_o (session_token, cape_str_size (session_token), err);
  if (session_token_hash == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_s_mv   (params, "token"         , &session_token_hash);
    cape_udc_add_n      (values, "wp_active"     , 0);
    cape_udc_add_d      (values, "lu"            , NULL);
    cape_udc_add_n      (values, "vp"            , 0);
    cape_udc_add_n      (values, "wpid"          , 0);
    cape_udc_add_n      (values, "gpid"          , 0);
    cape_udc_add_s_cp   (values, "roles"         , "{\"size\": 2000}");
    cape_udc_add_s_cp   (values, "firstname"     , NULL);
    cape_udc_add_s_cp   (values, "lastname"      , NULL);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "auth_sessions_wp_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_get_first (query_results);
  if (first_row == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "token not found");
    goto exit_and_cleanup;
  }

  vp = cape_udc_get_n (first_row, "vp", 0);
  if (vp)
  {
    CapeDatetime dt;
    time_t t1, t2;
    
    // in case the valid period was set we need to check it
    const CapeDatetime* lu = cape_udc_get_d (first_row, "lu", NULL);
    if (lu == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "token not found");
      goto exit_and_cleanup;
    }
    
    // get current datetime
    cape_datetime_utc (&dt);
    
    // convert into unix time_t
    t1 = cape_datetime_n__unix (lu);
    t2 = cape_datetime_n__unix (&dt);
    
    if (t1 + vp < t2)
    {
      res = cape_err_set (err, CAPE_ERR_NO_AUTH, "timeout");
      goto exit_and_cleanup;
    }
  }
  
  self->wpid = cape_udc_get_n (first_row, "wpid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing wpid");
    goto exit_and_cleanup;
  }
  
  self->gpid = cape_udc_get_n (first_row, "gpid", 0);
  if (self->gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing gpid");
    goto exit_and_cleanup;
  }

  self->vsec = cape_str_cp (auth_vault__vsec (self->vault, self->wpid));
  if (self->vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing vault");
    goto exit_and_cleanup;
  }

  roles_ecrypted = cape_udc_get_s (first_row, "roles", NULL);
  if (roles_ecrypted == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing rinfo");
    goto exit_and_cleanup;
  }
  
  h1 = qcrypt__decrypt (self->vsec, roles_ecrypted, err);
  if (h1 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "can't decrypt roles");
    goto exit_and_cleanup;
  }
  
  roles = cape_json_from_s (h1);
  if (roles == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "can't deserialize roles");
    goto exit_and_cleanup;
  }
  
  res = auth_session_get__update (self, cape_udc_get_n (first_row, "id", 0), err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // to be safe
  if (qout->rinfo == NULL)
  {
    qout->rinfo = cape_udc_new (CAPE_UDC_NODE, NULL);
  }
  
  // construct the rinfo output
  cape_udc_add_name (qout->rinfo, &roles, "roles");
  cape_udc_add_n    (qout->rinfo, "wpid", self->wpid);
  cape_udc_add_n    (qout->rinfo, "gpid", self->gpid);

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_str_del (&session_token_hash);
  cape_udc_del (&query_results);
  cape_str_del (&h1);
  cape_udc_del (&roles);

  auth_session_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_session_roles (AuthSession* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthSession self = *p_self;

  // local objects
  CapeUdc roles = NULL;
  
  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
    goto exit_and_cleanup;
  }

  roles = cape_udc_ext (qin->rinfo, "roles");
  if (roles == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing roles");
    goto exit_and_cleanup;
  }
  
  qout->cdata = cape_udc_mv (&roles);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&roles);
  
  auth_session_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
