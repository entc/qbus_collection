#include "stc/cape_str.h"
#include <stdio.h>
#include <limits.h>

int main (int argc, char *argv[])
{
  {
    CapeString s = cape_str_fmt ("%lld", 82361783681);
    
    printf ("T1 = '%s'\n", s);

    cape_str_del (&s);
  }

  {
    CapeString s = cape_str_sanitize_utf8 ("hello 🇩world!");

    printf ("T2 = '%s'\n", s);

    cape_str_del (&s);
  }
  
  {
    CapeString s = cape_str_to_word ("…e…in…v…er…st…a…nd…e…n");

    printf ("T3 = '%s'\n", s);

    cape_str_del (&s);
  }

  {
    CapeString s = cape_str_to_word ("…H…äl…lö🇩w…ïr…l…ß…");

    printf ("T4 = '%s'\n", s);

    cape_str_del (&s);
  }

  return 0;
}

