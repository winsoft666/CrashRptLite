project(libcrashrpt)

# Create the list of source files
aux_source_directory(. source_files )
file(GLOB header_files *.h ./include/*.h )

if(NOT CRASHRPT_BUILD_STATIC_LIBS)
  list(APPEND source_files ./libcrashrpt.def)
endif(CRASHRPT_BUILD_STATIC_LIBS)

# Enable usage of precompiled header
set(srcs_using_precomp ${source_files})
list(REMOVE_ITEM srcs_using_precomp ./StdAfx.cpp ./libcrashrpt.def)
add_msvc_precompiled_header(stdafx.h ./StdAfx.cpp srcs_using_precomp)

# Define _UNICODE (use wide-char encoding)
add_definitions(-D_UNICODE -D_CRT_SECURE_NO_DEPRECATE)

# Modify default compiler settings
fix_default_compiler_settings_()

# Add include dir
include_directories(./include)

# Add library build target
if(CRASHRPT_BUILD_STATIC_LIBS)
  add_definitions(-DCRASHRPT_STATIC_LIB)
	add_library(libcrashrpt STATIC ${source_files} ${header_files})
else(CRASHRPT_BUILD_STATIC_LIBS)
	add_library(libcrashrpt SHARED ${source_files} ${header_files})
endif(CRASHRPT_BUILD_STATIC_LIBS)

# Specify what libraries to link
target_link_libraries(libcrashrpt rpcrt4.dll version.lib)

foreach (flag_var
	CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE )
	string(REPLACE "/O2" "-Od" ${flag_var} "${${flag_var}}")	
	string(REPLACE "/Ob2" "" ${flag_var} "${${flag_var}}")	
	set(${flag_var} "${${flag_var}} /Zi /fp:except /MP") 
endforeach()    

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG") 


if(CRASHRPT_BUILD_STATIC_LIBS)
	set_target_properties(libcrashrpt PROPERTIES 
									DEBUG_POSTFIX LIBd 
									RELEASE_POSTFIX LIB )
else(CRASHRPT_BUILD_STATIC_LIBS)
	set_target_properties(libcrashrpt PROPERTIES 
									DEBUG_POSTFIX d)
endif(CRASHRPT_BUILD_STATIC_LIBS)



install(TARGETS libcrashrpt
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
    
install(FILES ./include/CrashRpt.h 			DESTINATION include/crashreport)