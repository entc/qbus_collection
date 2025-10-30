#include "fmt/cape_template.h"
#include "fmt/cape_json.h"
#include "sys/cape_file.h"

// c includes
#include <stdio.h>

//-----------------------------------------------------------------------------

static char* __STDCALL main__on_pipe (const char* name, const char* pipe, const char* value)
{
  //printf ("ON PIPE: name = '%s', pipe = '%s', value = '%s'\n", pipe, value);
  
  return cape_str_cp ("13");
}

//-----------------------------------------------------------------------------

int __STDCALL test_custom_files__on_load (void* ptr, const char* bufdat, number_t buflen, CapeErr err)
{
  cape_stream_append_buf (ptr, bufdat, buflen);
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL test_custom_files__on_text (void* ptr, const char* text)
{
  cape_stream_append_str (ptr, text);
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void test_custom_files (CapeErr err)
{
  int res;
  
  // local objects
  CapeStream s1 = NULL;
  CapeStream s2 = NULL;
  CapeUdc params = NULL;
  CapeTemplate template_engine = NULL;

  params = cape_json_from_file ("params.json", err);
  if (NULL == params)
  {
    goto exit_and_cleanup;
  }

  s1 = cape_stream_new ();
  
  res = cape_fs_file_load (NULL, "template.txt", s1, test_custom_files__on_load, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  template_engine = cape_template_new ();
  
  res = cape_template_compile_str (template_engine, cape_stream_get (s1), err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  s2 = cape_stream_new ();

  res = cape_template_apply (template_engine, params, s2, test_custom_files__on_text, NULL, NULL, NULL, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  printf ("-----------------------------------\n");
  printf ("%s\n", cape_stream_get (s2));
  printf ("-----------------------------------\n");

exit_and_cleanup:
  
  cape_template_del (&template_engine);
  cape_udc_del (&params);
  cape_stream_del (&s1);
  cape_stream_del (&s2);
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
    res = cape_eval_b ("{{extras.val1}} = 1234", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T10: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{val_bool_true}} = TRUE AND NOT {{extras.val1}} = 0001 AND NOT {{extras.val1}} = 0002", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T11: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{extras.val2|substr:2%3}} = 678", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T12: %i\n", ret);
  }

  {
    res = cape_eval_b ("{{extras.val1}} I [678, 1234]", values, &ret, main__on_pipe, err);
    if (res)
    {
      goto exit_and_cleanup;
    }
    
    printf ("T13: %i\n", ret);
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
    CapeString h = cape_template_run ("'{{val_float5|lpad:30}}'", values, NULL, NULL, err);
    
    if (h)
    {
      printf ("LPAD: %s\n", h);
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
      printf ("MATH1: %s\n", h);
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
    
    cape_udc_add_f (n, "val1", 30.2);
    cape_udc_add_s_cp (n, "val2", "5000");
    cape_udc_add_f (n, "val3", 10.4);

    CapeString h = cape_template_run ("{{$math{val1 / 100 * val2 / 12 + val3 / 12}|decimal:1%,%2}}", n, NULL, NULL, err);

    if (h)
    {
      printf ("MATH2: %s\n", h);
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
    CapeString h = cape_template_run ("{{val_float7|decimal:((fraction:12,digits:2,round:halfup,delimiter:','))}}", values, NULL, NULL, err);
    
    if (h)
    {
      printf ("HALFUP: %s\n", h);
    }
    else
    {
      printf ("ERR %s\n", cape_err_text(err));
    }

  }
  {
    CapeUdc n1 = cape_udc_new (CAPE_UDC_NODE, NULL);
    CapeUdc n2 = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_n (n1, "data1", 1);
    cape_udc_add_n (n2, "data2", 2);

    cape_udc_add_name (n1, &n2, "sub");
    
    CapeString h = cape_template_run ("d1: {{data1}}, {{#sub}}d2: {{data2}}, d1: {{data1}}{{/sub}}, d2: {{sub.data2}}", n1, NULL, NULL, err);

    if (h)
    {
      printf ("SUB: %s\n", h);
      
      if (!cape_str_equal ("d1: 1, d2: 2, d1: 1, d2: 2", h))
      {
        cape_err_set (err, CAPE_ERR_WRONG_VALUE, "missmatch sub");
      }
    }
    else
    {
      printf ("ERR %s\n", cape_err_text(err));
    }
    
    cape_str_del (&h);
    
    cape_udc_del (&n1);
    cape_udc_del (&n2);
  }

  test_custom_files (err);
  
exit_and_cleanup:

  if (cape_err_code(err))
  {
    printf ("ERROR: %s\n", cape_err_text(err));
  }
  
  cape_udc_del (&values);
  
  cape_err_del (&err);
  return 0;
}

