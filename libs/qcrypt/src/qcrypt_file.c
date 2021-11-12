#include "qcrypt.h"
#include "qcrypt_file.h"
#include "qcrypt_decrypt.h"
#include "qcrypt_encrypt.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include <fmt/cape_json.h>
#include <stc/cape_str.h>
#include <stc/cape_cursor.h>

//-----------------------------------------------------------------------------

#if defined __WINDOWS_OS

#include <windows.h>
#include <wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

#else

#include <openssl/md5.h>

#endif

// define the buffer size for read and write operations on a file
#define QCRYPT_BUFFER_SIZE  10000

//-----------------------------------------------------------------------------

int qcrypt_decrypt_file (const CapeString vsec, const CapeString file, void* ptr, fct_cape_fs_file_load cb, CapeErr err)
{
  int res;
  
  // local objects
  CapeFileHandle fh = cape_fh_new (NULL, file);
  QDecryptAES dec = NULL;
  
  // buffers
  char* buffer = CAPE_ALLOC (QCRYPT_BUFFER_SIZE);
  CapeStream outb = cape_stream_new ();

  res = cape_fh_open (fh, O_RDONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // create the decryption context
  dec = qdecrypt_aes_new (outb, vsec, 0, 0);
  
  while (TRUE)
  {
    number_t bytes_read = cape_fh_read_buf (fh, buffer, QCRYPT_BUFFER_SIZE);
    if (bytes_read > 0)
    {
      res = qdecrypt_aes_process (dec, buffer, bytes_read, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
      
      res = cb (ptr, cape_stream_data (outb), cape_stream_size (outb), err);
      if (res)
      {
        goto exit_and_cleanup;
      }
      
      cape_stream_clr (outb);
    }
    else
    {
      break;
    }
  }

  res = qdecrypt_aes_finalize (dec, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = cb (ptr, cape_stream_data (outb), cape_stream_size (outb), err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  qdecrypt_aes_del (&dec);

  cape_fh_del (&fh);

  cape_stream_del (&outb);
  CAPE_FREE (buffer);

  return res;
}

//-----------------------------------------------------------------------------

struct QCryptDecrypt_s
{
  QDecryptAES dec;
  
  CapeFileHandle fh;
  
  int state;
  
  CapeStream product;
};

//-----------------------------------------------------------------------------

QCryptDecrypt qcrypt_decrypt_new (const CapeString path, const CapeString file, const CapeString vsec)
{
  QCryptDecrypt self = CAPE_NEW (struct QCryptDecrypt_s);

  // create the file handle
  self->fh = cape_fh_new (path, file);
  
  // create the buffer
  self->product = cape_stream_new ();
  
  // create the decryption context
  self->dec = qdecrypt_aes_new (self->product, vsec, 0, 0);
  
  self->state = TRUE;

  return self;
}

//-----------------------------------------------------------------------------

void qcrypt_decrypt_del (QCryptDecrypt* p_self)
{
  if (*p_self)
  {
    QCryptDecrypt self = *p_self;
    
    qdecrypt_aes_del (&(self->dec));

    cape_stream_del (&(self->product));
    
    cape_fh_del (&(self->fh));
    
    CAPE_DEL (p_self, struct QCryptDecrypt_s);
  }
}

//-----------------------------------------------------------------------------

int qcrypt_decrypt_open (QCryptDecrypt self, CapeErr err)
{
  return cape_fh_open (self->fh, O_RDONLY, err);
}

//-----------------------------------------------------------------------------

number_t qcrypt_decrypt_next (QCryptDecrypt self, char* buffer, number_t len)
{
  int res;
  number_t ret = 0;
  
  CapeErr err = cape_err_new ();
  
  if (self->state)
  {
    // use the extern buffer to load the data
    number_t bytes_read = cape_fh_read_buf (self->fh, buffer, len);
    if (bytes_read > 0)
    {
      res = qdecrypt_aes_process (self->dec, buffer, bytes_read, err);
      if (res)
      {
        //res = cape_err_set_fmt (err, CAPE_ERR_RUNTIME, "update AES decryption: %s", entc_err->text);
        //goto exit_and_cleanup;
      }
      else
      {
        // copy data
        ret = cape_stream_size (self->product);
        memcpy (buffer, cape_stream_data (self->product), ret);
        
        // clear the buffer
        cape_stream_clr (self->product);
      }
    }
    else
    {
      res = qdecrypt_aes_finalize (self->dec, err);
      if (res)
      {
        //res = cape_err_set_fmt (err, CAPE_ERR_RUNTIME, "finalize AES decryption: %s", entc_err->text);
        //goto exit_and_cleanup;
      }
      else
      {
        // copy data
        ret = cape_stream_size (self->product);
        memcpy (buffer, cape_stream_data (self->product), ret);
        
        // clear the buffer
        cape_stream_clr (self->product);
      }
      
      self->state = FALSE;
    }
  }
  
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------

struct QCryptFile_s
{
  QEncryptAES enc;

  CapeFileHandle fh;

  // the buffer
  CapeStream product;
};

//-----------------------------------------------------------------------------

QCryptFile qcrypt_file_new (const CapeString file)
{
  QCryptFile self = CAPE_NEW (struct QCryptFile_s);
  
  self->enc = NULL;
  self->fh = cape_fh_new (NULL, file);
  
  self->product = cape_stream_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qcrypt_file_del (QCryptFile* p_self)
{
  if (*p_self)
  {
    QCryptFile self = *p_self;
    
    qencrypt_aes_del (&(self->enc));
    
    if (self->fh)
    {
      cape_fh_del (&(self->fh));
    }

    cape_stream_del (&(self->product));

    CAPE_DEL (p_self, struct QCryptFile_s);
  }
}

//-----------------------------------------------------------------------------

int qcrypt_file_encrypt (QCryptFile self, const CapeString vsec, CapeErr err)
{
  int res;
  
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "vsec is not set");
    goto exit_and_cleanup;
  }
  
  // create encryption engine
  self->enc = qencrypt_aes_new (self->product, 0, 0, vsec, 0);
  if (self->enc == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "can't initialize crypt");
    goto exit_and_cleanup;
  }
  
  res = cape_fh_open (self->fh, O_CREAT | O_WRONLY | O_TRUNC, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int qcrypt_file_write (QCryptFile self, const char* bufdat, number_t buflen, CapeErr err)
{
  int res;
  
  // encrypt the buffer 'decrypted_text'
  res = qencrypt_aes_process (self->enc, bufdat, buflen, err);
  if (res)
  {
    goto exit;
  }

  // write the buffer into the file
  cape_fh_write_buf (self->fh, cape_stream_data (self->product), cape_stream_size (self->product));
  
  // clean the buffer
  cape_stream_clr (self->product);
  
  res = CAPE_ERR_NONE;

exit:

  return res;
}

//-----------------------------------------------------------------------------

int qcrypt_file_finalize (QCryptFile self, CapeErr err)
{
  int res;
  
  // finalize the encryption
  res = qencrypt_aes_finalize (self->enc, err);
  if (res)
  {
    goto exit;
  }

  // write the buffer into the file
  cape_fh_write_buf (self->fh, cape_stream_data (self->product), cape_stream_size (self->product));
  
  // clean the buffer
  cape_stream_clr (self->product);

  res = CAPE_ERR_NONE;

exit:

  return res;
}

//-----------------------------------------------------------------------------

int qcrypt__decrypt_file (const CapeString source, const CapeString dest, const CapeString vsec, CapeErr err)
{
  int res;
  
  // local objects
  char* buffer = CAPE_ALLOC (QCRYPT_BUFFER_SIZE);
  CapeFileHandle fh = cape_fh_new (NULL, dest);
  QCryptDecrypt decrypt = qcrypt_decrypt_new (NULL, source, vsec);
  
  //cape_log_fmt (CAPE_LL_TRACE, "QCRYPT", "decrypt file", "using file = '%s'", source);

  res = qcrypt_decrypt_open (decrypt, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = cape_fh_open (fh, O_CREAT | O_WRONLY | O_TRUNC, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  while (TRUE)
  {
    number_t bytes_decrypted = qcrypt_decrypt_next (decrypt, buffer, QCRYPT_BUFFER_SIZE);
    if (bytes_decrypted)
    {
      cape_fh_write_buf (fh, buffer, bytes_decrypted);
    }
    else
    {
      break;
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_fh_del (&fh);
  qcrypt_decrypt_del (&decrypt);
  CAPE_FREE (buffer);
  
  return res;
}

//-----------------------------------------------------------------------------

int qcrypt__encrypt_file (const CapeString source, const CapeString dest, const CapeString vsec, CapeErr err)
{
  int res;
  
  // local objects
  char* buffer = CAPE_ALLOC (QCRYPT_BUFFER_SIZE);
  CapeFileHandle fh = cape_fh_new (NULL, source);
  QCryptFile encrypt = qcrypt_file_new (dest);

  res = cape_fh_open (fh, O_RDONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = qcrypt_file_encrypt (encrypt, vsec, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  while (TRUE)
  {
    number_t bytes_read = cape_fh_read_buf (fh, buffer, QCRYPT_BUFFER_SIZE);
    if (bytes_read > 0)
    {
      res = qcrypt_file_write (encrypt, buffer, bytes_read, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
    else
    {
      break;
    }
  }
  
  res = qcrypt_file_finalize (encrypt, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  cape_fh_del (&fh);
  qcrypt_file_del (&encrypt);
  CAPE_FREE (buffer);

  return res;
}

//-----------------------------------------------------------------------------

int qcrypt__decrypt_json (const CapeString source, CapeUdc* p_content, const CapeString vsec, CapeErr err)
{
  int res;
  
  // local objects
  char* buffer = CAPE_ALLOC (QCRYPT_BUFFER_SIZE);
  QCryptDecrypt decrypt = qcrypt_decrypt_new (NULL, source, vsec);
  CapeStream data = cape_stream_new ();
  
  //cape_log_fmt (CAPE_LL_TRACE, "QCRYPT", "decrypt file", "using file = '%s'", source);
  
  res = qcrypt_decrypt_open (decrypt, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  while (TRUE)
  {
    number_t bytes_decrypted = qcrypt_decrypt_next (decrypt, buffer, QCRYPT_BUFFER_SIZE);
    if (bytes_decrypted)
    {
      cape_stream_append_buf (data, buffer, bytes_decrypted);
    }
    else
    {
      break;
    }
  }

  *p_content = cape_json_from_buf (cape_stream_data (data), cape_stream_size (data), qcrypt__stream_base64_decode);
  if (*p_content == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_PARSER, "can't deserialze json stream");
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_stream_del (&data);
  qcrypt_decrypt_del (&decrypt);
  CAPE_FREE (buffer);
  
  return res;
}

//-----------------------------------------------------------------------------

int qcrypt__encrypt_json (const CapeUdc content, const CapeString dest, const CapeString vsec, CapeErr err)
{
  int res;
  
  // local objects
  CapeCursor cursor = cape_cursor_new ();
  QCryptFile encrypt = qcrypt_file_new (dest);
  CapeStream source = NULL;
  CapeString data = NULL;
  
  res = qcrypt_file_encrypt (encrypt, vsec, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // serialize the object
  source = cape_json_to_stream (content);
  if (source == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't serialize UDC object");
    goto exit_and_cleanup;
  }
  
  cape_cursor_set (cursor, cape_stream_data (source), cape_stream_size (source));
  
  while (TRUE)
  {
    number_t tail = cape_cursor_tail (cursor);
    
    if (tail < QCRYPT_BUFFER_SIZE)
    {
      cape_str_del (&data);
      data = cape_cursor_scan_s (cursor, tail);

      res = qcrypt_file_write (encrypt, data, tail, err);
      if (res)
      {
        goto exit_and_cleanup;
      }

      break;
    }
    else
    {
      cape_str_del (&data);
      data = cape_cursor_scan_s (cursor, QCRYPT_BUFFER_SIZE);
      
      res = qcrypt_file_write (encrypt, data, QCRYPT_BUFFER_SIZE, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  
  res = qcrypt_file_finalize (encrypt, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  qcrypt_file_del (&encrypt);
  cape_stream_del (&source);
  cape_cursor_del (&cursor);
  cape_str_del (&data);

  return res;
}

//-----------------------------------------------------------------------------

CapeString qcrypt__hash_md5_file (const CapeString source, CapeErr err)
{
  int res;
  CapeString ret = NULL;
  
  MD5_CTX ctx;
  unsigned char c[MD5_DIGEST_LENGTH];

  // local objects
  char* buffer = CAPE_ALLOC (QCRYPT_BUFFER_SIZE);
  CapeFileHandle fh = cape_fh_new (NULL, source);
  
  res = cape_fh_open (fh, O_RDONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // init the MD5 content
  MD5_Init (&ctx);

  while (TRUE)
  {
    number_t bytes_read = cape_fh_read_buf (fh, buffer, QCRYPT_BUFFER_SIZE);
    if (bytes_read > 0)
    {
      MD5_Update (&ctx, buffer, bytes_read);
    }
    else
    {
      break;
    }
  }

  // the result will be stored in an array of short values
  MD5_Final (c, &ctx);

  // convert the md5 result into a hex-string
  {
    number_t i;
    number_t l = MD5_DIGEST_LENGTH * 2;  // for each short -> 2x hex char
    
    char* buf_hex = CAPE_ALLOC (l + 1); // add an extra byte for termination
    
    for (i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
      // print for each charater the hex presentation
      snprintf ((char*)buf_hex + (i * 2), 4, "%02x", c[i]);
    }
    
    buf_hex[l] = '\0';
    
    // create the string object
    // -> in order to keep malloc/free within the borders of a library
    // -> the malloc must be done with a cape function
    ret = cape_str_cp (buf_hex);
    
    CAPE_FREE (buf_hex);
  }
  
exit_and_cleanup:
  
  cape_fh_del (&fh);
  CAPE_FREE (buffer);

  return ret;
}

//-----------------------------------------------------------------------------
