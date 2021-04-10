#include "auth_session.h"
#include "auth_rinfo.h"

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
};

//-----------------------------------------------------------------------------

AuthSession auth_session_new (AdblSession adbl_session, AuthVault vault)
{
  AuthSession self = CAPE_NEW (struct AuthSession_s);
  
  self->adbl_session = adbl_session;
  self->vault = vault;
  
  self->wpid = 0;
  self->gpid = 0;
  
  return self;
}

//-----------------------------------------------------------------------------

void auth_session_del (AuthSession* p_self)
{
  if (*p_self)
  {
    AuthSession self = *p_self;
    

    CAPE_DEL (p_self, struct AuthSession_s);
  }
}

//-----------------------------------------------------------------------------

int auth_session_add (AuthSession* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthSession self = *p_self;
  

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  auth_session_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------

int auth_session_get (AuthSession* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  AuthSession self = *p_self;
  
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  auth_session_del (p_self);
  return res;
}

//-----------------------------------------------------------------------------
