#include "stc/cape_map.h"
#include "stc/cape_str.h"
#include "stc/cape_list.h"

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

CapeMap test_create_map_n__chain (number_t max_entries, CapeList entries)
{
  CapeMap ret = cape_map_new (cape_map__compare__n, NULL, NULL);

  {
    number_t i;

    for (i = 0; i < max_entries; i++)
    {
      cape_map_insert (ret, (void*)i, NULL);
      cape_list_push_back (entries, (void*)i);
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

void __STDCALL list_on_del (void* ptr)
{
}

//-----------------------------------------------------------------------------------

void test01_delete_validate (CapeList l, CapeMap m)
{
  CapeMapCursor* cursor_m = cape_map_cursor_new (m, CAPE_DIRECTION_FORW);
  CapeListCursor* cursor_l = cape_list_cursor_new (l, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor_m) && cape_list_cursor_next (cursor_l))
  {
    number_t val1 = (number_t)cape_map_node_key (cursor_m->node);
    number_t val2 = (number_t)cape_list_node_data (cursor_l->node);
    
    if (val1 != val2)
    {
      printf ("ERROR: %lu <> %lu\n", val1, val2);
    }
  }
  
  cape_list_cursor_del (&cursor_l);
  cape_map_cursor_del (&cursor_m);
}

//-----------------------------------------------------------------------------------

void test01_delete_run (number_t x)
{
  CapeList l1 = cape_list_new (list_on_del);
  CapeMap m1 = test_create_map_n__chain (x, l1);

  number_t i;
  
  for (i = 0; i < x; i++)
  {
    CapeMapCursor* cursor_m = cape_map_cursor_new (m1, CAPE_DIRECTION_FORW);
    CapeListCursor* cursor_l = cape_list_cursor_new (l1, CAPE_DIRECTION_FORW);
    
    number_t steps = cape_rand_n (1, x - i);
    number_t j;
    
    for (j = 0; j < steps; j++)
    {
      cape_map_cursor_next (cursor_m);
      cape_list_cursor_next (cursor_l);      
    }
    
    cape_map_cursor_erase (m1, cursor_m);
    cape_list_cursor_erase (l1, cursor_l);

    test01_delete_validate (l1, m1);
        
    cape_list_cursor_del (&cursor_l);
    cape_map_cursor_del (&cursor_m);
  }
    
  cape_map_del (&m1);
  cape_list_del (&l1);
}

//-----------------------------------------------------------------------------------

void test01_delete (number_t size, number_t runs)
{
  number_t i;
  
  for (i = 0; i < runs; i++)
  {
    test01_delete_run (cape_rand_n (10, size));
  }
}

//-----------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  test01_iteration ();
  
  test01_balance (10000);
  
  test01_delete (30, 10000);
  
  return 0;
}

//-----------------------------------------------------------------------------------

