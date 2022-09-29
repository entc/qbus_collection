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
  
  if (qbus_message_role_or2 (qin, "admin", "auth_wacc"))
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

int auth_roles__internal__input (QBusM qin, number_t* p_wpid, number_t* p_gpid, number_t* p_userid, CapeErr err)
{
  number_t wpid;
  number_t gpid;
  number_t userid;

  if (qbus_message_role_has (qin, "admin"))
  {
    if (NULL == qin->cdata)
    {
      return cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NO_CDATA");
    }
    
    wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
    if (0 == wpid)
    {
      return cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_WPID");
    }
    
    gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
    if (0 == gpid)
    {
      return cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_GPID");
    }
    
    userid = cape_udc_get_n (qin->cdata, "userid", 0);
    if (0 == userid)
    {
      return cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_USERID");
    }
  }
  else if (qbus_message_role_has (qin, "auth_wacc"))
  {
    if (NULL == qin->cdata)
    {
      return cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.NO_CDATA");
    }
    
    wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
    if (0 == wpid)
    {
      return cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_WPID");
    }
    
    gpid = cape_udc_get_n (qin->cdata, "gpid", 0);
    if (0 == gpid)
    {
      return cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_GPID");
    }
    
    userid = cape_udc_get_n (qin->cdata, "userid", 0);
    if (0 == userid)
    {
      return cape_err_set (err, CAPE_ERR_NO_ROLE, "ERR.NO_USERID");
    }
  }
  else
  {
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_AUTH");
  }
  
  *p_wpid = wpid;
  *p_gpid = gpid;
  *p_userid = userid;
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int auth_roles_ui_get (AuthRoles* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthRoles self = *p_self;

  number_t gpid;
  number_t userid;

  // local objects
  CapeUdc query_results = NULL;
  number_t wpid = 0;

  res = auth_roles__internal__input (qin, &wpid, &gpid, &userid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

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

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&query_results);
  
  auth_roles_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_roles_ui_add (AuthRoles* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthRoles self = *p_self;
  
  // local objects
  AdblTrx trx = NULL;
  
  if (qbus_message_role_has (qin, "admin"))
  {
    number_t userid;
    number_t roleid;

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

    roleid = cape_udc_get_n (qin->cdata, "roleid", 0);
    if (0 == roleid)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_ROLEID");
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
      cape_udc_add_n      (values, "userid"       , userid);
      cape_udc_add_n      (values, "roleid"       , roleid);
      
      // execute query
      id = adbl_trx_insert (trx, "rbac_users_roles", &values, err);
      if (0 == id)
      {
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }

    adbl_trx_commit (&trx, err);
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);
  
  auth_roles_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_roles_ui_rm (AuthRoles* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthRoles self = *p_self;
  
  // local objects
  AdblTrx trx = NULL;

  if (qbus_message_role_has (qin, "admin"))
  {
    number_t userid;
    number_t urid;
    number_t roleid;
    
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

    urid = cape_udc_get_n (qin->cdata, "urid", 0);
    if (0 == urid)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_URID");
      goto exit_and_cleanup;
    }

    roleid = cape_udc_get_n (qin->cdata, "roleid", 0);
    if (0 == roleid)
    {
      res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "ERR.NO_ROLEID");
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
      cape_udc_add_n   (params, "id"           , urid);
      cape_udc_add_n   (params, "userid"       , userid);
      cape_udc_add_n   (params, "roleid"       , roleid);

      // execute query
      res = adbl_trx_delete (trx, "rbac_users_roles", &params, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
    
    adbl_trx_commit (&trx, err);
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  adbl_trx_rollback (&trx, err);

  auth_roles_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_roles_wp_get (AuthRoles* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthRoles self = *p_self;

  number_t gpid;
  number_t userid;
  
  // local objects
  CapeUdc query_results = NULL;
  number_t wpid = 0;

  res = auth_roles__internal__input (qin, &wpid, &gpid, &userid, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

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

  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  auth_roles_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
