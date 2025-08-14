#include "cape_aio_file.h"

#if defined __BSD_OS || defined __LINUX_OS

#include <memory.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#elif defined __WINDOWS_OS

#include <Windows.h>

#endif

//-----------------------------------------------------------------------------

struct CapeAioFileReader_s
{
  
    void* handle;
    
    CapeAioHandle aioh;
    
    char* bufdat;
    
    number_t buflen;
    
    void* ptr;
    
    fct_cape_aio_file__on_read on_read;
};

//-----------------------------------------------------------------------------

CapeAioFileReader cape_aio_freader_new (void* handle, void* ptr, fct_cape_aio_file__on_read on_read)
{
  CapeAioFileReader self = CAPE_NEW(struct CapeAioFileReader_s);
  
  self->handle = handle;
  self->aioh = NULL;
  
  self->bufdat = CAPE_ALLOC(1024);
  self->buflen = 1024;
  
  self->ptr = ptr;
  self->on_read = on_read;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_freader_del (CapeAioFileReader* p_self)
{
  CapeAioFileReader self = *p_self;
  
  // delete the AIO handle
  cape_aio_handle_del (&(self->aioh));
  
#if defined __BSD_OS || defined __LINUX_OS

  // close the file handle
  close ((int)((number_t)(self->handle)));
 
#elif defined __WINDOWS_OS


#endif

  CAPE_FREE(self->bufdat);
  
  CAPE_DEL(p_self, struct CapeAioFileReader_s);
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_freader_onEvent (void* ptr, int hflags, unsigned long events, unsigned long param1)
{
  CapeAioFileReader self = ptr;
  
#if defined __BSD_OS || defined __LINUX_OS

  number_t bytes_read = read ((int)((number_t)self->handle), self->bufdat, self->buflen);

  if (bytes_read > 0)
  {
    if (self->on_read)
    {
      self->on_read (self->ptr, self, self->bufdat, bytes_read);
    }
  }

#elif defined __WINDOWS_OS


#endif

  return hflags;
} 

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_freader_onDestroy (void* ptr, CapeAioHandle aioh, int force_close)
{
  CapeAioFileReader self = ptr;
  
  cape_aio_freader_del (&self);
}

//-----------------------------------------------------------------------------

int cape_aio_freader_add (CapeAioFileReader* p_self, CapeAioContext aio)
{
  CapeAioFileReader self = *p_self;
  
  *p_self = NULL;
    
  self->aioh = cape_aio_handle_new (CAPE_AIO_READ, self, cape_aio_freader_onEvent, cape_aio_freader_onDestroy);
  
  return cape_aio_context_add (aio, self->aioh, self->handle, 0);   
}

//-----------------------------------------------------------------------------
