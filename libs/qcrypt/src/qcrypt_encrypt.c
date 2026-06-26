#include "qcrypt_encrypt.h"
#include "qcrypt_aes.h"
#include "qcrypt.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

#if defined __WINDOWS_OS

#include <windows.h>
#include <wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

#else

#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#endif

//-----------------------------------------------------------------------------

struct QEncryptBase64_s
{
  CapeStream product;

#if defined __WINDOWS_OS

  
#else

  EVP_ENCODE_CTX* ctx;

#endif
};

//-----------------------------------------------------------------------------

QEncryptBase64 qencrypt_base64_new (CapeStream r_product)
{
  QEncryptBase64 self = CAPE_NEW (struct QEncryptBase64_s);
  
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
  EVP_EncodeInit (self->ctx);

#endif
  
  return self;
}

//-----------------------------------------------------------------------------

void qencrypt_base64_del (QEncryptBase64* p_self)
{
  if (*p_self)
  {
    QEncryptBase64 self = *p_self;
    
#if defined __WINDOWS_OS

  
#else

#if OPENSSL_VERSION_NUMBER < 0x10100000L

    CAPE_DEL (&(self->ctx), EVP_ENCODE_CTX);
    
#else
    
    EVP_ENCODE_CTX_free (self->ctx);

#endif
    
#endif
    
    CAPE_DEL (p_self, struct QEncryptBase64_s);
  }
}

//-----------------------------------------------------------------------------

number_t qencrypt_base64_size (number_t size)
{
  // following RFC 2045
  number_t bytes_for_linebreaks = (number_t)(size / 65) + 2;
  
  // this calculates the result size of the base64 encoded string
  return ((size + 2) / 3 * 4) + 4 + 64 + bytes_for_linebreaks;
}

//-----------------------------------------------------------------------------

int qencrypt_base64_process (QEncryptBase64 self, const char* bufdat, number_t buflen, CapeErr err)
{
#if defined __WINDOWS_OS

  
#else

  int len;
  
  // increase the stream to fit the expected size of the base64 serialization
  cape_stream_cap (self->product, qencrypt_base64_size (buflen));
  
  // call the crypt librarie's function
  EVP_EncodeUpdate (self->ctx, (unsigned char*)cape_stream_pos (self->product), &len, (const unsigned char*)bufdat, buflen);
 
  // adjust the stream to the correct written bytes
  cape_stream_set (self->product, len);
  
  return CAPE_ERR_NONE;

#endif
}

//-----------------------------------------------------------------------------

int qencrypt_base64_finalize (QEncryptBase64 self, CapeErr err)
{
#if defined __WINDOWS_OS

  
#else

  int len;
  
  // base64 alignment is 24bit, use 5 bytes to be on the safe side
  cape_stream_cap (self->product, 5);

  // the base64 encoding needs to be finalized to align to a certain amount of bytes
  EVP_EncodeFinal (self->ctx, (unsigned char*)cape_stream_pos (self->product), &len);
  
  // adjust the stream to the correct written bytes
  cape_stream_set (self->product, len);
  
  // remove last byte
  cape_stream_dec (self->product, 1);
  
  return CAPE_ERR_NONE;

#endif
}

//-----------------------------------------------------------------------------

struct QEncryptAES_s
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
    int taglen;

#endif
};

//-----------------------------------------------------------------------------

QEncryptAES qencrypt_aes_new (CapeStream r_product, number_t cypher_type, const CapeString secret)
{
    QEncryptAES self = CAPE_NEW (struct QEncryptAES_s);
    
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
    self->padding_type = QCRYPT_PADDING_NONE;
    
    // key settings
    self->secret = cape_str_cp (secret);
    self->key_type = QCRYPT_KEY_NONE;

    self->keys = NULL;
    self->taglen = 0;

#endif
  
    return self;
}

//-----------------------------------------------------------------------------

void qencrypt_aes_del (QEncryptAES* p_self)
{
  if (*p_self)
  {
    QEncryptAES self = *p_self;
    
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

    qcrypt_aes_keys_del (&(self->keys));
    cape_str_del (&(self->secret));
    
#endif
    
    CAPE_DEL (p_self, struct QEncryptAES_s);
  }
}

//-----------------------------------------------------------------------------

int qencrypt_aes__cfb (QEncryptAES self, const EVP_CIPHER* cypher, number_t padding_type, number_t key_type, CapeErr err)
{
    int res;
  
    self->padding_type = padding_type;
    self->key_type = key_type;
    
    switch (self->key_type)
    {
            /*
      case QCRYPT_KEY_SHA256:
      {
        self->keys = qcrypt_aes_keys_new__sha256 (self->secret, cypher, err);
        break;
      }
             */
      case QCRYPT_KEY_PASSPHRASE_MD5:
      {
        number_t size = cape_stream_size (self->product);
        
        self->keys = qcrypt_aes_keys_new__md5_en (self->secret, cypher, self->product);
        
        // increase the total size by the delta of the last size
        self->total_size += (cape_stream_size (self->product) - size);
        
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
      
    res = EVP_EncryptInit_ex (self->ctx, cypher, NULL, (unsigned char*)qcrypt_aes_key (self->keys), (unsigned char*)qcrypt_aes_iv (self->keys));
    
    if (res == 0)
    {
      return qcrypt_aes__handle_error (self->ctx, err);
    }
    
    // check for the blocksize
    self->blocksize = EVP_CIPHER_CTX_block_size (self->ctx);
    
    if (self->padding_type)
    {
      // disable automatic padding
      EVP_CIPHER_CTX_set_padding (self->ctx, 0);
    }
    else
    {
      EVP_CIPHER_CTX_set_padding (self->ctx, 1);
    }
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

#define AES_256_PBKDF2__ITERATIONS   100000

//-----------------------------------------------------------------------------

int qencrypt_aes__gcm (QEncryptAES self, CapeErr err)
{
    const EVP_CIPHER* cipher = EVP_aes_256_gcm();
        
    // use the default value here
    // can be changed in the future
    self->taglen = AES_256_GCM__TAG_LEN;
    
    // create the keys needed for GCM
    self->keys = qcrypt_aes_keys_new__pbkdf2 (self->secret, AES_256_PBKDF2__ITERATIONS, err);
    if (NULL == self->keys)
    {
        return cape_err_code (err);
    }

    // IMPORTANT: set cipher first
    if (!EVP_EncryptInit_ex (self->ctx, cipher, NULL, NULL, NULL))
    {
        return qcrypt_aes__handle_error (self->ctx, err);
    }
    
    // set IV length (REQUIRED for GCM)
    if (!EVP_CIPHER_CTX_ctrl (self->ctx, EVP_CTRL_GCM_SET_IVLEN, AES_256_GCM__IV_LEN, NULL))
    {
        return qcrypt_aes__handle_error (self->ctx, err);
    }
    
    // now set key + iv
    if (!EVP_EncryptInit_ex (self->ctx, NULL, NULL, (const unsigned char*)qcrypt_aes_key (self->keys), (const unsigned char*)qcrypt_aes_iv (self->keys)))
    {
        return qcrypt_aes__handle_error (self->ctx, err);
    }
    
    if (!EVP_CIPHER_CTX_set_padding (self->ctx, 0))
    {
        return qcrypt_aes__handle_error (self->ctx, err);
    }
    
    // (optional) set tag length HERE
    EVP_CIPHER_CTX_ctrl (self->ctx, EVP_CTRL_AEAD_SET_TAG, self->taglen, NULL);
    
    // clear the output stream
    cape_stream_clr (self->product);

    // add magic bytes + type 
    cape_stream_append_buf (self->product, QCRYPT_MAGIC_BYTES, 3);
    cape_stream_append_c (self->product, QCRYPT_AES_TYPE_256_GCM);
    
    // add iterations
    cape_stream_append_32 (self->product, AES_256_PBKDF2__ITERATIONS, TRUE);
    
    // add the salt
    cape_stream_append_buf (self->product, qcrypt_aes_salt (self->keys), AES_256_GCM__SALT_LEN);
    
    // add the iv
    cape_stream_append_buf (self->product, qcrypt_aes_iv (self->keys), AES_256_GCM__IV_LEN);

    // add iterations
    cape_stream_append_08 (self->product, self->taglen);

    // we don't need padding
    self->padding_type = QCRYPT_PADDING_NONE;
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qencrypt_aes__init (QEncryptAES self, CapeErr err)
{
#if defined __WINDOWS_OS


#else

    switch (self->cypher_type)
    {
        case QCRYPT_AES_TYPE_256_GCM:
        {
            return qencrypt_aes__gcm (self, err);
        }
        case QCRYPT_AES_TYPE_256_CBC:
        {
            return qencrypt_aes__cfb (self, EVP_aes_256_cbc(), QCRYPT_PADDING_ANSI_X923, QCRYPT_KEY_PASSPHRASE_MD5, err);
        }
        case QCRYPT_AES_TYPE_256_CFB:
        {
            return qencrypt_aes__cfb (self, EVP_aes_256_cfb(), QCRYPT_PADDING_ANSI_X923, QCRYPT_KEY_PASSPHRASE_MD5, err);
        }
        case QCRYPT_AES_TYPE_256_CFB_1:
        {
            return qencrypt_aes__cfb (self, EVP_aes_256_cfb1(), QCRYPT_PADDING_ANSI_X923, QCRYPT_KEY_PASSPHRASE_MD5, err);
        }
        case QCRYPT_AES_TYPE_256_CFB_8:
        {
            return qencrypt_aes__cfb (self, EVP_aes_256_cfb8(), QCRYPT_PADDING_ANSI_X923, QCRYPT_KEY_PASSPHRASE_MD5, err);
        }
        case QCRYPT_AES_TYPE_256_CFB_128:
        {
            return qencrypt_aes__cfb (self, EVP_aes_256_cfb128(), QCRYPT_PADDING_ANSI_X923, QCRYPT_KEY_PASSPHRASE_MD5, err);
        }
        default:
        {
            return cape_err_set (err, CAPE_ERR_NOT_SUPPORTED, "cypher version is not supported");
        }
    }

#endif
}

//-----------------------------------------------------------------------------

int qencrypt_aes_process (QEncryptAES self, const char* bufdat, number_t buflen, CapeErr err)
{
#if defined __WINDOWS_OS

    
#else

  if (self->keys == NULL)
  {
    int res = qencrypt_aes__init (self, err);
    if (res)
    {
      return res;
    }
  }
  
  cape_stream_cap (self->product, buflen + self->blocksize);
  
  {
    int lenLast;
    
    if (EVP_EncryptUpdate (self->ctx, (unsigned char*)cape_stream_pos (self->product), &lenLast, (const unsigned char*)bufdat, (int)buflen) == 0)
    {
      return qcrypt_aes__handle_error (self->ctx, err);
    }
    
    cape_stream_set (self->product, lenLast);
    self->total_size += lenLast;
  }

  return CAPE_ERR_NONE;
  
#endif
}

//-----------------------------------------------------------------------------

int qencrypt_aes_finalize (QEncryptAES self, CapeErr err)
{
#if defined __WINDOWS_OS

    
#else

  switch (self->padding_type)
  {
    default:
    {
      break;
    }
    case QCRYPT_PADDING_ANSI_X923:   // force padding
    {
      int lenLast;
      int encrRes;
      unsigned char* padding;
      
      // we need to encrypt the padding
      // calculate how much we need to pad
      number_t padlen = ((self->total_size / 8) + 1) * 8 - self->total_size;
      
      padding = CAPE_ALLOC (padlen);
      
      // set the padding for padding
      qcrypt_aes_padding__ansiX923_pad (padding, padlen, 0);
      
      // reserve memory in the buffer
      cape_stream_cap (self->product, padlen);

      // encrypt the padding
      encrRes = EVP_EncryptUpdate (self->ctx, (unsigned char*)cape_stream_pos (self->product), &lenLast, padding, (int)padlen);
      
      CAPE_FREE (padding);
        
      if (encrRes == 0)
      {
        return qcrypt_aes__handle_error (self->ctx, err);
      }
      
      cape_stream_set (self->product, lenLast);
      self->total_size += lenLast;

      break;
    }
  }
  
    cape_stream_cap (self->product, self->blocksize);

    {
        int lenLast;

        if (EVP_EncryptFinal_ex (self->ctx, (unsigned char*)cape_stream_pos (self->product), &lenLast) == 0)
        {
            return qcrypt_aes__handle_error (self->ctx, err);
        }

        cape_stream_set (self->product, lenLast);
    }

    if (self->taglen)
    {
        char* tag_buffer = CAPE_ALLOC (self->taglen);

        // retrieve the cryptographic authentication tag
        // computed from: the ciphertext, the key, the IV
        // detects tampering of the encrypted product
        if (!EVP_CIPHER_CTX_ctrl (self->ctx, EVP_CTRL_GCM_GET_TAG, self->taglen, tag_buffer))
        {
            CAPE_FREE (tag_buffer);
            
            return qcrypt_aes__handle_error (self->ctx, err);
        }
        
        // append the tag to the end of the product
        cape_stream_append_buf (self->product, tag_buffer, self->taglen);

        CAPE_FREE (tag_buffer);
    }
        
    return CAPE_ERR_NONE;

#endif
}

//-----------------------------------------------------------------------------
