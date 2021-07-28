#include "fmt/cape_template.h"

// c includes
#include <stdio.h>

//-----------------------------------------------------------------------------

static char* __STDCALL main__on_pipe (const char* name, const char* pipe, const char* value)
{
  //printf ("ON PIPE: name = '%s', pipe = '%s', value = '%s'\n", pipe, value);
  
  return cape_str_cp ("13");
}

//-----------------------------------------------------------------------------

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
  cape_udc_add_s_cp (values, "val_float5", "10.35555");
  cape_udc_add_s_cp (values, "val_float6", "10.35444");

  cape_udc_add_b (values, "val_bool_true", TRUE);
  cape_udc_add_b (values, "val_bool_false", FALSE);

  {
    res = cape_eval_b ("{{val_str1}} = 1 AND {{val_str2}} = 1 OR {{val_str2}} = 2", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T1: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_float1|decimal:10%,%2}} = 0,11", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T2: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_float2|decimal:10%,%2}} = 0,11", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T3: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_float3|decimal:10%,%3}} = 113,432", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T4: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_float4|decimal:10%,%3}} = 113,432", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T5: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_float5|decimal:1%,%2}} = 10,36", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T6: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_float6|decimal:1%,%2}} = 10,35", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T7: %i\n", ret);
  }

  // user parsing
  {
    res = cape_eval_b ("{{val_float4|pipe_test:xyz}} = 13", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T8: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_bool_true}} = TRUE", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T9: %i\n", ret);
  }
  
  {
    CapeString h = cape_template_run ("bool_t: {{val_bool_true}}, bool_f: {{val_bool_false}}", values, NULL, NULL, err);

    if (h)
    {
      printf ("VALUES: %s\n", h);
    }
    else
    {
      printf ("ERR %s\n", cape_err_text(err));
    }
    
    cape_str_del (&h);
  }

  {
    CapeUdc n = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n (n, "val", 123);
    
    CapeString h = cape_template_run ("{{$math{val - 0.2}|decimal:1%,%2}}", n, NULL, NULL, err);

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
  {
    CapeUdc n = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    CapeDatetime dt; cape_datetime_utc (&dt);
    
    cape_udc_add_d (n, "date", &dt);
    
    CapeString h = cape_template_run ("{{$date{date + D14}|date:%d.%m.%Y}}", n, NULL, NULL, err);
    
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

  {
    CapeUdc n1 = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc n2 = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n (n1, "data1", 1);
    cape_udc_add_n (n2, "data2", 2);

    cape_udc_add_name (n1, &n2, "sub");
    
    CapeString h = cape_template_run ("d1: {{data1}}\n sub {{#sub}}d2: {{data2}} d1: {{data1}}{{/sub}}", n1, NULL, NULL, err);

    if (h)
    {
      printf ("SUB: %s\n", h);
    }
    else
    {
      printf ("ERR %s\n", cape_err_text(err));
    }

    cape_str_del (&h);
    
    cape_udc_del (&n1);
    cape_udc_del (&n2);
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

