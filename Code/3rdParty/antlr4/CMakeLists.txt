#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(antlr4
    VERSION "4.13.2"
    LICENSE "BSD-3-Clause"
    URL "https://github.com/antlr/antlr4/archive/8e6fd9147b3c9d36b60e2b6656871a55227efb1b.tar.gz" # 2026-01-01
    URL_HASH "c52fb2a90ae082ab5b5ec6b41f6c0e66f691962e4b4f772580e5da678fe51fed"
    GIT "https://github.com/antlr/antlr4.git"
    GIT_HASH "8e6fd9147b3c9d36b60e2b6656871a55227efb1b"
)
FetchContent_MakeAvailable(antlr4)

set(antlr4_runtime_source_dir "${antlr4_SOURCE_DIR}/runtime/Cpp/runtime/src")
set(antlr4_runtime_include_dir "${CMAKE_CURRENT_BINARY_DIR}/Include")
file(COPY "${antlr4_runtime_source_dir}/"
    DESTINATION "${antlr4_runtime_include_dir}"
    FILES_MATCHING PATTERN "*.h"
)

find_package(Threads REQUIRED)

o3de_disable_warnings()

ly_add_target(
    NAME antlr4 STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    COMPILE_DEFINITIONS
        PUBLIC
            ANTLR4CPP_STATIC
    BUILD_DEPENDENCIES
        PUBLIC
            Threads::Threads
)

target_include_directories(antlr4 SYSTEM
    PRIVATE "${antlr4_runtime_source_dir}"
    PUBLIC "${antlr4_runtime_include_dir}"
)

if(APPLE)
    ly_target_link_libraries(antlr4 PUBLIC "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>")
endif()

target_compile_options(antlr4 PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/antlr4
)
