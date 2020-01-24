#include "stc/cape_str.h"
#include <stdio.h>
#include <limits.h>

int main (int argc, char *argv[])
{
  long long test = 82361783681;
  
  CapeString h = cape_str_fmt ("%lld", test);
  
  if (h)
  {
    printf ("%s\n", h);
  }
  
  return 0;
}


