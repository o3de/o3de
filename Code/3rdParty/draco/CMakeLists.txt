#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(draco
    VERSION "1.5.9"
    LICENSE "Apache-2.0"
    URL "https://github.com/google/draco/archive/554628821c3448319d7d4ae48db170b8c63e1aaf.tar.gz"
    URL_HASH "a0805527f2c2f983e409a2b353d115606d30084177252f61241d4d5239213900"
    GIT "https://github.com/google/draco.git"
    GIT_HASH "554628821c3448319d7d4ae48db170b8c63e1aaf"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(draco)

file(COPY
    "${draco_SOURCE_DIR}/src/draco"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
    FILES_MATCHING PATTERN "*.h"
)

set(draco_build "${CMAKE_CURRENT_BINARY_DIR}/Include")
include("${draco_SOURCE_DIR}/cmake/draco_options.cmake")
set(DRACO_GLTF_BITSTREAM OFF)
set(DRACO_MESH_COMPRESSION ON)
set(DRACO_POINT_CLOUD_COMPRESSION ON)
set(DRACO_PREDICTIVE_EDGEBREAKER ON)
set(DRACO_STANDARD_EDGEBREAKER ON)
set(DRACO_BACKWARDS_COMPATIBILITY ON)
set(DRACO_UNITY_PLUGIN OFF)
set(DRACO_MAYA_PLUGIN OFF)
set(DRACO_TRANSCODER_SUPPORTED OFF)
set(DRACO_PMC_SUPPORTED OFF)
set(DRACO_DEBUG_COMPILER_WARNINGS ON)
draco_set_optional_features()
draco_generate_features_h()

o3de_disable_warnings()

ly_add_target(
    NAME draco STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE "${draco_SOURCE_DIR}/src"
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        PUBLIC DRACO_CMAKE=1
        PRIVATE
            NOMINMAX=1
            _CRT_SECURE_NO_DEPRECATE=1
)

set_target_properties(draco PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

target_compile_options(draco PRIVATE ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS})

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/draco
)
