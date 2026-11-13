#include "cape_types.h"

// some extra headers
#include <stdio.h>

//-----------------------------------------------------------------------------

void* cape_alloc (number_t size)
{
  void* ptr = malloc (size);

  if (ptr == NULL)
  {
    // write some last words
    printf ("*** FATAL *** CAN't ALLOCATE MEMORY *** FATAL ***\n");

    // abort everything
    abort ();
  }

  memset (ptr, 0, size);

  return ptr;
}

//-----------------------------------------------------------------------------

void* cape_calloc (number_t count, number_t size)
{
    void* ptr = calloc (count, size);

    if (ptr == NULL)
    {
        // write some last words
        printf("*** FATAL *** CAN't ALLOCATE MEMORY *** FATAL ***\n");

        // abort everything
        abort();
    }

    return ptr;
}

//-----------------------------------------------------------------------------

void cape_free (void* ptr)
{
  free (ptr);
}

//-----------------------------------------------------------------------------

void* cape_mv (void** p_ptr)
{
    void* tmp = *p_ptr;
    *p_ptr = NULL;
    
    return tmp;
}

//-----------------------------------------------------------------------------

inline number_t cape_max_n (number_t x, number_t y)
{
    return (x > y) ? x : y;
}

//-----------------------------------------------------------------------------

inline number_t cape_rand_n (number_t min, number_t max)
{
    return rand() % (max - min + 1) + min;
}

//-----------------------------------------------------------------------------
