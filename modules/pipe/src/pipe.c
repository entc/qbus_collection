#include "qbus.h"

// cape includes
#include <aio/cape_aio_ctx.h>
#include <aio/cape_aio_file.h>
#include <sys/cape_err.h>
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include <sys/cape_mutex.h>
#include <fmt/cape_tokenizer.h>
#include <fmt/cape_json.h>
#include <sys/cape_pipe.h>

//-------------------------------------------------------------------------------------

void __STDCALL qbus_pipe__on_read (void* ptr, CapeAioFileReader freader, const char* bufdat, number_t buflen)
{
  printf ("PIPE: %s\n", bufdat);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_pipe_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  int res;

  const CapeString path = qbus_config_s (qbus, "path", "/tmp");
  const CapeString name = qbus_config_s (qbus, "name", "qmod_pipe");
  
  // this should always work
  void* handle = cape_pipe_create_or_connect (path, name, err);
  if (handle == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  {
    CapeAioFileReader file_reader = cape_aio_freader_new (handle, NULL, qbus_pipe__on_read);

    if (!cape_aio_freader_add (&file_reader, qbus_aio (qbus)))
    {
      
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "PIPE", "init", "can't initialize: %s", cape_err_text (err));
  }
  
  return res;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_pipe_done (QBus qbus, void* ptr, CapeErr err)
{
  return CAPE_ERR_NONE;
}

//-------------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("PIPE", NULL, qbus_pipe_init, qbus_pipe_done, argc, argv);
  return 0;
}

//-------------------------------------------------------------------------------------
