#include "qbus_tl1.h" 

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_mutex.h>

//-----------------------------------------------------------------------------

struct QBusManifoldMember_s
{
  CapeString name;
  
  void* user_ptr;
  
  fct_qbus_manifold__on_add on_add;
  fct_qbus_manifold__on_rm on_rm;
  fct_qbus_manifold__on_call on_call;
  fct_qbus_manifold__on_recv on_recv;
  fct_qbus_manifold__on_emit on_emit;
  
}; typedef struct QBusManifoldMember_s* QBusManifoldMember;

//-----------------------------------------------------------------------------

QBusManifoldMember qbus_manifold_member_new (const CapeString name, void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call,fct_qbus_manifold__on_recv on_recv, fct_qbus_manifold__on_emit on_emit)
{
  QBusManifoldMember self = CAPE_NEW (struct QBusManifoldMember_s);
  
  self->name = cape_str_cp (name);
  
  self->user_ptr = user_ptr;
  
  self->on_call = on_call;
  self->on_emit = on_emit;
  
  self->on_add = on_add;
  self->on_rm = on_rm;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_member_del (QBusManifoldMember* p_self)
{
  if (*p_self)
  {
    QBusManifoldMember self = *p_self;
    
    cape_str_del (&(self->name));
    
    CAPE_DEL (p_self, struct QBusManifoldMember_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_manifold_member_add (QBusManifoldMember self, const char* uuid, const char* module, void* node)
{
  if (self->on_add)
  {
    self->on_add (self->user_ptr, uuid, module, node);
  }
}

//-----------------------------------------------------------------------------

int qbus_manifold_member_call (QBusManifoldMember self, const CapeString method_name, QBusMethod* p_qbus_method, QBusM msg, CapeErr err)
{
  if (self->on_call)
  {
    self->on_call (self->user_ptr, method_name, p_qbus_method, msg);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

struct QBusManifold_s
{
  CapeMap members;
  CapeMap chains;
  CapeMutex chains_mutex;
  
  
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

void __STDCALL qbus_manifold__chains__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusMethod method = val; qbus_method_del (&method);
  }
}

//-----------------------------------------------------------------------------

QBusManifold qbus_manifold_new ()
{
  QBusManifold self = CAPE_NEW (struct QBusManifold_s);

  self->members = cape_map_new (cape_map__compare__s, qbus_manifold__members__on_del, NULL);
  self->chains = cape_map_new (cape_map__compare__s, qbus_manifold__chains__on_del, NULL);
  
  self->chains_mutex = cape_mutex_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_del (QBusManifold* p_self)
{
  if (*p_self)
  {
    QBusManifold self = *p_self;
    
    cape_mutex_del (&(self->chains_mutex));
    
    cape_map_del (&(self->chains));
    cape_map_del (&(self->members));
    
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
        qbus_manifold_member_add (member, cape_map_node_key (cursor->node), m->name, cursor->node);
      }
    }
    
    cape_map_cursor_del (&cursor);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void qbus_manifold_emit (QBusManifold self)
{
  
}

//-----------------------------------------------------------------------------

void qbus_manifold_subscribe (QBusManifold self)
{
  
}

//-----------------------------------------------------------------------------

QBusMethod qbus_manifold_response__pop (QBusManifold self, const CapeString method_ident)
{
  QBusMethod ret = NULL;
  
  cape_mutex_lock (self->chains_mutex);
  
  CapeMapNode n2 = cape_map_find (self->chains, (void*)method_ident);
  if (n2)
  {
    ret = cape_map_node_value (n2);
    
    cape_map_node_set (n2, (void*)NULL);
    
    cape_map_erase (self->chains, n2);
  }

  cape_mutex_unlock (self->chains_mutex);
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_manifold_response (QBusManifold self, QBusM msg)
{
  // TODO check for p_node
  printf ("response -> {%s} CK[%s]\n", msg->module_ident, msg->method_ident);
  
  CapeMapNode n = cape_map_find (self->members, (void*)msg->module_ident);
  
  if (n)
  {
    QBusManifoldMember m = cape_map_node_value (n);
    
    printf ("found node\n");

    QBusMethod method = qbus_manifold_response__pop (self, msg->method_ident);

    if (method)
    {
      CapeErr err = cape_err_new ();

      // move module identifiactions into the method object
      qbus_method_idents (method, &(msg->module_ident), &(msg->method_ident));
            
      int res = qbus_manifold_member_call (m, NULL, &method, msg, err);
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
  
  cape_mutex_lock (self->chains_mutex);

  cape_map_insert (self->chains, (void*)cape_str_cp (msg->method_ident), (void*)*p_qbus_method);
  *p_qbus_method = NULL;

  cape_mutex_unlock (self->chains_mutex);

  // shortcut of sending it to the peer
  {
    QBusManifoldMember m = cape_map_node_value (n);
    
    printf ("send {%s} -> %s {%s} -> %s CK[%s]\n", module_ident, m->name, (CapeString)cape_map_node_key (n), method_name, msg->method_ident);
    
    return qbus_manifold_member_call (m, method_name, NULL, msg, err);
  }
}

//-----------------------------------------------------------------------------
