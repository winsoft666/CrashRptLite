project(crashdumper)

# Create the list of source files
aux_source_directory( . source_files )
file( GLOB header_files *.h )

# Enable usage of precompiled header
set(srcs_using_precomp ${source_files})
list(REMOVE_ITEM srcs_using_precomp ./stdafx.cpp)
add_msvc_precompiled_header(stdafx.h ./stdafx.cpp srcs_using_precomp)

list(APPEND source_files	
	${CMAKE_SOURCE_DIR}/libcrashrpt/SharedMem.cpp)

# Define _UNICODE (use wide-char encoding)
add_definitions(-D_UNICODE )

fix_default_compiler_settings_()

# Add include dir
include_directories( ${CMAKE_SOURCE_DIR}/libcrashrpt/include 
                            ${CMAKE_SOURCE_DIR}/libcrashrpt
                            ${CMAKE_SOURCE_DIR}/thirdparty/zlib
                            ${CMAKE_SOURCE_DIR}/thirdparty/minizip  
                            ${CMAKE_SOURCE_DIR}/thirdparty/jpeg 
                            ${CMAKE_SOURCE_DIR}/thirdparty/libpng 
                            ${CMAKE_SOURCE_DIR}/thirdparty/tinyxml
                            ${CMAKE_SOURCE_DIR}/thirdparty/dbghelp/include)

if(NOT CMAKE_CL_64)
	link_directories( ${CMAKE_SOURCE_DIR}/thirdparty/dbghelp/lib )
else(NOT CMAKE_CL_64)
	link_directories( ${CMAKE_SOURCE_DIR}/thirdparty/dbghelp/lib/amd64 )
endif(NOT CMAKE_CL_64)

# Add executable build target
if(CRASHRPT_BUILD_STATIC_LIBS)
  SET(CMAKE_MFC_FLAG 1)
else()
  ADD_DEFINITIONS(-D_AFXDLL)
  SET(CMAKE_MFC_FLAG 2)
endif()

add_executable(crashdumper WIN32 ${source_files} ${header_files})

# Add input link libraries
target_link_libraries(crashdumper zlib minizip libjpeg libpng tinyxml Rpcrt4.lib Gdi32.lib shell32.lib version.lib psapi.lib)

# Add compiler flags (/MP for multi-processor compilation, /Os to favor small code)
set_target_properties(crashdumper PROPERTIES COMPILE_FLAGS "/Os")
#if(${MSVC_VERSION} GREATER 1310)
#	set_target_properties(crashdumper PROPERTIES COMPILE_FLAGS "/MP")
#endif(${MSVC_VERSION} GREATER 1310)

set_target_properties(crashdumper PROPERTIES 
				DEBUG_POSTFIX d)
        
install(TARGETS crashdumper
	RUNTIME DESTINATION tools)