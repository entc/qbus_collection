#ifndef __CAPE_FMT__DRAGON4__H
#define __CAPE_FMT__DRAGON4__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "stc/cape_str.h"
#include "sys/cape_err.h"

//-----------------------------------------------------------------------------

typedef enum CapeDragon4DigitMode
{
  CAPE_DRAGON4__DMODE_UNIQUE,            // Round digits to print shortest uniquely identifiable number
  CAPE_DRAGON4__DMODE_EXACT,             // Output the digits of the number as if with infinite precision
  
} CapeDragon4DigitMode;

//-----------------------------------------------------------------------------

typedef enum CapeDragon4CutoffMode
{
  CAPE_DRAGON4__CMODE_TOTAL,             // up to cutoffNumber significant digits
  CAPE_DRAGON4__CMODE_FRACTION,          // up to cutoffNumber significant digits past the decimal point
  
} CapeDragon4CutoffMode;

//-----------------------------------------------------------------------------

typedef enum CapeDragon4TrimMode
{
  CAPE_DRAGON4__TMODE_NONE,              // don't trim zeros, always leave a decimal point
  CAPE_DRAGON4__TMODE_ONE_ZERO,          // trim all but the zero before the decimal point
  CAPE_DRAGON4__TMODE_ZEROS,             // trim all trailing zeros, leave decimal point
  CAPE_DRAGON4__TMODE_BOTH,              // trim trailing zeros & trailing decimal point
 
} CapeDragon4TrimMode;

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

struct CapeDragon4_s; typedef struct CapeDragon4_s* CapeDragon4;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeDragon4       cape_dragon4_new           (void);

__CAPE_LIBEX   void              cape_dragon4_del           (CapeDragon4*);

__CAPE_LIBEX   int               cape_dragon4_run           (CapeDragon4, char* bufdat, number_t buflen, double value, CapeErr err);

__CAPE_LIBEX   number_t          cape_dragon4_len           (CapeDragon4);

__CAPE_LIBEX   CapeString        cape_dragon4_get           (CapeDragon4);

__CAPE_LIBEX   void              cape_dragon4_positional    (CapeDragon4, CapeDragon4DigitMode, CapeDragon4CutoffMode cutoff_mode, int precision, int sign, CapeDragon4TrimMode trim, int pad_left, int pad_right);

__CAPE_LIBEX   void              cape_dragon4_scientific    (CapeDragon4, CapeDragon4DigitMode, int precision, int sign, CapeDragon4TrimMode trim, int pad_left, int exp_digits);

//-----------------------------------------------------------------------------

#endif


