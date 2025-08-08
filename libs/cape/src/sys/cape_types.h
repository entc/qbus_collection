#ifndef __CAPE_TYPES__H
#define __CAPE_TYPES__H 1

#include <sys/cape_export.h>

#if defined __APPLE__

#include <malloc/malloc.h>
#include <stdlib.h>
#include <memory.h>

#define u_t unsigned
#define number_t long

#elif defined __OpenBSD__

#include <memory.h>
#include <stdlib.h>

#elif defined __WINDOWS_OS

#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <sys/types.h>

#define u_t unsigned
#define number_t long long

#else

#include <malloc.h>
#include <memory.h>
#include <stddef.h>
#include <stdlib.h>

#define u_t unsigned
#define number_t long

#endif

#include <stdio.h>

//-----------------------------------------------------------------------------

static void* cape_alloc (number_t size)
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

static void cape_free (void* ptr)
{
  free (ptr);
}

//-----------------------------------------------------------------------------

#define CAPE_ALLOC(size) cape_alloc(size)
#define CAPE_FREE(ptr) cape_free(ptr)

//-----------------------------------------------------------------------------

#define CAPE_NEW(type) (type*)cape_alloc(sizeof(type))
#define CAPE_DEL(ptr, type) { memset(*ptr, 0, sizeof(type)); free(*ptr); *ptr = 0; }

//-----------------------------------------------------------------------------

__CAPE_LIBEX   number_t           cape_max_n (number_t x, number_t y);

__CAPE_LIBEX   number_t           cape_rand_n (number_t min, number_t max);

//-----------------------------------------------------------------------------

#define CAPE_DIRECTION_FORW 0x0001
#define CAPE_DIRECTION_PREV 0x0002

//-----------------------------------------------------------------------------

#define CAPE_BIT_SET(arg, pos) ((arg) |= (1L << (pos)))
#define CAPE_BIT_GET(arg, pos) int((arg) & (1L << (pos)))
#define CAPE_BIT_CLR(arg, pos) ((arg) & ~(1L << (pos)))
#define CAPE_BIT_FLP(arg, pos) ((arg) ^ (1L << (pos)))

//-----------------------------------------------------------------------------

#if defined __WINDOWS_OS

typedef unsigned __int64    cape_uint64;
typedef unsigned __int32    cape_uint32;
typedef __int32             cape_int32;
typedef __int16             cape_uint16;
typedef unsigned __int8     cape_uint8;
typedef int                 cape_bool;
typedef double              cape_float64;
typedef float               cape_float32;

#define __CAPE_INLINE __inline

#else

#include <sys/types.h>
#include <stdint.h> 

// have cape specific declaration
typedef uint64_t    cape_uint64;
typedef uint32_t    cape_uint32;
typedef int32_t     cape_int32;
typedef uint16_t    cape_uint16;
typedef uint8_t     cape_uint8;
typedef int         cape_bool;
typedef double      cape_float64;
typedef float       cape_float32;

#define __CAPE_INLINE inline

#endif

#endif
