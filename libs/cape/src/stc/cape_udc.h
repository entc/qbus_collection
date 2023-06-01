#ifndef __CAPE_STC__UDC__H
#define __CAPE_STC__UDC__H 1

#include "sys/cape_export.h"
#include "sys/cape_time.h"
#include "stc/cape_str.h"
#include "stc/cape_list.h"
#include "stc/cape_stream.h"

//-----------------------------------------------------------------------------

#define CAPE_UDC_UNDEFINED    0
#define CAPE_UDC_NODE         1
#define CAPE_UDC_LIST         2
#define CAPE_UDC_STRING       3
#define CAPE_UDC_NUMBER       4
#define CAPE_UDC_FLOAT        5
#define CAPE_UDC_BOOL         6
#define CAPE_UDC_NULL         7
#define CAPE_UDC_DATETIME     8
#define CAPE_UDC_STREAM       9

//=============================================================================

struct CapeUdc_s; typedef struct CapeUdc_s* CapeUdc;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc              cape_udc_new              (u_t type, const CapeString name);

__CAPE_LIBEX   void                 cape_udc_del              (CapeUdc*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const CapeString     cape_udc_name             (const CapeUdc);

__CAPE_LIBEX   u_t                  cape_udc_type             (const CapeUdc);

__CAPE_LIBEX   void*                cape_udc_data             (const CapeUdc);   // returns the pointer of type address

__CAPE_LIBEX   number_t             cape_udc_size             (const CapeUdc);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc              cape_udc_cp               (const CapeUdc);

__CAPE_LIBEX   CapeUdc              cape_udc_mv               (CapeUdc*);

__CAPE_LIBEX   void                 cape_udc_replace_cp       (CapeUdc*, const CapeUdc replace_with_copy);

__CAPE_LIBEX   void                 cape_udc_replace_mv       (CapeUdc*, CapeUdc* replace_with);

__CAPE_LIBEX   void                 cape_udc_merge_mv         (CapeUdc, CapeUdc*);

__CAPE_LIBEX   void                 cape_udc_merge_cp         (CapeUdc, const CapeUdc);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc              cape_udc_add              (CapeUdc, CapeUdc*);

__CAPE_LIBEX   CapeUdc              cape_udc_add_name         (CapeUdc, CapeUdc*, const CapeString name);

__CAPE_LIBEX   CapeUdc              cape_udc_get              (CapeUdc, const CapeString name);

__CAPE_LIBEX   CapeUdc              cape_udc_ext              (CapeUdc, const CapeString name);

__CAPE_LIBEX   void                 cape_udc_rm               (CapeUdc, const CapeString name);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void                 cape_udc_set_s_cp         (CapeUdc, const CapeString val);

__CAPE_LIBEX   void                 cape_udc_set_s_mv         (CapeUdc, CapeString* p_val);

__CAPE_LIBEX   void                 cape_udc_set_n            (CapeUdc, number_t val);

__CAPE_LIBEX   void                 cape_udc_set_f            (CapeUdc, double val);

__CAPE_LIBEX   void                 cape_udc_set_b            (CapeUdc, int val);

__CAPE_LIBEX   void                 cape_udc_set_d            (CapeUdc, const CapeDatetime* val);

__CAPE_LIBEX   void                 cape_udc_set_m_cp         (CapeUdc, const CapeStream val);

__CAPE_LIBEX   void                 cape_udc_set_m_mv         (CapeUdc, CapeStream* p_val);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const CapeString     cape_udc_s                (CapeUdc, const CapeString alt);

__CAPE_LIBEX   CapeString           cape_udc_s_mv             (CapeUdc, const CapeString alt);

__CAPE_LIBEX   number_t             cape_udc_n                (CapeUdc, number_t alt);

__CAPE_LIBEX   double               cape_udc_f                (CapeUdc, double alt);

__CAPE_LIBEX   int                  cape_udc_b                (CapeUdc, int alt);

__CAPE_LIBEX   const CapeDatetime*  cape_udc_d                (CapeUdc, const CapeDatetime* alt);

__CAPE_LIBEX   CapeDatetime*        cape_udc_d_mv             (CapeUdc, const CapeDatetime* alt);

__CAPE_LIBEX   CapeList             cape_udc_list_mv          (CapeUdc);

__CAPE_LIBEX   const CapeStream     cape_udc_m                (CapeUdc);

__CAPE_LIBEX   CapeStream           cape_udc_m_mv             (CapeUdc);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc              cape_udc_add_s_cp         (CapeUdc, const CapeString name, const CapeString val);

__CAPE_LIBEX   CapeUdc              cape_udc_add_s_mv         (CapeUdc, const CapeString name, CapeString* p_val);

__CAPE_LIBEX   CapeUdc              cape_udc_add_n            (CapeUdc, const CapeString name, number_t val);

__CAPE_LIBEX   CapeUdc              cape_udc_add_f            (CapeUdc, const CapeString name, double val);

__CAPE_LIBEX   CapeUdc              cape_udc_add_b            (CapeUdc, const CapeString name, int val);

__CAPE_LIBEX   CapeUdc              cape_udc_add_d            (CapeUdc, const CapeString name, const CapeDatetime* val);

__CAPE_LIBEX   CapeUdc              cape_udc_add_z            (CapeUdc, const CapeString name);   // NULL value

__CAPE_LIBEX   CapeUdc              cape_udc_add_node         (CapeUdc, const CapeString name);

__CAPE_LIBEX   CapeUdc              cape_udc_add_list         (CapeUdc, const CapeString name);

__CAPE_LIBEX   CapeUdc              cape_udc_add_m_cp         (CapeUdc, const CapeString name, const CapeStream val);

__CAPE_LIBEX   CapeUdc              cape_udc_add_m_mv         (CapeUdc, const CapeString name, CapeStream* p_val);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const CapeString     cape_udc_get_s            (CapeUdc, const CapeString name, const CapeString alt);

__CAPE_LIBEX   number_t             cape_udc_get_n            (CapeUdc, const CapeString name, number_t alt);

__CAPE_LIBEX   double               cape_udc_get_f            (CapeUdc, const CapeString name, double alt);

__CAPE_LIBEX   int                  cape_udc_get_b            (CapeUdc, const CapeString name, int alt);

__CAPE_LIBEX   const CapeDatetime*  cape_udc_get_d            (CapeUdc, const CapeString name, const CapeDatetime* alt);

__CAPE_LIBEX   const CapeStream     cape_udc_get_m            (CapeUdc, const CapeString name);

__CAPE_LIBEX   CapeUdc              cape_udc_get_node         (CapeUdc, const CapeString name);

__CAPE_LIBEX   CapeUdc              cape_udc_get_list         (CapeUdc, const CapeString name);

__CAPE_LIBEX   CapeUdc              cape_udc_get_first        (CapeUdc);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeString           cape_udc_ext_s            (CapeUdc, const CapeString name);

__CAPE_LIBEX   CapeDatetime*        cape_udc_ext_d            (CapeUdc, const CapeString name);

__CAPE_LIBEX   CapeStream           cape_udc_ext_m            (CapeUdc, const CapeString name);

__CAPE_LIBEX   CapeUdc              cape_udc_ext_node         (CapeUdc, const CapeString name);

__CAPE_LIBEX   CapeUdc              cape_udc_ext_list         (CapeUdc, const CapeString name);

__CAPE_LIBEX   CapeUdc              cape_udc_ext_first        (CapeUdc);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void                 cape_udc_put_s_cp         (CapeUdc, const CapeString name, const CapeString val);

__CAPE_LIBEX   void                 cape_udc_put_s_mv         (CapeUdc, const CapeString name, CapeString* p_val);

__CAPE_LIBEX   void                 cape_udc_put_n            (CapeUdc, const CapeString name, number_t val);

__CAPE_LIBEX   void                 cape_udc_put_f            (CapeUdc, const CapeString name, double val);

__CAPE_LIBEX   void                 cape_udc_put_b            (CapeUdc, const CapeString name, int val);

__CAPE_LIBEX   void                 cape_udc_put_m_cp         (CapeUdc, const CapeString name, const CapeStream val);

__CAPE_LIBEX   void                 cape_udc_put_m_mv         (CapeUdc, const CapeString name, CapeStream* p_val);

__CAPE_LIBEX   void                 cape_udc_put_node_cp      (CapeUdc, const CapeString name, CapeUdc);

__CAPE_LIBEX   void                 cape_udc_put_node_mv      (CapeUdc, const CapeString name, CapeUdc*);

//-----------------------------------------------------------------------------

/* the following group of functions are converting the udc object into a specific type
    -> depending on the original type a transmutation will be performed
    -> at the moment only simple types are supported
    -> the return value [TRUE|FALSE] indicates that the transmutation was possible
       and has been performed
 */

__CAPE_LIBEX   int                  cape_udc_cto_s            (CapeUdc);  // into string

__CAPE_LIBEX   int                  cape_udc_cto_n            (CapeUdc);  // into number

__CAPE_LIBEX   int                  cape_udc_cto_f            (CapeUdc);  // into float

__CAPE_LIBEX   int                  cape_udc_cto_b            (CapeUdc);  // into boolean

__CAPE_LIBEX   int                  cape_udc_cto_d            (CapeUdc);  // into datetime

//-----------------------------------------------------------------------------

typedef int (__STDCALL *fct_cape_udc__on_compare) (CapeUdc obj1, CapeUdc obj2);

__CAPE_LIBEX   void                 cape_udc_sort_list        (CapeUdc, fct_cape_udc__on_compare);

                                    /* only works for list */
__CAPE_LIBEX   void                 cape_udc_add_n__max       (CapeUdc, const CapeString name, number_t val, number_t max_length);

//-----------------------------------------------------------------------------

typedef struct
{
  
  CapeUdc item;
  
  int position;
  
  int direction;
  
  void* data;   // for internal use (don't change it)
  u_t type;     // for internal use (don't change it)
  
} CapeUdcCursor;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdcCursor*       cape_udc_cursor_new       (CapeUdc, int direction);

__CAPE_LIBEX   void                 cape_udc_cursor_del       (CapeUdcCursor**);

__CAPE_LIBEX   int                  cape_udc_cursor_next      (CapeUdcCursor*);

__CAPE_LIBEX   int                  cape_udc_cursor_prev      (CapeUdcCursor*);

__CAPE_LIBEX   void                 cape_udc_cursor_rm        (CapeUdc, CapeUdcCursor*);

__CAPE_LIBEX   CapeUdc              cape_udc_cursor_ext       (CapeUdc, CapeUdcCursor*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void                 cape_udc_print            (const CapeUdc);

//=============================================================================

#endif
