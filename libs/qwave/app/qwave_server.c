#include "qwave.h"

#include "fmt/cape_args.h"

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    int res;

    // convert arguments
    CapeUdc parameters = cape_args_from_args (argc, argv, NULL);

    // local objects
    CapeErr err = cape_err_new ();
    QWave qwave_instance = qwave_new (&parameters);
    CapeAio aio_main = cape_aio_new();

    // initialize main AIO event handler
    res = cape_aio_init (aio_main, err);
    if (res)
    {
      goto cleanup_and_exit;
    }

    // run the qwave server in background
    res = qwave_run__d (qwave_instance, err);
    if (res)
    {
      goto cleanup_and_exit;
    }

    res = cape_aio_wait (aio_main, err);
  
cleanup_and_exit:
  
    qwave_del (&qwave_instance);
    cape_aio_del (&aio_main);
    cape_err_del (&err);

    return res;
}
