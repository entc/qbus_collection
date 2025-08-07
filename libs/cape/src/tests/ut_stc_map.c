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

void test_print_map_s__forward (CapeMap m)
{
  CapeMapCursor* cursor = cape_map_cursor_new (m, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    printf ("[%04lu] = '%s'\n", cursor->position, (const char*)cape_map_node_key (cursor->node));
  }
  
  cape_map_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------------

void test_print_map_s__rewiend (CapeMap m)
{
  CapeMapCursor* cursor = cape_map_cursor_new (m, CAPE_DIRECTION_PREV);
  
  while (cape_map_cursor_prev (cursor))
  {
    printf ("[%04lu] = '%s'\n", cursor->position, (const char*)cape_map_node_key (cursor->node));
  }
  
  cape_map_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------------

void test_print_map_n (CapeMap m)
{
  CapeMapCursor* cursor = cape_map_cursor_new (m, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    printf ("[%04lu] = '%lu'\n", cursor->position, (number_t)cape_map_node_key (cursor->node));
  }
  
  cape_map_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------------

CapeMap test_create_map_n__chain (number_t max_entries)
{
  CapeMap ret = cape_map_new (cape_map__compare__n, NULL, NULL);

  {
    number_t i;

    for (i = 0; i < max_entries; i++)
    {
      cape_map_insert (ret, (void*)i, NULL);
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------------

CapeMap test_create_map_s (number_t max_entries)
{
  CapeMap ret = cape_map_new (cape_map__compare__s, string__on_del, NULL);

  {
    number_t i;

    for (i = 0; i < max_entries; i++)
    {
      cape_map_insert (ret, cape_str_uuid (), NULL);
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------------

void test01_iteration ()
{
  CapeMap m1 = test_create_map_s (100);
  
  test_print_map_s__forward (m1);
  
  test_print_map_s__rewiend (m1);
  
  cape_map_del (&m1);
}

//-----------------------------------------------------------------------------------

void test01_balance (number_t x)
{
  number_t i;
  double avg_height = 0;

  for (i = 0; i < x; i++)
  {
    CapeMap m = test_create_map_s (10);

    avg_height = avg_height + cape_map_max_height (m);

    cape_map_del (&m);
  }
  
  avg_height = avg_height / x;

  printf ("max height = %f\n", avg_height);
}

//-----------------------------------------------------------------------------------

void test01_delete (number_t x)
{
  CapeMap m1 = test_create_map_n__chain (x);

  CapeMapCursor* cursor = cape_map_cursor_new (m1, CAPE_DIRECTION_FORW);

  cape_map_cursor_next (cursor);
  cape_map_cursor_next (cursor);

  cape_map_cursor_erase (m1, cursor);
  
  test_print_map_n (m1);
  
  cape_map_cursor_del (&cursor);

  cape_map_del (&m1);
}

//-----------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  test01_iteration ();
  
  test01_balance (10000);
  
  test01_delete (10);
  
  return 0;
}

//-----------------------------------------------------------------------------------

