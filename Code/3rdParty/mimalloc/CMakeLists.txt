#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(mimalloc
    VERSION "3.7.0"
    LICENSE "MIT"
    URL "https://github.com/microsoft/mimalloc/archive/refs/tags/v3.7.0.tar.gz"
    URL_HASH "1b8653b9ff9b841a97f2c50fd996e89e1a0cb712c3c5b0980ef16a027eb7acf8"
    GIT "https://github.com/microsoft/mimalloc.git"
    GIT_HASH "1ed67b5156a763441d2ec3c20221a7b3cd866129"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(mimalloc)

file(
    COPY "${mimalloc_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME mimalloc STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            MI_OPT_SIMD=1
            MI_SKIP_COLLECT_ON_EXIT=1
            MI_PROFILE=0
)

target_include_directories(mimalloc SYSTEM
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

target_compile_options(mimalloc PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

if(WIN32)
    ly_target_link_libraries(mimalloc PRIVATE advapi32)
endif()

if(NOT MSVC)
    # Match mimalloc's default to avoid a general-dynamic TLS lookup in the allocation fast path.
    target_compile_options(mimalloc PRIVATE -ftls-model=initial-exec)
endif()

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/mimalloc
)
