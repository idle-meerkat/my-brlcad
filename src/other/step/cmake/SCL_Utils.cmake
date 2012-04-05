# Will need windows logic someday, even though it doesn't work yet, so go ahead
# and add the DLL_DEFINE macro

MACRO(DLL_DEFINE libname)
	IF(MSVC)
		STRING(REGEX REPLACE "lib" "" LOWERCORE "${libname}")
		STRING(TOUPPER ${LOWERCORE} UPPER_CORE)
		add_definitions("-D${UPPER_CORE}_EXPORT_DLL")
	ENDIF(MSVC)
ENDMACRO()


MACRO(SCL_ADDEXEC execname srcslist libslist)
  add_executable(${execname} ${srcslist})
  target_link_libraries(${execname} ${libslist})
  INSTALL(TARGETS ${execname} DESTINATION bin)
  # Enable extra compiler flags if local executables and/or global options dictate
  SET(LOCAL_COMPILE_FLAGS "")
  FOREACH(extraarg ${ARGN})
	  IF(${extraarg} MATCHES "STRICT" AND SCL-ENABLE_STRICT)
		  SET(LOCAL_COMPILE_FLAGS "${LOCAL_COMPILE_FLAGS} ${STRICT_FLAGS}")
	  ENDIF(${extraarg} MATCHES "STRICT" AND SCL-ENABLE_STRICT)
  ENDFOREACH(extraarg ${ARGN})
  IF(LOCAL_COMPILE_FLAGS)
	  SET_TARGET_PROPERTIES(${execname} PROPERTIES COMPILE_FLAGS ${LOCAL_COMPILE_FLAGS})
  ENDIF(LOCAL_COMPILE_FLAGS)
ENDMACRO(SCL_ADDEXEC execname srcslist libslist)

MACRO(SCL_ADDLIB libname srcslist libslist)
  DLL_DEFINE(${libname})
  IF(SCL_BUILD_SHARED_LIBS)
	  add_library(${libname} SHARED ${srcslist})
	  if(NOT ${libslist} MATCHES "NONE")
		  target_link_libraries(${libname} ${libslist})
	  endif(NOT ${libslist} MATCHES "NONE")
	  SET_TARGET_PROPERTIES(${libname} PROPERTIES VERSION ${SCL_ABI_VERSION} SOVERSION ${SCL_ABI_SOVERSION} )
	  INSTALL(TARGETS ${libname} DESTINATION lib)
  ENDIF(SCL_BUILD_SHARED_LIBS)
  IF(SCL_BUILD_STATIC_LIBS AND NOT MSVC)
	  add_library(${libname}-static STATIC ${srcslist})
	  if(NOT ${libslist} MATCHES "NONE")
		  target_link_libraries(${libname}-static ${libslist})
	  endif(NOT ${libslist} MATCHES "NONE")
	  IF(NOT WIN32)
		  SET_TARGET_PROPERTIES(${libname}-static PROPERTIES OUTPUT_NAME "${libname}")
	  ENDIF(NOT WIN32)
	  IF(WIN32)
		  # We need the lib prefix on win32, so add it even if our add_library
		  # wrapper function has removed it due to the target name - see
		  # http://www.cmake.org/Wiki/CMake_FAQ#How_do_I_make_my_shared_and_static_libraries_have_the_same_root_name.2C_but_different_suffixes.3F
		  SET_TARGET_PROPERTIES(${libname}-static PROPERTIES PREFIX "lib")
	  ENDIF(WIN32)
	  INSTALL(TARGETS ${libname}-static DESTINATION lib)
  ENDIF(SCL_BUILD_STATIC_LIBS AND NOT MSVC)
  # Enable extra compiler flags if local libraries and/or global options dictate
  SET(LOCAL_COMPILE_FLAGS "")
  FOREACH(extraarg ${ARGN})
	  IF(${extraarg} MATCHES "STRICT" AND SCL-ENABLE_STRICT)
		  SET(LOCAL_COMPILE_FLAGS "${LOCAL_COMPILE_FLAGS} ${STRICT_FLAGS}")
	  ENDIF(${extraarg} MATCHES "STRICT" AND SCL-ENABLE_STRICT)
  ENDFOREACH(extraarg ${ARGN})
  IF(LOCAL_COMPILE_FLAGS)
	  IF(BUILD_SHARED_LIBS)
		  SET_TARGET_PROPERTIES(${libname} PROPERTIES COMPILE_FLAGS ${LOCAL_COMPILE_FLAGS})
	  ENDIF(BUILD_SHARED_LIBS)
	  IF(BUILD_STATIC_LIBS AND NOT MSVC)
		  SET_TARGET_PROPERTIES(${libname}-static PROPERTIES COMPILE_FLAGS ${LOCAL_COMPILE_FLAGS})
	  ENDIF(BUILD_STATIC_LIBS AND NOT MSVC)
  ENDIF(LOCAL_COMPILE_FLAGS)
ENDMACRO(SCL_ADDLIB libname srcslist libslist)
