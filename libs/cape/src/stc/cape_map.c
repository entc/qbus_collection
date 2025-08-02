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

inline number_t cape_max_n (number_t x, number_t y)
{
  return (x > y) ? x : y;
}

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
  void* key;
  void* val;
  
  CapeMapNode left;          // the left node as refernece
  CapeMapNode right;         // the right node as reference
  
  number_t height;
  
  CapeMapNode parent;        // we need this for traversal
};

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_new (void* key, void* val)
{
  CapeMapNode self = CAPE_NEW (struct CapeMapNode_s);
  
  self->key = key;
  self->val = val;

  self->left = NULL;
  self->right = NULL;
  
  self->height = 0;
  
  self->parent = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_map_node_del (CapeMapNode* p_self, fct_cape_map_destroy on_del)
{
  if (*p_self)
  {
    CapeMapNode self = *p_self;
    
    // delete all nodes from the left
    if (self->left)
    {
      cape_map_node_del (&(self->left), on_del);
    }

    // delete all nodes from the right
    if (self->right)
    {
      cape_map_node_del (&(self->right), on_del);
    }

    // correct parent assignments
    if (self->parent)
    {
      if (self->parent->left == self)
      {
        self->parent->left = NULL;
      }
      else if (self->parent->right == self)
      {
        self->parent->right = NULL;
      }
    }

    if (on_del)
    {
      on_del (self->key, self->val);
    }
        
    CAPE_DEL(p_self, struct CapeMapNode_s);
  }
}

//-----------------------------------------------------------------------------

inline number_t cape_map_node__height (CapeMapNode self)
{
  return (self != NULL) ? self->height : -1;
}

//-----------------------------------------------------------------------------

inline void cape_map_node__update_height (CapeMapNode self)
{
  self->height = cape_max_n (cape_map_node__height (self->left), cape_map_node__height (self->right)) + 1;
}

//-----------------------------------------------------------------------------

inline number_t cape_map_node__balance_factor (CapeMapNode self)
{
  return cape_map_node__height (self->right) - cape_map_node__height (self->left);
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node__rotate_right (CapeMapNode self)
{
  CapeMapNode child_left = self->left;

  if (child_left)
  {
    child_left->parent = self->parent;

    self->left = child_left->right;

    if (self->left)
    {
      self->left->parent = self;
    }

    child_left->right = self;
    
    self->parent = child_left;
    
    cape_map_node__update_height (self);
    cape_map_node__update_height (child_left);
  }
  
  return child_left;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node__rotate_left (CapeMapNode self)
{
  CapeMapNode child_right = self->right;

  if (child_right)
  {
    child_right->parent = self->parent;

    self->right = child_right->left;

    if (self->right)
    {
      self->right->parent = self;
    }
    
    child_right->left = self;
    
    self->parent = child_right;

    cape_map_node__update_height (self);
    cape_map_node__update_height (child_right);
  }
  
  return child_right;  
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node__most_left (CapeMapNode self)
{
  CapeMapNode ret = self;
  
  while (ret)
  {
    ret = ret->left;
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node__most_right (CapeMapNode self)
{
  CapeMapNode ret = self;
  
  while (ret)
  {
    ret = ret->right;
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_rebalance (CapeMapNode self)
{
  CapeMapNode ret = NULL;
  
  number_t balance_factor = cape_map_node__balance_factor (self);
  
  // left heavy
  if (balance_factor < -1)
  {
    if (cape_map_node__balance_factor (self->left) <= 0)
    {
      ret = cape_map_node__rotate_right (self);
    }
    else
    {
      self->left = cape_map_node__rotate_left (self->left);
      ret = cape_map_node__rotate_right (self);
    }    
  }
  // right heavy
  else if (balance_factor > 1)
  {
    if (cape_map_node__balance_factor (self->right) >= 0)
    {
      ret = cape_map_node__rotate_left (self);
    }
    else
    {
      self->right = cape_map_node__rotate_right (self->right);
      ret = cape_map_node__rotate_left (self);
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_find (const CapeMapNode self, const void* key, fct_cape_map_cmp on_cmp, void* user_ptr)
{
  CapeMapNode ret = self;
  int cmp;
  
  while (ret)
  {
    cmp = on_cmp (key, ret->key, user_ptr);
    
    if (cmp == 0)
    {
      break;
    }
    else if (cmp < 0)
    {
      ret = ret->left;
    }
    else
    {
      ret = ret->right;
    }
  }
  
  return ret;
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

CapeMapNode cape_map_node_next (CapeMapNode self)
{
  CapeMapNode ret = NULL;
  
  if (self->right)
  {
    ret = cape_map_node__most_left (self->right);
  }
  else
  {
    CapeMapNode last;
    
    for (last = self; last->parent; last = last->parent)
    {
      if (last->parent->left == last)
      {
        ret = last->parent;
        break;
      }
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_prev (CapeMapNode self)
{
  CapeMapNode ret = NULL;

  if (self->left)
  {
    ret = cape_map_node__most_right (self->left);
  }
  else
  {
    CapeMapNode last;
    
    for (last = self; last->parent; last = last->parent)
    {
      if (last->parent->right == last)
      {
        ret = last->parent;
        break;
      }
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

void cape_map_del_node (CapeMap self, CapeMapNode* pself)
{
  CapeMapNode n = *pself;
  
  cape_map_node_del (pself, self->del_fct);
}

//-----------------------------------------------------------------------------

void cape_map_clr (CapeMap self)
{  
  // this will clear all nodes
  cape_map_node_del (&(self->root), self->del_fct);
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_find (CapeMap self, const void* key)
{
  return  cape_map_node_find (self->root, key, self->cmp_fct, self->cmp_ptr);
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_find_cmd (CapeMap self, const void* key, fct_cape_map_cmp on_cmp)
{
  return  cape_map_node_find (self->root, key, on_cmp, self->cmp_ptr);
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
  return cape_map_node__most_left (self->root);
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_last (CapeMap self)
{
  return cape_map_node__most_right (self->root);
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

CapeMapCursor* cape_map_cursor_new (CapeMap self, int direction)
{
  CapeMapCursor* cursor = (CapeMapCursor*)malloc (sizeof(CapeMapCursor));
  
  cape_map_cursor_init (self, cursor, direction);
  
  return cursor;
}

//-----------------------------------------------------------------------------

void cape_map_cursor_del (CapeMapCursor** pcursor)
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


