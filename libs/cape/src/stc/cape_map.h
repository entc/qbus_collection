#ifndef __CAPE_STC__MAP__H
#define __CAPE_STC__MAP__H 1

#include "sys/cape_export.h"

//=============================================================================

/* this class implements a balanced binary tree
 *
 * -> there is a simple implementation of a binary tree used as the basis tree
 * -> the balancing part is implemented by rotating of the tree nodes 
 *    following the RED / BLACK balanced tree principles
 * -> the cursor part is not a basic feature of balanced tree and was
 *    implemented following the cape_list API for cursor's
 * 
 * remarks: maybe in the future the BST and RB tree can be split into separated classes
 *          the rotating algorithm is complex and was not fully tested in cases of removing nodes
 * 
 * -> the result might be: that the tree is not optimal balanced
 */

//=============================================================================

struct CapeMap_s; typedef struct CapeMap_s* CapeMap;
struct CapeMapNode_s; typedef struct CapeMapNode_s* CapeMapNode;

typedef int   (__STDCALL *fct_cape_map_cmp)      (const void* a, const void* b, void* ptr);
typedef void  (__STDCALL *fct_cape_map_destroy)  (void* key, void* val);

//-----------------------------------------------------------------------------

int __STDCALL cape_map__compare__s (const void* a, const void* b, void* ptr);
int __STDCALL cape_map__compare__n (const void* a, const void* b, void* ptr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeMap           cape_map_new               (fct_cape_map_cmp, fct_cape_map_destroy, void* ptr_cmp);

__CAPE_LIBEX   void              cape_map_del               (CapeMap*);

__CAPE_LIBEX   void              cape_map_clr               (CapeMap);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeMapNode       cape_map_insert            (CapeMap, void* key, void* data);

__CAPE_LIBEX   CapeMapNode       cape_map_find              (CapeMap, const void* key);

__CAPE_LIBEX   CapeMapNode       cape_map_find_cmd          (CapeMap, const void* key, fct_cape_map_cmp);

__CAPE_LIBEX   void              cape_map_erase             (CapeMap, CapeMapNode);       // removes the node, calls the onDestroy callback and releases the node

__CAPE_LIBEX   CapeMapNode       cape_map_extract           (CapeMap, CapeMapNode);       // extracts the node from the container and returns it

__CAPE_LIBEX   void              cape_map_del_node          (CapeMap, CapeMapNode*);      // calls the onDestroy callback and releases the node

__CAPE_LIBEX   unsigned long     cape_map_size              (CapeMap);

__CAPE_LIBEX   CapeMapNode       cape_map_first             (CapeMap);

__CAPE_LIBEX   CapeMapNode       cape_map_last              (CapeMap);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void*             cape_map_node_value        (CapeMapNode);

__CAPE_LIBEX   void*             cape_map_node_key          (CapeMapNode);

__CAPE_LIBEX   void              cape_map_node_set          (CapeMapNode, void*);         // use with care

__CAPE_LIBEX   void              cape_map_node_del          (CapeMapNode*);               // don't calls the onDestroy method, only releases the memory

//-----------------------------------------------------------------------------

typedef void (__STDCALL *fct_cape_map__on_clone) (void* key_original, void* val_original, void** key_clone, void** val_clone);

__CAPE_LIBEX   CapeMap           cape_map_clone             (CapeMap, fct_cape_map__on_clone on_clone);

//-----------------------------------------------------------------------------

typedef struct
{  
  CapeMapNode node;    // the tree node
  int direction;       // the direction of the cursor
  int position;        // the current position
  
} CapeMapCursor;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeMapCursor*    cape_map_cursor_create     (CapeMap, int direction);

__CAPE_LIBEX   void              cape_map_cursor_destroy    (CapeMapCursor**);

__CAPE_LIBEX   void              cape_map_cursor_init       (CapeMap, CapeMapCursor*, int direction);

__CAPE_LIBEX   int               cape_map_cursor_next       (CapeMapCursor*);

__CAPE_LIBEX   int               cape_map_cursor_prev       (CapeMapCursor*);

__CAPE_LIBEX   void              cape_map_cursor_erase      (CapeMap, CapeMapCursor*);

__CAPE_LIBEX   CapeMapNode       cape_map_cursor_extract    (CapeMap, CapeMapCursor*);

//-----------------------------------------------------------------------------

#endif
