#include "auth_msgs.h"

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_mutex.h>
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

struct AuthMsgs_s
{
  AdblSession adbl_session;  // reference
  AuthVault vault;           // reference

  number_t wpid;             // rinfo: mandatory
  number_t gpid;             // rinfo: mandatory
  
  CapeString vsec;
};

//-----------------------------------------------------------------------------

AuthMsgs auth_msgs_new (AdblSession adbl_session, AuthVault vault)
{
  AuthMsgs self = CAPE_NEW (struct AuthMsgs_s);
  
  self->adbl_session = adbl_session;
  self->vault = vault;
  
  self->wpid = 0;
  self->gpid = 0;
  
  self->vsec = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void auth_msgs_del (AuthMsgs* p_self)
{
  if (*p_self)
  {
    AuthMsgs self = *p_self;
    
    cape_str_del (&(self->vsec));

    CAPE_DEL (p_self, struct AuthMsgs_s);
  }
}

//-----------------------------------------------------------------------------

int auth_msgs__intern__check_input (AuthMsgs self, QBusM qin, CapeErr err)
{
  if (qin->rinfo == NULL)
  {
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_AUTH");
  }
  
  if (qin->cdata == NULL)
  {
    return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_CDATA");
  }
  
  switch (cape_udc_get_n (qin->cdata, "scope", 0))
  {
    case 0:
    {
      self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
      self->gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
      
      break;
    }
    case 1:
    {
      if (qbus_message_role_has (qin, "admin"))
      {
        self->wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
        self->gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
      }
      else if (qbus_message_role_has (qin, "auth_wacc"))
      {
        self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
        self->gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
      }
      
      break;
    }
  }
  
  if (0 == self->wpid)
  {
    return cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_WPID");
  }
  
  if (0 == self->gpid)
  {
    return cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_GPID");
  }

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int auth_msgs_get (AuthMsgs* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthMsgs self = *p_self;

  const CapeString vsec;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdcCursor* cursor = NULL;

  res = auth_msgs__intern__check_input (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  vsec = auth_vault__vsec (self->vault, self->wpid);
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_VAULT");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "gpid"        , self->gpid);
    
    cape_udc_add_n      (values, "id"          , 0);
    cape_udc_add_s_cp   (values, "email"       , NULL);
    cape_udc_add_n      (values, "type"        , 0);

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
      cape_err_clr (err);
    }
  }
  
  res = CAPE_ERR_NONE;
  cape_udc_replace_mv (&(qout->cdata), &query_results);
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  cape_udc_del (&query_results);
  
  auth_msgs_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_msgs_add (AuthMsgs* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthMsgs self = *p_self;
  
  const CapeString vsec;
  number_t type;
  const CapeString email;
  
  // local objects
  AdblTrx trx = NULL;
  CapeString h1 = NULL;

  res = auth_msgs__intern__check_input (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  vsec = auth_vault__vsec (self->vault, self->wpid);
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing vault");
    goto exit_and_cleanup;
  }
    
  type = cape_udc_get_n (qin->cdata, "type", 0);
  if (type == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing type");
    goto exit_and_cleanup;
  }
  
  email = cape_udc_get_s (qin->cdata, "email", NULL);
  if (email == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing email");
    goto exit_and_cleanup;
  }
  
  h1 = qcrypt__encrypt (vsec, email, err);
  if (h1 == NULL)
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
    
    cape_udc_add_n      (values, "id"          , ADBL_AUTO_SEQUENCE_ID);
    cape_udc_add_n      (values, "gpid"        , self->gpid);
    cape_udc_add_s_mv   (values, "email"       , &h1);
    cape_udc_add_n      (values, "type"        , type);
    
    // execute query
    id = adbl_trx_insert (trx, "glob_emails", &values, err);
    if (id == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  adbl_trx_commit (&trx, err);
  
exit_and_cleanup:
  
  cape_str_del (&h1);
  adbl_trx_rollback (&trx, err);

  auth_msgs_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_msgs_set (AuthMsgs* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthMsgs self = *p_self;
  
  const CapeString vsec;
  number_t msid, type;
  const CapeString email;

  // local objects
  AdblTrx trx = NULL;
  CapeString h1 = NULL;

  res = auth_msgs__intern__check_input (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  vsec = auth_vault__vsec (self->vault, self->wpid);
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "missing vault");
    goto exit_and_cleanup;
  }

  msid = cape_udc_get_n (qin->cdata, "id", 0);
  if (msid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing id");
    goto exit_and_cleanup;
  }
  
  type = cape_udc_get_n (qin->cdata, "type", 0);
  if (type == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing type");
    goto exit_and_cleanup;
  }

  email = cape_udc_get_s (qin->cdata, "email", NULL);
  if (email == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing email");
    goto exit_and_cleanup;
  }

  h1 = qcrypt__encrypt (vsec, email, err);
  if (h1 == NULL)
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
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"          , msid);
    cape_udc_add_n      (params, "gpid"        , self->gpid);

    cape_udc_add_s_mv   (values, "email"       , &h1);
    cape_udc_add_n      (values, "type"        , type);

    // execute query
    res = adbl_trx_update (trx, "glob_emails", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  adbl_trx_commit (&trx, err);

exit_and_cleanup:
  
  cape_str_del (&h1);
  adbl_trx_rollback (&trx, err);

  auth_msgs_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_msgs_rm (AuthMsgs* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthMsgs self = *p_self;
  
  number_t msid;
  
  // local objects
  AdblTrx trx = NULL;
  
  res = auth_msgs__intern__check_input (self, qin, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  msid = cape_udc_get_n (qin->cdata, "id", 0);
  if (msid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing id");
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
    
    cape_udc_add_n      (params, "id"          , msid);
    cape_udc_add_n      (params, "gpid"        , self->gpid);
    
    // execute query
    res = adbl_trx_delete (trx, "glob_emails", &params, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;
  adbl_trx_commit (&trx, err);
  
exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);

  auth_msgs_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
