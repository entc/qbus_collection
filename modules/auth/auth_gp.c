#include "auth_gp.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

struct AuthGP_s
{
  AdblSession adbl_session;   // reference
  
  number_t wpid;
};

//-----------------------------------------------------------------------------

AuthGP auth_gp_new (AdblSession adbl_session)
{
  AuthGP self = CAPE_NEW (struct AuthGP_s);
  
  self->adbl_session = adbl_session;
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
      CapeUdc role_admin = cape_udc_get (roles, "wspc_admin");
      if (role_admin)
      {
        gpid = 0;
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
  
  cape_udc_replace_mv (&(qout->cdata), &query_results);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  
  auth_gp_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
