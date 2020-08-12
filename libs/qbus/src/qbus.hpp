#ifndef __QBUS__HPP__H
#define __QBUS__HPP__H 1

// STL includes
#include <stdexcept>
#include <memory>

// cape includes
#include <hpp/cape_stc.hpp>
#include <hpp/cape_sys.hpp>

// qbus include
#include <qbus.h>

namespace qbus
{

  //-----------------------------------------------------------------------------
  
  class Message
  {
    
  public:
    
    Message (QBus qbus, QBusM qin, QBusM qout)
    : m_qbus (qbus)
    , m_qin (qin)
    , m_qout (qout)
    , m_cdata_in (qin->cdata)
    , m_pdata_in (qin->pdata)
    , m_ret (CAPE_ERR_NONE)
    {
    }
    
    ~Message ()
    {
      if (m_qout)
      {
        if (m_out_cdata.valid())
        {
          std::cout << m_out_cdata << std::endl;
          
          m_qout->cdata = m_out_cdata.release();
        }
        else
        {
          m_qout->cdata = NULL;
        }
      }
    }
    
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

    cape::Udc& pdata () { return m_pdata_in; }
    
    cape::Udc& odata_owned (int type)
    {
      if (m_qout == NULL)
      {
        throw std::runtime_error ("output can't be set");
      }
      
      m_out_cdata.reset (cape::Udc (type));
      return m_out_cdata;
    }
    
    cape::Udc output (int type)
    {
      if (m_qout == NULL)
      {
        throw std::runtime_error ("output can't be set");
      }

      m_out_cdata.reset (cape::Udc (type));

      cape::Udc h;
      h.reset_as_reference (m_out_cdata);
      
      return h;
    }
    
    void throw_error ()
    {
      if (m_qin->err)
      {
        throw cape::Exception (cape_err_code (m_qin->err), cape_err_text (m_qin->err));
      }
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
    cape::Udc m_pdata_in;
    
    int m_ret;
    
    cape::Udc m_out_cdata;
    
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

  template <typename C> struct NewMessage
  {
    typedef void(C::* fct_next_message)(qbus::Message& msg);

    static void send (QBus qbus, const char* module, const char* method, cape::Udc& pdata, cape::Udc& cdata, C* ptr, fct_next_message fct)
    {
      cape::ErrHolder errh;
      
      QBusM msg = qbus_message_new (NULL, NULL);
      
      if (pdata.valid())
      {
        msg->pdata = pdata.release ();
      }
      
      if (cdata.valid())
      {
        msg->cdata = cdata.release ();
      }
      
      NextMessageContext<C>* ctx = new NextMessageContext<C>;
      
      ctx->fct = fct;
      ctx->ptr = ptr;
      
      int res = qbus_send (qbus, module, method, msg, (void*)ctx, send_next_message__on_message <C>, errh.err);

      qbus_message_del (&msg);
      
      if (res)
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
      if (ctx->fct && ctx->ptr)
      {
        qbus::Message msg (qbus, qin, qout);
        
        // black magic to call a method
        (ctx->ptr->*ctx->fct)(msg);
        
        res = msg.ret();
      }
      else
      {
        res = CAPE_ERR_NONE;
      }
    }
    catch (cape::Exception& e)
    {
      cape_log_fmt (CAPE_LL_ERROR, "QBUS", "on message", "catched error: %s", e.what());
      
      res = cape_err_set (err, e.code(), e.what());
    }
    catch (std::runtime_error& e)
    {
      cape_log_fmt (CAPE_LL_ERROR, "QBUS", "on message", "catched error: %s", e.what());

      res = cape_err_set (err, CAPE_ERR_RUNTIME, e.what());
    }
    catch (...)
    {
      cape_log_fmt (CAPE_LL_ERROR, "QBUS", "on message", "catched error: [unknown exception]");

      res = cape_err_set (err, CAPE_ERR_RUNTIME, "[unknown exception]");
    }
    
    delete ctx;
    return res;
  }
  
  //-----------------------------------------------------------------------------

  template <typename C> int __STDCALL message_send__on_message (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err);

  template <typename C> struct MessageSendContext
  {
    typedef void(*fct_next_message)(QBus qbus, std::unique_ptr<C>& self, qbus::Message& msg);

    fct_next_message fct;
    C* ptr;
  };

  //-----------------------------------------------------------------------------

  template <typename C> class MessageSend
  {
    typedef void(*fct_next_message)(QBus qbus, std::unique_ptr<C>& self, qbus::Message& msg);

  public:

    MessageSend () : m_msg (qbus_message_new (NULL, NULL))
    {
    }
    
    ~MessageSend ()
    {
      qbus_message_del (&m_msg);
    }
    
    void set_cdata (cape::Udc& cdata)
    {
      if (cdata.valid() && cdata.type() == CAPE_UDC_NODE)
      {
        m_msg->cdata = cdata.release ();
      }
    }

    void set_pdata (cape::Udc& pdata)
    {
      if (pdata.valid() && pdata.type() == CAPE_UDC_NODE)
      {
        m_msg->pdata = pdata.release ();
      }
    }

    void set_clist (cape::Udc& clist)
    {
      if (clist.valid() && clist.type() == CAPE_UDC_LIST)
      {
        m_msg->clist = clist.release ();
      }
    }

    void send (QBus qbus, const char* module, const char* method, std::unique_ptr<C>& self, fct_next_message fct)
    {
      cape::ErrHolder errh;

      MessageSendContext<C>* ctx = new MessageSendContext<C>;
      
      ctx->fct = fct;
      ctx->ptr = self.release();
      
      int res = qbus_send (qbus, module, method, m_msg, (void*)ctx, message_send__on_message <C>, errh.err);
      if (res)
      {
        throw cape::Exception (cape_err_code (errh.err), errh.text());
      }
    }
    
  private:
    
    QBusM m_msg;
    
  };

  //-----------------------------------------------------------------------------

  template <typename C> int __STDCALL message_send__on_message (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
  {
    int res;
    MessageSendContext<C>* ctx = static_cast<MessageSendContext<C>*> (ptr);
    
    try
    {
      std::unique_ptr<C> self (static_cast<C*>(ctx->ptr));
      
      qbus::Message msg (qbus, qin, qout);
      
      // black magic to call a method
      ctx->fct (qbus, self, msg);
      
      res = msg.ret();
    }
    catch (cape::Exception& e)
    {
      cape_log_fmt (CAPE_LL_ERROR, "QBUS", "on message", "catched error: %s", e.what());
      
      res = cape_err_set (err, e.code(), e.what());
    }
    catch (std::runtime_error& e)
    {
      cape_log_fmt (CAPE_LL_ERROR, "QBUS", "on message", "catched error: %s", e.what());

      res = cape_err_set (err, CAPE_ERR_RUNTIME, e.what());
    }
    catch (...)
    {
      cape_log_fmt (CAPE_LL_ERROR, "QBUS", "on message", "catched error: [unknown exception]");

      res = cape_err_set (err, CAPE_ERR_RUNTIME, "[unknown exception]");
    }
    
    delete ctx;
    return res;
  }

  //-----------------------------------------------------------------------------

}

#endif
