#include "cape_buf.h"

//-----------------------------------------------------------------------------

struct CapeBuffer_s
{
    number_t size;
    
    char* data;
};

//-----------------------------------------------------------------------------

CapeBuffer cape_buf_new (number_t size)
{
  CapeBuffer self = CAPE_NEW(struct CapeBuffer_s);
  
  self->size = size;
  self->data = CAPE_ALLOC (size + 1);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_buf_del (CapeBuffer* p_self)
{
  CapeBuffer self = *p_self;

  // release memory  
  CAPE_FREE(self->data);
  
  CAPE_DEL(p_self, struct CapeBuffer_s);
}

//-----------------------------------------------------------------------------

CapeBuffer cape_buf_reg (char* data, number_t size)
{
  
}

//-----------------------------------------------------------------------------

number_t cape_buf_size (CapeBuffer self)
{
  return self->size;
}

//-----------------------------------------------------------------------------

const char* cape_buf_data (CapeBuffer self)
{
  return self->data;
}

//-----------------------------------------------------------------------------
