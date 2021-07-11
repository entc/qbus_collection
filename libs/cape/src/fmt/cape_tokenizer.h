#ifndef __CAPE_FMT__TOKENIZER__H
#define __CAPE_FMT__TOKENIZER__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "stc/cape_list.h"
#include "stc/cape_udc.h"
#include "stc/cape_str.h"

//-----------------------------------------------------------------------------

                                  /* returns a list of CapeString's */
__CAPE_LIBEX   CapeList           cape_tokenizer_buf           (const char* buffer, number_t len, char token);

                                  /* returns a list of CapeString's, ignore all empty parts */
__CAPE_LIBEX   CapeList           cape_tokenizer_buf__noempty  (const char* buffer, number_t len, char token);

                                  /* returns TRUE / FALSE if successfull */
__CAPE_LIBEX   int                cape_tokenizer_split         (const CapeString source, char token, CapeString* p_left, CapeString* p_right);

                                  /* returns TRUE / FALSE if successfull */
__CAPE_LIBEX   int                cape_tokenizer_split_last    (const CapeString source, char token, CapeString* p_left, CapeString* p_right);

                                  /* returns a list of CapeString's */
__CAPE_LIBEX   CapeList           cape_tokenizer_str           (const CapeString haystack, const CapeString needle);

//-----------------------------------------------------------------------------

                                  /* returns a list with number_t values of the position
                                     -> WARNING: utf8 characters are not considered
                                                 positions are related to chars only
                                   */
__CAPE_LIBEX   CapeList           cape_tokenizer_str_pos       (const CapeString haystack, const CapeString needle);

                                  /* returns a list with number_t values
                                      -> use this to get utf8 char positions
                                   */
__CAPE_LIBEX   CapeList           cape_tokenizer_str_utf8_pos  (const CapeString haystack, const CapeString needle);

//-----------------------------------------------------------------------------

                                  /* returns an UDC structure from an option string
                                   -> input format: "text ((key:val))" which will be parsed
                                   -> output: {"text": text, "key": val}
                                   -> returns NULL if parsing returns no results
                                        -> source text has no (())
                                   */
__CAPE_LIBEX   CapeUdc            cape_tokenizer_options (const CapeString source);

//-----------------------------------------------------------------------------

#endif


