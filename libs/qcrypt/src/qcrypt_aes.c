#include "qcrypt_aes.h"
#include "qcrypt.h"

// cape includes
#include <sys/cape_log.h>

#if defined __WINDOWS_OS

#include <windows.h>
#include <wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

#else

#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/err.h>

#endif

#if defined __WINDOWS_OS

//-----------------------------------------------------------------------------

HCRYPTPROV ecencrypt_aes_acquireContext (DWORD provType, EcErr err)
{
  HCRYPTPROV provHandle = (HCRYPTPROV)NULL;
  
  if (!CryptAcquireContext (&provHandle, NULL, NULL, provType, 0))
  {
    DWORD errCode = GetLastError ();

    if (errCode == NTE_BAD_KEYSET)
    {
      if (!CryptAcquireContext (&provHandle, NULL, NULL, provType, CRYPT_NEWKEYSET))
      {
        ecerr_lastErrorOS (err, ENTC_LVL_ERROR);
        return NULL;
      }
    }
  }
  
  return provHandle;
}

//-----------------------------------------------------------------------------

#else

//-----------------------------------------------------------------------------

struct QCryptAESKeys_s
{
  unsigned char* key;
  unsigned char* iv;
  
};

//-----------------------------------------------------------------------------

void qcrypt_aes_keys_del (QCryptAESKeys* p_self)
{
  if (*p_self)
  {
    QCryptAESKeys self = *p_self;
    
    if (self->key)
    {
      CAPE_FREE (self->key);
    }

    if (self->iv)
    {
      CAPE_FREE (self->iv);
    }
    
    CAPE_DEL (p_self, struct QCryptAESKeys_s);
  }
}

//-----------------------------------------------------------------------------

unsigned char* qcrypt_aes_key (QCryptAESKeys self)
{
  return self->key;
}

//-----------------------------------------------------------------------------

unsigned char* qcrypt_aes_iv (QCryptAESKeys self)
{
  return self->iv;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__sha256 (const CapeString secret, const EVP_CIPHER* cypher, CapeErr err)
{
  QCryptAESKeys self = NULL;
  
  int keyLength = EVP_CIPHER_key_length (cypher);

  // length in 8 bit blocks
  if (keyLength != 32)   // 8 * 32 = 256
  {
    cape_err_set_fmt (err, CAPE_ERR_RUNTIME, "cypher has unsupported key-length for padding (SHA256): %i", keyLength);
    goto exit_and_cleanup;
  }
  
  self = CAPE_NEW (struct QCryptAESKeys_s);

  self->key = NULL;
  self->iv = NULL;

  {
    CapeStream h = qcrypt__hash_sha256__bin_o (secret, cape_str_size (secret), err);
    if (NULL == h)
    {
      // destroy the object
      qcrypt_aes_keys_del (&self);
      goto exit_and_cleanup;
    }

    self->key = (unsigned char*)cape_stream_to_str (&h);
  }
  
exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "QCRYPT", "aes keys sha256", cape_err_text(err));
  }
  
  return self;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__md5_en (const CapeString secret, const EVP_CIPHER* cypher, CapeStream product)
{
  QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

  int rounds = 1;
  int res;
  
  self->key = CAPE_ALLOC (EVP_MAX_KEY_LENGTH);
  self->iv = CAPE_ALLOC (EVP_MAX_IV_LENGTH);
  
  {
    CapeString random_text = cape_str_random_s (8);
    
    res = EVP_BytesToKey (cypher, EVP_md5(), (unsigned char*)random_text, (unsigned char*)secret, cape_str_size (secret), rounds, self->key, self->iv);

    // set termination
    self->key[res] = 0;

    cape_stream_append_buf (product, "Salted__", 8);
    cape_stream_append_buf (product, random_text, 8);

    cape_str_del (&random_text);
  }
  
  // for debug
  /*
  {
      EcBuffer a0 = ecbuf_create_buffer_cp(self->buf->buffer + 8, 8);
      
      EcBuffer h0 = ecbuf_bin2hex (a0);
      EcBuffer h1 = ecbuf_bin2hex (self->keys->key);
      EcBuffer h2 = ecbuf_bin2hex (self->keys->iv);
      
      eclog_fmt (LL_TRACE, "ENTC", "eccrypt", "SLT: %s", h0->buffer);
      eclog_fmt (LL_TRACE, "ENTC", "eccrypt", "KEY: %s", h1->buffer);
      eclog_fmt (LL_TRACE, "ENTC", "eccrypt", " IV: %s", h2->buffer);
      
      ecbuf_destroy(&h1);
      ecbuf_destroy(&h2);
      ecbuf_destroy(&h0);
      ecbuf_destroy(&a0);
  }
  */

  return self;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__md5_de (const CapeString secret, const EVP_CIPHER* cypher, const char* bufdat, number_t buflen, CapeErr err)
{
  QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

  // do some pre-checks
  if (buflen < 16)
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "source buffer has less than 16 bytes");
    goto exit_and_cleanup;
  }
  
  // cypher options
  int keyLength = EVP_CIPHER_key_length (cypher);
  
  //eclog_fmt (LL_TRACE, "ENTC", "eccrypt", "passphrase with key-length %i", keyLength);
  
  self->key = CAPE_ALLOC (keyLength);
  self->iv = CAPE_ALLOC (16);
  
  EVP_BytesToKey (cypher, EVP_md5(), (const unsigned char*)bufdat + 8, (unsigned char*)secret, cape_str_size (secret), 1, self->key, self->iv);

exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "QCRYPT", "aes keys sha256", cape_err_text(err));
  }

  return self;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__padding_zero (const CapeString secret, const EVP_CIPHER* cypher)
{
  QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

  int size = cape_str_size (secret);

  // cypher options
  int keyLength = EVP_CIPHER_key_length (cypher);
  
  // using the whole keylength for padding
  self->key = CAPE_ALLOC (keyLength);
  
  // add the zeros (padding)
  memset (self->key, 0, keyLength);

  // fill the buffer with they key
  memcpy (self->key, secret, size);
  
  // the rest is empty
  self->iv = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qcrypt_aes_padding__ansiX923_pad (unsigned char* bufdat, number_t buflen, number_t offset)
{
  int64_t diff = buflen - offset;  // calculate the difference
  
  // add the zeros (padding)
  memset (bufdat + offset, 0, diff);

  // add the last byte (padding)
  memset (bufdat + buflen - 1, diff, 1);
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__ansiX923 (const CapeString secret, const EVP_CIPHER* cypher)
{
  QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

  int size = cape_str_size (secret);

  // cypher options
  int keyLength = EVP_CIPHER_key_length (cypher);
  
  // using the whole keylength for padding
  self->key = CAPE_ALLOC (keyLength);

  // add the zeros (padding)
  memset (self->key, 0, keyLength);

  // fill the buffer with they key
  memcpy (self->key, secret, size);
  
  // add the last byte (padding)
  memset (self->key + keyLength - 1, keyLength - size, 1);
  
  // the rest is empty
  self->iv = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__padding_pkcs7 (const CapeString secret, const EVP_CIPHER* cypher)
{
  QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

  int size = cape_str_size (secret);

  // cypher options
  int keyLength = EVP_CIPHER_key_length (cypher);
  
  int diff = keyLength - size;
        
  // using the whole keylength for padding
  self->key = CAPE_ALLOC (keyLength);

  // add the padding
  memset (self->key, diff, keyLength);

  // fill the buffer with the key
  memcpy (self->key, secret, size);
  
  // the rest is empty
  self->iv = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

const EVP_CIPHER* qencrypt_aes__get_cipher (number_t type)
{
  switch (type)
  {
    case QCRYPT_AES_TYPE_CBC:
    {
      return EVP_aes_256_cbc();
    }
    case QCRYPT_AES_TYPE_CFB:
    {
      return EVP_aes_256_cfb();
    }
    case QCRYPT_AES_TYPE_CFB_1:
    {
      return EVP_aes_256_cfb1();
    }
    case QCRYPT_AES_TYPE_CFB_8:
    {
      return EVP_aes_256_cfb8();
    }
    case QCRYPT_AES_TYPE_CFB_128:
    {
      return EVP_aes_256_cfb128();
    }
  }
  
  return EVP_aes_256_cbc();
}

//-----------------------------------------------------------------------------

int qcrypt_aes__handle_error (EVP_CIPHER_CTX* ctx, CapeErr err)
{
  unsigned long errCode;
  int res;
  
  CapeStream stream = cape_stream_new ();
  
  while ((errCode = ERR_get_error()))
  {
    cape_stream_append_str (stream, ERR_error_string (errCode, NULL));
  }
  
  res = cape_err_set (err, CAPE_ERR_PROCESS_FAILED, cape_stream_get (stream));
  
  cape_stream_del (&stream);
  
  return res;
}

//-----------------------------------------------------------------------------

#endif

