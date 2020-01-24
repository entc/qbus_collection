#ifndef __QWEBS_FILES__H
#define __QWEBS_FILES__H 1

#include "qwebs_structs.h"

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "stc/cape_stream.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX     QWebsFiles         qwebs_files_new           (const CapeString path, QWebs webs);

__CAPE_LIBEX     void               qwebs_files_del           (QWebsFiles*);

__CAPE_LIBEX     CapeStream         qwebs_files_get           (QWebsFiles, const CapeString path);

//-----------------------------------------------------------------------------

#endif
