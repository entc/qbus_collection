if (NOT EXECINFO_FOUND)

	IF (WIN32)
	  # we can use windows onw DLo libs
	ELSE()

	  INCLUDE(FindPackageHandleStandardArgs)

	  ##____________________________________________________________________________
	  ## Check for the header files

	  find_path (EXECINFO_INCLUDES NAMES execinfo.h)

	  ##____________________________________________________________________________
	  ## Check for the library

	  find_library (EXECINFO_LIBRARIES NAMES execinfo HINTS ${CMAKE_INSTALL_PREFIX} PATH_SUFFIXES lib)

	  ##____________________________________________________________________________
	  ## Actions taken when all components have been found

	  if (EXECINFO_INCLUDES AND EXECINFO_LIBRARIES)
  		SET(EXECINFO_FOUND TRUE)
		ELSE()
			SET(EXECINFO_LIBRARIES "")
    endif ()

	  if (EXECINFO_FOUND)
			if (NOT EXECINFO_FIND_QUIETLY)
		  	message (STATUS "EXECINFO_INCLUDES  = ${EXECINFO_INCLUDES}")
		  	message (STATUS "EXECINFO_LIBRARIES = ${EXECINFO_LIBRARIES}")
			endif (NOT EXECINFO_FIND_QUIETLY)
	  else (EXECINFO_FOUND)
			if (EXECINFO_FIND_REQUIRED)
		  	message (FATAL_ERROR "Could not find EXECINFO!")
			endif (EXECINFO_FIND_REQUIRED)
	  endif (EXECINFO_FOUND)

	ENDIF(WIN32)

endif (NOT EXECINFO_FOUND)
