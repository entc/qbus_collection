#include "qwave.h"

#include "fmt/cape_args.h"

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;

  // local objects
  CapeErr err = cape_err_new ();
  CapeUdc parameters = cape_args_from_args (argc, argv, NULL);
  QWave qwave_instance = qwave_new (cape_udc_get_s (parameters, "h", "127.0.0.1"), cape_udc_get_n (parameters, "p", 8000), parameters);

  res = qwave_run__d (qwave_instance, err);
  if (res)
  {
    goto cleanup_and_exit;
  }

  
  {
    int i;
    
    printf ("Program aborts with any key\n");
    
    scanf("%d", &i);    

    printf ("Program starts shutdown process\n");
  }
  
  res = CAPE_ERR_NONE;
  
cleanup_and_exit:
  
  qwave_del (&qwave_instance);
  cape_udc_del (&parameters);
  cape_err_del (&err);

  return res;
}
