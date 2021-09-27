#include "stc/cape_map.h"
#include "stc/cape_str.h"
#include <stdio.h>
#include <limits.h>

//-----------------------------------------------------------------------------------

void __STDCALL string__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    CapeString h = val; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  {
    CapeMap m1 = cape_map_new (cape_map__compare__s, string__on_del, NULL);
    
    cape_map_insert (m1, cape_str_cp ("test1"), cape_str_cp ("world1"));
    
    cape_map_del (&m1);
  }
  
  return 0;
}

//-----------------------------------------------------------------------------------

