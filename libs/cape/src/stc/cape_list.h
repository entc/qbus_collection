#ifndef __CAPE_STC__LIST__H
#define __CAPE_STC__LIST__H 1

#include "sys/cape_export.h"

//=============================================================================

struct CapeList_s; typedef struct CapeList_s* CapeList;
struct CapeListNode_s; typedef struct CapeListNode_s* CapeListNode;

typedef void (__STDCALL *fct_cape_list_onDestroy) (void* ptr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeList          cape_list_new              (fct_cape_list_onDestroy);

__CAPE_LIBEX   void              cape_list_del              (CapeList*);

__CAPE_LIBEX   void              cape_list_clr              (CapeList);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeListNode      cape_list_push_back        (CapeList, void* data);

__CAPE_LIBEX   CapeListNode      cape_list_push_front       (CapeList, void* data);

__CAPE_LIBEX   void*             cape_list_pop_front        (CapeList);

__CAPE_LIBEX   void*             cape_list_pop_back         (CapeList);

__CAPE_LIBEX   unsigned long     cape_list_size             (CapeList);

__CAPE_LIBEX   int               cape_list_empty            (CapeList);

__CAPE_LIBEX   int               cape_list_hasContent       (CapeList);

__CAPE_LIBEX   void*             cape_list_position         (CapeList, int position);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              cape_list_node_replace     (CapeList, CapeListNode, void* data);

__CAPE_LIBEX   void*             cape_list_node_data        (CapeListNode);

__CAPE_LIBEX   void*             cape_list_node_extract     (CapeList, CapeListNode);

__CAPE_LIBEX   void              cape_list_node_erase       (CapeList, CapeListNode);

__CAPE_LIBEX   CapeListNode      cape_list_node_next        (CapeListNode);

__CAPE_LIBEX   CapeListNode      cape_list_node_front       (CapeList);

__CAPE_LIBEX   CapeListNode      cape_list_node_back        (CapeList);

__CAPE_LIBEX   void              cape_list_node_swap        (CapeListNode, CapeListNode);

//-----------------------------------------------------------------------------

typedef int (__STDCALL *fct_cape_list_onCompare) (void* ptr1, void* ptr2);

__CAPE_LIBEX   void              cape_list_sort             (CapeList, fct_cape_list_onCompare);

//-----------------------------------------------------------------------------

typedef void* (__STDCALL *fct_cape_list_onClone) (void* ptr);

__CAPE_LIBEX   CapeList           cape_list_clone           (CapeList, fct_cape_list_onClone);

//-----------------------------------------------------------------------------

typedef struct
{
  
  CapeListNode node;
  
  int position;
  
  int direction;
  
} CapeListCursor;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeListCursor*   cape_list_cursor_create    (CapeList, int direction);

__CAPE_LIBEX   void              cape_list_cursor_destroy   (CapeListCursor**);

__CAPE_LIBEX   void              cape_list_cursor_init      (CapeList, CapeListCursor*, int direction);

__CAPE_LIBEX   int               cape_list_cursor_next      (CapeListCursor*);

__CAPE_LIBEX   int               cape_list_cursor_prev      (CapeListCursor*);

__CAPE_LIBEX   void              cape_list_cursor_erase     (CapeList, CapeListCursor*);

__CAPE_LIBEX   void*             cape_list_cursor_extract   (CapeList, CapeListCursor*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeList          cape_list_slice_extract    (CapeList, CapeListNode nodeFrom, CapeListNode nodeTo);

__CAPE_LIBEX   void              cape_list_slice_insert     (CapeList, CapeListCursor*, CapeList* pslice);

//-----------------------------------------------------------------------------

#endif
