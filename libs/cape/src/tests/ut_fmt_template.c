#include "fmt/cape_template.h"

// c includes
#include <stdio.h>

int main (int argc, char *argv[])
{
  int res, ret;
  CapeErr err = cape_err_new ();

  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  cape_udc_add_s_cp (values, "val1_str", "1");
  cape_udc_add_s_cp (values, "val2_str", "2");

  res = cape_eval_b ("{{val1_str}} = 1 AND {{val2_str}} = 1 OR {{val2_str}} = 2", values, &ret, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  printf ("T1: %i\n", ret);

  
exit_and_cleanup:

  cape_udc_del (&values);
  
  cape_err_del (&err);
  return 0;
}

