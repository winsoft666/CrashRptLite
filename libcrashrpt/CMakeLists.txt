project(libCrashRptLite)

# Create the list of source files
aux_source_directory(. source_files )
file(GLOB header_files *.h ./include/*.h )

if(CRASHREPORT_BUILD_SHARED_LIBS)
  list(APPEND source_files ./libcrashrpt.def)
endif()

# Enable usage of precompiled header
set(srcs_using_precomp ${source_files})
list(REMOVE_ITEM srcs_using_precomp ./StdAfx.cpp ./libcrashrpt.def)
fix_add_msvc_precompiled_header(stdafx.h ./StdAfx.cpp srcs_using_precomp)

# Define _UNICODE (use wide-char encoding)
add_definitions(-D_UNICODE -D_CRT_SECURE_NO_DEPRECATE)

# Modify default compiler settings
fix_default_compiler_settings()

# Add include dir
include_directories(./include)

# Add library build target
if(CRASHREPORT_BUILD_SHARED_LIBS)
  add_library(libCrashRptLite SHARED ${source_files} ${header_files})
else()
  add_definitions(-DCRASHRPT_STATIC_LIB)
  add_library(libCrashRptLite STATIC ${source_files} ${header_files})
endif()

# Specify what libraries to link
target_link_libraries(libCrashRptLite rpcrt4.dll version.lib)

foreach (flag_var
	CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE )
	string(REPLACE "/O2" "-Od" ${flag_var} "${${flag_var}}")	
	string(REPLACE "/Ob2" "" ${flag_var} "${${flag_var}}")	
	set(${flag_var} "${${flag_var}} /Zi /fp:except /MP") 
endforeach()    

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG") 
    
install(FILES ./include/CrashRpt.h 	DESTINATION include/crashreport)
install(TARGETS libCrashRptLite
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
