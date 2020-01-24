#include "qcrypt_decrypt.h"
#include "qcrypt_aes.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

// entc includes
#include <tools/eccrypt.h>
#include <tools/eccode.h>
#include <types/ecstream.h>
#include <types/ecerr.h>

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

struct QDecryptAES_s
{
  CapeStream product;

#if defined __WINDOWS_OS

    
#else

  EVP_CIPHER_CTX* ctx;

  int blocksize;
  
  number_t cypher_type;
  number_t key_type;
  
  number_t total_size;
  
  // this are our secrets
  
  CapeString secret;
  
  QCryptAESKeys keys;

#endif
};

//-----------------------------------------------------------------------------

QDecryptAES qdecrypt_aes_new (CapeStream r_product, const CapeString secret, number_t cypher_type, number_t key_type)
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

#if OPENSSL_VERSION_NUMBER < 0x10100000L

    CAPE_FREE (self->ctx);
    
#else
    
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
  int res;
  
  // get the cypher
  const EVP_CIPHER* cypher = qencrypt_aes__get_cipher (self->cypher_type);
    
  switch (self->key_type)
  {
    case ENTC_KEY_SHA256:
    {
      self->keys = qcrypt_aes_keys_new__sha256 (self->secret, cypher, err);
      break;
    }
    case ENTC_KEY_PASSPHRASE_MD5:
    {
      self->keys = qcrypt_aes_keys_new__md5_de (self->secret, cypher, bufdat, buflen, err);
      
      // set the correct offset
      *p_buffer_offset = 16;
      
      break;
    }
    case ENTC_PADDING_ZEROS:
    {
      self->keys = qcrypt_aes_keys_new__padding_zero (self->secret, cypher);
      break;
    }
    case ENTC_PADDING_ANSI_X923:
    {
      self->keys = qcrypt_aes_keys_new__ansiX923 (self->secret, cypher);
      break;
    }
    case ENTC_PADDING_PKCS7:
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
  
  return ENTC_ERR_NONE;
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
  
  cape_stream_cap (self->product, buflen + self->blocksize);
  
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

  return CAPE_ERR_NONE;

#endif
}

//-----------------------------------------------------------------------------
