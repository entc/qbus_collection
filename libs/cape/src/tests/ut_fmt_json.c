#include "fmt/cape_json.h"
#include "fmt/cape_parser_json.h"
#include <sys/cape_log.h>

#include <stdio.h>
#include <math.h>

int main (int argc, char *argv[])
{
  // float
  {
    CapeString s1;
    CapeString s2;
    CapeUdc m;
    
    CapeUdc n = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_f (n, "f1", .0);
    cape_udc_add_f (n, "f2", -0.0);
    cape_udc_add_f (n, "f3", 0.0000000000000000000000001);
    cape_udc_add_f (n, "f4", 100000000000000000000.00000000000000001);
    cape_udc_add_f (n, "f5", 9.59409594095941);
    cape_udc_add_f (n, "f6", CAPE_MATH_NAN);
    cape_udc_add_f (n, "f7", CAPE_MATH_INFINITY);
    
    s1 = cape_json_to_s (n);
    
    printf ("OUT1: %s\n", s1);
    
    m = cape_json_from_s (s1);
    
    s2 = cape_json_to_s (m);
    
    printf ("OUT2: %s\n", s2);
    
    
    cape_str_del (&s1);
    cape_str_del (&s2);
    
    cape_udc_del (&n);
    cape_udc_del (&m);
  }
  
  // datetime
  {
    CapeString s1;
    CapeString s2;
    CapeUdc m;
    
    CapeUdc n = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    CapeDatetime dt01;
    
    cape_datetime_utc (&dt01);
    
    cape_udc_add_d (n, "d1", &dt01);
    
    s1 = cape_json_to_s (n);
    
    printf ("OUT1: %s\n", s1);
    
    m = cape_json_from_s (s1);
    
    s2 = cape_json_to_s (m);
    
    printf ("OUT2: %s\n", s2);
    
    const CapeDatetime* dt02 = cape_udc_get_d (m, "d1", NULL);
    
    if (dt02 == NULL)
    {
      cape_log_msg (CAPE_LL_ERROR, "TEST", "udc datetime", "datetime returned is NULL");
    }
    
    cape_str_del (&s1);
    cape_str_del (&s2);
    
    cape_udc_del (&n);
    cape_udc_del (&m);
  }
  
  return 0;
}
