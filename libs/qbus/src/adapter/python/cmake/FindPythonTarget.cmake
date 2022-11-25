#-------------------------------------------------------------------------------
# Copyright (c) 2018-2022, Alexander Kalkhof <alex@kalkhof.org>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#  * Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#-------------------------------------------------------------------------------

if(DEFINED ENV{python_target})

  message (STATUS "SEEK FOR PYTHON (ENV)")

  IF($ENV{python_target} STREQUAL "2")

    message (STATUS "Look for Python 2")

    find_package (Python2 COMPONENTS Development REQUIRED)

    IF(Python2_FOUND)

      SET(PYTHON_TARGET_FOUND TRUE)
      SET(PYTHON_TARGET_INCLUDES ${Python2_INCLUDE_DIRS})
      SET(PYTHON_TARGET_LIBRARIES ${Python2_LIBRARIES})
      SET(PYTHON_TARGET_VERSION ${Python2_VERSION})

    ENDIF()

  ELSEIF($ENV{python_target} STREQUAL "3")

find_package (Python3 COMPONENTS Development REQUIRED)

IF(Python3_FOUND)

  SET(PYTHON_TARGET_FOUND TRUE)
  SET(PYTHON_TARGET_INCLUDES ${Python3_INCLUDE_DIRS})
  SET(PYTHON_TARGET_LIBRARIES ${Python3_LIBRARIES})
  SET(PYTHON_TARGET_VERSION ${Python3_VERSION})

ENDIF()

ENDIF()

ELSE()

  message (STATUS "SEEK FOR PYTHON (DEFAULT)")

find_package (Python3 COMPONENTS Interpreter Development QUIET)

IF(Python3_FOUND)

  SET(PYTHON_TARGET_FOUND TRUE)
  SET(PYTHON_TARGET_INCLUDES ${Python3_INCLUDE_DIRS})
  SET(PYTHON_TARGET_LIBRARIES ${Python3_LIBRARIES})
  SET(PYTHON_TARGET_VERSION ${Python3_VERSION})

ELSE()

find_package (Python2 COMPONENTS Interpreter Development QUIET)

IF(Python2_FOUND)

  SET(PYTHON_TARGET_FOUND TRUE)
  SET(PYTHON_TARGET_INCLUDES ${Python2_INCLUDE_DIRS})
  SET(PYTHON_TARGET_LIBRARIES ${Python2_LIBRARIES})
  SET(PYTHON_TARGET_VERSION ${Python2_VERSION})

ELSE()

find_package (PythonLibs)

IF(PYTHONLIBS_FOUND)

  SET(PYTHON_TARGET_FOUND TRUE)
  SET(PYTHON_TARGET_INCLUDES ${PYTHON_INCLUDE_DIRS})
  SET(PYTHON_TARGET_LIBRARIES ${PYTHON_LIBRARIES})
  SET(PYTHON_TARGET_VERSION ${PYTHONLIBS_VERSION_STRING})

ENDIF()

ENDIF()

ENDIF()

ENDIF()

IF(PYTHON_TARGET_FOUND)

  message (STATUS "PYTHON_INCLUDES  = ${PYTHON_TARGET_INCLUDES}")
  message (STATUS "PYTHON_LIBRARIES = ${PYTHON_TARGET_LIBRARIES}")
  message (STATUS "PYTHON_VERSION   = ${PYTHON_TARGET_VERSION}")

ENDIF()
