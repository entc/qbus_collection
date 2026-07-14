#ifndef __CAPE_AIO__HPP__H
#define __CAPE_AIO__HPP__H 1

// cape c includes
#include "sys/cape_aio.h"
#include "sys/cape_thread.h"

namespace cape
{

    class Aio;

    //-----------------------------------------------------------------------------------------------------

    class AioItem
    {

    private:

        AioItem (CapeAioItem obj)
        : m_obj (obj)
        {
        }

    public:

        //----------------------------------------------------------------------------------------------------

        void set (void* user_ptr, fct_cape_aio_item__on_event on_recv, fct_cape_aio_item__on_event on_send, fct_cape_aio_item__on_done on_done)
        {
            cape_aio_item_set (m_obj, user_ptr, on_recv, on_send, on_done);
        }

    private:

        CapeAioItem m_obj;

    friend Aio;

    };

    //-----------------------------------------------------------------------------------------------------

    class Aio
    {

    private:

        static int __STDCALL on_worker (void* user_ptr);

    public:

        Aio ()
        : m_obj (cape_aio_new ())
        , m_thread (NULL)
        {
        }

        //----------------------------------------------------------------------------------------------------

        ~Aio ()
        {
            if (m_thread)
            {
                cape_aio_stop (m_obj);

                cape_thread_join (m_thread);

                cape_thread_del (&m_thread);
            }

            cape_aio_del (&m_obj);
        }

        //----------------------------------------------------------------------------------------------------

        void init ()
        {
            cape::ErrHolder errh;

            // try to initialize the AIO subsystem
            if (cape_aio_init (m_obj, errh.err))
            {
                // in case of error, throw exception
                throw cape::Exception (errh.code(), errh.text());
            }
        }

        //----------------------------------------------------------------------------------------------------

        void wait ()
        {
            cape::ErrHolder errh;

            // try to initialize the AIO subsystem
            if (cape_aio_wait (m_obj, errh.err))
            {
                // in case of error, throw exception
                throw cape::Exception (errh.code(), errh.text());
            }
        }

        //----------------------------------------------------------------------------------------------------

        void wait_d ()
        {
            m_thread = cape_thread_new ();

            cape_thread_start (m_thread, Aio::on_worker, this);
        }

        //----------------------------------------------------------------------------------------------------

        void stop ()
        {
            cape_aio_stop (m_obj);
        }

        //----------------------------------------------------------------------------------------------------

        std::unique_ptr<AioItem> add (void* handle, int inital_mode = CAPE_AIO_MODE__RECV)
        {
            cape::ErrHolder errh;

            CapeAioItem item = cape_aio_add (m_obj, handle, inital_mode, errh.err);

            if (NULL == item)
            {
                // in case of error, throw exception
                throw cape::Exception (errh.code(), errh.text());
            }

            return std::unique_ptr<AioItem> (new AioItem (item));
        }

        //----------------------------------------------------------------------------------------------------

        std::unique_ptr<AioItem> add_timer (number_t interval_in_ms)
        {
            cape::ErrHolder errh;

            CapeAioItem item = cape_aio_add__timer (m_obj, interval_in_ms, errh.err);

            if (NULL == item)
            {
                // in case of error, throw exception
                throw cape::Exception (errh.code(), errh.text());
            }

            return std::unique_ptr<AioItem> (new AioItem (item));
        }

        //----------------------------------------------------------------------------------------------------

    public:

        CapeAio m_obj;

        CapeThread m_thread;

    };

    //----------------------------------------------------------------------------------------------------

    inline int __STDCALL Aio::on_worker (void* user_ptr)
    {
        try
        {
            static_cast<Aio*> (user_ptr)->wait ();
        }
        catch (cape::Exception& e)
        {

        }
        catch (std::exception& e)
        {

        }
        catch (...)
        {

        }

        return FALSE;
    }

    //----------------------------------------------------------------------------------------------------

}

#endif
