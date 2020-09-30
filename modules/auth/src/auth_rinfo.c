#include "auth_rinfo.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>

//---------------------------------------------------------------------------

struct AuthRInfo_s
{
  AdblSession adbl_session;
  
  number_t userid;
  number_t wpid;
};

//---------------------------------------------------------------------------

AuthRInfo auth_rinfo_new (AdblSession adbl_session, number_t userid, number_t wpid)
{
  AuthRInfo self = CAPE_NEW (struct AuthRInfo_s);
  
  self->adbl_session = adbl_session;
  
  self->userid = userid;
  self->wpid = wpid;
  
  return self;
}

//---------------------------------------------------------------------------

void auth_rinfo_del (AuthRInfo* p_self)
{
  
  CAPE_DEL (p_self, struct AuthRInfo_s);
}

//---------------------------------------------------------------------------

void auth_rinfo__workspaces (AuthRInfo self, CapeUdc results, CapeUdc cdata)
{
  CapeUdcCursor* cursor = cape_udc_cursor_new (results, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    CapeUdc workspace_node = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // wpid
    {
      CapeUdc h = cape_udc_ext (cursor->item, "wpid");
      if (h)
      {
        cape_udc_add (workspace_node, &h);
      }
      else
      {
        cape_log_msg (CAPE_LL_WARN, "AUTH", "get rinfo", "result row has no wpid");
      }
    }

    // workspace (optional)
    {
      CapeUdc h = cape_udc_ext (cursor->item, "workspace");
      if (h)
      {
        cape_udc_add (workspace_node, &h);
      }
    }

    // clts (optional)
    {
      CapeUdc h = cape_udc_ext (cursor->item, "clts");
      if (h)
      {
        cape_udc_add (workspace_node, &h);
      }
    }

    cape_udc_add (cdata, &workspace_node);
  }
  
  cape_udc_cursor_del (&cursor);
}

//---------------------------------------------------------------------------

void auth_rinfo__merge (AuthRInfo self, CapeUdc first_row, CapeUdc rinfo)
{
  // add essentials
  cape_udc_add_n (rinfo, "userid", self->userid);
  cape_udc_add_n (rinfo, "wpid", self->wpid);
  
  // ** add others **
  
  {
    CapeUdc h = cape_udc_ext (first_row, "gpid");
    if (h)
    {
      cape_udc_add (rinfo, &h);
    }
    else
    {
      cape_log_msg (CAPE_LL_WARN, "AUTH", "get rinfo", "result row has no gpid");
    }
  }
  
  {
    CapeUdc h = cape_udc_ext (first_row, "token");
    if (h)
    {
      cape_udc_add (rinfo, &h);
    }
    else
    {
      cape_log_msg (CAPE_LL_WARN, "AUTH", "get rinfo", "result row has no token");
    }
  }

  {
    CapeUdc h = cape_udc_ext (first_row, "secret");
    if (h)
    {
      cape_udc_add (rinfo, &h);
    }
    else
    {
      cape_log_msg (CAPE_LL_WARN, "AUTH", "get rinfo", "result row has no secret");
    }
  }
  
  {
    CapeUdc h = cape_udc_ext (first_row, "workspace");
    if (h)
    {
      cape_udc_add (rinfo, &h);
    }
    else
    {
      cape_log_msg (CAPE_LL_WARN, "AUTH", "get rinfo", "result row has no workspace");
    }
  }
  
  {
    CapeUdc h = cape_udc_ext (first_row, "title");
    if (h)
    {
      cape_udc_add (rinfo, &h);
    }
    else
    {
      cape_log_msg (CAPE_LL_WARN, "AUTH", "get rinfo", "result row has no title");
    }
  }
  
  {
    CapeUdc h = cape_udc_ext (first_row, "firstname");
    if (h)
    {
      cape_udc_add (rinfo, &h);
    }
    else
    {
      cape_log_msg (CAPE_LL_WARN, "AUTH", "get rinfo", "result row has no firstname");
    }
  }
  
  {
    CapeUdc h = cape_udc_ext (first_row, "lastname");
    if (h)
    {
      cape_udc_add (rinfo, &h);
    }
    else
    {
      cape_log_msg (CAPE_LL_WARN, "AUTH", "get rinfo", "result row has no lastname");
    }
  }
  
  {
    CapeUdc h = cape_udc_ext (first_row, "clts");
    if (h)
    {
      cape_udc_add (rinfo, &h);
    }
  }
  
  {
    CapeUdc h = cape_udc_ext (first_row, "cltsid");
    if (h)
    {
      cape_udc_add (rinfo, &h);
    }
  }
}

//---------------------------------------------------------------------------

int auth_rinfo__user_roles (AuthRInfo self, CapeUdc roles, CapeErr err)
{
  CapeUdc results = NULL;
  
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // conditions
  cape_udc_add_n      (params, "userid"       , self->userid);
  // return values
  cape_udc_add_s_cp   (values, "name"        , NULL);
  
  // execute the query
  results = adbl_session_query (self->adbl_session, "rbac_roles_view", &params, &values, err);
  if (results == NULL)
  {
    return cape_err_code (err);
  }
  
  // merge results
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (results, CAPE_DIRECTION_FORW);
    
    while (cape_udc_cursor_next (cursor))
    {
      const CapeString name = cape_udc_get_s (cursor->item, "name", NULL);
      if (name)
      {
        // todo: extend here to usefull values
        cape_udc_add_s_cp (roles, name, "role");
      }
    }
    
    cape_udc_cursor_del (&cursor);
  }
  
  cape_udc_del (&results);
  return CAPE_ERR_NONE;
}

//---------------------------------------------------------------------------

int auth_rinfo__workspace_roles (AuthRInfo self, CapeUdc cdata, CapeErr err)
{
  CapeUdc results = NULL;
  
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // conditions
  cape_udc_add_n      (params, "wpid"        , self->wpid);
  // return values
  cape_udc_add_s_cp   (values, "name"        , NULL);
  
  // execute the query
  results = adbl_session_query (self->adbl_session, "rbac_workspaces_roles_view", &params, &values, err);
  if (results == NULL)
  {
    return cape_err_code (err);
  }
  
  // merge results
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (results, CAPE_DIRECTION_FORW);
    
    while (cape_udc_cursor_next (cursor))
    {
      const CapeString name = cape_udc_get_s (cursor->item, "name", NULL);
      if (name)
      {
        // todo: extend here to usefull values
        cape_udc_add_s_cp (cdata, name, "role");
      }
    }
    
    cape_udc_cursor_del (&cursor);
  }
  
  cape_udc_del (&results);
  return CAPE_ERR_NONE;
}
//---------------------------------------------------------------------------

int auth_rinfo__roles (AuthRInfo self, CapeUdc rinfo, CapeErr err)
{
  int res;
  
  // create roles node
  CapeUdc roles = cape_udc_new (CAPE_UDC_NODE, "roles");

  // fetch all roles assign to the user
  res = auth_rinfo__user_roles (self, roles, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // fetch all roles assign to the workspace
  res = auth_rinfo__workspace_roles (self, roles, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  cape_udc_add (rinfo, &roles);
  return res;
}

//---------------------------------------------------------------------------

int auth_rinfo__userinfo (AuthRInfo self, CapeUdc results, CapeUdc rinfo, CapeErr err)
{
  CapeUdc first_row = cape_udc_get_first (results);
  
  // this should not happen
  if (first_row == NULL)
  {
    cape_log_msg (CAPE_LL_ERROR, "AUTH", "get rinfo", "result has no first row");
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "no rinfo found");
  }
  
  self->wpid = cape_udc_get_n (first_row, "wpid", 0);
  if (self->wpid == 0)
  {
    cape_log_msg (CAPE_LL_ERROR, "AUTH", "get rinfo", "result has no wpid");
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "no rinfo found");
  }
  
  // fill out the rinfo
  auth_rinfo__merge (self, first_row, rinfo);

  // fill out the roles
  return auth_rinfo__roles (self, rinfo, err);
}

//---------------------------------------------------------------------------

int auth_rinfo__results (AuthRInfo self, CapeUdc results, CapeUdc rinfo, CapeUdc cdata, CapeErr err)
{
  number_t ressize = cape_udc_size (results);
  
  if (ressize == 0)
  {
    return cape_err_set (err, CAPE_ERR_NO_AUTH, "no rinfo found");
  }
  
  if (ressize > 1)
  {
    auth_rinfo__workspaces (self, results, cdata);
    
    return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "too many results for rbac");
  }

  return auth_rinfo__userinfo (self, results, rinfo, err);
}

//---------------------------------------------------------------------------

// TODO: replace this method by a similar as auth_rinfo_get_gpid
int auth_rinfo_get (AuthRInfo* p_self, QBusM qout, CapeErr err)
{
  int res;
  AuthRInfo self = *p_self;
  
  CapeUdc results = NULL;
  
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // conditions
  cape_udc_add_n      (params, "userid"       , self->userid);
  // return values
  cape_udc_add_n      (values, "wpid"         , 0);
  cape_udc_add_n      (values, "gpid"         , 0);
  cape_udc_add_s_cp   (values, "workspace"    , NULL);
  cape_udc_add_s_cp   (values, "clts"         , NULL);
  cape_udc_add_n      (values, "cltsid"       , 0);
  cape_udc_add_s_cp   (values, "title"        , NULL);
  cape_udc_add_s_cp   (values, "firstname"    , NULL);
  cape_udc_add_s_cp   (values, "lastname"     , NULL);
  cape_udc_add_s_cp   (values, "token"        , NULL);
  cape_udc_add_s_cp   (values, "secret"       , NULL);

  // optional conditions
  if (self->wpid)
  {
    cape_udc_add_n     (params, "wpid"       , self->wpid);
  }

  // execute the query
  results = adbl_session_query (self->adbl_session, "rbac_users_view", &params, &values, err);
  if (results == NULL)
  {
    cape_log_msg (CAPE_LL_ERROR, "AUTH", "crypt4", cape_err_text (err));
    
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  // create the rinfo and cdata node
  if (qout->rinfo == NULL)
  {
    qout->rinfo = cape_udc_new (CAPE_UDC_NODE, NULL);
  }
  
  if (qout->cdata == NULL)
  {
    qout->cdata = cape_udc_new (CAPE_UDC_LIST, NULL);
  }
  
  res = auth_rinfo__results (self, results, qout->rinfo, qout->cdata, err);
  
  if (cape_udc_size (qout->cdata) == 0)
  {
    cape_udc_del (&(qout->cdata));
  }
  
exit_and_cleanup:
  
  cape_udc_del (&results);
  
  auth_rinfo_del (p_self);
  
  return res;
}

//---------------------------------------------------------------------------

int auth_rinfo_get_gpid (AuthRInfo* p_self, number_t gpid, CapeUdc* p_rinfo, CapeUdc* p_cdata, CapeErr err)
{
  int res;
  AuthRInfo self = *p_self;
  
  // local objects
  CapeUdc query_results = NULL;
  CapeUdc rinfo = NULL;
  CapeUdc cdata = NULL;
  
  {
    CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    // conditions
    cape_udc_add_n      (params, "gpid"         , gpid);
    // return values
    cape_udc_add_n      (values, "wpid"         , 0);
    cape_udc_add_n      (values, "gpid"         , 0);
    cape_udc_add_s_cp   (values, "workspace"    , NULL);
    cape_udc_add_s_cp   (values, "title"        , NULL);
    cape_udc_add_s_cp   (values, "firstname"    , NULL);
    cape_udc_add_s_cp   (values, "lastname"     , NULL);
    cape_udc_add_s_cp   (values, "token"        , NULL);
    cape_udc_add_s_cp   (values, "secret"       , NULL);
    
    // execute the query
    query_results = adbl_session_query (self->adbl_session, "rbac_users_view", &params, &values, err);
    if (query_results == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
  }

  rinfo = cape_udc_new (CAPE_UDC_NODE, NULL);
  cdata = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  res = auth_rinfo__results (self, query_results, rinfo, cdata, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
  cape_udc_replace_mv (p_rinfo, &rinfo);
  cape_udc_replace_mv (p_cdata, &cdata);

exit_and_cleanup:

  cape_udc_del (&rinfo);
  cape_udc_del (&cdata);

  return res;
}

//---------------------------------------------------------------------------
