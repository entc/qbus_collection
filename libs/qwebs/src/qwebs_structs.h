#ifndef __QWEBS_STRUCTS__H
#define __QWEBS_STRUCTS__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"

//-----------------------------------------------------------------------------

struct QWebsConnection_s; typedef struct QWebsConnection_s* QWebsConnection;

struct QWebs_s; typedef struct QWebs_s* QWebs;

struct QWebsFiles_s; typedef struct QWebsFiles_s* QWebsFiles;

struct QWebsRequest_s; typedef struct QWebsRequest_s* QWebsRequest;

//-----------------------------------------------------------------------------

struct QWebsApi_s; typedef struct QWebsApi_s* QWebsApi;
struct QWebsUpgrade_s; typedef struct QWebsUpgrade_s* QWebsUpgrade;

//-----------------------------------------------------------------------------

typedef void     (__STDCALL *fct_qwebs__on_recv)      (void* user_ptr, QWebsConnection, const char* bufdat, number_t buflen);
typedef void     (__STDCALL *fct_qwebs__on_del)       (void** user_ptr);

//-----------------------------------------------------------------------------

#endif
