#ifndef __CAPE_FMT__PARSER_JSON__H
#define __CAPE_FMT__PARSER_JSON__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"

#if defined __WINDOWS_OS

#define CAPE_MATH_NAN         0x7fc00000  // only works for x86 try this instead 0.0 / 0.0 ?
#define CAPE_MATH_INFINITY    0x7F800000  // only works for x86 try this instead 1.0 / 0.0

#else

#define CAPE_MATH_NAN NAN
#define CAPE_MATH_INFINITY INFINITY

#endif

//=============================================================================

#define CAPE_JPARSER_UNDEFINED       0
#define CAPE_JPARSER_OBJECT_NODE     1
#define CAPE_JPARSER_OBJECT_LIST     2
#define CAPE_JPARSER_OBJECT_TEXT     3
#define CAPE_JPARSER_OBJECT_NUMBER   4
#define CAPE_JPARSER_OBJECT_FLOAT    5
#define CAPE_JPARSER_OBJECT_BOLEAN   6
#define CAPE_JPARSER_OBJECT_NULL     7
#define CAPE_JPARSER_OBJECT_DATETIME 8

//-----------------------------------------------------------------------------

struct CapeParserJson_s; typedef struct CapeParserJson_s* CapeParserJson;

// callbacks
typedef void   (__STDCALL *fct_parser_json_onItem)          (void* ptr, void* obj, int type, void* val, const char* key, int index);
typedef void*  (__STDCALL *fct_parser_json_onObjNew)        (void* ptr, int type);
typedef void   (__STDCALL *fct_parser_json_onObjDel)        (void* ptr, void* obj);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeParserJson    cape_parser_json_new       (void* ptr, fct_parser_json_onItem, fct_parser_json_onObjNew, fct_parser_json_onObjDel);

__CAPE_LIBEX   void              cape_parser_json_del       (CapeParserJson*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int               cape_parser_json_process   (CapeParserJson, const char* buffer, number_t size, CapeErr err);

__CAPE_LIBEX   void*             cape_parser_json_object    (CapeParserJson);

//-----------------------------------------------------------------------------

#endif
