#include "qbus_tl1.h"
#include "qbus_chains.h"
#include "qbus_member.h"
#include "qbus_subscriber.h"

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_mutex.h>

//-----------------------------------------------------------------------------

struct QBusManifold_s
{
  CapeMap members;
  CapeMap subs;
  CapeMutex subs_mutex;
  
  QBusChains chains;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_manifold__members__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusManifoldMember h = val; qbus_manifold_member_del (&h);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_manifold__subs__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    CapeList h = val; cape_list_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusManifold qbus_manifold_new ()
{
  QBusManifold self = CAPE_NEW (struct QBusManifold_s);

  self->members = cape_map_new (cape_map__compare__s, qbus_manifold__members__on_del, NULL);
  self->subs = cape_map_new (cape_map__compare__s, qbus_manifold__subs__on_del, NULL);

  self->subs_mutex = cape_mutex_new ();
  
  self->chains = qbus_chains_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_del (QBusManifold* p_self)
{
  if (*p_self)
  {
    QBusManifold self = *p_self;
    
    qbus_chains_del (&(self->chains));
    
    cape_mutex_del (&(self->subs_mutex));
    
    cape_map_del (&(self->members));
    cape_map_del (&(self->subs));

    CAPE_DEL (p_self, struct QBusManifold_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_manifold_init (QBusManifold self, const CapeString uuid, const CapeString name, void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call, fct_qbus_manifold__on_recv on_recv, fct_qbus_manifold__on_emit on_emit, CapeErr err)
{
  printf ("manifold init uuid = %s\n", uuid);
  
  QBusManifoldMember member;
  
  CapeMapNode n = cape_map_find (self->members, (void*)uuid);
  
  if (n)
  {
    return cape_err_set (err, CAPE_ERR_EOF, "already set");
  }

  member = qbus_manifold_member_new (name, user_ptr, on_add, on_rm, on_call, on_recv, on_emit);
  
  n = cape_map_insert (self->members, (void*)cape_str_cp (uuid), member);

  // tell all others the new 'connection'
  {
    CapeMapCursor* cursor = cape_map_cursor_new (self->members, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      if (cursor->node != n)
      {
        QBusManifoldMember m = cape_map_node_value (cursor->node);

        // tell the other 'module' the new 'module'
        qbus_manifold_member_add (m, uuid, name, n);
        
        // tell the new 'module' having this 'module'
        qbus_manifold_member_add (member, cape_map_node_key (cursor->node), qbus_manifold_member_name (m), cursor->node);
      }
    }
    
    cape_map_cursor_del (&cursor);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

QBusManifoldMember qbus_manifold__get_member (QBusManifold self, const CapeString module_ident)
{
  QBusManifoldMember ret = NULL;

  // try to find the member
  CapeMapNode n = cape_map_find (self->members, (void*)module_ident);
  if (n)
  {
    ret = cape_map_node_value (n);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_manifold_emit__subslist (QBusManifold self, CapeList subslist, CapeUdc val, const CapeString module_ident, const CapeString module_name)
{
  CapeListCursor* cursor = cape_list_cursor_new (subslist, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    qbus_manifold_subscriber_emit (cape_list_node_data (cursor->node), val, module_ident, module_name);
  }
  
  cape_list_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------

void qbus_manifold_emit (QBusManifold self, const CapeString uuid, CapeUdc val)
{
  QBusManifoldMember member = qbus_manifold__get_member (self, uuid);
  
  if (member)
  {
    //printf ("EMIT [%s]: %s -> %s\n", uuid, val_name, val_data);

    // check if we find subscriptors
    cape_mutex_lock (self->subs_mutex);

    {
      CapeMapNode n = cape_map_find (self->subs, (void*)cape_udc_name (val));
      if (n)
      {
        qbus_manifold_emit__subslist (self, cape_map_node_value (n), val, uuid, qbus_manifold_member_name (member));
      }
    }
    
    cape_mutex_unlock (self->subs_mutex);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_manifold__subslist__on_del (void* ptr)
{
  QBusManifoldSubscriber h = ptr; qbus_manifold_subscriber_del (&h);
}

//-----------------------------------------------------------------------------

void qbus_manifold_subscribe (QBusManifold self, const CapeString uuid, const CapeString module_uuid, const CapeString module_name, const CapeString value_name)
{
  QBusManifoldMember member = qbus_manifold__get_member (self, uuid);
  
  if (member)
  {
    cape_mutex_lock (self->subs_mutex);

    {
      CapeList subslist = NULL;

      {
        CapeMapNode n = cape_map_find (self->subs, (void*)value_name);
        if (NULL == n)
        {
          subslist = cape_list_new (qbus_manifold__subslist__on_del);
          
          cape_map_insert (self->subs, (void*)cape_str_cp (value_name), subslist);
        }
        else
        {
          subslist = cape_map_node_value (n);
        }
      }
      
      cape_list_push_back (subslist, (void*)qbus_manifold_subscriber_new (member, module_uuid, module_name));
            
      printf ("[subscriber] -> add, ident = %s, name = %s, value = %s\n", module_uuid, module_name, value_name);
    }
    
    cape_mutex_unlock (self->subs_mutex);
  }
}

//-----------------------------------------------------------------------------

void qbus_manifold_response (QBusManifold self, QBusM msg)
{
  QBusManifoldMember member = qbus_manifold__get_member (self, msg->module_ident);

  // TODO check for p_node
  printf ("response -> {%s} CK[%s]\n", msg->module_ident, msg->method_ident);

  if (member)
  {
    printf ("found node\n");

    QBusMethod method = qbus_chains_pop (self->chains, msg->method_ident);

    if (method)
    {
      CapeErr err = cape_err_new ();

      // move module identifiactions into the method object
      qbus_method_idents (method, &(msg->module_ident), &(msg->method_ident));
            
      int res = qbus_manifold_member_call (member, NULL, &method, msg, err);
      if (res)
      {
        
      }

      cape_err_del (&err);
      qbus_method_del (&method);
    }
  }
}

//-----------------------------------------------------------------------------

int qbus_manifold_send (QBusManifold self, const CapeString module_ident, void** p_node, const CapeString method_name, QBusM msg, QBusMethod* p_qbus_method, CapeErr err)
{
  CapeMapNode n = *p_node;
  
  cape_str_del (&(msg->method_ident));
  msg->method_ident = cape_str_uuid ();

  cape_str_del (&(msg->module_ident));
  msg->module_ident = cape_str_cp (module_ident);
  
  qbus_chains_push (self->chains, msg->method_ident, p_qbus_method);

  // shortcut of sending it to the peer
  {
    QBusManifoldMember m = cape_map_node_value (n);
    
    printf ("send {%s} -> %s {%s} -> %s CK[%s]\n", module_ident, qbus_manifold_member_name (m), (CapeString)cape_map_node_key (n), method_name, msg->method_ident);
    
    return qbus_manifold_member_call (m, method_name, NULL, msg, err);
  }
}

//-----------------------------------------------------------------------------
