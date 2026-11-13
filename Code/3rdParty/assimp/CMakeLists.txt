#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(assimp
    VERSION "6.0.6"
    LICENSE "Custom BSD-3-Clause"
    URL "https://github.com/assimp/assimp/archive/refs/tags/v6.0.6.tar.gz"
    URL_HASH "5f7ed412b76575621df0525a762b31da5874afff352b3a890c6308db4ac7204c"
    GIT "https://github.com/assimp/assimp.git"
    GIT_HASH "a79ff6bbce54d3e01186a918dc6374ed55317426"
    PATCH_FILES "${CMAKE_CURRENT_LIST_DIR}/assimp.patch"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(assimp)

file(COPY
    "${assimp_SOURCE_DIR}/include/assimp"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*.inl"
)

set(ASSIMP_VERSION_MAJOR 6)
set(ASSIMP_VERSION_MINOR 0)
set(ASSIMP_VERSION_PATCH 6)
set(ASSIMP_PACKAGE_VERSION 0)
set(GIT_COMMIT_HASH a79ff6b)
set(GIT_BRANCH "")
set(LIBRARY_SUFFIX "")
set(ASSIMP_DOUBLE_PRECISION OFF)

configure_file(
    "${assimp_SOURCE_DIR}/include/assimp/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/assimp/config.h"
)
configure_file(
    "${assimp_SOURCE_DIR}/include/assimp/revision.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Include/assimp/revision.h"
    @ONLY
)

if(UNIX AND NOT APPLE)
    find_library(assimp_rt_library rt)
endif()

o3de_disable_warnings()

ly_add_target(
    NAME assimp STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            "${assimp_SOURCE_DIR}/code"
            "${assimp_SOURCE_DIR}"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PRIVATE
            ASSIMP_BUILD_NO_OWN_ZLIB
            ASSIMP_BUILD_NO_C4D_IMPORTER
            ASSIMP_BUILD_NO_M3D_IMPORTER
            ASSIMP_BUILD_NO_M3D_EXPORTER
            ASSIMP_BUILD_NO_VRML_IMPORTER
            ASSIMP_ENABLE_DRACO
            RAPIDJSON_HAS_STDSTRING=1
            RAPIDJSON_NOMEMBERITERATORCLASS
            _CRT_SECURE_NO_WARNINGS
            _SCL_SECURE_NO_WARNINGS
            ASSIMP_IMPORTER_GLTF_USE_OPEN3DGC=1
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::pugixml
            3rdParty::zlib
        PRIVATE
            3rdParty::draco
            3rdParty::earcut
            3rdParty::minizip
            3rdParty::openddl-parser
            3rdParty::poly2tri
            3rdParty::rapidjson
            3rdParty::stb
            3rdParty::tinyusdz
            3rdParty::utf8cpp
            3rdParty::zip
)

set_target_properties(assimp PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

target_compile_options(assimp PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

if(UNIX AND NOT APPLE AND assimp_rt_library)
    ly_target_link_libraries(assimp PRIVATE "${assimp_rt_library}")
endif()

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/assimp
)

ly_install(
    FILES licenses/clipper.txt
    DESTINATION 3rdParty/clipper
    RENAME LICENSE.txt
)

ly_install(
    FILES licenses/open3dgc.txt
    DESTINATION 3rdParty/open3dgc
    RENAME LICENSE.txt
)
