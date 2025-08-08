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
  void* key;
  void* val;
  
  CapeMapNode left;          // the left node as refernece
  CapeMapNode right;         // the right node as reference
  
  number_t height;
  char balance_factor;       // the balance factor to indicate height
  
  CapeMapNode parent;        // we need this for traversal
};

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_new (void* key, void* val, CapeMapNode parent)
{
  CapeMapNode self = CAPE_NEW (struct CapeMapNode_s);
  
  self->key = key;
  self->val = val;

  self->left = NULL;
  self->right = NULL;
  
  self->height = 0;
  self->balance_factor = 0;
  
  self->parent = parent;
  
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

    if (on_del)
    {
      on_del (self->key, self->val);
    }
        
    CAPE_DEL(p_self, struct CapeMapNode_s);
  }
}

//-----------------------------------------------------------------------------

number_t cape_map_node__height (CapeMapNode self)
{
  return (self != NULL) ? self->height : -1;
}

//-----------------------------------------------------------------------------

void cape_map_node__update_height (CapeMapNode self)
{
  self->height = cape_max_n (cape_map_node__height (self->left), cape_map_node__height (self->right)) + 1;
}

//-----------------------------------------------------------------------------

number_t cape_map_node__balance_factor (CapeMapNode self)
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
  CapeMapNode ret = NULL;

  if (self)
  {
    for (ret = self ; ret->left; ret = ret->left);
  }
    
  return ret;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node__most_right (CapeMapNode self)
{
  CapeMapNode ret = NULL;

  if (self)
  {
    for (ret = self; ret->right; ret = ret->right);
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

CapeMapNode cape_map_node__update_height_and_rebalance (CapeMapNode self)
{
  cape_map_node__update_height (self);
  
  return cape_map_node_rebalance (self);
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

//-----------------------------------------------------------------------------

CapeMapNode cape_map_node_insert (CapeMapNode self, void* key, void* val, fct_cape_map_cmp cmp_fct, void* user_ptr)
{
  CapeMapNode ret = NULL;
  CapeMapNode current_node = self;
  
  while (TRUE)
  {
    int compare_result = cmp_fct (key, current_node->key, user_ptr);
    
    if (compare_result < 0)
    {
      if (current_node->left)
      {
        // follow left subtree
        current_node = current_node->left;
      }
      else
      {
        // create a new node
        current_node->left = cape_map_node_new (key, val, current_node);
        
        ret = cape_map_node__update_height_and_rebalance (current_node->left);
        break;
      }
    }
    else if (compare_result > 0)
    {
      if (current_node->right)
      {
        // follow right subtree
        current_node = current_node->right;
      }
      else
      {
        current_node->right = cape_map_node_new (key, val, current_node);

        ret = cape_map_node__update_height_and_rebalance (current_node->right);
        break;
      }
    }
    else
    {
      // is the compare function known as string
      if (cmp_fct == cape_map__compare__s)
      {
        cape_log_fmt (CAPE_LL_WARN, "CAPE", "map insert", "key already exists '%s'", (char*)key);
      }
      else
      {
        cape_log_msg (CAPE_LL_WARN, "CAPE", "map insert", "key already exists");
      }

      // key already exists
      break;
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void cape_map_node__swap (CapeMapNode self, CapeMapNode repl)
{
  CapeMapNode self_left = self->left;
  CapeMapNode self_right = self->right;
  CapeMapNode self_parent = self->parent;

  CapeMapNode repl_left = self->left;
  CapeMapNode repl_right = self->right;
  CapeMapNode repl_parent = self->parent;

  if (self_parent)
  {
    if (self_parent->left == self)
    {
      self_parent->left = repl;
    }
    else
    {
      self_parent->right = repl;
    }
  }

  if (repl_parent)
  {
    if (repl_parent->left == repl)
    {
      repl_parent->left = self;
    }
    else
    {
      repl_parent->right = self;
    }
  }

  if (self_left)
  {
    // parent must exists
    self_left->parent = repl;
  }
  
  if (repl_left)
  {
    // parent must exists
    repl_left->parent = self;
  }

  if (self_right)
  {
    // parent must exists
    self_right->parent = repl;
  }

  if (repl_right)
  {
    // parent must exists
    repl_right->parent = self;
  }

  self->left = repl_left == self ? repl : repl_left;
  self->right = repl_right == self ? repl : repl_right;

  repl->left = self_left == repl ? self : self_left;
  repl->right = self_right == repl ? self : self_right;
}

//-----------------------------------------------------------------------------

void cape_map_node_delete__one_child (CapeMapNode* p_self, CapeMapNode child, CapeMapNode* p_root)
{
  CapeMapNode self = *p_self;
  
  CapeMapNode parent = self->parent;
  
  if (parent)
  {
    if (child)
    {
      child->parent = parent;
    }

    if (parent->left == self)
    {
      parent->left = child;
    }
    else
    {
      parent->right = child;
    }
  }
  else  // root node
  {
    if (child)
    {
      child->parent = NULL;
    }
    
    if (p_root)
    {
      *p_root = child;
    }
  }
}

//-----------------------------------------------------------------------------

void cape_map_node_delete__two_children (CapeMapNode* p_self, CapeMapNode* p_root)
{
  CapeMapNode self = *p_self;
  
  CapeMapNode successor = cape_map_node__most_left (self->right);

  // exchange data
  void* data = self->val;
  self->val = successor->val;
  successor->val = data;

  void* key = self->key;
  self->key = successor->key;
  successor->key = key;

  // transwer ownership
  *p_self = successor;

  cape_map_node_delete (p_self, p_root);
}

//-----------------------------------------------------------------------------

void cape_map_node_delete (CapeMapNode* p_self, CapeMapNode* p_root)
{
  CapeMapNode self = *p_self;
  
  if (self->left)
  {
    if (self->right)
    {
      cape_map_node_delete__two_children (p_self, p_root);
    }
    else
    {
      cape_map_node_delete__one_child (p_self, self->left, p_root);
      
      // clean left
      self->left = NULL;
    }
  }
  else
  {
    if (self->right)
    {
      cape_map_node_delete__one_child (p_self, self->right, p_root);
      
      // clean right
      self->right = NULL;
    }
    else
    {
      cape_map_node_delete__one_child (p_self, NULL, p_root);
    }
  }
}

//-----------------------------------------------------------------------------

number_t cape_map_node__max_height (CapeMapNode self)
{
  if (self)
  {
    return 1 + cape_max_n (cape_map_node__max_height (self->left), cape_map_node__max_height (self->right));
  }
  else
  {
    return 0;
  }
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

void cape_map_del_node (CapeMap self, CapeMapNode* p_node)
{
  cape_map_node_del (p_node, self->del_fct);
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
  CapeMapNode ret;
  
  if (self->root)
  {
    ret = cape_map_node_insert (self->root, key, val, self->cmp_fct, self->cmp_ptr);
    
    self->size++;
  }
  else
  {
    self->root = cape_map_node_new (key, val, NULL);

    ret = self->root;
    
    self->size = 1;
  }

  return ret;
}

//-----------------------------------------------------------------------------

CapeMapNode cape_map_extract (CapeMap self, CapeMapNode node)
{
  CapeMapNode ret = node;
  
  cape_map_node_delete (&ret, &(self->root));
  
  return ret;
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

number_t cape_map_size (CapeMap self)
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

number_t cape_map_max_height (CapeMap self)
{
  return cape_map_node__max_height (self->root);
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


