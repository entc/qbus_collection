#include "qbus_chain.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_queue.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct QBusChain_s
{
  
};

//-----------------------------------------------------------------------------

QBusChain qbus_chain_new ()
{
  QBusChain self = CAPE_NEW (struct QBusChain_s);
  
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_chain_del (QBusChain* p_self)
{
  if (*p_self)
  {
    QBusChain self = *p_self;
    
    
    CAPE_DEL (p_self, struct QBusChain_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_chain_add (QBusChain self, void* ptr, fct_qbus_onMessage onMsg, CapeString* p_last_chainkey, CapeString* p_next_chainkey, CapeString* p_last_sender, CapeUdc* p_rinfo)
{
  
}

//-----------------------------------------------------------------------------
