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
#include <openssl/evp.h>
#include <openssl/rand.h>

#endif

#if defined __WINDOWS_OS

//-----------------------------------------------------------------------------

HCRYPTPROV ecencrypt_aes_acquireContext (DWORD provType, CapeErr err)
{
  HCRYPTPROV provHandle = (HCRYPTPROV)NULL;
  
  if (!CryptAcquireContext (&provHandle, NULL, NULL, provType, 0))
  {
    DWORD errCode = GetLastError ();

    if (errCode == NTE_BAD_KEYSET)
    {
      if (!CryptAcquireContext (&provHandle, NULL, NULL, provType, CRYPT_NEWKEYSET))
      {
        cape_err_lastOSError (err);
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
    CapeString iv;
    CapeString salt;

    // the very secret key
    CapeString key;
    number_t key_len;
};

//-----------------------------------------------------------------------------

void qcrypt_aes_keys_del (QCryptAESKeys* p_self)
{
  if (*p_self)
  {
      QCryptAESKeys self = *p_self;
      
      if (self->key)
      {
          // this prevents derived AES keys from remaining in heap memory
          OPENSSL_cleanse (self->key, self->key_len);
          CAPE_FREE (self->key);
      }

      if (self->iv)
      {
        CAPE_FREE (self->iv);
      }

      if (self->salt)
      {
        CAPE_FREE (self->salt);
      }

      CAPE_DEL (p_self, struct QCryptAESKeys_s);
  }
}

//-----------------------------------------------------------------------------

const CapeString qcrypt_aes_key (QCryptAESKeys self)
{
    return self->key;
}

//-----------------------------------------------------------------------------

const CapeString qcrypt_aes_iv (QCryptAESKeys self)
{
    return self->iv;
}

//-----------------------------------------------------------------------------

const CapeString qcrypt_aes_salt (QCryptAESKeys self)
{
    return self->salt;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__sha256 (const CapeString secret, const EVP_CIPHER* cypher, CapeErr err)
{
  QCryptAESKeys self = NULL;
  
  self->key_len = EVP_CIPHER_key_length (cypher);

  // length in 8 bit blocks
  if (self->key_len != 32)   // 8 * 32 = 256
  {
    cape_err_set_fmt (err, CAPE_ERR_RUNTIME, "cypher has unsupported key-length for padding (SHA256): %i", self->key_len);
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

    self->key = cape_stream_to_str (&h);
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
  self->key_len = EVP_MAX_KEY_LENGTH;
    
  self->iv = CAPE_ALLOC (EVP_MAX_IV_LENGTH);
  
  {
    CapeString random_text = cape_str_random_s (8);
    
    res = EVP_BytesToKey (cypher, EVP_md5(), (unsigned char*)random_text, (unsigned char*)secret, (int)cape_str_size (secret), rounds, (unsigned char*)self->key, (unsigned char*)self->iv);

    // set termination
    self->key[res] = 0;

    cape_stream_append_buf (product, "Salted__", 8);
    cape_stream_append_buf (product, random_text, 8);

    cape_str_del (&random_text);
  }
  
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
  self->key_len = EVP_CIPHER_key_length (cypher);
  
  //eclog_fmt (LL_TRACE, "ENTC", "eccrypt", "passphrase with key-length %i", keyLength);
  
  self->key = CAPE_ALLOC (self->key_len);
  self->iv = CAPE_ALLOC (16);
  
  EVP_BytesToKey (cypher, EVP_md5(), (const unsigned char*)bufdat + 8, (unsigned char*)secret, (int)cape_str_size (secret), 1, (unsigned char*)self->key, (unsigned char*)self->iv);

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

  number_t size = cape_str_size (secret);

  // cypher options
  self->key_len = EVP_CIPHER_key_length (cypher);
  
  // using the whole keylength for padding
  self->key = CAPE_ALLOC (self->key_len);
  
  // add the zeros (padding)
  memset (self->key, 0, self->key_len);

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

  number_t size = cape_str_size (secret);

  // cypher options
  self->key_len = EVP_CIPHER_key_length (cypher);
  
  // using the whole keylength for padding
  self->key = CAPE_ALLOC (self->key_len);

  // add the zeros (padding)
  memset (self->key, 0, self->key_len);

  // fill the buffer with they key
  memcpy (self->key, secret, size);
  
  // add the last byte (padding)
  memset (self->key + self->key_len - 1, self->key_len - size, 1);
  
  // the rest is empty
  self->iv = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__padding_pkcs7 (const CapeString secret, const EVP_CIPHER* cypher)
{
  QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

  number_t size = cape_str_size (secret);

  // cypher options
  self->key_len = EVP_CIPHER_key_length (cypher);
  
  number_t diff = self->key_len - size;
        
  // using the whole keylength for padding
  self->key = CAPE_ALLOC (self->key_len);

  // add the padding
  memset (self->key, diff, self->key_len);

  // fill the buffer with the key
  memcpy (self->key, secret, size);
  
  // the rest is empty
  self->iv = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

#define AES_256_PBKDF2__KEY_LEN      32
#define AES_256_PBKDF2__ITERATIONS   100000
//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__pbkdf2 (const CapeString secret, number_t iv_len, number_t salt_len, CapeErr err)
{
    QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

    self->key = CAPE_ALLOC (AES_256_PBKDF2__KEY_LEN);
    self->key_len = AES_256_PBKDF2__KEY_LEN;
    
    self->salt = CAPE_ALLOC (salt_len);
    self->iv = CAPE_ALLOC (iv_len);

    // create the salt
    if (RAND_bytes ((unsigned char *)self->salt, (int)salt_len) != 1)
    {
        cape_err_set (err, CAPE_ERR_3RDPARTY_LIB, "rand failed on openssl");
        goto cleanup_and_exit;
    }
    
    if (RAND_bytes ((unsigned char *)self->iv, (int)iv_len) != 1)
    {
        cape_err_set (err, CAPE_ERR_3RDPARTY_LIB, "rand failed on openssl");
        goto cleanup_and_exit;
    }
    
    if (PKCS5_PBKDF2_HMAC (secret, (int)cape_str_size (secret), (unsigned char *)self->salt, (int)salt_len, AES_256_PBKDF2__ITERATIONS, EVP_sha256(), (int)self->key_len, (unsigned char *)self->key) != 1)
    {
        cape_err_set (err, CAPE_ERR_3RDPARTY_LIB, "hmac failed on openssl");
        goto cleanup_and_exit;
    }
    
    return self;
    
cleanup_and_exit:
    
    qcrypt_aes_keys_del (&self);
    return NULL;
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

