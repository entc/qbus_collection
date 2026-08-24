#include "qdata.h"

// qcrypt includes
#include <qcrypt.h>
#include <qcrypt_file.h>

// cape includes
#include <sys/cape_file.h>
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

struct QData_s
{
    CapeString path;
    CapeString mime;
    CapeString uuid;
    
    // for legacy reasons
    CapeStream data;
};

//-----------------------------------------------------------------------------

QData qdata_new (CapeString* p_path)
{
    QData self = CAPE_NEW (struct QData_s);
        
    self->mime = NULL;
    self->uuid = NULL;
    self->data = NULL;

    // allow to have an empty path
    if (p_path)
    {
        self->path = cape_str_mv (p_path);

        // extend the path
        {
            // construct a new path
            CapeString path_doca = cape_fs_path_merge (self->path, "qdata");
            
            cape_str_replace_mv (&(self->path), &path_doca);
        }
    }
    else
    {
        self->path = NULL;
    }
    
    return self;
}

//-----------------------------------------------------------------------------

void qdata_del (QData* p_self)
{
    if (*p_self)
    {
        QData self = *p_self;

        cape_stream_del (&(self->data));
        
        cape_str_del (&(self->uuid));
        cape_str_del (&(self->mime));
        cape_str_del (&(self->path));
        
        CAPE_DEL (p_self, struct QData_s);
    }
}

//-----------------------------------------------------------------------------

CapeString qdata_dform_to_text (const CapeString designation)
{
    CapeString ret = NULL;
    
    number_t pos;

    if (!cape_str_begins (designation, "data:"))
    {
        return NULL;
    }
    
    if (cape_str_find (designation + 5, ";base64,", &pos))
    {
        CapeString mime = cape_str_sub (designation + 5, pos);
        
        if (cape_str_equal (mime, "text/plain"))
        {
            // convert this into plain text
            CapeStream s = qcrypt__decode_base64_s (designation + 5 + pos + 8);

            // as long the content si really only text this is safe
            ret = cape_stream_to_str (&s);
        }
        
        cape_str_del (&mime);
    }
    
    return ret;
}

//-----------------------------------------------------------------------------

static int qdata__designation_to_mimeuuid (QData self, const CapeString designation, CapeErr err)
{
    number_t pos;

    // clear previous state
    cape_str_del (&(self->uuid));
    cape_str_del (&(self->mime));

    // clear previous buffer
    cape_stream_del (&(self->data));

    if (!cape_str_begins (designation, "data:"))
    {
        cape_log_fmt (CAPE_LL_TRACE, "QDATA", "convert", "designation: %s", designation);
        
        return cape_err_set(err, CAPE_ERR_RUNTIME, "designation has no 'data' format");
    }
    
    if (cape_str_find (designation + 5, ";qdata,", &pos))
    {
        self->uuid = cape_str_cp (designation + 5 + pos + 7);
        self->mime = cape_str_sub (designation + 5, pos);
        
        return CAPE_ERR_NONE;
    }
    else if (cape_str_find (designation + 5, ";base64,", &pos))
    {
        self->data = qcrypt__decode_base64_s (designation + 5 + pos + 8);
        
        if (NULL == self->data)
        {
            return cape_err_set (err, CAPE_ERR_RUNTIME, "can't decode base64");
        }

        {
            CapeString mime = cape_str_sub (designation + 5, pos);

            // use mime type withtin the buffer
            cape_stream_mime_set (self->data, mime);
            
            cape_str_del (&mime);
        }
        
        return CAPE_ERR_NONE;
    }
    else
    {
        return cape_err_set (err, CAPE_ERR_RUNTIME, "designation must contain ';qdata,' or ';base64,'");
    }
}

//-----------------------------------------------------------------------------

static CapeString qdata__mimeuuid_to_designation (QData self)
{
    if (self->data)
    {
        return cape_stream_serialize (self->data, qcrypt__stream_base64_encode);
    }
    else
    {
        return cape_str_fmt ("data:%s;qdata,%s", self->mime, self->uuid);
    }
}

//-----------------------------------------------------------------------------

static int qdata__construct_filepath (QData self, CapeString* p_file, CapeErr err)
{
    // local objects
    CapeString file = NULL;

    if (NULL == self->path)
    {
        return cape_err_set (err, CAPE_ERR_RUNTIME, "path is not set");
    }

    if (NULL == self->uuid)
    {
        return cape_err_set (err, CAPE_ERR_RUNTIME, "uuid is not set");
    }

    // create path, only if not exists
    if (cape_fs_path_create_xe (self->path, NULL, err))
    {
        return cape_err_code (err);
    }

    // combine the path with the name
    file = cape_fs_path_merge (self->path, self->uuid);
    
    // transfer
    cape_str_replace_mv (p_file, &file);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int qdata__save_file (QData self, const CapeString vsec, const char* bufdat, number_t buflen, const CapeString file_path, CapeErr err)
{
    int res;
    
    // local objects
    QCryptFile file_crypt = NULL;
    
    cape_log_fmt (CAPE_LL_TRACE, "QDATA", "save", "save file: %s", file_path);
    
    // allocate object
    file_crypt = qcrypt_file_new (file_path);

    // create the crypt context and open the file
    res = qcrypt_file_encrypt (file_crypt, vsec, err);
    if (res)
    {
        goto exit_and_cleanup;
    }
    
    res = qcrypt_file_write (file_crypt, bufdat, buflen, err);
    if (res)
    {
        goto exit_and_cleanup;
    }

    res = qcrypt_file_finalize (file_crypt, err);
    if (res)
    {
        goto exit_and_cleanup;
    }

    res = CAPE_ERR_NONE;
    
exit_and_cleanup:
    
    qcrypt_file_del (&file_crypt);
    return res;
}

//-----------------------------------------------------------------------------

static int qdata__load_file (QData self, const CapeString current_file, const CapeString vsec, CapeStream s, CapeErr err)
{
    int res;

    // local objects
    QCryptDecrypt file_crypt = NULL;
    
    // allocate object
    file_crypt = qcrypt_decrypt_new (NULL, current_file, vsec);

    // create the crypt context and open the file
    res = qcrypt_decrypt_open (file_crypt, err);
    if (res)
    {
        goto exit_and_cleanup;
    }
        
    while (TRUE)
    {
        number_t decrypted_bytes;
        
        cape_stream_cap (s, 1024);
        
        decrypted_bytes = qcrypt_decrypt_next (file_crypt, cape_stream_pos (s), 1024);
        if (decrypted_bytes)
        {
            cape_stream_set (s, decrypted_bytes);
        }
        else
        {
            break;
        }
    }

    res = CAPE_ERR_NONE;
    
exit_and_cleanup:
    
    qcrypt_decrypt_del (&file_crypt);
    return res;
}

//-----------------------------------------------------------------------------

int qdata_set__load_enc (QData self, const CapeString vsec, const CapeString designation_encrypted, CapeErr err)
{
    int res;
    
    // local objects
    CapeString h2 = NULL;

    if (NULL == designation_encrypted)
    {
        return CAPE_ERR_NONE;
    }
    
    h2 = qcrypt__decrypt (vsec, designation_encrypted, err);
    if (NULL == h2)
    {
        return cape_err_code (err);
    }
    
    res = qdata_set__load (self, h2, err);
    
    cape_str_del (&h2);
    return res;
}

//-----------------------------------------------------------------------------

int qdata_set__load (QData self, const CapeString designation, CapeErr err)
{
    int res;
    
    // convert designation to internal members of mime and uuid
    res = qdata__designation_to_mimeuuid (self, designation, err);

    if (self->data)
    {
        cape_log_fmt (CAPE_LL_TRACE, "QDATA", "load", "inline mime = %s size = %u", cape_stream_mime_get(self->data), cape_stream_size(self->data));
    }
    else
    {
        cape_log_fmt (CAPE_LL_TRACE, "QDATA", "load", "mime = %s, uuid = %s", self->mime, self->uuid);
    }
    
    return res;
}

//-----------------------------------------------------------------------------

int qdata_set__load_m__mv  (QData self, CapeStream* p_designation, CapeErr err)
{
    self->data = cape_stream_mv (p_designation);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qdata_rm (QData self, const CapeString vsec, const CapeString designation_encrypted, CapeErr err)
{
    int res;
    
    // local objects
    CapeString file = NULL;
    
    res = qdata_set__load_enc (self, vsec, designation_encrypted, err);
    if (res)
    {
        // ignore error
        cape_err_clr (err);

        res = CAPE_ERR_NONE;
        goto cleanup_and_exit;
    }

    if (self->data)
    {
        res = CAPE_ERR_NONE;
        goto cleanup_and_exit;
    }
    
    // construct the filepath to the real file
    res = qdata__construct_filepath (self, &file, err);
    if (res)
    {
        // ignore error
        cape_err_clr (err);

        res = CAPE_ERR_NONE;
        goto cleanup_and_exit;
    }

    // try to remove the file
    res = cape_fs_file_rm (file, err);
    if (res)
    {
        // ignore error
        cape_err_clr (err);
    }
    
    cape_log_fmt (CAPE_LL_TRACE, "QDATA", "rm", "file removed: %s", file);
    
    res = CAPE_ERR_NONE;
    
cleanup_and_exit:
    
    cape_str_del (&file);
    return res;
}

//-----------------------------------------------------------------------------

CapeString qdata_gen (QData self)
{
    return qdata__mimeuuid_to_designation (self);
}

//-----------------------------------------------------------------------------

CapeString qdata_gen__enc (QData self, const CapeString vsec, CapeErr err)
{
    // local objects
    CapeString designation = qdata_gen (self);
    CapeString h2 = NULL;
    
    h2 = qcrypt__encrypt (vsec, designation, err);

    cape_str_del (&designation);
    return h2;
}

//-----------------------------------------------------------------------------

CapeStream qdata_ext_data__m (QData self, const CapeString vsec, CapeErr err)
{
    CapeStream ret = NULL;

    // local objects
    CapeString file = NULL;

    if (self->data)
    {
        ret = cape_stream_mv (&(self->data));
    }
    else
    {
        if (NULL == vsec)
        {
            goto cleanup_and_exit;
        }
        
        // construct the filepath to the real file
        if (qdata__construct_filepath (self, &file, err))
        {
            goto cleanup_and_exit;
        }

        cape_log_fmt (CAPE_LL_TRACE, "QDATA", "ext", "file = %s", file);

        // instanciate a new stream object
        ret = cape_stream_new ();
        
        // try to load and encrypt the file
        if (qdata__load_file (self, file, vsec, ret, err))
        {
            cape_stream_del (&ret);
            goto cleanup_and_exit;
        }

        cape_stream_mime_set (ret, self->mime);
    }

cleanup_and_exit:
    
    cape_str_del (&file);
    return ret;
}

//-----------------------------------------------------------------------------

CapeString qdata_ext_data__s (QData self, const CapeString vsec, CapeErr err)
{
    CapeString ret = NULL;

    // local objects
    CapeString file = NULL;
    CapeStream data = NULL;

    if (self->data)
    {
        ret = cape_stream_serialize (self->data, qcrypt__stream_base64_encode);
    }
    else
    {
        if (NULL == vsec)
        {
            goto cleanup_and_exit;
        }
        
        // construct the filepath to the real file
        if (qdata__construct_filepath (self, &file, err))
        {
            goto cleanup_and_exit;
        }

        cape_log_fmt (CAPE_LL_TRACE, "QDATA", "ext", "file = %s", file);

        // instanciate a new stream object
        data = cape_stream_new ();
        
        // try to load and encrypt the file
        if (qdata__load_file (self, file, vsec, data, err))
        {
            goto cleanup_and_exit;
        }

        cape_stream_mime_set (data, self->mime);

        ret = cape_stream_serialize (data, qcrypt__stream_base64_encode);
    }

cleanup_and_exit:
    
    cape_stream_del (&data);
    cape_str_del (&file);
    return ret;
}

//-----------------------------------------------------------------------------

int qdata_set (QData self, const CapeString vsec, const char* bufdat, number_t buflen, const CapeString mime, CapeErr err)
{
    int res;
    
    // local objects
    CapeString file = NULL;
    
    // cleanup previous values
    cape_str_del (&(self->uuid));
    cape_str_del (&(self->mime));

    // reset inline state
    cape_stream_del (&(self->data));

    if (buflen > 2000)
    {
        // store the mime type
        self->mime = cape_str_cp (mime);

        // create a new uuid for designation
        self->uuid = cape_str_uuid ();
        
        res = qdata__construct_filepath (self, &file, err);
        if (res)
        {
            // cleanup values
            cape_str_del (&(self->uuid));
            cape_str_del (&(self->mime));

            goto cleanup_and_exit;
        }

        res = qdata__save_file (self, vsec, bufdat, buflen, file, err);
        if (res)
        {
            // cleanup values
            cape_str_del (&(self->uuid));
            cape_str_del (&(self->mime));

            goto cleanup_and_exit;
        }
    }
    else
    {
        self->data = cape_stream_new ();

        cape_stream_append_buf (self->data, bufdat, buflen);
        
        // mime type is stored within the buffer
        cape_stream_mime_set (self->data, mime);

        res = CAPE_ERR_NONE;
    }

cleanup_and_exit:
    
    cape_str_del (&file);
    return res;
}

//-----------------------------------------------------------------------------

int qdata_set_s__cp (QData self, const CapeString vsec, const CapeString designation, const CapeString previous_designation_encrypted, CapeErr err)
{
    number_t pos;
    
    int res = qdata_rm (self, vsec, previous_designation_encrypted, err);
    if (res)
    {
        return res;
    }
    
    if (cape_str_begins (designation, "data:") && cape_str_find (designation + 5, ";base64,", &pos))
    {
        CapeStream data = qcrypt__decode_base64_s (designation + 5 + pos + 8);
        
        if (NULL == data)
        {
            return cape_err_set (err, CAPE_ERR_RUNTIME, "can't decode base64");
        }
        else
        {
            CapeString mime = cape_str_sub (designation + 5, pos);

            res = qdata_set (self, vsec, cape_stream_data (data), cape_stream_size (data), mime, err);
            
            cape_stream_del (&data);
            cape_str_del (&mime);
            
            return res;
        }
    }
    else
    {
        cape_str_del (&(self->uuid));
        cape_str_del (&(self->mime));

        return cape_err_set (err, CAPE_ERR_RUNTIME, "invalid or unknown designation");
    }
}

//-----------------------------------------------------------------------------

int qdata_set_m__cp (QData self, const CapeString vsec, const CapeStream data, const CapeString previous_designation_encrypted, CapeErr err)
{
    int res = qdata_rm (self, vsec, previous_designation_encrypted, err);
    if (res)
    {
        return res;
    }
    
    return qdata_set (self, vsec, cape_stream_data (data), cape_stream_size (data), cape_stream_mime_get (data), err);
}

//-----------------------------------------------------------------------------

QData qdata_factory (CapeUdc item, CapeString* p_path, CapeErr err)
{
    QData ret = NULL;
    
    CapeUdc qdata_node;
    CapeUdc img_node;

    // local objects
    CapeString designation = NULL;
    CapeStream s = NULL;

    // support img node
    img_node = cape_udc_get (item, "img");
    if (img_node)
    {
        cape_log_msg (CAPE_LL_TRACE, "QDATA", "factory", "use image node");

        switch (cape_udc_type (img_node))
        {
            case CAPE_UDC_STREAM:
            {
                // extract the stream from the UDC node
                s = cape_udc_m_mv (img_node);
                
                // create and set the return value
                ret = qdata_new (p_path);

                if (qdata_set__load_m__mv (ret, &s, err))
                {
                    // cleanup qdata
                    qdata_del (&ret);
                    goto cleanup_and_exit;
                }

                goto cleanup_and_exit;
            }
            default:
            {
                cape_log_msg (CAPE_LL_WARN, "QDATA", "factory", "not supported type found in image node");
                goto cleanup_and_exit;
            }
        }
    }

    // get the qdata node object
    qdata_node = cape_udc_get (item, "qdata");
    if (qdata_node)
    {
        cape_log_msg (CAPE_LL_TRACE, "QDATA", "factory", "use qdata node");

        // first gather the designation of the qdata object
        switch (cape_udc_type (qdata_node))
        {
            case CAPE_UDC_STREAM:
            {
                // TODO: support a designation as direct stream
                designation = cape_stream_serialize (cape_udc_m (qdata_node), qcrypt__stream_base64_encode);
                break;
            }
            case CAPE_UDC_STRING:
            {
                designation = cape_udc_s_mv (qdata_node, NULL);
                break;
            }
            default:
            {
                cape_log_msg (CAPE_LL_WARN, "QDATA", "factory", "not supported type found in qdata node");
                goto cleanup_and_exit;
            }
        }

        if (NULL == designation)
        {
            cape_err_set (err, CAPE_ERR_RUNTIME, "ERR.NO_DESIGNATION");
            goto cleanup_and_exit;
        }
        
        // create and set the return value
        ret = qdata_new (p_path);
        
        if (qdata_set__load (ret, designation, err))
        {
            // cleanup
            qdata_del (&ret);
            goto cleanup_and_exit;
        }

        goto cleanup_and_exit;
    }

    cape_err_set (err, CAPE_ERR_RUNTIME, "ERR.NO_DESIGNATION");
    
cleanup_and_exit:
    
    cape_stream_del (&s);
    cape_str_del (&designation);
    return ret;
}

//-----------------------------------------------------------------------------
