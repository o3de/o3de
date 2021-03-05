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

if (PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

    include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
    if (NOT PAL_TRAIT_BUILD_HLSLCC_METAL)
        return()
    endif()

    ly_add_target(
        NAME HLSLcc_Metal EXECUTABLE
        NAMESPACE AZ
        OUTPUT_NAME HLSLcc
        OUTPUT_SUBDIRECTORY Compiler/PCGMETAL/HLSLcc
        FILES_CMAKE
            hlslcc_metal_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                include
                src
                src/cbstring
                offline/cjson
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
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
            src/decode.c
            src/toMETAL.c
            src/toMETALDeclaration.c
        PROPERTY COMPILE_DEFINITIONS 
        VALUES _CRT_SECURE_NO_WARNINGS
    )

endif()
