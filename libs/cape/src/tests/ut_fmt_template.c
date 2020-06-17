#include "fmt/cape_template.h"

// c includes
#include <stdio.h>

int main (int argc, char *argv[])
{
  int res, ret;
  CapeErr err = cape_err_new ();

  CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);

  cape_udc_add_s_cp (values, "val_str1", "1");
  cape_udc_add_s_cp (values, "val_str2", "2");

  cape_udc_add_s_cp (values, "val_float1", "1.1");
  cape_udc_add_s_cp (values, "val_float2", "1,1");
  cape_udc_add_s_cp (values, "val_float3", "1,134.32");
  cape_udc_add_s_cp (values, "val_float4", "1.134,32");

  {
    res = cape_eval_b ("{{val_str1}} = 1 AND {{val_str2}} = 1 OR {{val_str2}} = 2", values, &ret, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T1: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_float1|decimal:10%,%2}} = 0,11", values, &ret, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T2: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_float2|decimal:10%,%2}} = 0,11", values, &ret, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T2: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_float3|decimal:10%,%3}} = 113,432", values, &ret, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T2: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_float4|decimal:10%,%3}} = 113,432", values, &ret, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T2: %i\n", ret);
  }

  {
    CapeUdc n = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n (n, "val", 123);
    
    CapeString h = cape_template_run ("{{$math{val - 0.2}|decimal:1%,%2}}", n, err);

    if (h)
    {
      printf ("MATH: %s\n", h);
    }
    else
    {
      printf ("ERR %s\n", cape_err_text(err));
    }

    cape_str_del (&h);
    cape_udc_del (&n);
  }

exit_and_cleanup:

  if (cape_err_code(err))
  {
    printf ("ERROR: %s\n", cape_err_text(err));
  }
  
  cape_udc_del (&values);
  
  cape_err_del (&err);
  return 0;
}

