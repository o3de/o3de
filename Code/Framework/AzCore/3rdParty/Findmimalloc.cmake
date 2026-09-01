#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::mimalloc)
    return()
endif()

block()
    set(MIMALLOC_GIT_REPOSITORY "https://github.com/microsoft/mimalloc.git")
    set(MIMALLOC_GIT_TAG "34fbd7e7cd4627424490afe19b20f8066bfc537d")
    set(MIMALLOC_VERSION_STRING "v3.5.1")
    set(MIMALLOC_URL_HASH "2602daad9b64b213a8835dee6fadda96d2081c0171bfcd3fb2db39bdc669d6b3")

    o3de_fetch_content(mimalloc
        EXCLUDE_FROM_ALL
        VERSION ${MIMALLOC_VERSION_STRING}
        LICENSE "MIT"
        URL "https://github.com/microsoft/mimalloc/archive/refs/tags/${MIMALLOC_VERSION_STRING}.tar.gz"
        URL_HASH ${MIMALLOC_URL_HASH}
        GIT ${MIMALLOC_GIT_REPOSITORY}
        GIT_HASH ${MIMALLOC_GIT_TAG}
    )

    set(MI_DEBUG OFF)
    set(MI_BUILD_SHARED OFF)
    set(MI_BUILD_OBJECT OFF)
    set(MI_BUILD_TESTS OFF)
    set(MI_FREE_IS_CHECKED OFF)
    set(MI_OVERRIDE OFF) # Not globally overriding malloc for now
    set(MI_OPT_ARCH ON)
    set(MI_OPT_SIMD ON)
    set(MI_OSX_INTERPOSE OFF)  # do not use interpose to override standard malloc on osx
    set(MI_OSX_ZONE OFF) # do not use malloc zone to override standard malloc on osx

    set(OLD_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})

    FetchContent_MakeAvailable(mimalloc)

    set(CMAKE_MESSAGE_LOG_LEVEL ${OLD_LOG_LEVEL})

    set_target_properties(mimalloc-static PROPERTIES OUTPUT_NAME "mimalloc")
    set_target_properties(mimalloc-static PROPERTIES FOLDER "3rdParty Dependencies")
    target_compile_options(mimalloc-static ${O3DE_COMPILE_OPTION_DISABLE_WARNINGS})

    add_library(3rdParty::mimalloc ALIAS mimalloc-static)

    # Copy headers and license files, as well as a custom "find" file that declares the targets as IMPORTED
    FetchContent_GetProperties(mimalloc SOURCE_DIR mimalloc_source_dir)
    ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/Findmimalloc.cmake DESTINATION cmake/3rdParty)
    ly_install(FILES ${mimalloc_source_dir}/LICENSE DESTINATION include/mimalloc COMPONENT CORE)

    # install the libraries making sure to use different directories for debug/release/etc
    set(BASE_LIBRARY_FOLDER "lib/${PAL_PLATFORM_NAME}")
    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} UCONF)
        ly_install(TARGETS mimalloc-static
            ARCHIVE
                DESTINATION "${BASE_LIBRARY_FOLDER}/${conf}"
                COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
                CONFIGURATIONS ${conf}
        )
    endforeach()
endblock()

set(mimalloc_FOUND TRUE)
