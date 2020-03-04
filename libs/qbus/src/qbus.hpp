#ifndef __QBUS__HPP__H
#define __QBUS__HPP__H 1

// STL includes
#include <stdexcept>

// cape includes
#include <hpp/cape_stc.hpp>
#include <hpp/cape_sys.hpp>

// qbus include
#include <qbus.h>
#include <stdexcept>

namespace qbus
{

  //-----------------------------------------------------------------------------
  
  class Message
  {
    
  public:
    
    Message (QBus qbus, QBusM qin, QBusM qout) : m_qbus (qbus), m_qin (qin), m_qout (qout), m_cdata_in (qin->cdata), m_ret (CAPE_ERR_NONE) {}
    
    void set_continue (cape::Udc& content)
    {
      m_ret = CAPE_ERR_CONTINUE;
      
      qbus_message_clr (m_qin, CAPE_UDC_UNDEFINED);
      
      if (content.valid())
      {
        m_qin->cdata = content.release();
      }
    }
    
    cape::Udc& cdata () { return m_cdata_in; }
    
    cape::Udc& cdata_valid (int type)
    {
      if (m_cdata_in.empty())
      {
        throw std::runtime_error ("no 'cdata'");
      }
      
      if (m_cdata_in.type() != type)
      {
        throw std::runtime_error ("cdata' is not a node");
      }
      
      return m_cdata_in;
    }
    
    cape::Udc output (int type)
    {
      if (m_qout == NULL)
      {
        throw std::runtime_error ("output can't be set");
      }
      
      m_qout->cdata = cape_udc_new (type, NULL);
      
      return cape::Udc (m_qout->cdata);
    }
    
    void forward ()
    {
      cape_udc_replace_mv (&(m_qout->cdata), &(m_qin->cdata));
    }
    
    QBus qbus () { return m_qbus; }
    
    QBusM qin () { return m_qin; }
    
    int ret () { return m_ret; }
    
  private:
    
    QBus m_qbus;
    
    QBusM m_qin;
    
    QBusM m_qout;
    
    cape::Udc m_cdata_in;
    
    int m_ret;
    
  };
  
  //-----------------------------------------------------------------------------

  template <typename C> struct NextMessageContext
  {
    typedef void(C::* fct_next_message)(qbus::Message& msg);
    
    fct_next_message fct;
    C* ptr;
  };
  
  //-----------------------------------------------------------------------------

  template <typename C> int __STDCALL send_next_message__on_message (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err);
  
  template <typename C> struct NextMessage
  {
    typedef void(C::* fct_next_message)(qbus::Message& msg);
    
    static void send (qbus::Message& msg, const char* module, const char* method, cape::Udc& content, C* ptr, fct_next_message fct)
    {
      cape::ErrHolder errh;
      
      msg.set_continue (content);
      
      NextMessageContext<C>* ctx = new NextMessageContext<C>;
      
      ctx->fct = fct;
      ctx->ptr = ptr;
      
      int res = qbus_continue (msg.qbus(), module, method, msg.qin(), (void**)&ctx, send_next_message__on_message <C>, errh.err);      
      
      if (res != CAPE_ERR_CONTINUE)
      {
        throw cape::Exception (cape_err_code (errh.err), errh.text());
      }
    }
  };
  
  //-----------------------------------------------------------------------------

  template <typename C> int __STDCALL send_next_message__on_message (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
  {
    int res;
    NextMessageContext<C>* ctx = static_cast<NextMessageContext<C>*> (ptr);
    
    try
    {
      qbus::Message msg (qbus, qin, qout);
      
      // black magic to call a method
      (ctx->ptr->*ctx->fct)(msg);
      
      res = msg.ret();
    }
    catch (std::runtime_error& e)
    {
      res = cape_err_set (err, CAPE_ERR_RUNTIME, e.what());
    }
    
    delete ctx;
    return res;
  }
  
  //-----------------------------------------------------------------------------
}

#endif
