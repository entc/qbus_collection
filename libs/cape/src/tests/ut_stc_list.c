#include "stc/cape_list.h"
#include "stc/cape_str.h"
#include <stdio.h>
#include <limits.h>

//-----------------------------------------------------------------------------------

void __STDCALL list01__on_del (void* ptr)
{
  CapeString h = ptr; cape_str_del (&h);
}

//-----------------------------------------------------------------------------------

int __STDCALL list01__cmp01__on_cmp (void* ptr1, void* ptr2)
{
  CapeString s1 = ptr1;
  CapeString s2 = ptr2;
  
  return cape_str_compare_c (s2, s1);
}

//-----------------------------------------------------------------------------------

void dump_str_list (CapeList self)
{
  CapeListCursor* cursor = cape_list_cursor_create (self, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    const CapeString s = (CapeString)cape_list_node_data (cursor->node);
    
    printf ("P: %u, '%s'\n", cursor->position, s);
  }
  
  cape_list_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  CapeList list01 = cape_list_new (list01__on_del);
  
  {
    CapeString s = cape_str_cp ("hello");
    cape_list_push_back (list01, s);
  }

  {
    CapeString s = cape_str_cp ("world");
    cape_list_push_back (list01, s);
  }

  dump_str_list (list01);
  
  cape_list_sort (list01, list01__cmp01__on_cmp);
  
  dump_str_list (list01);
  
  cape_list_del (&list01);
  
  return 0;
}

//-----------------------------------------------------------------------------------

