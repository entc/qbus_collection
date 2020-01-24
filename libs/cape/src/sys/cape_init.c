#include <time.h>
#include <stdlib.h>

#if defined __GNUC__

//-----------------------------------------------------------------------------

__attribute__((constructor)) void library_constructor ()
{
  // initialize random generator
  srand (time (NULL));
}

//-----------------------------------------------------------------------------

__attribute__((destructor)) void library_destructor ()
{
  
}

//-----------------------------------------------------------------------------

#endif
