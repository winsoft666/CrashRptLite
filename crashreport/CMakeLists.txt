cmake_minimum_required (VERSION 3.16)
project(CrashRptLite)

# Create the list of source files
aux_source_directory( . source_files )
file( GLOB header_files *.h )

# Enable usage of precompiled header
set(srcs_using_precomp ${source_files})
list(REMOVE_ITEM srcs_using_precomp ./stdafx.cpp)
fix_add_msvc_precompiled_header(stdafx.h ./stdafx.cpp srcs_using_precomp)

list(APPEND source_files	
	${CMAKE_SOURCE_DIR}/libcrashrpt/SharedMem.cpp)

# Include resource file
list(APPEND source_files ./Resource.rc)

# Define _UNICODE (use wide-char encoding)
add_definitions(-D_UNICODE )

fix_default_compiler_settings()

# Add include dir
include_directories( ${CMAKE_SOURCE_DIR}/libcrashrpt/include 
                            ${CMAKE_SOURCE_DIR}/libcrashrpt
                            ${CMAKE_SOURCE_DIR}/thirdparty/zlib
                            ${CMAKE_SOURCE_DIR}/thirdparty/minizip  
                            ${CMAKE_SOURCE_DIR}/thirdparty/jpeg 
                            ${CMAKE_SOURCE_DIR}/thirdparty/libpng 
                            ${CMAKE_SOURCE_DIR}/thirdparty/tinyxml)


add_executable(CrashRptLite WIN32 ${source_files} ${header_files})

# Add input link libraries
target_link_libraries(CrashRptLite zlib minizip libjpeg libpng tinyxml Rpcrt4.lib Gdi32.lib shell32.lib version.lib psapi.lib)

# Add compiler flags (/MP for multi-processor compilation, /Os to favor small code)
set_target_properties(CrashRptLite PROPERTIES COMPILE_FLAGS "/Os")
#if(${MSVC_VERSION} GREATER 1310)
#	set_target_properties(CrashRptLite PROPERTIES COMPILE_FLAGS "/MP")
#endif(${MSVC_VERSION} GREATER 1310)

set_target_properties(CrashRptLite PROPERTIES 
				DEBUG_POSTFIX d)
        
set_property(TARGET CrashRptLite PROPERTY VS_DPI_AWARE "PerMonitor")

install(TARGETS CrashRptLite
	RUNTIME DESTINATION tools)