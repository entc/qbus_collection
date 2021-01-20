#ifndef __CAPE_SYS__TIME__H
#define __CAPE_SYS__TIME__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"

// c includes
#include <time.h>

//=============================================================================

#pragma pack(push, 16)
typedef struct
{  
  unsigned int day;
  unsigned int month;
  unsigned int year;
  
  unsigned int hour;
  unsigned int minute;
  
  unsigned int usec;
  unsigned int msec;
  unsigned int sec;
  
  int is_dst;
  int is_utc;
  
} CapeDatetime;
#pragma pack(pop)

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeDatetime*   cape_datetime_new          (void);

__CAPE_LIBEX   void            cape_datetime_del          (CapeDatetime**);

__CAPE_LIBEX   CapeDatetime*   cape_datetime_cp           (const CapeDatetime*);

__CAPE_LIBEX   CapeDatetime*   cape_datetime_mv           (CapeDatetime**);

__CAPE_LIBEX   void            cape_datetime_set          (CapeDatetime*, const CapeDatetime*);

// example:
/*
 * CapeDatetime* dt = cape_datetime_new ();
 * 
 * -> dt will be filled with a new object
 * 
 * cape_datetime_del (&dt);
 * 
 * -> dt will be set to NULL and all memeory will be freed
 * 
 */

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void            cape_datetime_utc          (CapeDatetime*);

__CAPE_LIBEX   void            cape_datetime_local        (CapeDatetime*);

__CAPE_LIBEX   void            cape_datetime_to_local     (CapeDatetime*);

                               // append the datetime to the certain time period defined in delta string format
                               /* D2, h2, m2, s2, u2 | D2:h2:m2:s2:u2 */
__CAPE_LIBEX   void            cape_datetime_add_s        (CapeDatetime*, const CapeString delta);

                               // reduce the datetime to the certain time period defined in delta string format
                               /* D2, h2, m2, s2, u2 | D2:h2:m2:s2:u2 */
__CAPE_LIBEX   void            cape_datetime_sub_s        (CapeDatetime*, const CapeString delta);

                               // append a certain time period to the current datetime in string format
                               /* D2, h2, m2, s2, u2 | D2:h2:m2:s2:u2 */
__CAPE_LIBEX   void            cape_datetime_utc__add_s   (CapeDatetime*, const CapeString delta);

                               // substract a certain time period to the current datetime in string format
                               /* D2, h2, m2, s2, u2 | D2:h2:m2:s2:u2 */
__CAPE_LIBEX   void            cape_datetime_utc__sub_s   (CapeDatetime*, const CapeString delta);

                               // reduce input datetine from a certain time period defined in delta string format and save it in result datetime
                               /* D2, h2, m2, s2, u2 | D2:h2:m2:s2:u2 */
__CAPE_LIBEX   void            cape_datetime__remove_s    (const CapeDatetime*, const CapeString delta, CapeDatetime* result);

                               // append a certain time period defined in delta string format and save it in result datetime
                               /* D2, h2, m2, s2, u2 | D2:h2:m2:s2:u2 */
__CAPE_LIBEX   void            cape_datetime__add_s       (const CapeDatetime*, const CapeString delta, CapeDatetime* result);

                               // reduce a certain time period defined in delta string format and save it in result datetime
                               /* D2, h2, m2, s2, u2 | D2:h2:m2:s2:u2 */
__CAPE_LIBEX   void            cape_datetime__sub_s       (const CapeDatetime*, const CapeString delta, CapeDatetime* result);

                               // append a certain time period defined as seconds: format and save it in result datetime
                               /* period in seconds */
__CAPE_LIBEX   void            cape_datetime__add_n       (const CapeDatetime*, number_t period, CapeDatetime* result);

                               // compares two datetimes
__CAPE_LIBEX   int             cape_datetime_cmp          (const CapeDatetime*, const CapeDatetime*);

//-----------------------------------------------------------------------------

                               /* generic method -> use the format for transformation */
__CAPE_LIBEX   CapeString      cape_datetime_s__fmt       (const CapeDatetime*, const CapeString format);

                               /* 2013-10-21T13:28:06.419Z */
__CAPE_LIBEX   CapeString      cape_datetime_s__std_msec  (const CapeDatetime*);   // RFC 3339

                               /* 2013-10-21T13:28:06Z */
__CAPE_LIBEX   CapeString      cape_datetime_s__std       (const CapeDatetime*);   // RFC 3339

                               /* 2019-09-01 12:08:21 */
__CAPE_LIBEX   CapeString      cape_datetime_s__str       (const CapeDatetime*);   // ISO format

                               /* 20190918-12:07:55.992 */
__CAPE_LIBEX   CapeString      cape_datetime_s__log       (const CapeDatetime*);   // LOG format

                               /* Sun, 11 May 2018 17:05:40 GMT */
__CAPE_LIBEX   CapeString      cape_datetime_s__gmt       (const CapeDatetime*);   // GMT format

                               /* Mon, 18 Nov 2019 18:56:20 +0000 (UTC) */
__CAPE_LIBEX   CapeString      cape_datetime_s__aph       (const CapeDatetime*);   // Alpha format

                               /* 2019_09_01__12_08_21__ */
__CAPE_LIBEX   CapeString      cape_datetime_s__pre       (const CapeDatetime*);   // prefix format

                               /* 20190901T120821Z */
__CAPE_LIBEX   CapeString      cape_datetime_s__ISO8601   (const CapeDatetime*);   // ISO8601

//-----------------------------------------------------------------------------

__CAPE_LIBEX   time_t          cape_datetime_n__unix      (const CapeDatetime*);

//-----------------------------------------------------------------------------

                               /* 1970-01-01T13:28:06.419Z */
__CAPE_LIBEX   int             cape_datetime__std_msec    (CapeDatetime*, const CapeString datetime_in_text);

                               /* 2019-09-01 12:08:21.231 */
__CAPE_LIBEX   int             cape_datetime__str_msec    (CapeDatetime*, const CapeString datetime_in_text);

                               /* 2019-09-01 12:08:21 */
__CAPE_LIBEX   int             cape_datetime__str         (CapeDatetime*, const CapeString datetime_in_text);

                               /* 01.01.1970 */
__CAPE_LIBEX   int             cape_datetime__date_de     (CapeDatetime*, const CapeString datetime_in_text);

                               /* 1970-01-01 */
__CAPE_LIBEX   int             cape_datetime__date_iso    (CapeDatetime*, const CapeString datetime_in_text);

//-----------------------------------------------------------------------------

struct CapeStopTimer_s; typedef struct CapeStopTimer_s* CapeStopTimer;

__CAPE_LIBEX   CapeStopTimer   cape_stoptimer_new         ();

__CAPE_LIBEX   void            cape_stoptimer_del         (CapeStopTimer*);

__CAPE_LIBEX   void            cape_stoptimer_start       (CapeStopTimer);           // starts / continue the time measurement

__CAPE_LIBEX   void            cape_stoptimer_stop        (CapeStopTimer);           // ends the time measurement

__CAPE_LIBEX   void            cape_stoptimer_set         (CapeStopTimer, double);   // sets the passed time

__CAPE_LIBEX   double          cape_stoptimer_get         (CapeStopTimer);           // returns the passed time in milliseconds

//-----------------------------------------------------------------------------

#endif
