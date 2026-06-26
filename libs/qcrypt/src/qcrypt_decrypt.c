#include "qcrypt_decrypt.h"
#include "qcrypt_aes.h"
#include "qcrypt.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>
#include <stc/cape_cursor.h>

#if defined __WINDOWS_OS

#include <windows.h>
#include <wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

#else

#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#endif

//-----------------------------------------------------------------------------

struct QDecryptBase64_s
{
  CapeStream product;
  
#if defined __WINDOWS_OS
  
  
#else
  
  EVP_ENCODE_CTX* ctx;

#endif
};

//-----------------------------------------------------------------------------

QDecryptBase64 qdecrypt_base64_new (CapeStream r_product)
{
  QDecryptBase64 self = CAPE_NEW (struct QDecryptBase64_s);
  
  self->product = r_product;
  
#if defined __WINDOWS_OS
  
  
#else
  
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  
  self->ctx = CAPE_NEW (EVP_ENCODE_CTX);
  
#else
  
  // it only allocates the memory
  self->ctx = EVP_ENCODE_CTX_new ();
  
#endif
  
  // this is important to initialize the structure
  EVP_DecodeInit (self->ctx);
  
#endif
  
  return self;
}

//-----------------------------------------------------------------------------

void qdecrypt_base64_del (QDecryptBase64* p_self)
{
  if (*p_self)
  {
    QDecryptBase64 self = *p_self;
    
#if defined __WINDOWS_OS
    
    
#else
    
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    
    CAPE_DEL (&(self->ctx), EVP_ENCODE_CTX);
    
#else
    
    EVP_ENCODE_CTX_free (self->ctx);
    
#endif
    
#endif
    
    CAPE_DEL (p_self, struct QDecryptBase64_s);
  }
}

//-----------------------------------------------------------------------------

number_t qdecrypt_base64_size (number_t size)
{
  return size;
}

//-----------------------------------------------------------------------------

int qdecrypt_base64_process (QDecryptBase64 self, const char* bufdat, number_t buflen, CapeErr err)
{
#if defined __WINDOWS_OS
  
  
#else
  
  int len;
  
  // increase the stream to fit the expected size of the base64 serialization
  cape_stream_cap (self->product, qdecrypt_base64_size (buflen));
  
  // call the crypt librarie's function
  EVP_DecodeUpdate (self->ctx, (unsigned char*)cape_stream_pos (self->product), &len, (const unsigned char*)bufdat, buflen);
  
  // adjust the stream to the correct written bytes
  cape_stream_set (self->product, len);
  
  return CAPE_ERR_NONE;
  
#endif
}

//-----------------------------------------------------------------------------

int qdecrypt_base64_finalize (QDecryptBase64 self, CapeErr err)
{
#if defined __WINDOWS_OS
  
  
#else
  
  int len;
  
  // base64 alignment is 24bit, use 5 bytes to be on the safe side
  cape_stream_cap (self->product, 5);
  
  // the base64 encoding needs to be finalized to align to a certain amount of bytes
  EVP_DecodeFinal (self->ctx, (unsigned char*)cape_stream_pos (self->product), &len);
  
  // adjust the stream to the correct written bytes
  cape_stream_set (self->product, len);
  
  return CAPE_ERR_NONE;
  
#endif
}

//-----------------------------------------------------------------------------

struct QDecryptAES_s
{
    CapeStream product;

#if defined __WINDOWS_OS

    
#else

    EVP_CIPHER_CTX* ctx;

    int blocksize;
    
    number_t cypher_type;
    number_t padding_type;
    number_t key_type;
    
    number_t total_size;
    
    // this are our secrets
    
    CapeString secret;
    
    QCryptAESKeys keys;
      
    CapeStream rolling_buffer;
    int tag_len;

#endif
};

//-----------------------------------------------------------------------------

QDecryptAES qdecrypt_aes_new (CapeStream r_product, number_t cypher_type, number_t padding_type, const CapeString secret, number_t key_type)
{
    QDecryptAES self = CAPE_NEW (struct QDecryptAES_s);
    
    self->product = r_product;

#if defined __WINDOWS_OS

  
#else

#if OPENSSL_VERSION_NUMBER < 0x10100000L
  
    self->ctx = CAPE_NEW (EVP_CIPHER_CTX);

#else

    // it only allocates the memory
    self->ctx = EVP_CIPHER_CTX_new ();

#endif

    self->blocksize = 0;
    self->total_size = 0;
    
    // cipher settings
    self->cypher_type = cypher_type;
    self->padding_type = padding_type;
    self->key_type = key_type;

    // key settings
    self->secret = cape_str_cp (secret);

    self->keys = NULL;
    self->rolling_buffer = NULL;
    self->tag_len = 0;
    
#endif
  
    return self;
}

//-----------------------------------------------------------------------------

void qdecrypt_aes_del (QDecryptAES* p_self)
{
    if (*p_self)
    {
        QDecryptAES self = *p_self;
    
#if defined __WINDOWS_OS

  
#else

        // call the cleanup to free memory
        EVP_CIPHER_CTX_cleanup (self->ctx);

#if OPENSSL_VERSION_NUMBER < 0x10100000L

        // free the object struct
        CAPE_FREE (self->ctx);

#else
    
        // free the object struct
        EVP_CIPHER_CTX_free (self->ctx);

#endif
    
        cape_stream_del (&(self->rolling_buffer));
        qcrypt_aes_keys_del (&(self->keys));
        cape_str_del (&(self->secret));

#endif
    
        CAPE_DEL (p_self, struct QDecryptAES_s);
    }
}

//-----------------------------------------------------------------------------

int qdecrypt_aes__cfb (QDecryptAES self, const EVP_CIPHER* cypher, const char* bufdat, number_t buflen, number_t* p_buffer_offset, CapeErr err)
{
    int res;
          
    switch (self->key_type)
    {
      case QCRYPT_KEY_SHA256:
      {
        self->keys = qcrypt_aes_keys_new__sha256 (self->secret, cypher, err);
        break;
      }
      case QCRYPT_KEY_PASSPHRASE_MD5:
      {
        self->keys = qcrypt_aes_keys_new__md5_de (self->secret, cypher, bufdat, buflen, err);
        
        // set the correct offset
        *p_buffer_offset = 16;
        
        break;
      }
      case QCRYPT_PADDING_ZEROS:
      {
        self->keys = qcrypt_aes_keys_new__padding_zero (self->secret, cypher);
        break;
      }
      case QCRYPT_PADDING_ANSI_X923:
      {
        self->keys = qcrypt_aes_keys_new__ansiX923 (self->secret, cypher);
        break;
      }
      case QCRYPT_PADDING_PKCS7:
      {
        self->keys = qcrypt_aes_keys_new__padding_pkcs7 (self->secret, cypher);
        break;
      }
    }
    
    if (self->keys == NULL)
    {
      return cape_err_set (err, CAPE_ERR_WRONG_STATE, "encoding of secret failed");
    }
      
    res = EVP_DecryptInit_ex (self->ctx, cypher, NULL, qcrypt_aes_key (self->keys), qcrypt_aes_iv (self->keys));
    
    if (res == 0)
    {
      return qcrypt_aes__handle_error (self->ctx, err);
    }
    
    // check for the blocksize
    self->blocksize = EVP_CIPHER_CTX_block_size (self->ctx);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qdecrypt_aes__gcm_256_pbkdf2 (QDecryptAES self, CapeCursor cursor, CapeErr err)
{
    int res;
    
    // local objects
    int iterations = 0;
    CapeString salt = NULL;
    CapeString iv = NULL;

    // check if there is enough data to parse the first 4 bytes
    if (FALSE == cape_cursor__has_data (cursor, 4 + AES_256_GCM__SALT_LEN + AES_256_GCM__IV_LEN))
    {
        res = cape_err_set(err, CAPE_ERR_WRONG_VALUE, "Invalid encrypted payload");
        goto cleanup_and_exit;
    }

    // scan the next values we need
    iterations      = cape_cursor_scan_32 (cursor, TRUE);
    salt            = cape_cursor_scan_s (cursor, AES_256_GCM__SALT_LEN);
    iv              = cape_cursor_scan_s (cursor, AES_256_GCM__IV_LEN);
    self->tag_len   = cape_cursor_scan_08 (cursor);

    // small sanity check
    if (self->tag_len < 4 || self->tag_len > 16)
    {
        res = cape_err_set_fmt (err, CAPE_ERR_WRONG_VALUE, "Invalid tag length = %i", self->tag_len);
        goto cleanup_and_exit;
    }
    
    cape_log_fmt (CAPE_LL_TRACE, "QCRYPT", "decrypt", "using tag length = %i", self->tag_len);
    
    // create the keys needed for GCM
    self->keys = qcrypt_aes_keys_gen__pbkdf2 (self->secret, iterations, salt, iv, err);
    if (NULL == self->keys)
    {
        res = cape_err_code (err);
        goto cleanup_and_exit;
    }

    if (!EVP_DecryptInit_ex (self->ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
    {
        res = qcrypt_aes__handle_error (self->ctx, err);
        goto cleanup_and_exit;
    }

    // set IV length (REQUIRED for GCM)
    if (!EVP_CIPHER_CTX_ctrl (self->ctx, EVP_CTRL_GCM_SET_IVLEN, AES_256_GCM__IV_LEN, NULL))
    {
        res = qcrypt_aes__handle_error (self->ctx, err);
        goto cleanup_and_exit;
    }
    
    // now set key + iv
    if (!EVP_DecryptInit_ex (self->ctx, NULL, NULL, (const unsigned char*)qcrypt_aes_key (self->keys), (const unsigned char*)qcrypt_aes_iv (self->keys)))
    {
        res = qcrypt_aes__handle_error (self->ctx, err);
        goto cleanup_and_exit;
    }

    if (!EVP_CIPHER_CTX_set_padding (self->ctx, 0))
    {
        res = qcrypt_aes__handle_error (self->ctx, err);
        goto cleanup_and_exit;
    }
    
    // create the rolling buffer to handle the tag
    self->rolling_buffer = cape_stream_new();
    
    res = CAPE_ERR_NONE;
    
cleanup_and_exit:
    
    cape_str_del (&salt);
    cape_str_del (&iv);
    return res;
}

//-----------------------------------------------------------------------------

int qdecrypt_aes__init (QDecryptAES self, const char* bufdat, number_t buflen, number_t* p_buffer_offset, CapeErr err)
{
#if defined __WINDOWS_OS


#else

    int res;
    CapeCursor cursor = cape_cursor_new ();
    
    switch (self->cypher_type)
    {
        case QCRYPT_AES_TYPE_DETECT:
        {
            cape_cursor_set (cursor, bufdat, buflen);
            
            // check if there is enough data to parse the first 4 bytes
            if (FALSE == cape_cursor__has_data (cursor, 4))
            {
                res = cape_err_set(err, CAPE_ERR_WRONG_VALUE, "Invalid encrypted payload");
                goto cleanup_and_exit;
            }
            
            // check the magic bytes
            if ((cape_cursor_scan_08 (cursor) == 81) && (cape_cursor_scan_08 (cursor) == 67) && (cape_cursor_scan_08 (cursor) == 77))
            {
                char v = cape_cursor_scan_08 (cursor);
                
                switch (v)
                {
                    case QCRYPT_AES_TYPE_256_GCM:
                    {
                        res = qdecrypt_aes__gcm_256_pbkdf2 (self, cursor, err);
                        
                        // set the offset from delta of the cursor's current position
                        *p_buffer_offset = cape_cursor_apos (cursor);
                        
                        break;
                    }
                    default:
                    {
                        res = cape_err_set_fmt(err, CAPE_ERR_WRONG_VALUE, "Unsupported encryption version: %c", v);
                        break;
                    }
                }
            }
            else
            {
                // current default version
                res = qdecrypt_aes__cfb (self, EVP_aes_256_cbc(), bufdat, buflen, p_buffer_offset, err);
            }
            
            break;
        }
        case QCRYPT_AES_TYPE_256_GCM:
        {
            cape_cursor_set (cursor, bufdat + 4, buflen - 4);

            res = qdecrypt_aes__gcm_256_pbkdf2 (self, cursor, err);
            
            // set the offset from delta of the cursor's current position
            *p_buffer_offset = cape_cursor_apos (cursor);

            break;
        }
        case QCRYPT_AES_TYPE_256_CBC:
        {
            res = qdecrypt_aes__cfb (self, EVP_aes_256_cbc(), bufdat, buflen, p_buffer_offset, err);
            break;
        }
        case QCRYPT_AES_TYPE_256_CFB:
        {
            res = qdecrypt_aes__cfb (self, EVP_aes_256_cfb(), bufdat, buflen, p_buffer_offset, err);
            break;
        }
        case QCRYPT_AES_TYPE_256_CFB_1:
        {
            res = qdecrypt_aes__cfb (self, EVP_aes_256_cfb1(), bufdat, buflen, p_buffer_offset, err);
            break;
        }
        case QCRYPT_AES_TYPE_256_CFB_8:
        {
            res = qdecrypt_aes__cfb (self, EVP_aes_256_cfb8(), bufdat, buflen, p_buffer_offset, err);
            break;
        }
        case QCRYPT_AES_TYPE_256_CFB_128:
        {
            res = qdecrypt_aes__cfb (self, EVP_aes_256_cfb128(), bufdat, buflen, p_buffer_offset, err);
            break;
        }
        default:
        {
            res = cape_err_set_fmt(err, CAPE_ERR_WRONG_VALUE, "Unsupported encryption version: %lu", self->cypher_type);
            break;
        }
    }
    
cleanup_and_exit:
    
    cape_cursor_del (&cursor);
    return res;

#endif
}

//-----------------------------------------------------------------------------

int qdecrypt_aes_process (QDecryptAES self, const char* bufdat, number_t buflen, CapeErr err)
{
#if defined __WINDOWS_OS

    
#else

    number_t buffer_offset = 0;
    
    if (self->keys == NULL)
    {
        // run the initialization at the beginning
        int res = qdecrypt_aes__init (self, bufdat, buflen, &buffer_offset, err);
        if (res)
        {
            return res;
        }
    }
    
    if (self->tag_len)
    {
        // copy the buffer into the temporary status in the rolling buffer
        cape_stream_append_buf (self->rolling_buffer, bufdat + buffer_offset, buflen - buffer_offset);

        number_t current_buffer_size = cape_stream_size (self->rolling_buffer);

        if (current_buffer_size > self->tag_len)
        {
            number_t bytes_to_decrypt = current_buffer_size - self->tag_len;
            
            // extend the buffer (use 2 bytes extra to be on the safe side)
            cape_stream_cap (self->product, bytes_to_decrypt + self->blocksize + 2);

            {
                int lenLast;
                
                if (EVP_DecryptUpdate (self->ctx, (unsigned char*)cape_stream_pos (self->product), &lenLast, (const unsigned char*)cape_stream_data (self->rolling_buffer), (int)bytes_to_decrypt) == 0)
                {
                    return qcrypt_aes__handle_error (self->ctx, err);
                }
                
                cape_stream_set (self->product, lenLast);
                self->total_size += lenLast;
            }

            // reduce the buffer to the last bytes of self->tag_len length
            cape_stream_shift_l (self->rolling_buffer, bytes_to_decrypt);
        }
    }
    else
    {
        // extend the buffer (use 2 bytes extra to be on the safe side)
        cape_stream_cap (self->product, buflen + self->blocksize + 2);

        {
            int lenLast;
            
            if (EVP_DecryptUpdate (self->ctx, (unsigned char*)cape_stream_pos (self->product), &lenLast, (const unsigned char*)bufdat + buffer_offset, buflen - buffer_offset) == 0)
            {
                return qcrypt_aes__handle_error (self->ctx, err);
            }
            
            cape_stream_set (self->product, lenLast);
            self->total_size += lenLast;
        }
    }

    return CAPE_ERR_NONE;

#endif
}

//-----------------------------------------------------------------------------

int qdecrypt_aes_finalize (QDecryptAES self, CapeErr err)
{
#if defined __WINDOWS_OS

    
#else

    if (self->rolling_buffer && self->tag_len)
    {
        if (cape_stream_size (self->rolling_buffer) != self->tag_len)
        {
            return cape_err_set (err, CAPE_ERR_WRONG_VALUE, "not enough bytes for setting tag");
        }
        
        if (EVP_CIPHER_CTX_ctrl (self->ctx, EVP_CTRL_GCM_SET_TAG, self->tag_len, (void*)cape_stream_get (self->rolling_buffer)) == 0)
        {
            return qcrypt_aes__handle_error (self->ctx, err);
        }
    }
        
    cape_stream_cap (self->product, self->blocksize);

    {
        int lenLast;
      
        if (EVP_DecryptFinal_ex (self->ctx, (unsigned char*)cape_stream_pos (self->product), &lenLast) == 0)
        {
            return qcrypt_aes__handle_error (self->ctx, err);
        }

        cape_stream_set (self->product, lenLast);
    }

    switch (self->padding_type)
    {
        case QCRYPT_PADDING_ANSI_X923:   // forced padding
        {
            // identify the last byte which contains the padding information
            // reduce the buffer length
            cape_stream_dec (self->product, cape_stream_last_c (self->product));
            break;
        }
    }
    
    return CAPE_ERR_NONE;

#endif
}

//-----------------------------------------------------------------------------
