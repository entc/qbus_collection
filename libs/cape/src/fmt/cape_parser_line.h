#ifndef __CAPE_FMT__PARSER_LINE__H
#define __CAPE_FMT__PARSER_LINE__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"

//-----------------------------------------------------------------------------

struct CapeParserLine_s; typedef struct CapeParserLine_s* CapeParserLine;

typedef void   (__STDCALL *fct_parser_line__on_newline)    (void* ptr, const CapeString line);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeParserLine    cape_parser_line_new       (void* ptr, fct_parser_line__on_newline);

__CAPE_LIBEX   void              cape_parser_line_del       (CapeParserLine*);

__CAPE_LIBEX   int               cape_parser_line_process   (CapeParserLine, const char* buffer, number_t size, CapeErr err);

__CAPE_LIBEX   int               cape_parser_line_finalize  (CapeParserLine, CapeErr err);

//-----------------------------------------------------------------------------

#endif
