#include "auth_gp.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>

// qcrypt
#include <qcrypt.h>

//-----------------------------------------------------------------------------

struct AuthGP_s
{
  AdblSession adbl_session;   // reference
  AuthVault vault;            // reference

  number_t wpid_default;      // default workspace
  
  number_t wpid;
};

//-----------------------------------------------------------------------------

AuthGP auth_gp_new (AdblSession adbl_session, AuthVault vault, number_t wpid)
{
  AuthGP self = CAPE_NEW (struct AuthGP_s);
  
  self->adbl_session = adbl_session;
  self->vault = vault;
  
  self->wpid_default = wpid;  
  self->wpid = 0;
  
  return self;
}

//---------------------------------------------------------------------------

void auth_gp_del (AuthGP* p_self)
{
  //AuthGP self = *p_self;
  
  
  CAPE_DEL (p_self, struct AuthGP_s);
}

//-----------------------------------------------------------------------------

int auth_gp_get (AuthGP* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthGP self = *p_self;
  
  // local vars (initialization)
  CapeUdc query_results = NULL;
  number_t gpid = 0;

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
  
  gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing gpid");
    goto exit_and_cleanup;
  }

  // check role
  {
    CapeUdc roles = cape_udc_get (qin->rinfo, "roles");
    if (roles)
    {
      {
        CapeUdc role_admin = cape_udc_get (roles, "wspc_admin");
        if (role_admin)
        {
          gpid = 0;
        }
      }

      {
        CapeUdc role_list_read = cape_udc_get (roles, "auth_gp_ls_r");
        if (role_list_read)
        {
          gpid = 0;
        }
      }
    }
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "wpid"        , self->wpid);
    
    if (gpid)
    {
      cape_udc_add_n    (params, "gpid"        , gpid);
    }
    
    cape_udc_add_n      (values, "gpid"        , 0);
    cape_udc_add_n      (values, "userid"      , 0);
    cape_udc_add_s_cp   (values, "title"       , NULL);
    cape_udc_add_s_cp   (values, "firstname"   , NULL);
    cape_udc_add_s_cp   (values, "lastname"    , NULL);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "rbac_users_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  cape_udc_replace_mv (&(qout->cdata), &query_results);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  
  auth_gp_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_gp_account (AuthGP* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthGP self = *p_self;
  
  CapeUdc first_row;
  
  // local vars (initialization)
  CapeUdc query_results = NULL;
  number_t gpid = 0;

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
  
  gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing gpid");
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n      (params, "wpid"        , self->wpid);
    cape_udc_add_n      (params, "gpid"        , gpid);
    
    cape_udc_add_n      (values, "wpid"        , 0);
    cape_udc_add_n      (values, "gpid"        , 0);
    cape_udc_add_n      (values, "userid"      , 0);
    cape_udc_add_s_cp   (values, "title"       , NULL);
    cape_udc_add_s_cp   (values, "firstname"   , NULL);
    cape_udc_add_s_cp   (values, "lastname"    , NULL);
    cape_udc_add_s_cp   (values, "workspace"   , NULL);
    cape_udc_add_s_cp   (values, "secret"      , NULL);

    // execute the query
    query_results = adbl_session_query (self->adbl_session, "rbac_users_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  first_row = cape_udc_get_first (query_results);
  if (first_row == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't get account info");
    goto exit_and_cleanup;
  }
  
  // add also allknown roles
  {
    CapeUdc roles = cape_udc_get (qin->rinfo, "roles");
    if (roles)
    {
      CapeUdc h = cape_udc_cp (roles);
      cape_udc_add_name (first_row, &h, "roles");
    }
  }
  
  cape_udc_replace_mv (&(qout->cdata), &query_results);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  
  auth_gp_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_gp_set (AuthGP* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthGP self = *p_self;
  
  number_t gpid;
  const CapeString title;
  const CapeString firstname;
  const CapeString lastname;
  const CapeString vsec;

  // local objects
  AdblTrx trx = NULL;
  CapeString title_encrypted = NULL;
  CapeString firstname_encrypted = NULL;
  CapeString lastname_encrypted = NULL;

  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "missing rinfo");
    goto exit_and_cleanup;
  }
  
  self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  if (self->wpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "{gp set} missing wpid");
    goto exit_and_cleanup;
  }
  
  gpid = cape_udc_get_n (qin->rinfo, "gpid", 0);
  if (gpid == 0)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "{gp set} missing gpid");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "{gp set} cdata is missing");
    goto exit_and_cleanup;
  }

  // optional
  title = cape_udc_get_s (qin->cdata, "title", NULL);
  firstname = cape_udc_get_s (qin->cdata, "firstname", NULL);
  lastname = cape_udc_get_s (qin->cdata, "lastname", NULL);

  if ((title == NULL) && (firstname == NULL) && (lastname == NULL))
  {
    res = CAPE_ERR_NONE;  // nothing to do
    goto exit_and_cleanup;
  }
  
  vsec = auth_vault__vsec (self->vault, self->wpid);
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "{gp set} vault is missing");
    goto exit_and_cleanup;
  }
  
  if (title)
  {
    title_encrypted = qcrypt__encrypt (vsec, title, err);
    if (title_encrypted == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  if (firstname)
  {
    firstname_encrypted = qcrypt__encrypt (vsec, firstname, err);
    if (firstname_encrypted == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  if (lastname)
  {
    lastname_encrypted = qcrypt__encrypt (vsec, lastname, err);
    if (lastname_encrypted == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  // start the transaction
  trx = adbl_trx_new (self->adbl_session, err);
  if (trx == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // parameters
    cape_udc_add_n      (params, "id"           , gpid);
    cape_udc_add_n      (params, "wpid"         , self->wpid);

    if (title_encrypted)
    {
      cape_udc_add_s_mv (values, "title", &title_encrypted);
    }

    if (firstname_encrypted)
    {
      cape_udc_add_s_mv (values, "firstname", &firstname_encrypted);
    }

    if (lastname_encrypted)
    {
      cape_udc_add_s_mv (values, "lastname", &lastname_encrypted);
    }

    // execute query
    res = adbl_trx_update (trx, "glob_persons", &params, &values, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;
  adbl_trx_commit (&trx, err);

exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  
  cape_str_del (&title_encrypted);
  cape_str_del (&firstname_encrypted);
  cape_str_del (&lastname_encrypted);

  auth_gp_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_wp_get (AuthGP* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthGP self = *p_self;

  // local objects
  CapeUdc query_results = NULL;

  // check roles
  if (qbus_message_role_has (qin, "admin") == FALSE)
  {
    res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NOROLE");
    goto exit_and_cleanup;
  }
  
  // query
  {
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (values, "id"               , 0);
    cape_udc_add_s_cp   (values, "name"             , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "rbac_workspaces", NULL, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }
  
  cape_udc_replace_mv (&(qout->cdata), &query_results);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  
  auth_gp_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_wp__internal__fetch_workspace (AuthGP self, number_t wpid, CapeUdc* p_result, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc first_row = NULL;
  
  // query
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n      (params, "id"               , wpid);
    cape_udc_add_n      (values, "active"           , 0);
    cape_udc_add_s_cp   (values, "name"             , NULL);
    cape_udc_add_s_cp   (values, "domain"           , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "rbac_workspaces", &params, &values, err);
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

  cape_udc_replace_mv (p_result, &first_row);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&first_row);
  cape_udc_del (&query_results);

  return res;
}

//-----------------------------------------------------------------------------

int auth_wp_info_get (AuthGP* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthGP self = *p_self;
  
  // check roles
  if (qbus_message_role_has (qin, "admin"))
  {
    if (qin->cdata == NULL)
    {
      res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NO_CDATA");
      goto exit_and_cleanup;
    }

    self->wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
    if (self->wpid == 0)
    {
      res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_WPID");
      goto exit_and_cleanup;
    }
  }
  else if (qin->rinfo)
  {
    self->wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
    if (self->wpid == 0)
    {
      res = cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_WPID");
      goto exit_and_cleanup;
    }
  }
  else
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_AUTH");
    goto exit_and_cleanup;
  }
  
  res = auth_wp__internal__fetch_workspace (self, self->wpid, &(qout->cdata), err);

exit_and_cleanup:
    
  auth_gp_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_wp_default_get (AuthGP* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthGP self = *p_self;
  
  if (self->wpid_default)
  {
    cape_log_fmt (CAPE_LL_DEBUG, "AUTH", "default get", "deliver default workspace info for wpid = %lu", self->wpid_default);
    
    res = auth_wp__internal__fetch_workspace (self, self->wpid_default, &(qout->cdata), err);
  }
  else
  {
    cape_log_msg (CAPE_LL_WARN, "AUTH", "default get", "no default workspace was set");

    res = CAPE_ERR_NONE;
  }
      
  auth_gp_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_wp_add (AdblTrx trx, const CapeString vsec, number_t wpid, CapeUdc gpdata, number_t* p_gpid, CapeErr err)
{
  int res;
  
  // prepare the insert query
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // insert values
  cape_udc_add_n      (values, "id"           , ADBL_AUTO_SEQUENCE_ID);
  cape_udc_add_n      (values, "wpid"         , wpid);
  
  {
    const CapeString h1 = cape_udc_get_s (gpdata, "title", NULL);
    if (h1)
    {
      CapeString h2 = qcrypt__encrypt (vsec, h1, err);
      if (h2)
      {
        cape_udc_add_s_mv (values, "title", &h2);
      }
      else
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }
  }
  
  {
    const CapeString h1 = cape_udc_get_s (gpdata, "firstname", NULL);
    if (h1)
    {
      CapeString h2 = qcrypt__encrypt (vsec, h1, err);
      if (h2)
      {
        cape_udc_add_s_mv (values, "firstname", &h2);
      }
      else
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }
  }
  
  {
    const CapeString h1 = cape_udc_get_s (gpdata, "lastname", NULL);
    if (h1)
    {
      CapeString h2 = qcrypt__encrypt (vsec, h1, err);
      if (h2)
      {
        cape_udc_add_s_mv (values, "lastname", &h2);
      }
      else
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }
  }
  {
    number_t id;
    
    // execute query
    id = adbl_trx_insert (trx, "glob_persons", &values, err);
    if (id == 0)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }

    *p_gpid = id;
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&values);
  
  return res;
}

//-----------------------------------------------------------------------------
