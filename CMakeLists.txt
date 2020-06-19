# Preserve backwards compatibility
cmake_minimum_required (VERSION 2.8)

set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE )

project (crashreport)


# Check supported generators
#if(NOT MSVC OR ${MSVC_VERSION} LESS 1400)
#	message(FATAL_ERROR "This version of Visual Studio is not supported: ${CMAKE_GENERATOR}.")
#endif(NOT MSVC OR ${MSVC_VERSION} LESS 1400)

# Build options
option(CRASHRPT_BUILD_STATIC_LIBS "If set (default), CrashReport modules are built as static-link libraries, otherwise as static libs." ON)
option(BUILD_TESTS "Build tests project" OFF)

# Set output directory for executable files
if(CMAKE_CL_64)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/x64)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/x64)
else(CMAKE_CL_64)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
endif(CMAKE_CL_64)

# Set output directory for DLL files
if(CMAKE_CL_64)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/x64)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/x64)
else(CMAKE_CL_64)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
endif(CMAKE_CL_64)

# Set output directory for LIB files
if(CMAKE_CL_64)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/x64)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/x64)
else(CMAKE_CL_64)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
endif(CMAKE_CL_64)
	
# Precompiled headers stuff (CMake doesn't have a standard command for enabling precompiled headers,
# so we have to use a macro)
MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  IF(MSVC)	
    GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${PrecompiledBasename}.pch")
    SET(Sources ${${SourcesVar}})

    SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_OUTPUTS "${PrecompiledBinary}")
    SET_SOURCE_FILES_PROPERTIES(${Sources}
                                PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledBinary}\" /FI\"${PrecompiledBinary}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_DEPENDS "${PrecompiledBinary}")  
    # Add precompiled header to SourcesVar
    LIST(APPEND ${SourcesVar} ${PrecompiledSource})
  ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

# Modifies CMake's default compiler/linker settings.
macro(fix_default_compiler_settings_)

	if (MSVC)
		
		# For MSVC, CMake sets certain flags to defaults we want to override.
		# This replacement code is taken from sample in the CMake Wiki at
		# http://www.cmake.org/Wiki/CMake_FAQ#Dynamic_Replace.
		foreach (flag_var
				CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
				CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE )
			if (CRASHRPT_BUILD_STATIC_LIBS)     					
				string(REPLACE "/MD" "-MT" ${flag_var} "${${flag_var}}")			  
			endif(CRASHRPT_BUILD_STATIC_LIBS)        

			# We prefer more strict warning checking.
			# Replaces /W3 with /W4 in defaults.
			string(REPLACE "/W3" "-W4" ${flag_var} "${${flag_var}}")
						
		endforeach()
		
		#SET (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING "MSVC C Debug MT flags " FORCE)    
		#SET (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" CACHE STRING "MSVC CXX Debug MT flags " FORCE)
		#SET (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "MSVC C Release MT flags " FORCE)
		#SET (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "MSVC CXX Release MT flags " FORCE)
		
    endif(MSVC)
  
endmacro()

if(MSVC)
  set(CRASHRPT_INSTALL_CMAKE_DIR share/crashreport)
elseif(WIN32 AND NOT CYGWIN)
  set(CRASHRPT_INSTALL_CMAKE_DIR CMake)
else()
  set(CRASHRPT_INSTALL_CMAKE_DIR lib/cmake/crashreport)
endif()

# Other CMakeLists are located in project subdirectories 

if(BUILD_TESTS)
  add_subdirectory("demos/ConsoleDemo")
  add_subdirectory("demos/MFCDemo")
endif()
add_subdirectory("crashreport")
add_subdirectory("libcrashrpt")


# Set output directory for LIB files
if(CMAKE_CL_64)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/thirdparty/lib/x64)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/thirdparty/lib/x64)
else(CMAKE_CL_64)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/thirdparty/lib)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/thirdparty/lib)
endif(CMAKE_CL_64)

add_subdirectory("thirdparty/tinyxml")
add_subdirectory("thirdparty/jpeg")
add_subdirectory("thirdparty/minizip")
add_subdirectory("thirdparty/zlib")
add_subdirectory("thirdparty/libpng")