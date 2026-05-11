#include "py_qbus.h"
#include "py_tools.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>
#include <aio/cape_aio_timer.h>

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_new (PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    PyObject* pyo_name;
    PyObject* pyo_args;
        
    printf("Py_None ref BEFORE=%ld\n", Py_REFCNT(Py_None));

    if (!PyArg_ParseTuple (args, "OO", &pyo_name, &pyo_args))
    {
        return NULL;  // Exception was already set
    }

    if (!PYOBJECT_IS_STRING (pyo_name))
    {
        PyErr_SetString (PyExc_ValueError, "parameter name must be a string value");
        return NULL;
    }

    if (!PyDict_Check (pyo_args))
    {
        PyErr_SetString (PyExc_ValueError, "parameter args must be an object");
        return NULL;
    }
    
    CapeUdc cape_args = py_transform_to_udc (pyo_args);
    if (NULL == cape_args)
    {
        PyErr_SetString (PyExc_ValueError, "parameter args are not valid arguments");
        return NULL;
    }

    {
        PyObject_QBus* self = (PyObject_QBus*)type->tp_alloc(type, 0);

        self->qbus = qbus_new (PYOBJECT_AS_STRING (pyo_name), &cape_args);

        return (PyObject*) self;
    }
}

//-----------------------------------------------------------------------------

void py_object_qbus_del (PyObject_QBus* self)
{
    // increase refcounter to avoid that Py_DECREF will call again the destructor
    Py_INCREF (self);
    
    qbus_del (&(self->qbus));

  //  Py_DECREF (self);
    
    Py_TYPE(self)->tp_free((PyObject *) self);
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_run (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
    PyObject* ret = Py_None;

    // local objects
    CapeErr err = cape_err_new ();

    if (qbus_run (self->qbus, err))
    {
        PyErr_SetString(PyExc_RuntimeError, cape_err_text (err));
        
        ret = NULL; // tell python an error as occoured
    }

    cape_err_del (&err);

    return ret;
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_run_d (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
    PyObject* ret = Py_None;

    // local objects
    CapeErr err = cape_err_new ();

    if (qbus_run__d (self->qbus, err))
    {
        PyErr_SetString(PyExc_RuntimeError, cape_err_text (err));
        
        ret = NULL; // tell python an error as occoured
    }

    cape_err_del (&err);

    return ret;
}

//-----------------------------------------------------------------------------

typedef struct {
  
    PyObject_QBus* self;
    
    PyObject* on_init;
    PyObject* on_done;
  
    PyObject* object;
    
} PyObjectCallbacks;

//-----------------------------------------------------------------------------

int __STDCALL py_object_qbus__on_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
    PyObjectCallbacks* callbacks_ctx = ptr;

    // return object
    PyObject* res;
    
    // use the same ptr
    *p_ptr = callbacks_ctx;
    
    printf("Py_None ref BEFORE=%ld\n", Py_REFCNT(Py_None));



    // call the python on_init method
    {
        PyObject* arglist = Py_BuildValue ("(O)", callbacks_ctx->self);

        res = PyObject_Call (callbacks_ctx->on_init, arglist, NULL);

        Py_DECREF (arglist);
    }

    if (NULL == res)
    {
        PyErr_Print();  // python error handling
        
        return cape_err_set (err, CAPE_ERR_RUNTIME, "runtime error");
    }
    else
    {
        callbacks_ctx->object = res;
        
        return CAPE_ERR_NONE;
    }
}

//-----------------------------------------------------------------------------

int __STDCALL py_object_qbus__on_done (QBus qbus, void* ptr, CapeErr err)
{
    PyObjectCallbacks* callbacks_ctx = ptr;

    // return object
    PyObject* res;

    // call the python on_done method
    {
        PyObject* arglist = Py_BuildValue ("(OO)", callbacks_ctx->self, callbacks_ctx->object);
        
        res = PyObject_Call (callbacks_ctx->on_done, arglist, NULL);

        Py_DECREF (arglist);
    }

    Py_DECREF (callbacks_ctx->on_init);
    Py_DECREF (callbacks_ctx->on_done);

    if (callbacks_ctx->object)
    {
        Py_DECREF (callbacks_ctx->object);
    }

    CAPE_DEL(&callbacks_ctx, PyObjectCallbacks);

    if (NULL == res)
    {
        PyErr_Print();  // python error handling

        return cape_err_set (err, CAPE_ERR_RUNTIME, "runtime error");
    }
    else
    {
        Py_DECREF (res);

        return CAPE_ERR_NONE;
    }
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_set_cb (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
    PyObject* on_init;
    PyObject* on_done;

    if (!PyArg_ParseTuple (args, "OO", &on_init, &on_done))
    {
        return NULL;  // Exception was already set
    }

    if (!PyCallable_Check (on_init))
    {
        PyErr_SetString (PyExc_ValueError, "parameter 1 must be an function");
        return NULL;
    }
    
    if (!PyCallable_Check (on_done))
    {
        PyErr_SetString (PyExc_ValueError, "parameter 2 must be an function");
        return NULL;
    }

    {
        PyObjectCallbacks* callbacks_ctx = CAPE_NEW (PyObjectCallbacks);
        
        callbacks_ctx->self = self;
        callbacks_ctx->on_init = on_init;
        callbacks_ctx->on_done = on_done;
        
        qbus_set_cb (self->qbus, callbacks_ctx, py_object_qbus__on_init, py_object_qbus__on_done);
    }
        
exit_and_error:
    
    return Py_None;
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_new_e (PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    PyObject_QBus* self = (PyObject_QBus*)type->tp_alloc(type, 0);
    
    self->qbus = NULL;
    
    return (PyObject*)self;
}

//-----------------------------------------------------------------------------

void py_object_qbus_del_e (PyObject_QBus* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_wait (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
  PyObject* ret = Py_None;

  CapeUdc cape_argument = NULL;

  PyObject* argument;

  CapeErr err = cape_err_new ();

  if (!PyArg_ParseTuple (args, "O", &argument))
  {
    return NULL;
  }

  if (argument)
  {
    cape_argument = py_transform_to_udc (argument);
  }

  if (NULL == cape_argument)
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid input parameters");
    goto exit_and_error;
  }

  if (cape_argument)
  {
    CapeString h = cape_json_to_s (cape_argument);

    printf ("B: %s\n", h);

    cape_str_del (&h);
  }

  {
    int res = qbus_wait (self->qbus, err);
    if (res)
    {
      printf ("ERROR: %s\n", cape_err_text (err));

      goto exit_and_error;
    }
  }

exit_and_error:

  if (cape_err_code (err))
  {
    PyErr_SetString(PyExc_RuntimeError, cape_err_text (err));

    // tell python an error ocoured
    ret = NULL;
  }

  cape_udc_del (&cape_argument);

  cape_err_del (&err);

  return ret;
}

//-----------------------------------------------------------------------------

typedef struct {

  PyObject* fct;
  PyObject_QBus* qbus;

} PythonCallbackData;

//-----------------------------------------------------------------------------

int __STDCALL py_object_qbus_register__on_message (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  PythonCallbackData* pcd = ptr;

  PyObject* arglist;
  PyObject* result;

  PyObject* py_qin = Py_None;

  if (qin->cdata)
  {
    py_qin = py_transform_to_pyo (qin->cdata);
  }

  arglist = Py_BuildValue ("(O)", py_qin);

  // depricated
  //result = PyEval_CallObject (pcd->fct, arglist);

  result = PyObject_Call (pcd->fct, arglist, NULL);
  if (result)
  {
    qout->cdata = py_transform_to_udc (result);
    qout->mtype = QBUS_MTYPE_JSON;

    Py_XDECREF (result);
  }
  else
  {
    // some error happened, tell python
    PyErr_Print();

    // we need to clean, otherwise it will crash at some point
    PyErr_Clear();
  }

  // cleanup
  Py_XDECREF (py_qin);
  Py_DECREF (arglist);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static void __STDCALL py_object_qbus_register__on_removed (void* ptr)
{
  PythonCallbackData* pcd = ptr;

  Py_DECREF (pcd->fct);

  CAPE_DEL(&pcd, PythonCallbackData);
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_register (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
  PyObject* ret = Py_None;

  CapeErr err = cape_err_new ();

  PyObject* name;
  PyObject* cbfct;

  if (!PyArg_ParseTuple (args, "OO", &name, &cbfct))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid parameters");
    goto exit_and_error;
  }

  if (!PYOBJECT_IS_STRING (name))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "1. parameter is not a string");
    goto exit_and_error;
  }

  if (!PyCallable_Check (cbfct))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "2. parameter is not a callback");
    goto exit_and_error;
  }

  {
    PythonCallbackData* pcd = CAPE_NEW (PythonCallbackData);

    pcd->fct = cbfct;

    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "py adapter", "register callback %s", PYOBJECT_AS_STRING (name));

    {
      int res = qbus_register (self->qbus, PYOBJECT_AS_STRING (name), pcd, py_object_qbus_register__on_message, py_object_qbus_register__on_removed, err);
      if (res)
      {
        goto exit_and_error;
      }
    }
  }

exit_and_error:

  if (cape_err_code (err))
  {
    PyErr_SetString(PyExc_RuntimeError, cape_err_text (err));

    // tell python an error occoured
    ret = NULL;
  }

  cape_err_del (&err);

  return ret;
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_modules (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
    CapeUdc list_of_modules = qbus_modules (self->qbus);

    PyObject* ret = py_transform_to_pyo (list_of_modules);

    cape_udc_del (&list_of_modules);

    return ret;
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_methods (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
    PyObject* ret;  // return value
    PyObject* cid;  // borrowed object

    if (!PyArg_ParseTuple (args, "O", &cid))
    {
        return NULL;  // Exception was already set
    }

    if (cid == Py_None)
    {
        {
            // create the methods of the own module
            CapeUdc list_of_methods = qbus_methods (self->qbus, NULL);

            ret = py_transform_to_pyo (list_of_methods);

            cape_udc_del (&list_of_methods);
        }
    }
    else
    {
        if (!PYOBJECT_IS_STRING (cid))
        {
            PyErr_SetString (PyExc_ValueError, "parameter name must be a string value");
            return NULL;
        }

        {
            // create the methods of a remote module identified by cid
            CapeUdc list_of_methods = qbus_methods (self->qbus, PYOBJECT_AS_STRING (cid));

            ret = py_transform_to_pyo (list_of_methods);

            cape_udc_del (&list_of_methods);
        }
    }

    return ret;
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_config (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
  PyObject* ret = Py_None;

  CapeErr err = cape_err_new ();

  PyObject* name;
  PyObject* default_val;

  if (!PyArg_ParseTuple (args, "OO", &name, &default_val))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid parameters");
    goto exit_and_error;
  }

  if (!PYOBJECT_IS_STRING (name))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "1. parameter is not a string");
    goto exit_and_error;
  }

  if (PYOBJECT_IS_STRING (default_val))
  {
    const CapeString h = qbus_config_s (qbus_config (self->qbus), PYOBJECT_AS_STRING (name), PYOBJECT_AS_STRING (default_val));

    ret = PYOBJECT_FROM_STRING (h);
  }
  else if (PyLong_Check (default_val))
  {
    number_t h = qbus_config_n (qbus_config (self->qbus), PYOBJECT_AS_STRING (name), PyLong_AsLong (default_val));

    ret = PyLong_FromLong (h);
  }
  else if (PyFloat_Check (default_val))
  {
    double h = qbus_config_f (qbus_config (self->qbus), PYOBJECT_AS_STRING (name), PyFloat_AsDouble (default_val));

    ret = PyFloat_FromDouble (h);
  }
  else if (PyBool_Check (default_val))
  {
    int h = qbus_config_b (qbus_config (self->qbus), PYOBJECT_AS_STRING (name), default_val == Py_True);

    ret = PyBool_FromLong (h);
  }
  else
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "2. parameter has not supported type");
    goto exit_and_error;
  }

exit_and_error:

  if (cape_err_code (err))
  {
    PyErr_SetString(PyExc_RuntimeError, cape_err_text (err));

    // tell python an error occoured
    ret = NULL;
  }

  cape_err_del (&err);

  return ret;
}

//-----------------------------------------------------------------------------

int __STDCALL py_object_qbus_send__on_event (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
    int res;
    PythonCallbackData* pcd = ptr;

    // local objects
    PyObject* arglist;
    PyObject* result;
    PyObject* py_qin;

    // IMPORTANT: start thread safe monitor
    PyGILState_STATE gstate = PyGILState_Ensure();
    
    if (qin->cdata)
    {
        py_qin = py_transform_to_pyo (qin->cdata);
    }
    else
    {
        py_qin = Py_None;
    }

    arglist = Py_BuildValue ("(OOO)", pcd->qbus, py_qin, Py_None);

    result = PyObject_Call (pcd->fct, arglist, NULL);

    /*
  if (result)
  {
    qout->cdata = py_transform_to_udc (result);
    qout->mtype = QBUS_MTYPE_JSON;

    Py_XDECREF (result);
  }
  else
  {
    // some error happened, tell python
    PyErr_Print();

    // we need to clean, otherwise it will crash at some point
    PyErr_Clear();
  }
  */

    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "py adapter", "cleanup from message");

    // cleanup
    Py_DECREF (py_qin);
    Py_DECREF (arglist);

    //Py_DECREF (pcd->fct);
    CAPE_DEL(&pcd, PythonCallbackData);

    PyGILState_Release(gstate);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_send (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
    PyObject* ret = Py_None;

    // local objects
    CapeErr err = cape_err_new ();
    QBusM qin = NULL;

    PyObject* module;
    PyObject* method;
    PyObject* clist;
    PyObject* cdata;
    PyObject* cbfct;

    // IMPORTANT: start thread safe monitor
    PyGILState_STATE gstate = PyGILState_Ensure();

    if (!PyArg_ParseTuple (args, "OOOOO", &module, &method, &clist, &cdata, &cbfct))
    {
        ret = NULL;  // Exception was already set
        goto exit_and_error;
    }

    if (!PYOBJECT_IS_STRING (module))
    {
        PyErr_SetString (PyExc_ValueError, "1. parameter is not a string");
        
        ret = NULL;
        goto exit_and_error;
    }

    if (!PYOBJECT_IS_STRING (method))
    {
        PyErr_SetString (PyExc_ValueError, "2. parameter is not a string");

        ret = NULL;
        goto exit_and_error;
    }

    // optional
    if ((clist != Py_None) && (!PyList_Check (clist)))
    {
        PyErr_SetString (PyExc_ValueError, "3. parameter is not an object");

        ret = NULL;
        goto exit_and_error;
    }

    // optional
    if ((cdata != Py_None) && (!PyDict_Check (cdata)))
    {
        PyErr_SetString (PyExc_ValueError, "4. parameter is not an object");

        ret = NULL;
        goto exit_and_error;
    }

    // optional
    if ((cbfct != Py_None) && (!PyCallable_Check (cbfct)))
    {
        PyErr_SetString (PyExc_ValueError, "5. parameter is not a callback");

        ret = NULL;
        goto exit_and_error;
    }
    
    qin = qbus_message_new (NULL, NULL);

    if (clist != Py_None)
    {
        qin->clist = py_transform_to_udc (clist);
    }

    if (cdata != Py_None)
    {
        qin->cdata = py_transform_to_udc (cdata);
    }

    if (cbfct == Py_None)
    {
        int res = qbus_send (self->qbus, PYOBJECT_AS_STRING (module), PYOBJECT_AS_STRING (method), qin, NULL, NULL, err);
        if (res && res != CAPE_ERR_CONTINUE)
        {

        }
    }
    else
    {
        PythonCallbackData* pcd = CAPE_NEW (PythonCallbackData);

        pcd->fct = cbfct;
        pcd->qbus = self;

        cape_log_fmt (CAPE_LL_TRACE, "QBUS", "py adapter", "send message to %s", PYOBJECT_AS_STRING (module));

        int res = qbus_send (self->qbus, PYOBJECT_AS_STRING (module), PYOBJECT_AS_STRING (method), qin, pcd, py_object_qbus_send__on_event, err);
        if (res && res != CAPE_ERR_CONTINUE)
        {

        }
    }
  
exit_and_error:

    PyGILState_Release(gstate);

    qbus_message_del (&qin);
    cape_err_del (&err);

    return ret;
}

//-----------------------------------------------------------------------------

int __STDCALL py_object_qbus_timer__on_timer (void* ptr)
{
  int ret = TRUE;
  PythonCallbackData* pcd = ptr;

  PyObject* arglist;
  PyObject* result;

  arglist = Py_BuildValue ("(O)", pcd->qbus);

  result = PyObject_Call (pcd->fct, arglist, NULL);

  /*
  if (result)
  {


    Py_XDECREF (result);
  }
  else
  {
    // some error happened, tell python
    PyErr_Print();

    // we need to clean, otherwise it will crash at some point
    PyErr_Clear();
  }
  */

  // cleanup
  Py_DECREF (arglist);

  // continue
  return TRUE;
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_timer (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
  PyObject* ret = Py_None;

  // local objects
  CapeErr err = cape_err_new ();

  PyObject* timeoit_in_ms;
  PyObject* cbfct;

  if (!PyArg_ParseTuple (args, "OO", &timeoit_in_ms, &cbfct))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid parameters");
    goto exit_and_error;
  }

  if (!PyLong_Check (timeoit_in_ms))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "1. parameter is not a number");
    goto exit_and_error;
  }

  if (!PyCallable_Check (cbfct))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "2. parameter is not a callback");
    goto exit_and_error;
  }

  {
    int res;
    CapeAioTimer timer = cape_aio_timer_new ();

    PythonCallbackData* pcd = CAPE_NEW (PythonCallbackData);

    pcd->fct = cbfct;

    Py_INCREF(self);
    pcd->qbus = self;

    res = cape_aio_timer_set (timer, PyLong_AsLong (timeoit_in_ms), pcd, py_object_qbus_timer__on_timer, err);
    if (res)
    {

    }

    res = cape_aio_timer_add (&timer, qbus_aio (self->qbus));
    if (res)
    {

    }
  }


exit_and_error:

  if (cape_err_code (err))
  {
    PyErr_SetString(PyExc_RuntimeError, cape_err_text (err));

    // tell python an error occoured
    ret = NULL;
  }

  cape_err_del (&err);

  return ret;
}

//-----------------------------------------------------------------------------

int __STDCALL py_object_qbus_timer__on_once (void* ptr)
{
  PythonCallbackData* pcd = ptr;
  
  PyObject* arglist = Py_BuildValue ("(O)", pcd->qbus);
  
  PyObject_Call (pcd->fct, arglist, NULL);
  
  // cleanup
  Py_DECREF (arglist);
  
  // continue
  return FALSE;
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_once (PyObject_QBus* self, PyObject* args, PyObject* kwds)
{
  PyObject* ret = Py_None;
  
  // local objects
  CapeErr err = cape_err_new ();
  
  PyObject* timeoit_in_ms;
  PyObject* cbfct;
  
  if (!PyArg_ParseTuple (args, "OO", &timeoit_in_ms, &cbfct))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid parameters");
    goto exit_and_error;
  }
  
  if (!PyLong_Check (timeoit_in_ms))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "1. parameter is not a number");
    goto exit_and_error;
  }
  
  if (!PyCallable_Check (cbfct))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "2. parameter is not a callback");
    goto exit_and_error;
  }
  
  {
    int res;
    CapeAioTimer timer = cape_aio_timer_new ();
    
    PythonCallbackData* pcd = CAPE_NEW (PythonCallbackData);
    
    pcd->fct = cbfct;
    
    Py_INCREF(self);
    pcd->qbus = self;
    
    res = cape_aio_timer_set (timer, PyLong_AsLong (timeoit_in_ms), pcd, py_object_qbus_timer__on_once, err);
    if (res)
    {
      
    }
    
    res = cape_aio_timer_add (&timer, qbus_aio (self->qbus));
    if (res)
    {
      
    }
  }
  
  
exit_and_error:
  
  if (cape_err_code (err))
  {
    PyErr_SetString(PyExc_RuntimeError, cape_err_text (err));
    
    // tell python an error occoured
    ret = NULL;
  }
  
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_msg_new (PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  PyObject_QBusMsg* self = (PyObject_QBusMsg*)type->tp_alloc(type, 0);

  self->message = NULL;

  return (PyObject*) self;
}

//-----------------------------------------------------------------------------

void py_object_qbus_msg_del (PyObject_QBusMsg* self)
{
  if (self->message)
  {
    qbus_message_del(&(self->message));
  }

  Py_TYPE(self)->tp_free((PyObject *) self);
}

//-----------------------------------------------------------------------------

int py_object_qbus_msg_init (PyObject_QBusMsg* self, PyObject *args, PyObject *kwds)
{
  PyObject* name;


  // create a new qbus object
  self->message = qbus_message_new (NULL, NULL);

  return 0;
}

//-----------------------------------------------------------------------------
