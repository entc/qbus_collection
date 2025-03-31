#ifndef __CAPE_SYS__HPP__H
#define __CAPE_SYS__HPP__H 1

#include <sys/cape_err.h>
#include <sys/cape_mutex.h>
#include <sys/cape_queue.h>
#include <sys/cape_log.h>
#include <stc/cape_udc.h>

// STL includes
#include <stdexcept>
#include <memory>

namespace cape
{
  
  //-----------------------------------------------------------------------------------------------------
  
  class Exception : public std::exception
  {
    
  public:
    
    Exception (number_t err_code, const char* err_text) : m_err_text (cape_str_cp (err_text)), m_err_code (err_code), m_udc (NULL)
    {
      if (err_text)
      {
        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "EXCEPTION", "THROW: %s", err_text);
      }
    }
    
    Exception (number_t err_code, const char* err_text, CapeUdc obj) : m_err_text (cape_str_cp (err_text)), m_err_code (err_code), m_udc (obj)
    {
      if (err_text)
      {
        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "EXCEPTION", "THROW: %s", err_text);
      }
    }
    
    ~Exception () throw()
    {
      cape_str_del (&m_err_text);
      cape_udc_del (&m_udc);
    }
    
    const char * what () const throw()
    {
      return m_err_text;
    }
    
    CapeUdc release_udc ()
    {
      return cape_udc_mv (&m_udc);
    }
    
    number_t code ()
    {
      return m_err_code;
    }
    
  private:
    
    CapeString m_err_text;
    
    number_t m_err_code;
    
    CapeUdc m_udc;
  };
  
  //-----------------------------------------------------------------------------------------------------
  
  struct ErrHolder
  {
    ErrHolder ()
    {
      err = cape_err_new ();
    }

    ~ErrHolder ()
    {
      cape_err_del (&err);
    }

    const char* text ()
    {
      return cape_err_text (err);
    }

    int code ()
    {
      return cape_err_code (err);
    }

    CapeErr err;

  };
  
  //-----------------------------------------------------------------------------------------------------
  
  class Mutex
  {

  public:

    Mutex ()
    {
      m_mutex = cape_mutex_new ();
    }

    ~Mutex ()
    {
      cape_mutex_del (&m_mutex);
    }

    void lock ()
    {
      cape_mutex_lock (m_mutex);
    }

    void unlock ()
    {
      cape_mutex_unlock (m_mutex);
    }

  private:

    CapeMutex m_mutex;

  };

  //-----------------------------------------------------------------------------------------------------
  
  class ScopeMutex
  {

  public:

    ScopeMutex (Mutex& mutex) : m_mutex (mutex)
    {
      m_mutex.lock ();
    }

    ~ScopeMutex ()
    {
      m_mutex.unlock();
    }

  private:

    Mutex& m_mutex;

  };

  //-----------------------------------------------------------------------------------------------------
  
  template <typename C> struct QueueCtx
  {
    typedef void(C::* fct_event)(number_t pos, number_t queue_size);
    
    QueueCtx (std::unique_ptr<C>& ptr, fct_event fct): m_ptr (ptr.release()), m_fct (fct) {}
    
    std::unique_ptr<C> m_ptr;
    fct_event m_fct;
  };

  //-----------------------------------------------------------------------------------------------------

  template <typename C> void __STDCALL queue__on_event (void* ptr, number_t pos, number_t queue_size)
  {
    QueueCtx<C>* ctx = static_cast<QueueCtx<C>*>(ptr);
    
    try
    {
      if (ctx->m_fct)
      {
        // black magic to call a method
        (ctx->m_ptr.get()->*ctx->m_fct)(pos, queue_size);
      }      
    }
    catch (cape::Exception& e)
    {
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "on event", "catched error: %s", e.what());
    }
    catch (std::runtime_error& e)
    {
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "on event", "catched error: %s", e.what());
    }
    catch (...)
    {
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "on event", "catched error: [unknown exception]");
    }
    
    delete ctx;
  }
  
  //-----------------------------------------------------------------------------------------------------
  
  class Queue
  {
    
  public:
    
    
    Queue (number_t timeout_in_ms): m_queue (cape_queue_new (timeout_in_ms))
    {
    }
    
    ~Queue ()
    {
      cape_queue_del (&m_queue);
    }
    
    void start (int amount_of_threads)
    {
      ErrHolder errh;
      
      int res = cape_queue_start (m_queue, amount_of_threads, errh.err);
      if (res)
      {
        throw Exception (errh.code(), errh.text());
      }
    }
    
    // add a new event to the queue
    template <typename C> void add (std::unique_ptr<C>& ptr, typename QueueCtx<C>::fct_event fct, number_t pos)
    {
      QueueCtx<C>* ctx = new QueueCtx<C> (ptr, fct);
      
      cape_queue_add (m_queue, NULL, queue__on_event <C>, NULL, NULL, ctx, pos);
    }
    
  private:
    
    CapeQueue m_queue;
    
  };

  //-----------------------------------------------------------------------------------------------------
}

#endif
