#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME ShaderCompiler STATIC
    NAMESPACE AZ
    FILES_CMAKE
        shadercompiler_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)

include(3rdParty/antlr4.cmake)
target_link_libraries(ShaderCompiler PUBLIC 3rdParty::antlr4)
target_include_directories(ShaderCompiler SYSTEM PRIVATE Source/External)

target_compile_options(ShaderCompiler PRIVATE
    ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS}
)

ly_add_target(
    NAME Azslc EXECUTABLE
    OUTPUT_NAME azslc
    NAMESPACE AZ
    FILES_CMAKE
        azslc_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::ShaderCompiler
)

target_include_directories(Azslc SYSTEM PRIVATE Source/External)

target_compile_options(Azslc PRIVATE
    ${O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS}
)

set_source_files_properties(
    Source/Grammar/azslLexer.cpp
    Source/Grammar/azslParser.cpp
    Source/Grammar/azslParserBaseListener.cpp
    Source/Grammar/azslParserListener.cpp
    PROPERTIES COMPILE_OPTIONS "$<IF:$<CXX_COMPILER_ID:MSVC>,/W0,-w>"
)
