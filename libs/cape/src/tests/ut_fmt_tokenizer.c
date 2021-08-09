#include "fmt/cape_tokenizer.h"
#include "fmt/cape_json.h"

// c includes
#include <stdio.h>

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();
  
  // local objects
  CapeUdc t1 = NULL;
  CapeUdc t2 = NULL;
  CapeUdc t3 = NULL;
  
  {
    t1 = cape_tokenizer_options ("Hello World ((a:12,b:\"Frank\"))");
    if (t1)
    {
      CapeString h = cape_json_to_s (t1);
      
      printf ("T1: %s\n", h);
      
      cape_str_del (&h);
    }
    else
    {
      // failed
    }
  }

  {
    t2 = cape_tokenizer_options ("Hello World");
    if (t2)
    {
      // failed
    }
    else
    {
      printf ("T2: %s\n", "Hello World");
    }
  }

  {
    t3 = cape_tokenizer_options ("Hello ((a:6789)) World");
    if (t3)
    {
      CapeString h = cape_json_to_s (t3);
      
      printf ("T3: %s\n", h);
      
      cape_str_del (&h);
    }
    else
    {
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  if (res)
  {
    printf ("ERROR: %s\n", cape_err_text (err));
  }

  cape_udc_del (&t1);
  cape_udc_del (&t2);
  cape_udc_del (&t3);
  
  cape_err_del (&err);
  
  return res;
}
