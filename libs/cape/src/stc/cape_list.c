#include "cape_list.h"

// cape includes
#include "sys/cape_types.h"

//-----------------------------------------------------------------------------

struct CapeListNode_s
{
  
  void* data;
  
  CapeListNode next;
  
  CapeListNode prev;
};

//-----------------------------------------------------------------------------

struct CapeList_s
{
  
  fct_cape_list_onDestroy onDestroy;
  
  CapeListNode fpos;   // first position
  
  CapeListNode lpos;   // last position (for fast add)
  
  unsigned long size;
  
};

//-----------------------------------------------------------------------------

CapeList cape_list_new (fct_cape_list_onDestroy onDestroy)
{
  CapeList self = CAPE_NEW(struct CapeList_s);
  
  self->onDestroy = onDestroy;
  self->fpos = NULL;
  self->lpos = NULL;
  
  self->size = 0;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_list_clr (CapeList self)
{
  CapeListCursor cursor;
  
  cape_list_cursor_init (self, &cursor, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (&cursor))
  {
    cape_list_cursor_erase (self, &cursor);
  }
}

//-----------------------------------------------------------------------------

void cape_list_del (CapeList* p_self)
{
  if (*p_self)
  {
    CapeList self = *p_self;
    
    cape_list_clr (self);	
    
    CAPE_DEL(p_self, struct CapeList_s);
  }
}

//-----------------------------------------------------------------------------

CapeListNode cape_list_push_back (CapeList self, void* data)
{
  CapeListNode node = CAPE_NEW(struct CapeListNode_s);
  
  node->data = data;
  
  node->next = NULL; // last element
  node->prev = self->lpos;
  
  // assign next node at last element
  if (self->lpos)
  {
    self->lpos->next = node;
  }
  
  // set new last element
  self->lpos = node;
  
  // if we don't have a first element, set it now
  if (self->fpos == NULL)
  {
    self->fpos = node;
  }
  
  self->size++;
  
  return node;
}

//-----------------------------------------------------------------------------

CapeListNode cape_list_push_front (CapeList self, void* data)
{
  CapeListNode node = CAPE_NEW(struct CapeListNode_s);
  
  node->data = data;
  
  node->next = self->fpos;
  node->prev = NULL;  // first element
  
  // assign next node at first element
  if (self->fpos)
  {
    self->fpos->prev = node;
  }
  
  // set new last element
  self->fpos = node;
  
  // if we don't have a last element, set it now
  if (self->lpos == NULL)
  {
    self->lpos = node;
  }
  
  self->size++;
  
  return node;
}

//-----------------------------------------------------------------------------

unsigned long cape_list_size (CapeList self)
{
  return self->size;
}

//-----------------------------------------------------------------------------

int cape_list_empty (CapeList self)
{
  return self->fpos == NULL;
}

//-----------------------------------------------------------------------------

int cape_list_hasContent (CapeList self)
{
  return self->fpos != NULL;
}

//-----------------------------------------------------------------------------

void* cape_list_position (CapeList self, int position)
{
  void* ret = NULL;
  
  CapeListCursor* cursor = cape_list_cursor_create (self, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor) && (ret == NULL))
  {
    if (cursor->position == position)
    {
      ret = cape_list_node_data (cursor->node);
    }
  }
  
  cape_list_cursor_destroy (&cursor);
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeListNode cape_list_push_max (CapeList self, void* data, number_t max_size)
{
  CapeListNode node = cape_list_push_back (self, data);
  
  if (max_size > 0)
  {
    while (self->size > max_size)
    {
      void* data_to_delete = cape_list_pop_front (self);
      
      if (data_to_delete && self->onDestroy)
      {
        self->onDestroy (data_to_delete);
      }
    }
  }
    
  return node;
}

//-----------------------------------------------------------------------------

void* cape_list_node_data (CapeListNode node)
{
  return node->data;
}

//-----------------------------------------------------------------------------

void* cape_list_node_extract (CapeList self, CapeListNode node)
{
  void* ret = NULL;
  
  if (node)
  {
    CapeListNode prev = node->prev;
    CapeListNode next = node->next;
    
    if (prev)
    {
      prev->next = next;
    }
    else
    {
      // this was the first node
      self->fpos = next;
    }
    
    if (next)
    {
      next->prev = prev;
    }
    else
    {
      // this was the last node
      self->lpos = prev;
    }
    
    ret = node->data;
    
    CAPE_DEL(&node, struct CapeListNode_s);
    
    if (self->size > 0)
    {
      self->size--;
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void cape_list_node_erase (CapeList self, CapeListNode node)
{
  void* data = cape_list_node_extract (self, node);
  
  if (data && self->onDestroy)
  {
    self->onDestroy (data);
  }
}

//-----------------------------------------------------------------------------

CapeListNode cape_list_noed_next (CapeListNode node)
{
  if (node)
  {
    return node->next;
  }
  else
  {
    return NULL;
  }
}

//-----------------------------------------------------------------------------

CapeListNode cape_list_node_front (CapeList self)
{
  return self->fpos;
}

//-----------------------------------------------------------------------------

CapeListNode cape_list_node_back (CapeList self)
{
  return self->lpos;
}

//-----------------------------------------------------------------------------

void cape_list_node_swap (CapeListNode node1, CapeListNode node2)
{
  if (node1 && node2)
  {
    void* h = node1->data;
    
    node1->data = node2->data;
    node2->data = h;
  }
}

//-----------------------------------------------------------------------------

CapeList cape_list_slice_extract (CapeList self, CapeListNode nodeFrom, CapeListNode nodeTo)
{
  CapeListNode prev = NULL;
  CapeListNode next = NULL;
  
  CapeList slice = CAPE_NEW(struct CapeList_s);
  
  slice->onDestroy = self->onDestroy;
  slice->fpos = NULL;
  slice->lpos = NULL;
  
  slice->size = 0;
  
  if (nodeFrom)
  {
    prev = nodeFrom->prev;
    nodeFrom->prev = NULL;      // terminate
    
    slice->fpos = nodeFrom;
  }
  else
  {
    slice->fpos = self->fpos;   // use first node
  }
  
  if (nodeTo)
  {
    next = nodeTo->next;
    nodeTo->next = NULL;        // terminate
    
    slice->lpos = nodeTo;
  }
  else
  {
    slice->lpos = self->lpos;   // use last node
  }
  
  // count elements
  if ((slice->fpos == self->fpos)&&(slice->lpos == self->lpos))
  {
    slice->size = self->size;
  }
  else
  {
    CapeListCursor cursor;
    cape_list_cursor_init (slice, &cursor, CAPE_DIRECTION_FORW);
    
    // iterate to find the correct size
    while (cape_list_cursor_next (&cursor))
    {
      slice->size++;
    }
  }
  
  self->size = self->size - slice->size;
  
  // cut out
  if (prev)
  {
    prev->next = next;
  }
  else
  {
    self->fpos = next;
  }
  
  if (next)
  {
    next->prev = prev;
  }
  else
  {
    self->lpos = prev;
  }
  
  return slice;
}

//-----------------------------------------------------------------------------

void cape_list_link_prev (CapeList self, CapeListNode node, CapeListNode prev)
{
  if (prev)
  {
    if (node)
    {
      node->prev = prev;
    }
    else
    {
      self->lpos = prev;
    }
    
    prev->next = node;
  }
  else
  {
    if (node)
    {
      node->prev = NULL;
    }
    else
    {
      // this should never happen
    }
    
    self->fpos = node;
  }
}

//-----------------------------------------------------------------------------

void cape_list_link_next (CapeList self, CapeListNode node, CapeListNode next)
{
  if (next)
  {
    if (node)
    {
      node->next = next;
    }
    else
    {
      self->fpos = next;
    }
    
    next->prev = node;
  }
  else
  {
    if (node)
    {
      node->next = NULL;
    }
    else
    {
      // this should never happen
    }
    
    self->lpos = node;
  }
}

//-----------------------------------------------------------------------------

void cape_list_sort (CapeList self, fct_cape_list_onCompare onCompare)
{
  CapeListNode list = self->fpos;
  CapeListNode tail;
  CapeListNode p;
  CapeListNode q;
  CapeListNode e;
  
  int insize = 1;
  int psize, qsize;
  int nmerges;
  
  // do some prechecks
  if (onCompare == NULL)
  {
    return;
  }
  
  if (list == NULL)
  {
    return;
  }
  
  while (1)
  {
    p = list;
    
    list = NULL;
    tail = NULL;
    
    nmerges = 0;
    
    while (p)
    {
      nmerges++;
      
      q = p;
      psize = 0;
      
      {
        int i;
        for (i = 0; i < insize; i++)
        {
          psize++;
          q = q->next;
          
          if (!q) break;
        }
      }
      
      qsize = insize;
      
      while (psize > 0 || (qsize > 0 && q))
      {
        if (psize == 0)
        {
          e = q;
          q = q->next;
          qsize--;
        }
        else if (qsize == 0 || !q)
        {
          e = p;
          p = p->next;
          psize--;
        }
        else if (onCompare(p->data, q->data) <= 0)
        {
          e = p;
          p = p->next;
          psize--;
        }
        else
        {
          e = q;
          q = q->next;
          qsize--;
        }
        
        if (tail)
        {
          tail->next = e;
        }
        else
        {
          list = e;
        }
        
        e->prev = tail;
        tail = e;
      }
      
      p = q;
    }
    
    tail->next = NULL;
    
    if (nmerges <= 1)
    {
      self->fpos = list;
      self->lpos = tail;
      return;
    }
    
    insize *= 2;
  }
}

//-----------------------------------------------------------------------------

CapeList cape_list_clone (CapeList orig, fct_cape_list_onClone onClone)
{
  CapeList self = CAPE_NEW(struct CapeList_s);
  
  self->onDestroy = orig->onDestroy;
  self->fpos = NULL;
  self->lpos = NULL;
  
  self->size = 0;
  
  {
    CapeListCursor cursor;
    cape_list_cursor_init (orig, &cursor, CAPE_DIRECTION_FORW);
    
    // iterate to find the correct size
    while (cape_list_cursor_next (&cursor))
    {
      void* data = NULL;
      
      if (onClone)  // if not, the value will be null
      {
        data = onClone (cape_list_node_data (cursor.node));
      }
      
      cape_list_push_back(self, data);
    }
  }

  return self;
}

//-----------------------------------------------------------------------------

void cape_list_node_replace (CapeList self, CapeListNode node, void* data)
{
  if (node)
  {
    if (self->onDestroy)
    {
      self->onDestroy (node->data);
    }
    
    node->data = data;
  }
}

//-----------------------------------------------------------------------------

CapeListCursor* cape_list_cursor_new (CapeList self, int direction)
{
  CapeListCursor* cursor = CAPE_NEW(CapeListCursor);
  
  cape_list_cursor_init (self, cursor, direction);
  
  return cursor;
}

//-----------------------------------------------------------------------------

void cape_list_cursor_del (CapeListCursor** p_cursor)
{
  if (*p_cursor)
  {
    CAPE_DEL (p_cursor, CapeListCursor);
  }
}

//-----------------------------------------------------------------------------

void cape_list_cursor_init (CapeList self, CapeListCursor* cursor, int direction)
{
  if (direction == CAPE_DIRECTION_FORW)
  {
    cursor->node = self->fpos;
  }
  else
  {
    cursor->node = self->lpos;
  }
  
  cursor->position = -1;
  cursor->direction = direction;
}

//-----------------------------------------------------------------------------

int cape_list_cursor_next (CapeListCursor* cursor)
{
  if (cursor->position < 0)
  {
    cursor->position = 0;
    return cursor->node != NULL;
  }
  else
  {
    if (cursor->node)
    {
      // advance position
      cursor->position++;
      
      // grap next node
      cursor->node = cursor->node->next;
      return cursor->node != NULL;
    }
    else
    {
      return 0;
    }
  }
}

//-----------------------------------------------------------------------------

int cape_list_cursor_prev (CapeListCursor* cursor)
{
  if (cursor->position < 0)
  {
    cursor->position = 0;
    return cursor->node != NULL;
  }
  else
  {
    if (cursor->node)
    {
      // advance position
      cursor->position++;
      
      // grap next node
      cursor->node = cursor->node->prev;
      return cursor->node != NULL;
    }
    else
    {
      return 0;
    }
  }
}

//-----------------------------------------------------------------------------

void* cape_list_cursor_extract (CapeList self, CapeListCursor* cursor)
{
  CapeListNode node = cursor->node;
  void* ret = NULL;
  
  if (node)
  {
    CapeListNode prev = node->prev;
    CapeListNode next = node->next;
    
    if (prev)
    {
      prev->next = next;
      
      if (cursor->direction == CAPE_DIRECTION_FORW)
      {
        // adjust the cursor
        cursor->node = prev;
      }
    }
    else
    {
      // this was the first node
      self->fpos = next;
      
      if (cursor->direction == CAPE_DIRECTION_FORW)
      {
        // reset cursor
        cape_list_cursor_init (self, cursor, cursor->direction);
      }
    }
    
    if (next)
    {
      next->prev = prev;
      
      if (cursor->direction == CAPE_DIRECTION_PREV)
      {
        // adjust the cursor
        cursor->node = next;
      }
    }
    else
    {
      // this was the last node
      self->lpos = prev;

      if (cursor->direction == CAPE_DIRECTION_PREV)
      {
        // reset cursor
        cape_list_cursor_init (self, cursor, cursor->direction);
      }
    }
    
    ret = node->data;
    
    CAPE_DEL(&node, struct CapeListNode_s);
    
    if (self->size > 0)
    {
      self->size--;
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void cape_list_cursor_erase (CapeList self, CapeListCursor* cursor)
{
  void* data = cape_list_cursor_extract (self, cursor);
  
  if (data && self->onDestroy)
  {
    self->onDestroy (data);
  }
}

//-----------------------------------------------------------------------------

void cape_list_insert_slice_flpos (CapeList self, CapeListCursor* cursor, CapeListNode fpos, CapeListNode lpos)
{
  CapeListNode node = cursor->node;
  
  if (cursor->direction == CAPE_DIRECTION_PREV)
  {
    // relink fpos
    cape_list_link_prev (self, fpos, node ? node->prev : self->lpos); // maybe self->lpos is reset
    
    // relink lpos
    cape_list_link_prev (self, node, lpos);   // lpos is always set
  }
  else
  {
    // relink lpos
    cape_list_link_next (self, lpos, node ? node->next : self->fpos); // maybe self->fpos is reset
    
    // relink fpos
    cape_list_link_next (self, node, fpos);   // fpos is always set
  }
}

//-----------------------------------------------------------------------------

void cape_list_slice_insert (CapeList self, CapeListCursor* cursor, CapeList* pslice)
{
  CapeList slice = *pslice;
  if (slice)
  {
    CapeListNode fpos = slice->fpos;
    CapeListNode lpos = slice->lpos;
    
    if (fpos && lpos)
    {
      cape_list_insert_slice_flpos (self, cursor, fpos, lpos);
    }
    
    self->size += slice->size;
    
    // clean
    slice->fpos = NULL;
    slice->lpos = NULL;
    
    cape_list_del (pslice);
  }
}

//-----------------------------------------------------------------------------

void* cape_list_pop_front (CapeList self)
{
  void* data = NULL;
  
  if (self->fpos)
  {
    CapeListNode node = self->fpos;
    CapeListNode next = node->next;
    
    if (next)
    {
      next->prev = NULL;
      self->fpos = next;
    }
    else
    {
      // this was the last node
      self->lpos = NULL;
      self->fpos = NULL;
    }
    
    data = node->data;
    
    CAPE_DEL(&node, struct CapeListNode_s);
    
    if (self->size > 0)
    {
      self->size--;
    }
  }
  
  return data;
}

//-----------------------------------------------------------------------------

void* cape_list_pop_back (CapeList self)
{
  void* data = NULL;
  
  if (self->lpos)
  {
    CapeListNode node = self->lpos;
    CapeListNode prev = node->prev;
    
    if (prev)
    {
      prev->next = NULL;
      self->lpos = prev;
    }
    else
    {
      // this was the last node
      self->lpos = NULL;
      self->fpos = NULL;
    }
    
    data = node->data;
    
    CAPE_DEL(&node, struct CapeListNode_s);
    
    if (self->size > 0)
    {
      self->size--;
    }
  }
  
  return data;
}

//-----------------------------------------------------------------------------
