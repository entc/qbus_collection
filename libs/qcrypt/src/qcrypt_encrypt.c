#include "qcrypt_encrypt.h"
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

#endif
};

//-----------------------------------------------------------------------------

QEncryptAES qencrypt_aes_new (CapeStream r_product, number_t cypher_type, number_t padding_type, const CapeString secret, number_t key_type)
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
  self->padding_type = padding_type;
  
  // key settings
  self->secret = cape_str_cp (secret);
  self->key_type = key_type;
  
  self->keys = NULL;

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

#if OPENSSL_VERSION_NUMBER < 0x10100000L

    EVP_CIPHER_CTX_cleanup (self->ctx);
    CAPE_FREE (self->ctx);
    
#else
    
    EVP_CIPHER_CTX_cleanup (self->ctx);
    EVP_CIPHER_CTX_free (self->ctx);

#endif

    qcrypt_aes_keys_del (&(self->keys));
    cape_str_del (&(self->secret));
    
#endif
    
    CAPE_DEL (p_self, struct QEncryptAES_s);
  }
}

//-----------------------------------------------------------------------------

int qencrypt_aes__init (QEncryptAES self, const char* bufdat, number_t buflen, CapeErr err)
{
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
    
  res = EVP_EncryptInit_ex (self->ctx, cypher, NULL, qcrypt_aes_key (self->keys), qcrypt_aes_iv (self->keys));
  
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

int qencrypt_aes_process (QEncryptAES self, const char* bufdat, number_t buflen, CapeErr err)
{
#if defined __WINDOWS_OS

    
#else

  if (self->keys == NULL)
  {
    int res = qencrypt_aes__init (self, bufdat, buflen, err);
    if (res)
    {
      return res;
    }
  }
  
  cape_stream_cap (self->product, buflen + self->blocksize);
  
  {
    int lenLast;
    
    if (EVP_EncryptUpdate (self->ctx, (unsigned char*)cape_stream_pos (self->product), &lenLast, (const unsigned char*)bufdat, buflen) == 0)
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
      encrRes = EVP_EncryptUpdate (self->ctx, (unsigned char*)cape_stream_pos (self->product), &lenLast, padding, padlen);
      
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

  return CAPE_ERR_NONE;

#endif
}

//-----------------------------------------------------------------------------
