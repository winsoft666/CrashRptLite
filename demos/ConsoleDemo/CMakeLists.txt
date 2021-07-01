cmake_minimum_required (VERSION 3.16)
project(ConsoleDemo)

# Create the list of source files
aux_source_directory( . source_files )
file( GLOB header_files *.h )

# Define _UNICODE (use wide-char encoding)
add_definitions(-D_UNICODE )

fix_default_compiler_settings()

# Add include dir
include_directories(${CMAKE_SOURCE_DIR}/include 
					${CMAKE_SOURCE_DIR}/libcrashrpt/Include )

# Add executable build target
add_executable(ConsoleDemo ${source_files} ${header_files})

# Add input link libraries
target_link_libraries(ConsoleDemo libcrashrpt)

set_target_properties(ConsoleDemo PROPERTIES DEBUG_POSTFIX d )

set_property(TARGET ConsoleDemo PROPERTY VS_DPI_AWARE "PerMonitor")
