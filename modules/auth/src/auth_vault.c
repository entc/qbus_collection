#include "auth_vault.h"

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_mutex.h>
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct AuthVault_s
{
  CapeMap contexts;
  
  CapeMutex mutex;
};

//-----------------------------------------------------------------------------

static int __STDCALL auth_vault__on_cmp (const void* a, const void* b, void* ptr)
{
  number_t v1 = (number_t)a;
  number_t v2 = (number_t)b;
  
  if (v1 < v2)
  {
    return -1;
  }
  else if (v1 > v2)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL auth_vault__on_del (void* key, void* val)
{
  {
    CapeString h = val; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

AuthVault auth_vault_new (void)
{
  AuthVault self = CAPE_NEW(struct AuthVault_s);
  
  self->contexts = cape_map_new (auth_vault__on_cmp, auth_vault__on_del, NULL);
  self->mutex = cape_mutex_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void auth_vault_del (AuthVault* p_self)
{
  AuthVault self = *p_self;
  
  cape_map_del (&(self->contexts));
  cape_mutex_del (&(self->mutex));
  
  CAPE_DEL (p_self, struct AuthVault_s);
}

//-----------------------------------------------------------------------------

int auth_vault_set (AuthVault self, QBusM qin, QBusM qout, CapeErr err)
{
  // do some security checks
  if (qin->rinfo == NULL)
  {
    return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing rinfo");
  }

  if (qin->cdata == NULL)
  {
    return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing cdata");
  }

  number_t wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);
  
  if (wpid == 0)
  {
    return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing wpid");
  }

  const CapeString secret = cape_udc_get_s (qin->cdata, "secret", NULL);
  if (secret == NULL)
  {
    return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing secret");
  }
  
  auth_vault__save (self, wpid, secret);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

const CapeString auth_vault__vsec (AuthVault self, number_t wpid)
{
  const CapeString secret = NULL;
  
  cape_mutex_lock (self->mutex);
  
  {
    CapeMapNode n = cape_map_find (self->contexts, (void*)wpid);
    
    if (n)
    {
      secret = cape_map_node_value (n);
    }
  }
  
  cape_mutex_unlock (self->mutex);

  return secret;
}

//-----------------------------------------------------------------------------

void auth_vault__save (AuthVault self, number_t wpid, const CapeString vsec)
{
  cape_mutex_lock (self->mutex);
  
  {
    CapeMapNode n = cape_map_find (self->contexts, (void*)wpid);
    
    if (n == NULL)
    {
      //cape_log_fmt (CAPE_LL_DEBUG, "AUTH", "vault set", "new vault secret inserted %i = '%s'", wpid, secret);
      
      cape_map_insert (self->contexts, (void*)wpid, cape_str_cp (vsec));
    }
  }
  
  cape_mutex_unlock (self->mutex);
  
  cape_log_fmt (CAPE_LL_TRACE, "AUTH", "vault", "value set wpid = %i", wpid);
}

//-----------------------------------------------------------------------------

int auth_vault_get (AuthVault self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  const CapeString secret;
  
  // local objects
  number_t wpid = 0;

  // do some security checks
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "ERR.NO_AUTH");
    goto exit_and_cleanup;
  }

  // retrieve the default wpid from the account
  wpid = cape_udc_get_n (qin->rinfo, "wpid", 0);

  if (qin->pdata)
  {
    // allow user role or admin role from workspace
    if (qbus_message_role_has (qin, "admin"))
    {
      wpid = cape_udc_get_n (qin->pdata, "wpid", 0);
    }
  }
  else if (qin->cdata)  // depricated way
  {
    // we need to set a special role
    CapeUdc roles = cape_udc_get (qin->rinfo, "roles");
    if (roles)
    {
      CapeUdc h = cape_udc_get (roles, "__auth_vsec");
      if (h)
      {
        wpid = cape_udc_get_n (qin->cdata, "wpid", 0);
      }
    }
  }

  if (wpid == 0)
  {
    CapeString h = cape_json_to_s (qin->rinfo);
    
    cape_log_fmt (CAPE_LL_WARN, "AUTH", "vault get", "wpid is missing: rinfo = %s", h);
    
    cape_str_del (&h);
    
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "missing wpid");
    goto exit_and_cleanup;
  }

  secret = auth_vault__vsec (self, wpid);
  
  if (secret == NULL)
  {
    cape_log_fmt (CAPE_LL_ERROR, "AUTH", "vault get", "no secret for wpid (%i)", wpid);
    
    res = cape_err_set (err, CAPE_ERR_NO_AUTH, "{OpenVault} secret not found");
    goto exit_and_cleanup;
  }
  
  // create the output
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_s_cp (qout->cdata, "secret", secret);

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------
