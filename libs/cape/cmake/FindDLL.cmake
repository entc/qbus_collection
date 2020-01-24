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
#  DLL_FOUND 
#  DLL_INCLUDES
#  DLL_LIBRARIES

if (NOT DLL_FOUND)

	IF (WIN32)
	  
	ELSE()

	  INCLUDE(FindPackageHandleStandardArgs)

	  ##____________________________________________________________________________
	  ## Check for the header files

	  find_path (DLL_INCLUDES
		NAMES dlfcn.h
		HINTS ${CMAKE_INSTALL_PREFIX}
		PATH_SUFFIXES include
		)

	  ##____________________________________________________________________________
	  ## Check for the library

	  find_library (DLL_LIBRARY_01
		NAMES dl
		HINTS ${CMAKE_INSTALL_PREFIX}
		PATH_SUFFIXES lib
		)

    # Correct Symlinks
    if(IS_SYMLINK ${DLL_LIBRARY_01})
      get_filename_component(TMP ${DLL_LIBRARY_01} REALPATH)
      list (APPEND DLL_LIBRARIES ${TMP})
    else()
      list (APPEND DLL_LIBRARIES ${DLL_LIBRARY_01})
    endif()

	  ##____________________________________________________________________________
	  ## Actions taken when all components have been found

	  message (STATUS "DLL_INCLUDES  = ${DLL_INCLUDES}")
	  message (STATUS "DLL_LIBRARIES = ${DLL_LIBRARIES}")

	  IF (NOT DLL_LIBRARIES)
	  
	    UNSET(DLL_LIBRARIES CACHE)
	  
	  ENDIF()
	  
	  ##____________________________________________________________________________
	  ## Mark advanced variables

	  #mark_as_advanced (
	#	DLL_ROOT_DIR
	#	DLL_INCLUDES
	#	DLL_LIBRARIES
	#	)

	ENDIF(WIN32)

endif (NOT DLL_FOUND)
