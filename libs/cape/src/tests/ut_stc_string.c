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

  {
    CapeString s = cape_str_to_latin1 ("Esslingen-Nürtingen");
    
    printf ("T5 = '%s'\n", s);
    
    cape_str_del (&s);
  }

  {
    const char* h1 = "M\u00FCnchen 10";
    const char* h2 = "Muenchen 10";

    number_t dist1 = cape_str_distance (h1, cape_str_size (h1), h2, cape_str_size (h2));

    printf ("DIST1 = %lu\n", dist1);
  }
  {
    const char* h1 = "M\u00FCnchen 10";
    const char* h2 = "München 10";
    
    number_t dist1 = cape_str_distance (h1, cape_str_size (h1), h2, cape_str_size (h2));
    
    printf ("DIST2 = %lu\n", dist1);
  }

  {
    const char* h1 = "M\u00FCnchen 10";
    const char* h2 = "münchen 10";

    printf ("COMP1 %s <> %s = %i\n", h1, h2, cape_str_compare_c (h1, h2));
  }
  {
    const char* h1 = "M\u00FCnchen 10";
    const char* h2 = "München 10";

    printf ("COMP1 %s <> %s = %i\n", h1, h2, strcmp (h1, h2));
  }

  // padding
  {
    CapeString p1 = cape_str_rpad ("432", '0', 10);
    printf ("P1: '%s'\n", p1);
    
    cape_str_del (&p1);
  }

  {
    CapeString p1 = cape_str_lpad ("432", '0', 10);
    printf ("P2: '%s'\n", p1);
    
    cape_str_del (&p1);
  }

  {
    CapeString p1 = cape_str_rpad ("1234567890", '0', 5);
    printf ("P3: '%s'\n", p1);
    
    cape_str_del (&p1);
  }

  {
    CapeString p1 = cape_str_lpad ("1234567890", '0', 5);
    printf ("P4: '%s'\n", p1);
    
    cape_str_del (&p1);
  }

  
  return 0;
}

