#include "auth_session.h"
#include "auth_rinfo.h"
#include "auth_ui.h"

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
  
  number_t session_ttl = 600;   // 10 minutes
  number_t type;
  
  // local objects
  CapeString session_token = cape_str_uuid ();
  CapeString session_token_hash = NULL;
  CapeString session_locale = NULL;
  CapeString sec = NULL;
  
  CapeString h1 = NULL;
  CapeString h2 = NULL;
  CapeString h3 = NULL;
  
  CapeString serialized_output = NULL;
  CapeString encrypted_output = NULL;
  
  AdblTrx trx = NULL;
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;

  QBusM msg = NULL;
  CapeUdc roles = NULL;

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

  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_RINFO");
    goto exit_and_cleanup;
  }

  sec = cape_udc_ext_s (qin->rinfo, "sec");
  if (NULL == sec)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "ERR.NO_SEC");
    goto exit_and_cleanup;
  }
  
  switch (type)
  {
    case 1:
    {
      self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
      if (self->wpid == 0)
      {
        res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_WPID");
        goto exit_and_cleanup;
      }
      
      self->gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
      if (self->gpid == 0)
      {
        res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_GPID");
        goto exit_and_cleanup;
      }

      break;
    }
    default:
    {
      // only admin can add other types
      if (qbus_message_role_has (qin, "admin"))
      {
        self->wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
        if (self->wpid == 0)
        {
          res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing wpid");
          goto exit_and_cleanup;
        }
        
        self->gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
        if (self->gpid == 0)
        {
          res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing gpid");
          goto exit_and_cleanup;
        }
        
        // optional
        session_ttl = cape_udc_get_n (qin->cdata, "session_ttl", 0);
      }

      break;
    }
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

    cape_udc_add_s_cp   (values, "opt_locale"  , NULL);
    cape_udc_add_n      (values, "opt_ttl"     , 0);

    /*
     auth_sessions_wp_view
     
     select au.id id, wp.id wpid, gp.id gpid, ru.id usid, ru.state state, gp.firstname, gp.lastname, wp.name workspace, au.token, au.lt lt, au.lu lu, au.vp vp, au.remote, au.roles, au.ha_value, wp.active wp_active, qu.secret, ru.opt_locale, ru.opt_ttl from rbac_workspaces wp join glob_persons gp on gp.wpid = wp.id join rbac_users ru on ru.wpid = wp.id and ru.gpid = gp.id left join auth_sessions au on au.wpid = wp.id and au.gpid = gp.id join q5_users qu on qu.id = ru.userid;
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

  {
    CapeUdc h = cape_udc_get (first_row, "opt_locale");
    if (h)
    {
      session_locale = cape_udc_s_mv (h, NULL);
    }
  }
  {
    CapeUdc h = cape_udc_get (first_row, "opt_ttl");
    if (h)
    {
      session_ttl = cape_udc_n (h, session_ttl);
    }
  }
  
  msg = qbus_message_new (NULL, NULL);

  {
    // use the rinfo classes
    AuthRInfo rinfo = auth_rinfo_new (self->adbl_session, self->wpid, self->gpid);
    
    // fetch all rinfo from database
    res = auth_rinfo_get (&rinfo, &(msg->rinfo), &(msg->cdata), err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  roles = cape_udc_ext (msg->rinfo, "roles");
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

    cape_udc_add_n      (values, "vp"            , session_ttl);

    cape_udc_add_s_mv   (values, "roles"         , &h2);
    
    // reset the HA value with a fresh session
    cape_udc_add_s_cp   (values, "ha_value"      , "0");
    
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
  
  // add to log
  // insert into logins
  res = auth_ui__save_log_entry (trx, self->wpid, self->gpid, cape_udc_get_n (qin->rinfo, "userid", 0), qin->rinfo, qin->cdata, 1, err);
  if (res)
  {
    goto exit_and_cleanup;
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
  cape_udc_add_n (first_row, "vp", session_ttl);
  

  // add token
  cape_udc_add_s_mv (first_row, "token", &session_token);
  
  // add roles
  cape_udc_add (first_row, &roles);

  serialized_output = cape_json_to_s (first_row);
  if (NULL == serialized_output)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't serialize roles");
    goto exit_and_cleanup;
  }
  
  encrypted_output = qcrypt__encrypt (sec, serialized_output, err);
  if (NULL == encrypted_output)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  // create the output
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_s_mv (qout->cdata, "aitem", &encrypted_output);
  
  res = CAPE_ERR_NONE;
  adbl_trx_commit (&trx, err);
  
exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  
  cape_str_del (&serialized_output);
  cape_str_del (&encrypted_output);
  
  cape_udc_del (&query_results);
  cape_udc_del (&roles);
  
  cape_str_del (&session_token);
  cape_str_del (&session_token_hash);
  
  cape_str_del (&h1);
  cape_str_del (&h2);

  auth_session_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_session_get__update (AuthSession self, number_t session_id, const CapeString ha, CapeErr err)
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

    cape_udc_add_s_cp (values, "ha_value", ha);
    
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

int auth_session__internal__check_da (const CapeString secret, const CapeString ha, const CapeString da, CapeErr err)
{
  int res;
  
  // local objects
  CapeString h1 = cape_str_catenate_c (ha, ':', secret);
  CapeString h2 = qcrypt__hash_sha256__hex_o (h1, cape_str_size (h1), err);
  
  if (h2 == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  res = cape_str_equal (h2, da) ? CAPE_ERR_NONE : cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.DA_MISSMATCH");
  
exit_and_cleanup:

  cape_str_del (&h1);
  cape_str_del (&h2);
  return res;
}

//-----------------------------------------------------------------------------

int auth_session_get (AuthSession* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthSession self = *p_self;
  
  const CapeString session_token;
  const CapeString session_ha;
  const CapeString session_da;
  const CapeString roles_ecrypted;
  const CapeString secret;
  CapeUdc first_row;
  number_t vp;
  
  // local objects
  CapeString session_token_hash = NULL;
  CapeUdc query_results = NULL;
  CapeString h1 = NULL;
  CapeUdc roles = NULL;
  CapeString ha_last = NULL;
  CapeString ha_current = NULL;
  
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
  
  session_ha = cape_udc_get_s (qin->pdata, "ha", NULL);
  if (session_ha == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing ha");
    goto exit_and_cleanup;
  }

  session_da = cape_udc_get_s (qin->pdata, "da", NULL);
  if (session_da == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing da");
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
    cape_udc_add_n      (values, "id"            , 0);
    cape_udc_add_n      (values, "wp_active"     , 0);
    cape_udc_add_d      (values, "lu"            , NULL);
    cape_udc_add_n      (values, "vp"            , 0);
    cape_udc_add_n      (values, "wpid"          , 0);
    cape_udc_add_n      (values, "gpid"          , 0);
    cape_udc_add_s_cp   (values, "roles"         , "{\"size\": 2000}");
    cape_udc_add_s_cp   (values, "firstname"     , NULL);
    cape_udc_add_s_cp   (values, "lastname"      , NULL);
    cape_udc_add_s_cp   (values, "secret"        , NULL);
    cape_udc_add_s_cp   (values, "ha_value"      , NULL);

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

  // AK: timeout can only be handled by the server
  // -> client must be synced with the server session timeouts
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
  
  secret = cape_udc_get_s (first_row, "secret", NULL);
  if (secret == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing secret");
    goto exit_and_cleanup;
  }

  // calculate the the DA from session_ha and secret
  // -> check if they match
  res = auth_session__internal__check_da (secret, session_ha, session_da, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  self->wpid = cape_udc_get_n (first_row, "wpid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing wpid");
    goto exit_and_cleanup;
  }
  
  self->gpid = cape_udc_get_n (first_row, "gpid", 0);
  if (self->gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing gpid");
    goto exit_and_cleanup;
  }

  self->vsec = cape_str_cp (auth_vault__vsec (self->vault, self->wpid));
  if (self->vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing vault");
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
  
  // fetch the last known ha value
  ha_last = cape_str_ln_normalize (cape_udc_get_s (first_row, "ha_value", NULL));
  ha_current = cape_str_ln_normalize (session_ha);
  
  //cape_log_fmt (CAPE_LL_TRACE, "AUTH", "session get", "compare ha: current = %s, last = %s", ha_current, ha_last);
  
  /*
   AK: check doesn't work, because the sorting is stable
  if (cape_str_ln_cmp (ha_current, ha_last) <= 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.ALREADY_GRANTED");
    goto exit_and_cleanup;
  }
   */

  res = auth_session_get__update (self, cape_udc_get_n (first_row, "id", 0), ha_current, err);
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

  // to be safe
  if (qout->pdata == NULL)
  {
    qout->pdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  }

  cape_udc_add_s_cp (qout->pdata, "vsec", secret);
  cape_udc_add_n (qout->pdata, "ttl", vp);

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_str_del (&ha_last);
  cape_str_del (&ha_current);
  cape_str_del (&session_token_hash);
  cape_udc_del (&query_results);
  cape_str_del (&h1);
  cape_udc_del (&roles);

  auth_session_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_session_rm (AuthSession* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthSession self = *p_self;

  number_t seid;
  
  // local objects
  AdblTrx trx = NULL;

  // do some security checks
  if (FALSE == qbus_message_role_has (qin, "admin"))
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_ROLE");
    goto exit_and_cleanup;
  }

  // do some security checks
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NO_CDATA");
    goto exit_and_cleanup;
  }

  self->wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_WPID");
    goto exit_and_cleanup;
  }

  self->gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
  if (self->gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_GPID");
    goto exit_and_cleanup;
  }

  seid = cape_udc_get_n (qin->cdata, "seid", 0);
  if (seid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_SEID");
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
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n   (params, "id"           , seid);
    cape_udc_add_n   (params, "wpid"         , self->wpid);
    cape_udc_add_n   (params, "gpid"         , self->gpid);
    
    // execute query
    res = adbl_trx_delete (trx, "auth_sessions", &params, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  adbl_trx_commit (&trx, err);
  
exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);

  auth_session_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_session_roles (AuthSession* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  //AuthSession self = *p_self;

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

int auth_session_wp_get (AuthSession* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthSession self = *p_self;
  
  // local objects
  CapeUdc query_results = NULL;
  
  // do some security checks
  if (FALSE == qbus_message_role_has (qin, "admin"))
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_ROLE");
    goto exit_and_cleanup;
  }
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NO_CDATA");
    goto exit_and_cleanup;
  }

  self->wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
  if (0 == self->wpid)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_WPID");
    goto exit_and_cleanup;
  }

  self->gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
  if (0 == self->gpid)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_GPID");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "wpid"          , self->wpid);
    cape_udc_add_n      (params, "gpid"          , self->gpid);

    cape_udc_add_n      (values, "id"            , 0);
    cape_udc_add_d      (values, "lt"            , NULL);
    cape_udc_add_d      (values, "lu"            , NULL);
    cape_udc_add_n      (values, "vp"            , 0);
    cape_udc_add_n      (values, "type"          , 0);
    cape_udc_add_s_cp   (values, "remote"        , NULL);
    cape_udc_add_s_cp   (values, "ha_value"      , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "auth_sessions", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  cape_udc_replace_mv (&(qout->cdata), &query_results);
  
exit_and_cleanup:
  
  auth_session_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
