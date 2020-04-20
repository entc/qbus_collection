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

  
  CapeString s2 = cape_str_sanitize_utf8 ("hello 🇩world!");

  printf ("S2 = '%s'\n", s2);
  
  return 0;
}

