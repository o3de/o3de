#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(mimalloc
    VERSION "3.5.1"
    LICENSE "MIT"
    URL "https://github.com/microsoft/mimalloc/archive/refs/tags/v3.5.1.tar.gz"
    URL_HASH "2602daad9b64b213a8835dee6fadda96d2081c0171bfcd3fb2db39bdc669d6b3"
    GIT "https://github.com/microsoft/mimalloc.git"
    GIT_HASH "34fbd7e7cd4627424490afe19b20f8066bfc537d"
    SOURCE_SUBDIR "no-add-subdir"
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
