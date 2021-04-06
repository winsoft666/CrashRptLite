vcpkg_fail_port_install(ON_ARCH "arm" "arm64" ON_TARGET "UWP" "LINUX" "OSX")

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO winsoft666/crashreport
    REF b566fc973321b8796ec8f19e294c1c449a95f8f0
    SHA512 d496cd3b292fb095a8df1a9473bc315794aafb823b08e7fa752258ceeefa91ff63a68049fe0baceaaa85aece3317f0e511b9db2063f53a195ea0f21cb564f81b
    HEAD_REF master
)

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static" CRASHRPT_BUILD_STATIC_LIBS)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS
        -DCRASHRPT_BUILD_STATIC_LIBS:BOOL=${CRASHRPT_BUILD_STATIC_LIBS}
        -DBUILD_TESTS=OFF
)

vcpkg_install_cmake()


file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/share)

vcpkg_copy_pdbs()
