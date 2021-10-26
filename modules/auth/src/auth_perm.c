#include "auth_perm.h"
#include "auth_rinfo.h"

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_mutex.h>
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

struct AuthPerm_s
{
  QBus qbus;                 // reference
  AdblSession adbl_session;  // reference
  AuthVault vault;           // reference
  QJobs jobs;                // reference

  number_t wpid;             // rinfo: mandatory
  number_t gpid;             // rinfo: mandatory
  number_t usid;             // rinfo: mandatory
  number_t rfid;             // input: mandatory

  CapeUdc roles;             // input: mandatory
  
  CapeString code;           // input: optional
  CapeString token;          // sha256 hash of the token

  number_t apid;             // item id
};

//-----------------------------------------------------------------------------

AuthPerm auth_perm_new (QBus qbus, AdblSession adbl_session, AuthVault vault, QJobs jobs)
{
  AuthPerm self = CAPE_NEW (struct AuthPerm_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;
  self->vault = vault;
  self->jobs = jobs;
  
  self->wpid = 0;
  self->gpid = 0;
  self->usid = 0;
  self->rfid = 0;
  self->roles = NULL;
  
  self->code = NULL;
  self->token = NULL;
  
  self->apid = 0;
  
  return self;
}

//-----------------------------------------------------------------------------

void auth_perm_del (AuthPerm* p_self)
{
  if (*p_self)
  {
    AuthPerm self = *p_self;
    
    cape_udc_del (&(self->roles));
    
    cape_str_del (&(self->code));
    cape_str_del (&(self->token));

    CAPE_DEL (p_self, struct AuthPerm_s);
  }
}

//-----------------------------------------------------------------------------

int auth_perm_remove (AuthPerm self, const CapeString token, CapeErr err)
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
    
    cape_udc_add_s_cp (params, "token", token);
    
    res = adbl_trx_delete (trx, "auth_perm", &params, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  adbl_trx_commit (&trx, err);
  
  cape_log_fmt (CAPE_LL_TRACE, "AUTH", "perm rm", "token was removed");

exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  return res;
}

//-----------------------------------------------------------------------------

int auth_perm__intern__check_input (AuthPerm self, QBusM qin, CapeErr err)
{
  int res;
  
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
  if (qin->pdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing pdata");
    goto exit_and_cleanup;
  }

  self->roles = cape_udc_ext (qin->pdata, "roles");
  if (self->roles == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing roles");
    goto exit_and_cleanup;
  }

  self->code = cape_udc_ext_s (qin->pdata, "code");
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_perm__intern__seek_item (AuthPerm self, CapeErr err)
{
  int res;
  CapeUdc first_row;
  
  // local objects
  CapeUdc query_results = NULL;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "rfid"          , self->rfid);
    cape_udc_add_n      (params, "wpid"          , self->wpid);
    cape_udc_add_n      (values, "id"            , 0);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "auth_perm", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_get_first (query_results);
  if (first_row == NULL)
  {
    self->apid = 0;
  }
  else
  {
    self->apid = cape_udc_get_n (first_row, "id", 0);
    if (self->apid == 0)
    {
      res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't find account");
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_perm__intern__encrypt_cdata (AuthPerm self, QBusM qin, const CapeString vsec, CapeString* p_content, CapeErr err)
{
  int res;
  
  // local objects
  CapeString h1 = NULL;
  CapeString h2 = NULL;

  if (qin->cdata == NULL)
  {
    res = CAPE_ERR_NONE;
    goto exit_and_cleanup;
  }
  
  h1 = cape_json_to_s (qin->cdata);
  if (h1 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't serialize cdata");
    goto exit_and_cleanup;
  }

  h2 = qcrypt__encrypt (vsec, h1, err);
  if (h1 == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  cape_str_replace_mv (p_content, &h2);

exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_perm__intern__encrypt_rinfo (AuthPerm self, QBusM qin, const CapeString vsec, CapeString* p_rinfo, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc rinfo = NULL;
  CapeString h1 = NULL;
  CapeString h2 = NULL;
  
  // construct rinfo
  rinfo = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  cape_udc_add_name (rinfo, &(self->roles), "roles");
  cape_udc_add_n (rinfo, "wpid", self->wpid);
  cape_udc_add_n (rinfo, "gpid", self->gpid);
  
  // TODO: see depricated but kept for backward compatiblity
  if (self->usid)
  {
    cape_udc_add_n (rinfo, "userid", self->usid);
  }
  
  h1 = cape_json_to_s (rinfo);
  if (h1 == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't serialize cdata");
    goto exit_and_cleanup;
  }
  
  h2 = qcrypt__encrypt (vsec, h1, err);
  if (h1 == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  cape_str_replace_mv (p_rinfo, &h2);
  
exit_and_cleanup:
  
  cape_udc_del (&rinfo);
  return res;
}

//-----------------------------------------------------------------------------

int auth_perm__intern__encrypt (AuthPerm self, QBusM qin, CapeString* p_rinfo, CapeString* p_content, CapeErr err)
{
  int res;
  const CapeString vsec;
  
  vsec = auth_vault__vsec (self->vault, self->wpid);
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing vault");
    goto exit_and_cleanup;
  }

  res = auth_perm__intern__encrypt_cdata (self, qin, vsec, p_content, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = auth_perm__intern__encrypt_rinfo (self, qin, vsec, p_rinfo, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_perm__intern__decrypt (AuthVault vault, CapeUdc row, number_t wpid, CapeErr err)
{
  int res;
  const CapeString vsec;
  
  vsec = auth_vault__vsec (vault, wpid);
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing vault");
    goto exit_and_cleanup;
  }

  res = qcrypt__decrypt_row_node (vsec, row, "rinfo", err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = qcrypt__decrypt_row_node (vsec, row, "cdata", err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

void auth_perm__intern__values (AuthPerm self, CapeUdc values, const CapeString token, CapeString* p_rinfo, CapeString* p_content)
{
  // unique key
  cape_udc_add_s_cp   (values, "token"        , self->token);
  
  // the code is optional
  if (self->code)
  {
    cape_udc_add_s_mv   (values, "code"        , &(self->code));
  }
  
  // created datetime
  {
    CapeDatetime dt; cape_datetime_utc (&dt);
    cape_udc_add_d (values, "toc", &dt);
  }

  // add rinfo
  if (*p_rinfo)
  {
    cape_udc_add_s_mv   (values, "rinfo"       , p_rinfo);
  }

  // cdata content is optional
  if (*p_content)
  {
    cape_udc_add_s_mv   (values, "cdata"       , p_content);
  }
}

//-----------------------------------------------------------------------------

int auth_perm_add (AuthPerm* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthPerm self = *p_self;

  // local objects
  number_t ttl = 0;
  CapeString content = NULL;
  CapeString rinfo = NULL;
  CapeString token = NULL;
  AdblTrx trx = NULL;

  // do some security checks
  res = auth_perm__intern__check_input (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // optional
  {
    // remove this node, so it is not used for cdata input / content
    CapeUdc ttl_node = cape_udc_ext (qin->cdata, "ttl");
    if (ttl_node)
    {
      ttl = cape_udc_n (ttl_node, 0);
      cape_udc_del (&ttl_node);
    }
  }

  /*
  // TODO: depricated -> remove userid
  // for backward compatibility the userid is still
  // added to the rinfo for the token object
  self->usid = cape_udc_get_n (qin->rinfo, "userid", 0);
  if (self->usid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing userid");
    goto exit_and_cleanup;
  }
   */
  
  // encrypt cdata and rinfo into rinfo and content
  res = auth_perm__intern__encrypt (self, qin, &rinfo, &content, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // generate the token
  token = cape_str_uuid ();
  
  self->token = qcrypt__hash_sha256__hex_o (token, cape_str_size(token), err);
  if (self->token == NULL)
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
    
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // insert values
    cape_udc_add_n      (values, "id"           , ADBL_AUTO_SEQUENCE_ID);
    
    // workspace ID
    cape_udc_add_n      (values, "wpid"         , self->wpid);
    
    // add values
    auth_perm__intern__values (self, values, token, &rinfo, &content);
    
    // execute query
    id = adbl_trx_insert (trx, "auth_perm", &values, err);
    if (id == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }

    if (ttl > 0)
    {
      CapeDatetime dt_current;
      CapeDatetime dt_event;
      
      CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      cape_udc_add_s_cp (params, "token", self->token);
      
      cape_datetime_utc (&dt_current);
      cape_datetime__add_n (&dt_current, ttl, &dt_event);

      // add a job to remove this token
      // don't use rinfo and vsec
      res = qjobs_add (self->jobs, &dt_event, ttl, &params, NULL, "AUTH", "PERM", id, 0, NULL, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  
  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;
  
  // add output
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_s_mv (qout->cdata, "token", &token);
  
exit_and_cleanup:
  
  cape_str_del (&content);
  cape_str_del (&rinfo);
  cape_str_del (&token);
  
  adbl_trx_rollback (&trx, err);
  
  auth_perm_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_perm_set (AuthPerm* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthPerm self = *p_self;
  
  // local objects
  CapeString content = NULL;
  CapeString rinfo = NULL;
  CapeString token = NULL;
  AdblTrx trx = NULL;

  // do some security checks
  res = auth_perm__intern__check_input (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  /*
  // TODO: depricated -> remove userid
  // for backward compatibility the userid is still
  // added to the rinfo for the token object
  self->usid = cape_udc_get_n (qin->rinfo, "userid", 0);
  if (self->usid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing userid");
    goto exit_and_cleanup;
  }
   */

  // check extra rfid
  self->rfid = cape_udc_get_n (qin->pdata, "rfid", 0);
  if (self->rfid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing rfid");
    goto exit_and_cleanup;
  }
  
  // encrypt cdata and rinfo
  res = auth_perm__intern__encrypt (self, qin, &rinfo, &content, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // find the entry in the database using rfid and wpid
  res = auth_perm__intern__seek_item (self, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // generate the token
  token = cape_str_uuid ();

  self->token = qcrypt__hash_sha256__hex_o (token, cape_str_size(token), err);
  if (self->token == NULL)
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

  if (self->apid)  // update
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_n      (params, "id"           , self->apid);
    
    // add values
    auth_perm__intern__values (self, values, token, &rinfo, &content);
    
    // execute query
    res = adbl_trx_update (trx, "auth_perm", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  else   // insert
  {
    number_t id;
    
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // insert values
    cape_udc_add_n      (values, "id"           , ADBL_AUTO_SEQUENCE_ID);
    
    // unique key
    cape_udc_add_n      (values, "rfid"         , self->rfid);
    
    // workspace ID
    cape_udc_add_n      (values, "wpid"         , self->wpid);
    
    // add values
    auth_perm__intern__values (self, values, token, &rinfo, &content);
    
    // execute query
    id = adbl_trx_insert (trx, "auth_perm", &values, err);
    if (id == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;

  // add output
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_s_mv (qout->cdata, "token", &token);
  
exit_and_cleanup:
  
  cape_str_del (&content);
  cape_str_del (&rinfo);
  cape_str_del (&token);

  adbl_trx_rollback (&trx, err);

  auth_perm_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_perm__helper__get (AdblSession adbl_session, AuthVault vault, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  CapeUdc first_row;
  const CapeString token;
  const CapeString code;
  number_t code_active;
  number_t wpid;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc rinfo = NULL;
  CapeUdc cdata = NULL;
  int remove_roles = FALSE;
  
  CapeString token_hash = NULL;
  CapeString code_hash = NULL;
  
  if (qin->pdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "missing pdata");
    goto exit_and_cleanup;
  }
  
  token = cape_udc_get_s (qin->pdata, "token", NULL);
  if (token == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing token");
    goto exit_and_cleanup;
  }
  
  token_hash = qcrypt__hash_sha256__hex_o (token, cape_str_size(token), err);
  if (token_hash == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  //cape_log_fmt (CAPE_LL_TRACE, "AUTH", "perm get", "seek token = %s", self->token);
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_s_cp   (params, "token"         , token_hash);
    cape_udc_add_n      (values, "wpid"          , 0);
    cape_udc_add_s_cp   (values, "code"          , NULL);
    cape_udc_add_n      (values, "code_active"   , 0);
    cape_udc_add_s_cp   (values, "rinfo"         , NULL);
    cape_udc_add_s_cp   (values, "cdata"         , NULL);
    
    // execute the query
    query_results = adbl_session_query (adbl_session, "auth_perm", &params, &values, err);
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
  
  // get the workspace ID
  wpid = cape_udc_get_n (first_row, "wpid", 0);
  if (wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "wpid not found");
    goto exit_and_cleanup;
  }
  
  // get active code status
  code_active = cape_udc_get_n (first_row, "code_active", 0);
  
  if (code_active)
  {
    if (qin->cdata == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing content");
      goto exit_and_cleanup;
    }
    
    code = cape_udc_get_s (qin->cdata, "acc_code", NULL);
    if (code == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing code");
      goto exit_and_cleanup;
    }
    
    if (cape_str_equal (code, "__NOCODE__"))
    {
      remove_roles = TRUE;
    }
    else
    {
      code_hash = qcrypt__hash_sha256__hex_o (code, cape_str_size(code), err);
      if (code_hash == NULL)
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
      
      if (!cape_str_equal (code_hash, cape_udc_get_s (first_row, "code", NULL)))
      {
        res = cape_err_set (err, CAPE_ERR_NO_AUTH, "code mismatch");
        goto exit_and_cleanup;
      }
    }
  }
  
  res = auth_perm__intern__decrypt (vault, first_row, wpid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // rinfo is mandatory
  rinfo = cape_udc_ext (first_row, "rinfo");
  if (rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "rinfo not found");
    goto exit_and_cleanup;
  }
  
  if (remove_roles)
  {
    CapeUdc roles = cape_udc_ext (rinfo, "roles");
    cape_udc_del (&roles);
  }
  
  cdata = cape_udc_ext (first_row, "cdata");
  
  res = CAPE_ERR_NONE;
  
  cape_udc_replace_mv (&(qout->rinfo), &rinfo);
  cape_udc_replace_mv (&(qout->cdata), &cdata);
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  cape_udc_del (&rinfo);
  cape_udc_del (&cdata);
  cape_str_del (&token_hash);
  cape_str_del (&code_hash);
  
  return res;
}

//-----------------------------------------------------------------------------

int auth_perm_get (AuthPerm* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthPerm self = *p_self;

  res = auth_perm__helper__get (self->adbl_session, self->vault, qin, qout, err);

  if (res == CAPE_ERR_NO_AUTH)
  {
    const CapeString remote = cape_udc_get_s (qin->cdata, "remote", NULL);
    
    if (cape_str_equal (remote, "0.0.0.0") || cape_str_equal (remote, "127.0.0.1"))
    {
      cape_log_msg (CAPE_LL_WARN, "AUTH", "perm get", "no valid remote for logging");
    }
    else
    {
      // using the new qbus interface
      qbus_log_msg (self->qbus, remote, "access denied");
    }
  }
  
  auth_perm_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_perm_code_set (AuthPerm* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthPerm self = *p_self;
  
  const CapeString token;
  CapeUdc active_node;
  const CapeString code;

  // local objects
  AdblTrx trx = NULL;
  CapeString hash = NULL;

  if (qin->pdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "missing pdata");
    goto exit_and_cleanup;
  }
  
  token = cape_udc_get_s (qin->pdata, "token", NULL);
  if (token == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing token");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "missing cdata");
    goto exit_and_cleanup;
  }

  // optional (at least one)
  active_node = cape_udc_get (qin->cdata, "active");
  
  // optional (at least one)
  code = cape_udc_get_s (qin->cdata, "code", NULL);

  if ((active_node == NULL) && (code == NULL))
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing content");
    goto exit_and_cleanup;
  }

  // create the hash of the token
  self->token = qcrypt__hash_sha256__hex_o (token, cape_str_size(token), err);
  if (self->token == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  if (code)
  {
    hash = qcrypt__hash_sha256__hex_o (code, cape_str_size(code), err);
    if (hash == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
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
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // unique key
    cape_udc_add_s_cp   (params, "token"           , self->token);
    
    if (active_node)
    {
      cape_udc_add_n      (values, "code_active"     , cape_udc_n (active_node, 0));
    }
    
    if (hash)
    {
      cape_udc_add_s_mv   (values, "code"            , &hash);
    }
    
    // execute query
    res = adbl_trx_update (trx, "auth_perm", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  adbl_trx_commit (&trx, err);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  
  cape_str_del (&hash);

  auth_perm_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_perm_rm (AuthPerm* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthPerm self = *p_self;

  const CapeString token;

  if (qin->pdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "missing pdata");
    goto exit_and_cleanup;
  }
  
  token = cape_udc_get_s (qin->pdata, "token", NULL);
  if (token == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing token");
    goto exit_and_cleanup;
  }

  // create the hash of the token
  self->token = qcrypt__hash_sha256__hex_o (token, cape_str_size(token), err);
  if (self->token == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  res = auth_perm_remove (self, self->token, err);
  
exit_and_cleanup:
  
  auth_perm_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
