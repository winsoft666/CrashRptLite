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
option(CRASHREPORT_BUILD_SHARED_LIBS "If set (default), CrashReport modules are built as dynamic-link libraries, otherwise as static libs." ON)
option(CRASHREPORT_BUILD_TESTS "Build tests project" OFF)

# Other CMakeLists are located in project subdirectories 

if(CRASHREPORT_BUILD_TESTS)
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