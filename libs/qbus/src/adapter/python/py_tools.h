#ifndef __QBUS_PY_TOOLS__H
#define __QBUS_PY_TOOLS__H 1

// python includes
#include <Python.h>
#include "structmember.h"

#include "sys/cape_export.h"
#include "stc/cape_udc.h"

//-----------------------------------------------------------------------------

#if PY_MAJOR_VERSION == 2  // support also old python versions
  #define PYOBJECT_AS_STRING(s) PyString_AsString(s)
  #define PYOBJECT_IS_STRING(o) PyString_Check(o)
  #define PYOBJECT_FROM_STRING(s) PyString_FromString(s)
#else
  #define PYOBJECT_AS_STRING(s) PyUnicode_AsUTF8(s)
  #define PYOBJECT_IS_STRING(o) PyUnicode_Check(o)
  #define PYOBJECT_FROM_STRING(s) PyUnicode_FromString(s)
#endif

//-----------------------------------------------------------------------------

__CAPE_LIBEX   PyObject*   py_transform_to_pyo     (CapeUdc o);

__CAPE_LIBEX   CapeUdc     py_transform_to_udc     (PyObject* o);

//-----------------------------------------------------------------------------


#endif
