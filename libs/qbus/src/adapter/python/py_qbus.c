#include "py_qbus.h"
#include "py_tools.h"

// cape includes
#include <fmt/cape_json.h>
#include <sys/cape_log.h>
#include <aio/cape_aio_timer.h>

//-----------------------------------------------------------------------------

PyObject* py_object_qbus_new (PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  PyObject_QBus* self = (PyObject_QBus*)type->tp_alloc(type, 0);

  self->qbus = NULL;
  
  return (PyObject*) self;
}

//-----------------------------------------------------------------------------

void py_object_qbus_del (PyObject_QBus* self)
{
  if (self->qbus)
  {
    qbus_del (&(self->qbus));
  }
  
  Py_TYPE(self)->tp_free((PyObject *) self);
}

//-----------------------------------------------------------------------------

int py_object_qbus_init (PyObject_QBus* self, PyObject *args, PyObject *kwds)
{
  PyObject* name;
  
  if (!PyArg_ParseTuple (args, "O", &name))
  {
    return -1;
  }  

  if (!PYOBJECT_IS_STRING (name))
  {
    return -1;
  }

  // create a new qbus object
  self->qbus = qbus_new (PYOBJECT_AS_STRING(name));
  
  return 0;
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
  
  CapeUdc cape_arg1 = NULL;
  CapeUdc cape_arg2 = NULL;
  
  PyObject* arg1;
  PyObject* arg2;
  
  CapeErr err = cape_err_new ();
  
  if (!PyArg_ParseTuple (args, "OO", &arg1, &arg2))
  {
    return NULL;
  }
  
  if (arg1)
  {
    cape_arg1 = py_transform_to_udc (arg1);
  }
  
  if (arg2)
  {
    cape_arg2 = py_transform_to_udc (arg2);
  }
  
  if ((NULL == cape_arg1) && (NULL == cape_arg2))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid input parameters");
    goto exit_and_error;
  }
  
  if (cape_arg1)
  {
    CapeString h = cape_json_to_s (cape_arg1);
    
    printf ("B: %s\n", h);
    
    cape_str_del (&h);
  }
  
  if (cape_arg2)
  {
    CapeString h = cape_json_to_s (cape_arg2);
    
    printf ("R: %s\n", h);
    
    cape_str_del (&h);
  }
  
  {
    int res = qbus_wait (self->qbus, cape_arg1, cape_arg2, 4, err);
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
  
  cape_udc_del (&cape_arg1);
  cape_udc_del (&cape_arg2);
  
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
    const CapeString h = qbus_config_s (self->qbus, PYOBJECT_AS_STRING (name), PYOBJECT_AS_STRING (default_val));

    ret = PYOBJECT_FROM_STRING (h);
  }
  else if (PyLong_Check (default_val))
  {
    number_t h = qbus_config_n (self->qbus, PYOBJECT_AS_STRING (name), PyLong_AsLong (default_val));
    
    ret = PyLong_FromLong (h);
  }
  else if (PyFloat_Check (default_val))
  {
    double h = qbus_config_f (self->qbus, PYOBJECT_AS_STRING (name), PyFloat_AsDouble (default_val));
    
    ret = PyFloat_FromDouble (h);
  }
  else if (PyBool_Check (default_val))
  {
    int h = qbus_config_b (self->qbus, PYOBJECT_AS_STRING (name), default_val == Py_True);
    
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
  
  PyObject* arglist;
  PyObject* result;
  
  PyObject* py_qin = Py_None;
  
  if (qin->cdata)
  {
    py_qin = py_transform_to_pyo (qin->cdata);
  }
  
  arglist = Py_BuildValue ("(OOO)", pcd->qbus, py_qin, Py_None);
  
  // depricated
  //result = PyEval_CallObject (pcd->fct, arglist);
  
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
  
  // cleanup
  Py_XDECREF (py_qin);
  Py_DECREF (arglist);
  
  //Py_DECREF (pcd->fct);
  CAPE_DEL(&pcd, PythonCallbackData);
  
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
  
  if (!PyArg_ParseTuple (args, "OOOOO", &module, &method, &clist, &cdata, &cbfct))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid parameters");
    goto exit_and_error;
  }
  
  if (!PYOBJECT_IS_STRING (module))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "1. parameter is not a string");
    goto exit_and_error;
  }

  if (!PYOBJECT_IS_STRING (method))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "2. parameter is not a string");
    goto exit_and_error;
  }
  
  if (!PyList_Check (clist))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "3. parameter is not an object");
    goto exit_and_error;
  }

  if (!PyDict_Check (cdata))
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "3. parameter is not an object");
    goto exit_and_error;
  }
  
  if (!PyCallable_Check (cbfct)) 
  {
    cape_err_set (err, CAPE_ERR_MISSING_PARAM, "4. parameter is not a callback");
    goto exit_and_error;
  }
  
  qin = qbus_message_new (NULL, NULL);
  
  qin->clist = py_transform_to_udc (clist);
  qin->cdata = py_transform_to_udc (cdata);

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
  
  if (cape_err_code (err))
  {
    PyErr_SetString(PyExc_RuntimeError, cape_err_text (err));
    
    // tell python an error occoured
    ret = NULL;
  }
  
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

  if (FALSE == ret)
  {
    // in case of the last event cleanup
    Py_DECREF (pcd->qbus);
    
    CAPE_DEL(&pcd, PythonCallbackData);
  }
    
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
