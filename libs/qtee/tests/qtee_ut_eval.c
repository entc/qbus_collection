#include "qtee_eval.h"

// cape inlcude
#include <fmt/cape_json.h>
#include <sys/cape_file.h>

// c includes
#include <stdio.h>

// cpputest includes
//#include "PureCTests_c.h" /* the offending C header */
#include "CppUTest/TestHarness_c.h"

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
  cape_udc_add_s_cp (values, "val_float7", "98.1");

  cape_udc_add_b (values, "val_bool_true", TRUE);
  cape_udc_add_b (values, "val_bool_false", FALSE);

  // subnode
  {
    CapeUdc extras = cape_udc_new (CAPE_UDC_NODE, "extras");
    
    cape_udc_add_n    (extras, "val1", 1234);
    cape_udc_add_s_cp (extras, "val2", "4567890");
    
    cape_udc_add (values, &extras);
  }
  
  {
    res = qtee_eval_b ("{{val_str1}} = 1 AND {{val_str2}} = 1 OR {{val_str2}} = 2", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T1: %i\n", ret);
  }

  {
    res = qtee_eval_b ("{{val_float1|decimal:10%,%2}} = 0,11", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T2: %i\n", ret);
  }

  {
    res = qtee_eval_b ("{{val_float2|decimal:10%,%2}} = 0,11", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T3: %i\n", ret);
  }

  {
    res = qtee_eval_b ("{{val_float3|decimal:10%,%3}} = 113,432", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T4: %i\n", ret);
  }

  {
    res = qtee_eval_b ("{{val_float4|decimal:10%,%3}} = 113,432", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }

    printf ("T5: %i\n", ret);
  }

  {
    res = qtee_eval_b ("{{val_float5|decimal:1%,%2}} = 10,36", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T6: %i\n", ret);
  }

  {
    res = qtee_eval_b ("{{val_float6|decimal:1%,%2}} = 10,35", values, &ret, NULL, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T7: %i\n", ret);
  }

  // user parsing
  {
    res = qtee_eval_b ("{{val_float4|pipe_test:xyz}} = 13", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T8: %i\n", ret);
  }

  {
    res = qtee_eval_b ("{{val_bool_true}} = TRUE", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T9: %i\n", ret);
  }

  {
    res = qtee_eval_b ("{{extras.val1}} = 1234", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T10: %i\n", ret);
  }

  {
    res = qtee_eval_b ("{{val_bool_true}} = TRUE AND NOT {{extras.val1}} = 0001 AND NOT {{extras.val1}} = 0002", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T11: %i\n", ret);
  }

  {
    res = qtee_eval_b ("{{extras.val2|substr:2%3}} = 678", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T12: %i\n", ret);
  }

  {
    res = qtee_eval_b ("{{extras.val1}} I [678, 1234]", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T13: %i\n", ret);
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

