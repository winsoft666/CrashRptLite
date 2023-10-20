# Preserve backwards compatibility
cmake_minimum_required(VERSION 3.4 FATAL_ERROR)

project (CrashRptLite)

include(common.cmake)

# Check supported generators
#if(NOT MSVC OR ${MSVC_VERSION} LESS 1400)
#	message(FATAL_ERROR "This version of Visual Studio is not supported: ${CMAKE_GENERATOR}.")
#endif(NOT MSVC OR ${MSVC_VERSION} LESS 1400)

# Build options
option(CRASHREPORT_BUILD_SHARED_LIBS "If set (default), CrashReportLite modules are built as dynamic-link libraries, otherwise as static libs." ON)
option(CRASHREPORT_BUILD_TESTS "Build tests project" OFF)
option(CRASHREPORT_USE_STATIC_CRT "Set to ON to build with static CRT on Windows (/MT)." OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Debug Output
message(STATUS "CRASHREPORT_BUILD_SHARED_LIBS=${CRASHREPORT_BUILD_SHARED_LIBS}")
message(STATUS "CRASHREPORT_USE_STATIC_CRT=${CRASHREPORT_USE_STATIC_CRT}")

# Other CMakeLists are located in project subdirectories 
add_subdirectory("libcrashrpt")
add_subdirectory("crashreport")

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

if(CRASHREPORT_BUILD_TESTS)
  add_subdirectory("demos/ConsoleDemo")
  add_subdirectory("demos/MFCDemo")
endif()
