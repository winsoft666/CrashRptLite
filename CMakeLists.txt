# Preserve backwards compatibility
cmake_minimum_required (VERSION 3.16)

set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE )

project (crashreport)

include(common.cmake)

# Check supported generators
#if(NOT MSVC OR ${MSVC_VERSION} LESS 1400)
#	message(FATAL_ERROR "This version of Visual Studio is not supported: ${CMAKE_GENERATOR}.")
#endif(NOT MSVC OR ${MSVC_VERSION} LESS 1400)

# Build options
option(CRASHRPT_BUILD_STATIC_LIBS "If set (default), CrashReport modules are built as static-link libraries, otherwise as static libs." OFF)
option(BUILD_TESTS "Build tests project" ON)

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