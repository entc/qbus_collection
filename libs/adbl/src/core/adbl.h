#ifndef __ADBL_CTX__H
#define __ADBL_CTX__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

//-----------------------------------------------------------------------------

#define ADBL_AUTO_SEQUENCE_ID     -300      // the database backend has sequence support -> get a new sequence ID automatically
#define ADBL_AUTO_INCREMENT       -200      // the database backend supports auto increment for this column

// use this only in combination with group by
#define ADBL_AGGREGATE_SUM        1         // creates a sum of all column values
#define ADBL_AGGREGATE_COUNT      2
#define ADBL_AGGREGATE_AVERAGE    3
#define ADBL_AGGREGATE_MAX        4
#define ADBL_AGGREGATE_MIN        5

//=============================================================================

struct AdblCtx_s; typedef struct AdblCtx_s* AdblCtx;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AdblCtx            adbl_ctx_new               (const char* path, const char* backend, CapeErr err);

__CAPE_LIBEX   void               adbl_ctx_del               (AdblCtx*);

//=============================================================================

struct AdblSession_s; typedef struct AdblSession_s* AdblSession;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AdblSession        adbl_session_open          (AdblCtx, CapeUdc connection_properties, CapeErr);

__CAPE_LIBEX   AdblSession        adbl_session_open_file     (AdblCtx, const char* config_file, CapeErr);

__CAPE_LIBEX   void               adbl_session_close         (AdblSession*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc            adbl_session_query         (AdblSession, const char* table, CapeUdc* p_params, CapeUdc* p_values, CapeErr);

                                  /* same as session query with extended options */
__CAPE_LIBEX   CapeUdc            adbl_session_query_ex      (AdblSession, const char* table, CapeUdc* p_params, CapeUdc* p_values, number_t limit, number_t offset, const CapeString group_by, const CapeString order_by, CapeErr);

                                  /* decreases a value of a column by 1 in case the value > 1, the result will be returned or an error if not possible */
__CAPE_LIBEX   number_t           adbl_session_atomic_dec    (AdblSession, const char* table, CapeUdc* p_params, const CapeString atomic_value, CapeErr);

                                  /* increase a value of a column by 1, the result will be returned or an error if not possible */
__CAPE_LIBEX   number_t           adbl_session_atomic_inc    (AdblSession, const char* table, CapeUdc* p_params, const CapeString atomic_value, CapeErr);

                                  /* apply a binaer operation, the result will be returned or an error if not possible */
__CAPE_LIBEX   number_t           adbl_session_atomic_or     (AdblSession, const char* table, CapeUdc* p_params, const CapeString atomic_value, number_t or_with, CapeErr);

//=============================================================================

struct AdblTrx_s; typedef struct AdblTrx_s* AdblTrx;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AdblTrx            adbl_trx_new               (AdblSession, CapeErr);

__CAPE_LIBEX   int                adbl_trx_commit            (AdblTrx*, CapeErr);

__CAPE_LIBEX   int                adbl_trx_rollback          (AdblTrx*, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc            adbl_trx_query             (AdblTrx, const char* table, CapeUdc* p_params, CapeUdc* p_values, CapeErr);

__CAPE_LIBEX   number_t           adbl_trx_insert            (AdblTrx, const char* table, CapeUdc* p_values, CapeErr);

__CAPE_LIBEX   int                adbl_trx_update            (AdblTrx, const char* table, CapeUdc* p_params, CapeUdc* p_values, CapeErr);

__CAPE_LIBEX   int                adbl_trx_delete            (AdblTrx, const char* table, CapeUdc* p_params, CapeErr);

__CAPE_LIBEX   number_t           adbl_trx_inorup            (AdblTrx, const char* table, CapeUdc* p_params, CapeUdc* p_values, CapeErr);

//-----------------------------------------------------------------------------

struct AdblCursor_s; typedef struct AdblCursor_s* AdblCursor;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AdblCursor         adbl_trx_cursor_new        (AdblTrx, const char* table, CapeUdc* p_params, CapeUdc* p_values, CapeErr);

__CAPE_LIBEX   void               adbl_trx_cursor_del        (AdblCursor*);

__CAPE_LIBEX   int                adbl_trx_cursor_next       (AdblCursor);

__CAPE_LIBEX   CapeUdc            adbl_trx_cursor_get        (AdblCursor);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void          adbl_param_add__greater_than_n  (CapeUdc params, const CapeString name, number_t value);

__CAPE_LIBEX   void          adbl_param_add__greater_than_d  (CapeUdc params, const CapeString name, const CapeDatetime* value);

                             /* greater or equal */
__CAPE_LIBEX   void          adbl_param_add__goe_than_n      (CapeUdc params, const CapeString name, number_t value);

                             /* greater or equal */
__CAPE_LIBEX   void          adbl_param_add__goe_than_d      (CapeUdc params, const CapeString name, const CapeDatetime* value);

__CAPE_LIBEX   void          adbl_param_add__less_than_n     (CapeUdc params, const CapeString name, number_t value);

__CAPE_LIBEX   void          adbl_param_add__less_than_d     (CapeUdc params, const CapeString name, const CapeDatetime* value);

                             /* less or equal */
__CAPE_LIBEX   void          adbl_param_add__loe_than_d      (CapeUdc params, const CapeString name, const CapeDatetime* value);

                             /* less or equal */
__CAPE_LIBEX   void          adbl_param_add__loe_than_n      (CapeUdc params, const CapeString name, number_t value);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void          adbl_param_add__between_n       (CapeUdc params, const CapeString name, number_t from, number_t until);

__CAPE_LIBEX   void          adbl_param_add__between_f       (CapeUdc params, const CapeString name, double from, double until);

__CAPE_LIBEX   void          adbl_param_add__between_d       (CapeUdc params, const CapeString name, const CapeDatetime* from, const CapeDatetime* until);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void          adbl_values_add__group_by       (CapeUdc values, const CapeString group);

//=============================================================================

#endif
