#-------------------------------------------------------------------------------
# Copyright (c) 2018-2018, Alexander Kalkhof <alex@kalkhof.org>
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

# The following variables are set when DL is found:
#  FTS_FOUND 
#  FTS_INCLUDES
#  FTS_LIBRARIES

if (NOT FTS_FOUND)

	IF (WIN32)
	  # we can use windows onw DLo libs
	ELSE()

	  INCLUDE(FindPackageHandleStandardArgs)

	  ##____________________________________________________________________________
	  ## Check for the header files

	  find_path (FTS_INCLUDES
		NAMES fts.h
		HINTS ${CMAKE_INSTALL_PREFIX}
		PATH_SUFFIXES include
		)

	  ##____________________________________________________________________________
	  ## Check for the library

	  find_library (FTS_LIBRARIES
		NAMES fts
		HINTS ${CMAKE_INSTALL_PREFIX}
		PATH_SUFFIXES lib
		)
		
		if (NOT FTS_LIBRARIES)
		
      SET(FTS_LIBRARIES "")
		
		endif ()

	  ##____________________________________________________________________________
	  ## Actions taken when all components have been found

	  #find_package_handle_standard_args (FTS DEFAULT_MSG FTS_LIBRARIES FTS_INCLUDES)
		
	  if (FTS_FOUND)
		if (NOT FTS_FIND_QUIETLY)
		  message (STATUS "FTS_INCLUDES  = ${FTS_INCLUDES}")
		  message (STATUS "FTS_LIBRARIES = ${FTS_LIBRARIES}")
		endif (NOT FTS_FIND_QUIETLY)
	  else (FTS_FOUND)
		if (FTS_FIND_REQUIRED)
		  message (FATAL_ERROR "Could not find FTS!")
		endif (FTS_FIND_REQUIRED)
	  endif (FTS_FOUND)

	  ##____________________________________________________________________________
	  ## Mark advanced variables


	ENDIF(WIN32)

endif (NOT FTS_FOUND)

