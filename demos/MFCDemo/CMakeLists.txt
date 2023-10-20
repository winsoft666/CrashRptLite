cmake_minimum_required (VERSION 3.16)
project(MFCDemo)

# Create the list of source files
aux_source_directory( . source_files )
file( GLOB header_files *.h )

# Enable usage of precompiled header
set(srcs_using_precomp ${source_files})
list(REMOVE_ITEM srcs_using_precomp ./stdafx.cpp)
fix_add_msvc_precompiled_header(stdafx.h ./stdafx.cpp srcs_using_precomp)

# Use MFC
if (NOT CRASHREPORT_USE_STATIC_CRT)     					
	# Use MFC as a shared lib
    ADD_DEFINITIONS(-D_AFXDLL)
	SET(CMAKE_MFC_FLAG 2) 
else()   
	# Use MFC as a static lib
	SET(CMAKE_MFC_FLAG 1) 
endif()   

# Include resource file
list(APPEND source_files ./MFCDemo.rc )

# Define _UNICODE (use wide-char encoding)
add_definitions(-D_UNICODE )

fix_default_compiler_settings()

# Add include dir
include_directories(${CMAKE_SOURCE_DIR}/libcrashrpt/include)

# Add executable build target
add_executable(MFCDemo WIN32 ${source_files} ${header_files})

# Add input link libraries
target_link_libraries(MFCDemo libCrashRptLite)

# Add "d" postfix to output file name in Debug
set_target_properties(MFCDemo PROPERTIES DEBUG_POSTFIX d )

set_property(TARGET MFCDemo PROPERTY VS_DPI_AWARE "PerMonitor")