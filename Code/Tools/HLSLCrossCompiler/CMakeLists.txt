#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

ly_add_target(
    NAME HLSLcc.Headers HEADERONLY
    NAMESPACE AZ
    FILES_CMAKE
        hlslcc_header_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            include
)

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME HLSLcc EXECUTABLE
    NAMESPACE AZ
    OUTPUT_SUBDIRECTORY Compiler/PCGL/V006
    FILES_CMAKE
        hlslcc_files.cmake
    PLATFORM_INCLUDE_FILES
        Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            src
            src/cbstring
            offline/cjson
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
        PUBLIC
            AZ::HLSLcc.Headers
)
ly_add_source_properties(
    SOURCES
        offline/compilerStandalone.cpp
        offline/cjson/cJSON.c
        src/toGLSL.c
        src/toGLSLDeclaration.c
        src/cbstring/bstrlib.c
        src/cbstring/bstraux.c
        src/reflect.c
        src/amazon_changes.c
    PROPERTY COMPILE_DEFINITIONS 
    VALUES _CRT_SECURE_NO_WARNINGS
)
