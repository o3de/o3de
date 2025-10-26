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

function(Getmimalloc)
    include(FetchContent)

    set(MIMALLOC_GIT_REPOSITORY "https://github.com/microsoft/mimalloc.git")
    set(MIMALLOC_GIT_TAG "dfa50c37d951128b1e77167dd9291081aa88eea4")
    set(MIMALLOC_VERSION_STRING "v3.1.5")

    FetchContent_Declare(
        mimalloc
        GIT_REPOSITORY ${MIMALLOC_GIT_REPOSITORY}
        GIT_TAG        ${MIMALLOC_GIT_TAG}
        GIT_SHALLOW    TRUE
        EXCLUDE_FROM_ALL
    )

    set(MI_BUILD_SHARED OFF)
    set(MI_BUILD_OBJECT OFF)
    set(MI_BUILD_TESTS OFF)
    set(MI_OVERRIDE OFF) # Not globally overriding malloc for now
    set(MI_OPT_ARCH ON)
    set(MI_LOCAL_DYNAMIC_TLS ON)

    message(STATUS "AzCore uses mimalloc ${MIMALLOC_VERSION_STRING} (MIT) from ${MIMALLOC_GIT_REPOSITORY}")
    FetchContent_MakeAvailable(mimalloc)
endfunction()

Getmimalloc()
unset(Getmimalloc)

# Disable compile warnings as errors.
target_compile_options(mimalloc-static ${O3DE_COMPILE_OPTION_DISABLE_WARNINGS})

# Copy headers and license files, as well as a custom "find" file that declares the targets as IMPORTED
FetchContent_GetProperties(mimalloc SOURCE_DIR mimalloc_source_dir)
ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/Findmimalloc.cmake DESTINATION cmake/3rdParty)
ly_install_directory(DIRECTORIES ${mimalloc_source_dir}/include DESTINATION include/mimalloc COMPONENT CORE)
ly_install(FILES ${mimalloc_source_dir}/LICENSE DESTINATION include/mimalloc COMPONENT CORE)

add_library(3rdParty::mimalloc ALIAS mimalloc-static)

set(mimalloc_FOUND TRUE)