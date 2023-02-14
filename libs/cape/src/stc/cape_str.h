#ifndef __CAPE_STC__STR__H
#define __CAPE_STC__STR__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"

// included for va_list
#include <stdarg.h>

//=============================================================================

#define CapeString char*

__CAPE_LIBEX   CapeString         cape_str_cp            (const CapeString);                            // allocate memory and initialize the object

__CAPE_LIBEX   CapeString         cape_str_mv            (CapeString*);                                 // move string

__CAPE_LIBEX   void               cape_str_del           (CapeString*);                                 // release memory

__CAPE_LIBEX   CapeString         cape_str_sub           (const CapeString, number_t len);              // copy a part of the substring

__CAPE_LIBEX   CapeString         cape_str_uuid          (void);                                        // create an UUID and copy it into the string

__CAPE_LIBEX   CapeString         cape_str_random_s      (number_t len);                                // create a randwom string with the length (len)

__CAPE_LIBEX   CapeString         cape_str_random_n      (number_t len);                                // create a randwom string with the length (len)

__CAPE_LIBEX   CapeString         cape_str_fmt           (const CapeString format, ...);                // format to string

__CAPE_LIBEX   CapeString         cape_str_flp           (const CapeString format, va_list);            // format to string

__CAPE_LIBEX   number_t           cape_str_len           (const CapeString);                            // string length in characters

__CAPE_LIBEX   number_t           cape_str_size          (const CapeString);                            // string length in bytes

__CAPE_LIBEX   CapeString         cape_str_f             (double);                                      // double to string

__CAPE_LIBEX   CapeString         cape_str_n             (number_t);                                    // number to string

__CAPE_LIBEX   CapeString         cape_str_hex           (const unsigned char* data, number_t len);     // binary buffer to hex

__CAPE_LIBEX   int                cape_str_empty         (const CapeString);                            // string length in bytes

__CAPE_LIBEX   int                cape_str_not_empty     (const CapeString);                            // string length in bytes

//-----------------------------------------------------------------------------

__CAPE_LIBEX   number_t           cape_str_to_n          (const CapeString);

__CAPE_LIBEX   double             cape_str_to_f          (const CapeString);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                cape_str_equal         (const CapeString, const CapeString);          // case sensitive

__CAPE_LIBEX   int                cape_str_compare       (const CapeString, const CapeString);          // no case sensitive

__CAPE_LIBEX   int                cape_str_compare_c     (const CapeString, const CapeString);          // no case sensitive

__CAPE_LIBEX   int                cape_str_begins        (const CapeString, const CapeString);          // case sensitive

__CAPE_LIBEX   int                cape_str_begins_i      (const CapeString, const CapeString);          // case insensitive

__CAPE_LIBEX   int                cape_str_ends          (const CapeString, const CapeString);          // case sensitive

__CAPE_LIBEX   int                cape_str_find          (const CapeString, const CapeString, number_t* pos);

__CAPE_LIBEX   int                cape_str_find_utf8     (const CapeString, const CapeString, number_t* pos_len, number_t* pos_size);

__CAPE_LIBEX   int                cape_str_next          (const CapeString, char, number_t* pos);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeString         cape_str_catenate_c    (const CapeString, char c, const CapeString);

__CAPE_LIBEX   CapeString         cape_str_catenate_2    (const CapeString, const CapeString);

__CAPE_LIBEX   CapeString         cape_str_catenate_3    (const CapeString, const CapeString, const CapeString);

__CAPE_LIBEX   CapeString         cape_str_lpad          (const CapeString, char padding_char, number_t max_length);

__CAPE_LIBEX   CapeString         cape_str_rpad          (const CapeString, char padding_char, number_t max_length);

__CAPE_LIBEX   CapeString         cape_str_trim_utf8     (const CapeString);

__CAPE_LIBEX   CapeString         cape_str_trim_lr       (const CapeString, char l, char r);

__CAPE_LIBEX   CapeString         cape_str_trim_c        (const CapeString, char c);

                                  /*
                                  removes invalid UTF-8 characters
                                   */
__CAPE_LIBEX   CapeString         cape_str_sanitize_utf8 (const CapeString source);

                                  /*
                                  converts UTF-8 into human readable characters
                                   */
__CAPE_LIBEX   CapeString         cape_str_to_word       (const CapeString source);

                                  /*
                                  converts UTF-8 into Latin-1 (ISO-8859-1)
                                   */
__CAPE_LIBEX   CapeString         cape_str_to_latin1     (const CapeString source);

__CAPE_LIBEX   CapeString         cape_str_cp_replaced   (const CapeString source, const CapeString seek, const CapeString replace_with);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               cape_str_replace_cp    (CapeString*, const CapeString source);      // replaces the object with a const string

__CAPE_LIBEX   void               cape_str_replace_mv    (CapeString*, CapeString*);                  // replaces the object with another object
  
__CAPE_LIBEX   void               cape_str_replace       (CapeString*, const CapeString seek, const CapeString replace_with);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               cape_str_fill          (CapeString, number_t len, const CapeString source);       // will cut the content if not enough memory

__CAPE_LIBEX   void               cape_str_to_upper      (CapeString);

__CAPE_LIBEX   void               cape_str_to_lower      (CapeString);

__CAPE_LIBEX   void               cape_str_override      (CapeString, number_t offset_start, number_t offset_end, char with);

//-----------------------------------------------------------------------------

                                  /* returns the character size, invalid = 1 */
__CAPE_LIBEX   number_t           cape_str_char__len     (unsigned char c);

                                  /* returns the character size, invalid = 0 */
__CAPE_LIBEX   number_t           cape_str_utf8__len     (unsigned char c);

//-----------------------------------------------------------------------------

                                  /* converts wide char into utf8, returns the utf8 length */
__CAPE_LIBEX   number_t           cape_str_wchar_utf8    (wchar_t s, char* bufdat);

//-----------------------------------------------------------------------------

                                  /* calculates the distance of two words */
                                  /* the damerau leventshein algorithm is used */
                                  /* WARNING: this can't handle UTF-8 characters */
__CAPE_LIBEX   number_t           cape_str_distance      (const CapeString s1, number_t l1, const CapeString s2, number_t l2);

                                  /* search the needle in the haystack */
                                  /* using the cape_str_distance method */
                                  /* returns the minimum the distance returns */
__CAPE_LIBEX   number_t           cape_str_distcont      (const CapeString haystack, const CapeString needle, number_t needle_len);

//-----------------------------------------------------------------------------

                                  /* convert into [0-9] to use as a large number */
                                  /* leading '0' will be removed */
__CAPE_LIBEX   CapeString         cape_str_ln_normalize  (const CapeString s);

                                  /* compare two large numbers */
                                  /* ! it is recomended to normalize the strings before ! */
__CAPE_LIBEX   int                cape_str_ln_cmp        (const CapeString lns1, const CapeString lns2);

//-----------------------------------------------------------------------------

#endif


