#include "auth_tokens.h"
#include "auth_rinfo.h"
#include "auth_perm.h"

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_mutex.h>
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

typedef struct
{
  CapeUdc extras;
  
  CapeUdc rinfo;
  
} AuthTokenItem;

//-----------------------------------------------------------------------------

static void __STDCALL auth_tokens__on_del (void* key, void* val)
{
  {
    CapeString h = key;
    cape_str_del (&h);
  }
  
  {
    AuthTokenItem* titem = val;
    
    cape_udc_del (&(titem->extras));
    cape_udc_del (&(titem->rinfo));
    
    CAPE_DEL (&titem, AuthTokenItem);
  }
}

//-----------------------------------------------------------------------------

struct AuthTokens_s
{
  CapeMap tokens;
  CapeMutex mutex;
  
  AdblSession adbl_session;  // reference
  AuthVault vault;           // reference
};

//-----------------------------------------------------------------------------

AuthTokens auth_tokens_new (AdblSession adbl_session, AuthVault vault)
{
  AuthTokens self = CAPE_NEW (struct AuthTokens_s);
  
  self->tokens = cape_map_new (NULL, auth_tokens__on_del, NULL);
  self->mutex = cape_mutex_new ();
  
  self->adbl_session = adbl_session;
  self->vault = vault;
  
  return self;
}

//-----------------------------------------------------------------------------

void auth_tokens_del (AuthTokens* p_self)
{
  AuthTokens self = *p_self;
  
  cape_map_del (&(self->tokens));
  cape_mutex_del (&(self->mutex));
  
  CAPE_DEL (p_self, struct AuthTokens_s);
}

//-----------------------------------------------------------------------------

int auth_tokens_add__database (AuthTokens self, number_t wpid, number_t gpid, const CapeString token, QBusM qin, CapeErr err)
{
  int res;
  
  number_t id;
  
  // start a new transaction
  AdblTrx trx = adbl_trx_new (self->adbl_session, err);
  
  if (trx == NULL)
  {
    return cape_err_code (err);
  }
  
  // prepare the insert query
  {
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // insert values
    cape_udc_add_n      (values, "id"           , ADBL_AUTO_SEQUENCE_ID);
    cape_udc_add_n      (values, "wpid"         , wpid);
    cape_udc_add_n      (values, "gpid"         , gpid);
    cape_udc_add_s_cp   (values, "token"        , token);
    
    cape_udc_add_n      (values, "ttype"        , 1);
    
    if (qin->cdata)
    {
      // check for content
      {
        CapeUdc h = cape_udc_ext (qin->cdata, "content");
        if (h)
        {
          cape_udc_add_name (values, &h, "content");
        }
      }
      // check for reference
      {
        CapeUdc h = cape_udc_ext (qin->cdata, "ref");
        if (h)
        {
          cape_udc_add_name (values, &h, "ref");
        }
      }
    }
    
    // debug
    {
      CapeString h = cape_json_to_s (values);
      
      cape_log_fmt (CAPE_LL_TRACE, "AUTH", "tokens add", "values: %s", h);
      
      cape_str_del (&h);
    }

    // execute query
    id = adbl_trx_insert (trx, "q5_tokens", &values, err);
  }
  
  if (id)
  {
    adbl_trx_commit (&trx, err);
    
    res = CAPE_ERR_NONE;
  }
  else
  {
    res = cape_err_code (err);
    
    adbl_trx_rollback (&trx, err);
  }

  return res;
}

//-----------------------------------------------------------------------------

int auth_tokens_add__memory (AuthTokens self, const CapeString uuid, QBusM qin, CapeErr err)
{
  AuthTokenItem* titem = CAPE_NEW (AuthTokenItem);
  
  titem->extras = qin->cdata;
  qin->cdata = NULL;
  
  // copy rinfo
  titem->rinfo = cape_udc_cp (qin->rinfo);
  
  cape_mutex_lock (self->mutex);
  
  cape_map_insert (self->tokens, cape_str_cp (uuid), titem);
  
  cape_mutex_unlock (self->mutex);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int auth_tokens_add (AuthTokens self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  number_t userid;
  number_t wpid;
  number_t gpid;
  
  const CapeString type = NULL;
  
  CapeString uuid = NULL;
  
  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
    goto exit_and_cleanup;
  }

  /*
  userid = cape_udc_get_n (qin->rinfo, "userid", 0);
  if (userid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'userid' is empty or missing");
    goto exit_and_cleanup;
  }
  */

  wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'wpid' is empty or missing");
    goto exit_and_cleanup;
  }
  
  gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "parameter 'gpid' is empty or missing");
    goto exit_and_cleanup;
  }

  // optional
  if (qin->cdata)
  {
    type = cape_udc_get_s (qin->cdata, "type", NULL);
  }
  
  uuid = cape_str_uuid ();

  if (cape_str_equal (type, "__L"))
  {
    res = auth_tokens_add__database (self, wpid, gpid, uuid, qin, err);
  }
  else
  {
    res = auth_tokens_add__memory (self, uuid, qin, err);
  }

  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // add output
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_s_mv (qout->cdata, "Token", &uuid);
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_str_del (&uuid);
  return res;
}

//-----------------------------------------------------------------------------

int auth_tokens_get (AuthTokens self, QBusM qin, QBusM qout, CapeErr err)
{
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int auth_tokens_rm (AuthTokens self, QBusM qin, QBusM qout, CapeErr err)
{
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int auth_tokens_fetch__database_q5 (AuthTokens self, const CapeString token, QBusM qin, CapeErr err)
{
  int res;
  CapeUdc results = NULL;
  
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // conditions
  cape_udc_add_s_cp   (params, "token"       , token);
  
  // return values
  cape_udc_add_n      (values, "wpid"        , 0);
  cape_udc_add_n      (values, "gpid"        , 0);

  // TODO: encrypt content
  cape_udc_add_node   (values, "content"     );
  cape_udc_add_n      (values, "ref"         , 0);
  
  // execute the query
  results = adbl_session_query (self->adbl_session, "q5_tokens", &params, &values, err);
  if (results == NULL)
  {
    return cape_err_code (err);
  }
  
  // debug
  {
    CapeString h = cape_json_to_s (results);
    
    cape_log_fmt (CAPE_LL_TRACE, "AUTH", "token fetch", "results: %s", h);
    
    cape_str_del (&h);
  }
  
  
  {
    number_t wpid;
    number_t gpid;

    CapeUdc row = cape_udc_get_first (results);
    if (row == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "token was not found");
      goto exit_and_cleanup;
    }
    
    wpid = cape_udc_get_n (row, "wpid", 0);
    if (wpid == 0)
    {
      res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "token's wpid is invalid");
      goto exit_and_cleanup;
    }

    gpid = cape_udc_get_n (row, "gpid", 0);
    if (gpid == 0)
    {
      res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "token's gpid is invalid");
      goto exit_and_cleanup;
    }
    
    {
      CapeUdc content = cape_udc_ext (row, "content");
      if (content)
      {
        // debug
        {
          CapeString h = cape_json_to_s (content);
          
          cape_log_fmt (CAPE_LL_TRACE, "AUTH", "token fetch", "content: %s", h);
          
          cape_str_del (&h);
        }
        
        cape_udc_replace_mv (&(qin->cdata), &content);
      }
    }
    
    {
      // not sure what happen with ref
      number_t ref = cape_udc_get_n (row, "ref", 0);
    }
    
    {
      // use the rinfo classes
      AuthRInfo rinfo = auth_rinfo_new (self->adbl_session, wpid, gpid);
      
      // fetch all rinfo from database
      res = auth_rinfo_get (&rinfo, qin, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  
exit_and_cleanup:
  
  cape_udc_del (&results);
  return res;
}

//-----------------------------------------------------------------------------

int auth_tokens_fetch__database_perm (AuthTokens self, const CapeString token, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  CapeUdc row;
  number_t wpid;
  const CapeString vsec;

  // local objects
  CapeUdc results = NULL;
  CapeString token_hash = NULL;

  res = auth_perm__helper__get (self->adbl_session, self->vault, qin, qout, err);
  if (res == CAPE_ERR_NONE)
  {
    goto exit_and_cleanup;
  }
  else
  {
    // try another method
    cape_err_clr (err);
    
    // try the old way using the q5_tokens table
    res = auth_tokens_fetch__database_q5 (self, token, qout, err);
  }

  /*
  token_hash = qcrypt__hash_sha256__hex_o (token, cape_str_size(token), err);
  if (token_hash == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // return values
    cape_udc_add_s_mv   (params, "token"       , &token_hash);
    
    cape_udc_add_n      (values, "wpid"        , 0);
    cape_udc_add_s_cp   (values, "rinfo"       , NULL);
    cape_udc_add_s_cp   (values, "content"     , NULL);
    
    // execute the query
    results = adbl_session_query (self->adbl_session, "q5_tokens", &params, &values, err);
    if (results == NULL)
    {
      // try the old way using the q5_tokens table
      res = auth_tokens_fetch__database_q5 (self, token, qin, err);
      goto exit_and_cleanup;
    }
  }

  row = cape_udc_get_first (results);
  if (row == NULL)
  {
    // try the old way using the q5_tokens table
    res = auth_tokens_fetch__database_q5 (self, token, qin, err);
    goto exit_and_cleanup;
  }

  wpid = cape_udc_get_n (row, "wpid", 0);
  if (wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "token's wpid is invalid");
    goto exit_and_cleanup;
  }

  // get the workspace encryption key
  vsec = auth_vault__vsec (self->vault, wpid);
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "vsec was not set for this workspace");
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

  // replace rinfo
  {
    CapeUdc h = cape_udc_ext (row, "rinfo");
    
    cape_udc_replace_mv (&(qin->rinfo), &h);
  }

  // replace cdata
  {
    CapeUdc h = cape_udc_ext (row, "cdata");
    
    cape_udc_replace_mv (&(qin->cdata), &h);
  }
   
  cape_log_fmt (CAPE_LL_TRACE, "AUTH", "tokens fetch", "found token in perm repo");

   */
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_str_del (&token_hash);
  cape_udc_del (&results);

  return res;
}

//-----------------------------------------------------------------------------

int auth_tokens_fetch (AuthTokens self, const CapeString token, QBusM qin, QBusM qout, CapeErr err)
{
  CapeMapNode n = NULL;
  
  cape_mutex_lock (self->mutex);

  // try to find the token in memory (fastest)
  n = cape_map_find (self->tokens, token);
  
  if (n)
  {
    // remove the token
    n = cape_map_extract (self->tokens, n);
  }
  
  cape_mutex_unlock (self->mutex);
  
  if (n)
  {
    AuthTokenItem* titem = cape_map_node_value (n);

    cape_udc_replace_mv (&(qout->cdata), &(titem->extras));
    cape_udc_replace_mv (&(qout->rinfo), &(titem->rinfo));

    // add the token to rinfo
    cape_udc_add_s_cp (qout->rinfo, "__T", token);
    
    cape_map_del_node (self->tokens, &n);
    
    return CAPE_ERR_NONE;
  }
  else
  {
    return auth_tokens_fetch__database_perm (self, token, qin, qout, err);
  }
}

//-----------------------------------------------------------------------------
