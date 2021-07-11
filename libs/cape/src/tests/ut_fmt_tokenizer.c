#include "fmt/cape_tokenizer.h"
#include "fmt/cape_json.h"

// c includes
#include <stdio.h>

int main (int argc, char *argv[])
{
  {
    CapeUdc t1 = cape_tokenizer_options ("Hello World ((a:12,b:\"Frank\"))");

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
    CapeUdc t2 = cape_tokenizer_options ("Hello World");
    
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
    CapeUdc t3 = cape_tokenizer_options ("Hello ((a:6789)) World");
    
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

  return 0;
}
