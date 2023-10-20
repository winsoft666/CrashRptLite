# Precompiled headers stuff (CMake doesn't have a standard command for enabling precompiled headers,
# so we have to use a macro)
MACRO(fix_add_msvc_precompiled_header PrecompiledHeader PrecompiledSource SourcesVar)
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
ENDMACRO()


# Modifies CMake's default compiler/linker settings.
macro(fix_default_compiler_settings)
	if (MSVC)
		if (CRASHREPORT_USE_STATIC_CRT)
			set(CompilerFlags
				CMAKE_CXX_FLAGS
				CMAKE_CXX_FLAGS_DEBUG
				CMAKE_CXX_FLAGS_RELEASE
				CMAKE_C_FLAGS
				CMAKE_C_FLAGS_DEBUG
				CMAKE_C_FLAGS_RELEASE
				)
			foreach(CompilerFlag ${CompilerFlags})
				string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
			endforeach()
		endif()
    endif(MSVC)
endmacro()