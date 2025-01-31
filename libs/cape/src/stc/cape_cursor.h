#ifndef __CAPE_STC__CURSOR__H
#define __CAPE_STC__CURSOR__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "stc/cape_str.h"

//=============================================================================

struct CapeCursor_s; typedef struct CapeCursor_s* CapeCursor;

//-----------------------------------------------------------------------------

                             /*

                              */
__CAPE_LIBEX CapeCursor      cape_cursor_new        (void);

                             /*

                              */
__CAPE_LIBEX void            cape_cursor_del        (CapeCursor*);

__CAPE_LIBEX void            cape_cursor_set        (CapeCursor, const char* bufdat, number_t buflen);

__CAPE_LIBEX number_t        cape_cursor_size       (CapeCursor);

__CAPE_LIBEX const char*     cape_cursor_data       (CapeCursor);

__CAPE_LIBEX int             cape_cursor__has_data  (CapeCursor, number_t len);

                             /* returns the bytes not scanned yet */
__CAPE_LIBEX number_t        cape_cursor_tail       (CapeCursor);

                             /* returns the pointer to the current position as char* */
__CAPE_LIBEX const char*     cape_cursor_dpos       (CapeCursor);

                             /* get the absolute number of the position, if = 0 -> no traversal was made */
__CAPE_LIBEX number_t        cape_cursor_apos       (CapeCursor);

                             /* get the current position and travers the len further */
__CAPE_LIBEX const char*     cape_cursor_tpos       (CapeCursor, number_t len);

                             /* tries to copy from cursor into bufdat, returns the amount of data copied */
__CAPE_LIBEX number_t        cape_cursor_copy       (CapeCursor, char* bufdat, number_t buflen);

//-----------------------------------------------------------------------------
// scan functions

__CAPE_LIBEX char*           cape_cursor_scan_s     (CapeCursor, number_t len);

__CAPE_LIBEX cape_uint8      cape_cursor_scan_08    (CapeCursor);

__CAPE_LIBEX cape_uint16     cape_cursor_scan_16    (CapeCursor, int network_byte_order);

__CAPE_LIBEX cape_uint32     cape_cursor_scan_32    (CapeCursor, int network_byte_order);

__CAPE_LIBEX cape_uint64     cape_cursor_scan_64    (CapeCursor, int network_byte_order);

__CAPE_LIBEX double          cape_cursor_scan_bd    (CapeCursor, int network_byte_order);

//-----------------------------------------------------------------------------

#endif
