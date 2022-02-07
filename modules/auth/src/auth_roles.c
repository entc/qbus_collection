#include "auth_roles.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

struct AuthRoles_s
{
  QBus qbus;                       // reference
  AdblSession adbl_session;        // reference

};

//-----------------------------------------------------------------------------

AuthRoles auth_roles_new (QBus qbus, AdblSession adbl_session)
{
  AuthRoles self = CAPE_NEW (struct AuthRoles_s);
  
  self->qbus = qbus;
  self->adbl_session = adbl_session;

  return self;
}

//---------------------------------------------------------------------------

void auth_roles_del (AuthRoles* p_self)
{
  if (*p_self)
  {
    AuthRoles self = *p_self;
    
    
    CAPE_DEL (p_self, struct AuthRoles_s);
  }
}

//-----------------------------------------------------------------------------

int auth_roles_get (AuthRoles* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthRoles self = *p_self;
  
  // local objects
  CapeUdc query_results = NULL;
  
  if (qbus_message_role_has (qin, "admin"))
  {
    // query
    {
      CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      cape_udc_add_n      (values, "id"               , 0);
      cape_udc_add_s_cp   (values, "name"             , NULL);
      cape_udc_add_s_cp   (values, "description"      , NULL);
      
      // execute the query
      query_results = adbl_session_query (self->adbl_session, "rbac_roles", NULL, &values, err);
      if (query_results == NULL)
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }

    // set output
    cape_udc_replace_mv (&(qout->cdata), &query_results);
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  
  auth_roles_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_roles_ui_get (AuthRoles* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthRoles self = *p_self;

  // local objects
  CapeUdc query_results = NULL;
  
  if (qbus_message_role_has (qin, "admin"))
  {
    number_t wpid;
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
    
    // query
    {
      CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
      CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      cape_udc_add_n      (params, "userid"           , userid);
      cape_udc_add_n      (params, "wpid"             , wpid);
      cape_udc_add_n      (params, "gpid"             , gpid);
      
      cape_udc_add_n      (values, "id"               , 0);
      cape_udc_add_n      (values, "roleid"           , 0);
      cape_udc_add_s_cp   (values, "name"             , NULL);
      cape_udc_add_s_cp   (values, "description"      , NULL);

      /*
       auth_roles_ui_view
       
       select ur.id, ur.userid, ru.wpid, ru.gpid, ur.roleid, rr.name, rr.description from rbac_users_roles ur join rbac_users ru on ru.userid = ur.userid join rbac_roles rr on rr.id = ur.roleid;

       */
      
      // execute the query
      query_results = adbl_session_query (self->adbl_session, "auth_roles_ui_view", &params, &values, err);
      if (query_results == NULL)
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }
    
    // set output
    cape_udc_replace_mv (&(qout->cdata), &query_results);
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  
  auth_roles_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_roles_wp_get (AuthRoles* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthRoles self = *p_self;

  // local objects
  CapeUdc query_results = NULL;
  
  if (qbus_message_role_has (qin, "admin"))
  {
    number_t wpid;
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
    
    // query
    {
      CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
      CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      cape_udc_add_n      (params, "wpid"             , wpid);
      
      cape_udc_add_n      (values, "id"               , 0);
      cape_udc_add_n      (values, "roleid"           , 0);
      cape_udc_add_s_cp   (values, "name"             , NULL);
      cape_udc_add_s_cp   (values, "description"      , NULL);
      
      /*
       auth_roles_wp_view
       
       select wr.id, wr.wpid, wr.roleid, rr.name, rr.description from rbac_workspaces_roles wr join rbac_roles rr on rr.id = wr.roleid;
       
       */
      
      // execute the query
      query_results = adbl_session_query (self->adbl_session, "auth_roles_wp_view", &params, &values, err);
      if (query_results == NULL)
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }
    
    // set output
    cape_udc_replace_mv (&(qout->cdata), &query_results);
  }
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  auth_roles_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
