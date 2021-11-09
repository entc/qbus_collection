#include "cape_map.h"

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_log.h"

// c includes
#include <assert.h>
#include <string.h>

//-----------------------------------------------------------------------------

#define CAPE_MAP_CHILD     0
#define CAPE_MAP_THREAD    1

//-----------------------------------------------------------------------------

int __STDCALL cape_map__compare__s (const void* a, const void* b, void* ptr)
{
  const char* s1 = a;
  const char* s2 = b;
  
  return strcmp(s1, s2);
}

//-----------------------------------------------------------------------------

int __STDCALL cape_map__compare__n (const void* a, const void* b, void* ptr)
{
  number_t ia = (number_t)a;
  number_t ib = (number_t)b;
  
  if (ia > ib)
  {
    return 1;
  }
  else if (ia < ib)
  {
    return -1;
  }
  
  return 0;
}

//=============================================================================

struct CapeMapNode_s
{
  CapeMapNode link[2];
    
  void* key;

  unsigned char tag[2];
  signed char balance;

  void* val;
};

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_new (void* key, void* val)
{
  CapeMapNode self = CAPE_NEW(struct CapeMapNode_s);
  
  self->key = key;
  self->val = val;
  
  self->tag[0] = 0;
  self->tag[1] = 0;
  
  self->balance = 0;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_map_node_del (CapeMapNode* pself)
{
  CAPE_DEL(pself, struct CapeMapNode_s);
}

//-----------------------------------------------------------------------------

void* cape_map_node_value (CapeMapNode self)
{
  return self->val;
}

//-----------------------------------------------------------------------------

void* cape_map_node_key (CapeMapNode self)
{
  return self->key;
}

//-----------------------------------------------------------------------------

void cape_map_node_set (CapeMapNode self, void* val)
{
  self->val = val;
}

//-----------------------------------------------------------------------------

void* cape_map_node_mv (CapeMapNode self)
{
  void* ret = self->val;
  self->val = NULL;
  
  return ret;  
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_next (CapeMapNode n)
{
  CapeMapNode ret = n;
  
  if (ret->tag[1] == CAPE_MAP_THREAD)
  {
    ret = ret->link[1];
  }
  else
  {
    ret = ret->link[1];
    
    while (ret->tag[0] == CAPE_MAP_CHILD)
    {
      ret = ret->link[0];
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_prev (CapeMapNode n)
{
  CapeMapNode ret = n;

  if (ret->tag[0] == CAPE_MAP_THREAD)
  {
    ret = ret->link[0];
  }
  else
  {
    ret = ret->link[0];

    while (ret->tag[1] == CAPE_MAP_CHILD)
    {
      ret = ret->link[1];
    }
  }
  
  return ret;
}

//=============================================================================

struct CapeMap_s
{
  CapeMapNode root;
  
  fct_cape_map_cmp cmp_fct;  
  void* cmp_ptr;
  
  fct_cape_map_destroy del_fct;
  
  size_t size;                  
};

//-----------------------------------------------------------------------------

CapeMap cape_map_new (fct_cape_map_cmp on_cmp, fct_cape_map_destroy on_del, void* ptr_cmp)
{
  CapeMap self = CAPE_NEW(struct CapeMap_s);
  
  self->root = NULL;
  self->size = 0;
  
  self->cmp_fct = on_cmp ? on_cmp : cape_map__compare__s;
  self->cmp_ptr = ptr_cmp;
  
  self->del_fct = on_del;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_map_del_node (CapeMap self, CapeMapNode* pself)
{
  CapeMapNode n = *pself;
  
  if (self->del_fct && n)
  {
    self->del_fct (n->key, n->val);
  }
  
  cape_map_node_del (pself);
}

//-----------------------------------------------------------------------------

void cape_map_clr (CapeMap self)
{  
  CapeMapNode p;    // current node
  CapeMapNode n;    // next node
  
  p = self->root;
  
  if (p != NULL) while (p->tag[0] == CAPE_MAP_CHILD)
  {
    p = p->link[0];
  }
    
  while (p != NULL)
  {
    n = p->link[1];
    
    if (p->tag[1] == CAPE_MAP_CHILD) while (n->tag[0] == CAPE_MAP_CHILD)
    {
      n = n->link[0];
    }
     
    cape_map_del_node (self, &p);
         
    p = n;
  }

  self->root = NULL;
  self->size = 0;
}

//-----------------------------------------------------------------------------

void cape_map_del (CapeMap* p_self)
{
  if (*p_self)
  {
    CapeMap self = *p_self;
    
    cape_map_clr (self);
    
    CAPE_DEL(p_self, struct CapeMap_s);
  }
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_find (CapeMap self, const void* key)
{
  CapeMapNode p;
  
  p = self->root;
  
  if (p == NULL)
  {
    return NULL;
  }
  
  for (;;)
  {
    int cmp, dir;

    // call the compare function
    cmp = self->cmp_fct (key, p->key, self->cmp_ptr);    

    // result is equal
    if (cmp == 0)
    {
      return p;
    }
    
    dir = cmp > 0;

    if (p->tag[dir] == CAPE_MAP_CHILD)
    {
      p = p->link[dir];
    }
    else
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_find_cmd (CapeMap self, const void* key, fct_cape_map_cmp on_cmp)
{
  CapeMapNode ret = NULL;
    
  if (on_cmp)
  {
    // save the default compare method
    fct_cape_map_cmp on_original_cmp = self->cmp_fct;

    // temporary set the other compare method
    self->cmp_fct = on_cmp;
    
    // do the search
    ret = cape_map_find (self, key);
    
    // set back
    self->cmp_fct = on_original_cmp;
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_insert (CapeMap self, void* key, void* val)
{
  CapeMapNode y;
  CapeMapNode z;          /* Top node to update balance factor, and parent. */
  CapeMapNode p;
  CapeMapNode q;          /* Iterator, and parent. */
  CapeMapNode n;          /* Newly inserted node. */
  CapeMapNode w;          /* New root of rebalanced subtree. */
  int dir;                /* Direction to descend. */
  
  unsigned char da[128];   /* Cached comparison results. */
  int k = 0;              /* Number of cached results. */
  
  memset (da, 0, 128);
  
  z = (CapeMapNode)&self->root;
  y = self->root;
  
  if (y != NULL)
  {
    for (q = z, p = y; ; q = p, p = p->link[dir])
    {
      int cmp = self->cmp_fct (key, p->key, self->cmp_ptr);      
      if (cmp == 0)
      {
        if (self->cmp_fct == cape_map__compare__s)
        {
          cape_log_fmt (CAPE_LL_WARN, "CAPE", "map insert", "key already exists '%s'", (char*)p->key);
        }
        else
        {
          cape_log_msg (CAPE_LL_WARN, "CAPE", "map insert", "key already exists");
        }
        
        return p;
      }
      
      if (p->balance != 0)
      {
        z = q, y = p, k = 0;
      }
      
      da[k++] = dir = cmp > 0;
      
      if (p->tag[dir] == CAPE_MAP_THREAD)
      {
        break;
      }
    }
  }
  else
  {
    p = z;
    dir = 0;
  }
  
  n = cape_map_node_new (key, val);
  
  self->size++;
  
  n->tag[0] = n->tag[1] = CAPE_MAP_THREAD;
  n->link[dir] = p->link[dir];
  
  if (self->root != NULL)
  {
    p->tag[dir] = CAPE_MAP_CHILD;
    n->link[!dir] = p;
  }
  else
  {
    n->link[1] = NULL;
  }
  
  p->link[dir] = n;
  n->balance = 0;

  if (self->root == n)
  {
    return n;
  }
  
  for (p = y, k = 0; p != n; p = p->link[da[k]], k++)
  {
    if (da[k] == 0)
    {
      p->balance--;
    }
    else
    {
      p->balance++;
    }
  }
    
  if (y->balance == -2)
  {
    CapeMapNode x = y->link[0];

    if (x->balance == -1)
    {
      w = x;

      if (x->tag[1] == CAPE_MAP_THREAD)
      {
        x->tag[1] = CAPE_MAP_CHILD;
        y->tag[0] = CAPE_MAP_THREAD;
        y->link[0] = x;
      }
      else
      {
        y->link[0] = x->link[1];
      }
      
      x->link[1] = y;
      x->balance = y->balance = 0;
    }
    else
    {
      assert (x->balance == +1);
      
      w = x->link[1];
      x->link[1] = w->link[0];
      w->link[0] = x;
      y->link[0] = w->link[1];
      w->link[1] = y;
      
      if (w->balance == -1)
      {
        x->balance = 0, y->balance = +1;
      }
      else if (w->balance == 0)
      {
        x->balance = y->balance = 0;
      }
      else /* |w->tavl_balance == +1| */
      {
        x->balance = -1, y->balance = 0;
      }
      
      w->balance = 0;
      
      if (w->tag[0] == CAPE_MAP_THREAD)
      {
        x->tag[1] = CAPE_MAP_THREAD;
        x->link[1] = w;
        w->tag[0] = CAPE_MAP_CHILD;
      }
      
      if (w->tag[1] == CAPE_MAP_THREAD)
      {
        y->tag[0] = CAPE_MAP_THREAD;
        y->link[0] = w;
        w->tag[1] = CAPE_MAP_CHILD;
      }
    }
  }
  else if (y->balance == +2)
  {
    CapeMapNode x = y->link[1];
    
    if (x->balance == +1)
    {
      w = x;
      if (x->tag[0] == CAPE_MAP_THREAD)
      {
        x->tag[0] = CAPE_MAP_CHILD;
        y->tag[1] = CAPE_MAP_THREAD;
        y->link[1] = x;
      }
      else
      {
        y->link[1] = x->link[0];
      }
      
      x->link[0] = y;
      x->balance = y->balance = 0;
    }
    else
    {
      assert (x->balance == -1);
      
      w = x->link[0];
      
      x->link[0] = w->link[1];
      w->link[1] = x;
      y->link[1] = w->link[0];
      w->link[0] = y;
      
      if (w->balance == +1)
      {
        x->balance = 0, y->balance = -1;
      }
      else if (w->balance == 0)
      {
        x->balance = y->balance = 0;
      }
      else /* |w->tavl_balance == -1| */
      {
        x->balance = +1, y->balance = 0;
      }
      
      w->balance = 0;
      
      if (w->tag[0] == CAPE_MAP_THREAD)
      {
        y->tag[1] = CAPE_MAP_THREAD;
        y->link[1] = w;
        w->tag[0] = CAPE_MAP_CHILD;
      }
      
      if (w->tag[1] == CAPE_MAP_THREAD)
      {
        x->tag[0] = CAPE_MAP_THREAD;
        x->link[0] = w;
        w->tag[1] = CAPE_MAP_CHILD;
      }
    }
  }
  else
  {
    return n;
  }
    
  z->link[y != z->link[0]] = w;
    
  return n;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_find_parent (CapeMap self, CapeMapNode node)
{
  if (node != self->root)
  {
    CapeMapNode x;
    CapeMapNode y;
    
    for (x = y = node; ; x = x->link[0], y = y->link[1])
    {
      if (y->tag[1] == CAPE_MAP_THREAD)
      {
        CapeMapNode p = y->link[1];
        
        if (p == NULL || p->link[0] != node)
        {
          while (x->tag[0] == CAPE_MAP_CHILD)
          {
            x = x->link[0];
          }
          
          p = x->link[0];
        }
        
        return p;
      }
      else if (x->tag[0] == CAPE_MAP_THREAD)
      {
        CapeMapNode p = x->link[0];
        
        if (p == NULL || p->link[1] != node)
        {
          while (y->tag[1] == CAPE_MAP_CHILD)
          {
            y = y->link[1];
          }
          
          p = y->link[1];
        }
        
        return p;
      }
    }
  }
  else
  {
    return (CapeMapNode) &self->root;
  }
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_extract (CapeMap self, CapeMapNode node)
{
  CapeMapNode item;
  CapeMapNode p;       /* Traverses tree to find node to delete. */
  CapeMapNode q;       /* Parent of |p|. */
  int dir;             /* Index into |q->tavl_link[]| to get |p|. */
  int cmp;             /* Result of comparison between |item| and |p|. */
  
  if (self->root == NULL)
  {
    return NULL;
  }
  
  q = (CapeMapNode) &self->root;
  p = self->root;
  dir = 0;

  // search for the node
  for (;;)
  {
    cmp = self->cmp_fct (node->key, p->key, self->cmp_ptr);
    if (cmp == 0)
    {
      break;
    }
    
    dir = cmp > 0;
    
    q = p;
    
    if (p->tag[dir] == CAPE_MAP_THREAD)
    {
      return NULL;
    }
    
    p = p->link[dir];
  }
  
  // set the return node
  item = p;
  
  if (p->tag[1] == CAPE_MAP_THREAD)
  {
    if (p->tag[0] == CAPE_MAP_CHILD)
    {
      CapeMapNode t = p->link[0];
      
      // traverse to the last child on the right side
      while (t->tag[1] == CAPE_MAP_CHILD)
      {
        t = t->link[1];
      }
      
      t->link[1] = p->link[1];
      q->link[dir] = p->link[0];
    }
    else
    {
      q->link[dir] = p->link[dir];
      
      if (q != (CapeMapNode) &self->root)
      {
        q->tag[dir] = CAPE_MAP_THREAD;
      }
    }
  }
  else
  {
    CapeMapNode r = p->link[1];
    
    if (r->tag[0] == CAPE_MAP_THREAD)
    {
      r->link[0] = p->link[0];
      r->tag[0] = p->tag[0];
      
      if (r->tag[0] == CAPE_MAP_CHILD)
      {
        CapeMapNode t = r->link[0];
        
        // traverse to the last child on the right side
        while (t->tag[1] == CAPE_MAP_CHILD)
        {
          t = t->link[1];
        }
        
        t->link[1] = r;
      }
      
      q->link[dir] = r;
      r->balance = p->balance;
      
      q = r;
      dir = 1;
    }
    else
    {
      CapeMapNode s;
      
      for (;;)
      {
        s = r->link[0];
        
        if (s->tag[0] == CAPE_MAP_THREAD)
        {
          break;
        }
        
        r = s;
      }
      
      if (s->tag[1] == CAPE_MAP_CHILD)
      {
        r->link[0] = s->link[1];
      }
      else
      {
        r->link[0] = s;
        r->tag[0] = CAPE_MAP_THREAD;
      }
      
      s->link[0] = p->link[0];
      
      if (p->tag[0] == CAPE_MAP_CHILD)
      {
        CapeMapNode t = p->link[0];
        
        // traverse to the last child on the right side
        while (t->tag[1] == CAPE_MAP_CHILD)
        {
          t = t->link[1];
        }
        
        t->link[1] = s;
        s->tag[0] = CAPE_MAP_CHILD;
      }
      
      s->link[1] = p->link[1];
      s->tag[1] = CAPE_MAP_CHILD;
      
      q->link[dir] = s;
      s->balance = p->balance;
      q = r;
      dir = 0;
    }
  }
  
  //tree->tavl_alloc->libavl_free (tree->tavl_alloc, p);
  
  while (q != (CapeMapNode) &self->root)
  {
    CapeMapNode y = q;
    
    q = cape_map_find_parent (self, y);
    
    if (dir == 0)
    {
      dir = q->link[0] != y;
      y->balance++;
      if (y->balance == +1)
      {
        break;
      }
      else if (y->balance == +2)
      {
        CapeMapNode x = y->link[1];
        
        assert (x != NULL);
        
        if (x->balance == -1)
        {
          CapeMapNode w;
          
          assert (x->balance == -1);
          
          w = x->link[0];
          x->link[0] = w->link[1];
          w->link[1] = x;
          y->link[1] = w->link[0];
          w->link[0] = y;
          
          if (w->balance == +1)
          {
            x->balance = 0, y->balance = -1;
          }
          else if (w->balance == 0)
          {
            x->balance = y->balance = 0;
          }
          else /* |w->tavl_balance == -1| */
          {
            x->balance = +1, y->balance = 0;
          }

          w->balance = 0;
          
          if (w->tag[0] == CAPE_MAP_THREAD)
          {
            y->tag[1] = CAPE_MAP_THREAD;
            y->link[1] = w;
            w->tag[0] = CAPE_MAP_CHILD;
          }
          
          if (w->tag[1] == CAPE_MAP_THREAD)
          {
            x->tag[0] = CAPE_MAP_THREAD;
            x->link[0] = w;
            w->tag[1] = CAPE_MAP_CHILD;
          }
          
          q->link[dir] = w;
        }
        else
        {
          q->link[dir] = x;
          
          if (x->balance == 0)
          {
            y->link[1] = x->link[0];
            x->link[0] = y;
            x->balance = -1;
            y->balance = +1;
            
            break;
          }
          else /* |x->tavl_balance == +1| */
          {
            if (x->tag[0] == CAPE_MAP_CHILD)
            {
              y->link[1] = x->link[0];
            }
            else
            {
              y->tag[1] = CAPE_MAP_THREAD;
              x->tag[0] = CAPE_MAP_CHILD;
            }
            
            x->link[0] = y;
            y->balance = x->balance = 0;
          }
        }
      }
    }
    else
    {
      dir = q->link[0] != y;
      y->balance--;
      if (y->balance == -1)
      {
        break;
      }
      else if (y->balance == -2)
      {
        CapeMapNode x = y->link[0];
        
        assert (x != NULL);
        
        if (x->balance == +1)
        {
          CapeMapNode w;
          
          assert (x->balance == +1);
          
          w = x->link[1];
          x->link[1] = w->link[0];
          w->link[0] = x;
          y->link[0] = w->link[1];
          w->link[1] = y;
          
          if (w->balance == -1)
          {
            x->balance = 0, y->balance = +1;
          }
          else if (w->balance == 0)
          {
            x->balance = y->balance = 0;
          }
          else /* |w->tavl_balance == +1| */
          {
            x->balance = -1, y->balance = 0;
          }
          
          w->balance = 0;
          
          if (w->tag[0] == CAPE_MAP_THREAD)
          {
            x->tag[1] = CAPE_MAP_THREAD;
            x->link[1] = w;
            w->tag[0] = CAPE_MAP_CHILD;
          }
          
          if (w->tag[1] == CAPE_MAP_THREAD)
          {
            y->tag[0] = CAPE_MAP_THREAD;
            y->link[0] = w;
            w->tag[1] = CAPE_MAP_CHILD;
          }
          
          q->link[dir] = w;
        }
        else
        {
          q->link[dir] = x;
          
          if (x->balance == 0)
          {
            y->link[0] = x->link[1];
            x->link[1] = y;
            x->balance = +1;
            y->balance = -1;
            
            break;
          }
          else /* |x->tavl_balance == -1| */
          {
            if (x->tag[1] == CAPE_MAP_CHILD)
            {
              y->link[0] = x->link[1];
            }
            else
            {
              y->tag[0] = CAPE_MAP_THREAD;
              x->tag[1] = CAPE_MAP_CHILD;
            }
            
            x->link[1] = y;
            y->balance = x->balance = 0;
          }
        }
      }
    }
  }
  
  self->size--;
  return item;
}

//-----------------------------------------------------------------------------

void cape_map_erase (CapeMap self, CapeMapNode node)
{
  if (node)
  {
    CapeMapNode node2 = cape_map_extract (self, node);
    
    if (node2)
    {
      cape_map_del_node (self, &node2);
    }    
  }
}

//-----------------------------------------------------------------------------

unsigned long cape_map_size (CapeMap self)
{
  return self->size;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_first (CapeMap self)
{
  CapeMapNode ret = self->root;

  if (ret != NULL)
  {
    while (ret->tag[0] == CAPE_MAP_CHILD)
    {
      ret = ret->link[0];
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_last (CapeMap self)
{
  CapeMapNode ret = self->root;
  
  if (ret != NULL)
  {
    while (ret->tag[1] == CAPE_MAP_CHILD)
    {
      ret = ret->link[1];
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeMap cape_map_clone (CapeMap self, fct_cape_map__on_clone on_clone)
{
  // create a new object
  CapeMap clone = cape_map_new (self->cmp_fct, self->del_fct, self->cmp_ptr);
  
  CapeMapCursor cursor;
  
  cape_map_cursor_init (self, &cursor, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (&cursor))
  {
    void* key_clone = NULL;
    void* val_clone = NULL;
    
    if (on_clone)
    {
      on_clone (cursor.node->key, cursor.node->val, &key_clone, &val_clone);
    }

    if (key_clone)
    {
      // trivial copying of the node
      cape_map_insert (clone, key_clone, val_clone);
    }    
  }
  
  return clone;
}

//=============================================================================

void cape_map_cursor_init (CapeMap self, CapeMapCursor* cursor, int direction)
{
  if (direction == CAPE_DIRECTION_FORW)
  {
    cursor->node = cape_map_first (self);   
  }
  else
  {
    cursor->node = cape_map_last (self);
  }
  
  cursor->position = -1;
  cursor->direction = direction;
}

//-----------------------------------------------------------------------------

CapeMapCursor* cape_map_cursor_create (CapeMap self, int direction)
{
  CapeMapCursor* cursor = (CapeMapCursor*)malloc (sizeof(CapeMapCursor));
  
  cape_map_cursor_init (self, cursor, direction);
  
  return cursor;
}

//-----------------------------------------------------------------------------

void cape_map_cursor_destroy (CapeMapCursor** pcursor)
{
  CapeMapCursor* cursor = *pcursor;
  
  free (cursor);
  *pcursor = NULL;
}

//-----------------------------------------------------------------------------

int cape_map_cursor_nextnode (CapeMapCursor* cursor, int dir)
{
  if (cursor->position < 0)
  {
    if (dir == cursor->direction)
    {
      if (cursor->node)
      {
        cursor->position = 0;
        return TRUE;
      }
    }
  }
  else
  {
    CapeMapNode n = cursor->node;
    
    if (n)
    {
      if (dir == CAPE_DIRECTION_FORW)
      {
        cursor->node = cape_map_node_next (n);        
      }
      else
      {
        cursor->node = cape_map_node_prev (n);         
      }

      if (cursor->direction == dir)
      {
        cursor->position++;
      }
      else
      {
        cursor->position--;
      }
      
      return (cursor->node != NULL);
    }
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int cape_map_cursor_next (CapeMapCursor* cursor)
{
  return cape_map_cursor_nextnode (cursor, CAPE_DIRECTION_FORW);
}

//-----------------------------------------------------------------------------

int cape_map_cursor_prev (CapeMapCursor* cursor)
{
  return cape_map_cursor_nextnode (cursor, CAPE_DIRECTION_PREV);
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_cursor_extract (CapeMap self, CapeMapCursor* cursor)
{
  CapeMapNode x = cursor->node;
  
  if (x)
  {
    CapeMapNode ret;  

    if (cursor->direction == CAPE_DIRECTION_FORW)
    {
      cursor->node = cape_map_node_prev (x);
    }
    else
    {
      cursor->node = cape_map_node_next (x);
    }
    
    cursor->position -= 1;

    ret = cape_map_extract (self, x);
    
    if (cursor->node == NULL)
    {
      cape_map_cursor_init (self, cursor, cursor->direction);
    }

    return ret;
  }
  else
  {
    return NULL;
  }
}

//-----------------------------------------------------------------------------

void cape_map_cursor_erase (CapeMap self, CapeMapCursor* cursor)
{
  CapeMapNode node2 = cape_map_cursor_extract (self, cursor);
  
  cape_map_del_node (self, &node2);
}

//-----------------------------------------------------------------------------


