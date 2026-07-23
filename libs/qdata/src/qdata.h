#ifndef QDATA_H
#define QDATA_H 1

// cape includes
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <sys/cape_queue.h>

//-----------------------------------------------------------------------------

struct QData_s; typedef struct QData_s* QData;

                                    /* constructor: create a new instance of the qdata class */
__CAPE_LIBEX     QData              qdata_new              (CapeString* p_path);

                                    /* destructor: cleans and frees all memory */
__CAPE_LIBEX     void               qdata_del              (QData*);

//-----------------------------------------------------------------------------

                                    /* generate the designation */
__CAPE_LIBEX     CapeString         qdata_gen              (QData);

                                    /* generate the encrypted designation */
__CAPE_LIBEX     CapeString         qdata_gen__enc         (QData, const CapeString vsec, CapeErr err);

                                    /* retrieve the data content as raw byte stream */
__CAPE_LIBEX     CapeStream         qdata_ext_data__m      (QData, const CapeString vsec, CapeErr err);

                                    /* retrieve the data content as data form */
__CAPE_LIBEX     CapeString         qdata_ext_data__s      (QData, const CapeString vsec, CapeErr err);

                                    /* set a new data content, a new data name will be generated */
__CAPE_LIBEX     int                qdata_set_s__cp        (QData, const CapeString vsec, const CapeString designation, const CapeString previous_designation_encrypted, CapeErr);

                                    /* set a new data content, a new data name will be generated */
__CAPE_LIBEX     int                qdata_set_m__cp        (QData, const CapeString vsec, const CapeStream data, const CapeString previous_designation_encrypted, CapeErr);

                                    /* set the data string by loading it from the file system, input designation muste be encrypted  */
__CAPE_LIBEX     int                qdata_set__load        (QData, const CapeString designation_encrypted, CapeErr);

                                    /* set the data string by loading it from the file system */
__CAPE_LIBEX     int                qdata_set__load_enc    (QData, const CapeString vsec, const CapeString designation_encrypted, CapeErr);

                                    /* removes the file with the designation */
__CAPE_LIBEX     int                qdata_rm               (QData, const CapeString vsec, const CapeString designation_encrypted, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     CapeString         qdata__dform_to_text   (const CapeString vsec);

//-----------------------------------------------------------------------------

#endif
