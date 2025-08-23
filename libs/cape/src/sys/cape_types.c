#include "cape_types.h"

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
