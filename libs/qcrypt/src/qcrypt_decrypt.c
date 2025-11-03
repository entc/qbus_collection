#include "qcrypt_decrypt.h"
#include "qcrypt_aes.h"

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
    
    qcrypt_aes_keys_del (&(self->keys));
    cape_str_del (&(self->secret));
    
#endif
    
    CAPE_DEL (p_self, struct QDecryptAES_s);
  }
}

//-----------------------------------------------------------------------------

int qdecrypt_aes__init (QDecryptAES self, const char* bufdat, number_t buflen, number_t* p_buffer_offset, CapeErr err)
{
#if defined __WINDOWS_OS


#else

  int res;
  
  // get the cypher
  const EVP_CIPHER* cypher = qencrypt_aes__get_cipher (self->cypher_type);
    
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
    int res = qdecrypt_aes__init (self, bufdat, buflen, &buffer_offset, err);
    if (res)
    {
      return res;
    }
  }
  
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

  return CAPE_ERR_NONE;
  
#endif
}

//-----------------------------------------------------------------------------

int qdecrypt_aes_finalize (QDecryptAES self, CapeErr err)
{
#if defined __WINDOWS_OS

    
#else

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
    default:
    {
      break;
    }
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
