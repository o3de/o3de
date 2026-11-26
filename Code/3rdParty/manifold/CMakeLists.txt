#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(manifold
    VERSION "3.5.5"
    LICENSE "Apache-2.0"
    URL "https://github.com/elalish/manifold/archive/refs/tags/v3.5.5.tar.gz"
    URL_HASH "e4542acaa8852e7151928a78f908711bf30f4da5132c0f735f1ba739997e19d1"
    GIT "https://github.com/elalish/manifold.git"
    GIT_HASH "32271a1a999ce721ffaf5ab10e9746ceeb696547"
    PATCH_FILES "${CMAKE_CURRENT_LIST_DIR}/manifold.patch"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(manifold)

file(COPY
    "${manifold_SOURCE_DIR}/include/manifold"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

set(MANIFOLD_VERSION_MAJOR 3)
set(MANIFOLD_VERSION_MINOR 5)
set(MANIFOLD_VERSION_PATCH 5)

configure_file(
    "${manifold_SOURCE_DIR}/cmake/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/manifold/version.h"
    @ONLY
)

o3de_disable_warnings()

ly_add_target(
    NAME manifold STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PUBLIC
            MANIFOLD_CROSS_SECTION
            MANIFOLD_PAR=-1
    BUILD_DEPENDENCIES
        PRIVATE 3rdParty::clipper2
)

add_library(manifold::manifold ALIAS manifold)

set_target_properties(manifold PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

target_compile_options(manifold PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

# Keep the floating-point settings used by upstream's geometric predicates.
if(MSVC)
    target_compile_definitions(manifold PRIVATE NOMINMAX)
    target_compile_options(manifold PRIVATE /bigobj /fp:precise)
else()
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-ffp-contract=off MANIFOLD_HAS_FP_CONTRACT)
    check_cxx_compiler_flag(-fexcess-precision=standard MANIFOLD_HAS_FP_PRECISION)
    if(MANIFOLD_HAS_FP_CONTRACT)
        target_compile_options(manifold PRIVATE -ffp-contract=off)
    endif()
    if(MANIFOLD_HAS_FP_PRECISION)
        target_compile_options(manifold PRIVATE -fexcess-precision=standard)
    endif()
endif()

ly_create_alias(NAME Manifold NAMESPACE 3rdParty TARGETS 3rdParty::manifold)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/manifold
)
