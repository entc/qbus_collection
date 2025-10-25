#include "qwave.h"

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;

  // local objects
  CapeErr err = cape_err_new ();
  CapeErr parameters = cape_args_from_args (argc, argv, NULL);
  QWave qwave_instance = qwave_new (cape_udc_get_s (parameters, "h", "127.0.0.1"), cape_udc_get_n (parameters, "p", 8000), parameters);

  res = qwave_run (qwave_instance, err);

  qwave_del (&qwave_instance);
  cape_udc_del (&parameters);
  cape_err_del (&err);

  return res;
}
