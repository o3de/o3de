#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(spirv-tools
    VERSION "2026.4"
    LICENSE "Apache-2.0"
    URL "https://github.com/KhronosGroup/SPIRV-Tools/archive/refs/tags/v2026.4.tar.gz"
    URL_HASH "92df4c377603f086c3d2a1eb2bd9b8f883a73ca890fb939d78a0cecdcc01686c"
    GIT "https://github.com/KhronosGroup/SPIRV-Tools.git"
    GIT_HASH "ef96ed763b43b59b33b31b362f09a02b729fa1c9"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(spirv-tools)

file(
    COPY "${spirv-tools_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME spirv-tools STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
        PRIVATE
            "${spirv-tools_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/Source"
    BUILD_DEPENDENCIES
        PUBLIC 3rdParty::spirv-headers
)

FetchContent_GetProperties(spirv-headers SOURCE_DIR spirv_headers_source_dir)
set(spirv_tools_grammar_dir "${spirv_headers_source_dir}/include/spirv/unified1")

file(COPY
    "${spirv_tools_grammar_dir}/DebugInfo.h"
    "${spirv_tools_grammar_dir}/OpenCLDebugInfo100.h"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Source"
)

set(spirv_tools_extinst_grammars
    ",${spirv_tools_grammar_dir}/extinst.glsl.std.450.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.opencl.std.100.grammar.json"
    "CLDEBUG100_,${spirv_tools_grammar_dir}/extinst.opencl.debuginfo.100.grammar.json"
    "SHDEBUG100_,${spirv_tools_grammar_dir}/extinst.nonsemantic.shader.debuginfo.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.spv-amd-shader-explicit-vertex-parameter.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.spv-amd-shader-trinary-minmax.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.spv-amd-gcn-shader.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.spv-amd-shader-ballot.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.debuginfo.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.nonsemantic.clspvreflection.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.nonsemantic.vkspreflection.grammar.json"
    "TOSA_,${spirv_tools_grammar_dir}/extinst.tosa.001000.1.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.arm.motion-engine.100.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.nonsemantic.graph.debuginfo.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.arm.experimental-ml-operations.grammar.json"
    ",${spirv_tools_grammar_dir}/extinst.nonsemantic.debugprintf.grammar.json"
)

list(
    TRANSFORM spirv_tools_extinst_grammars
    PREPEND "--extinst="
    OUTPUT_VARIABLE spirv_tools_extinst_arguments
)
list(
    TRANSFORM spirv_tools_extinst_grammars
    REPLACE "^[^,]*," ""
    OUTPUT_VARIABLE spirv_tools_extinst_dependencies
)

add_custom_command(
    OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/Source/core_tables_body.inc"
        "${CMAKE_CURRENT_BINARY_DIR}/Source/core_tables_header.inc"
    COMMAND ${LY_PYTHON_CMD}
        "${spirv-tools_SOURCE_DIR}/utils/ggt.py"
        "--core-tables-body-output=${CMAKE_CURRENT_BINARY_DIR}/Source/core_tables_body.inc"
        "--core-tables-header-output=${CMAKE_CURRENT_BINARY_DIR}/Source/core_tables_header.inc"
        "--spirv-core-grammar=${spirv_tools_grammar_dir}/spirv.core.grammar.json"
        ${spirv_tools_extinst_arguments}
    DEPENDS
        "${spirv-tools_SOURCE_DIR}/utils/ggt.py"
        "${spirv_tools_grammar_dir}/spirv.core.grammar.json"
        ${spirv_tools_extinst_dependencies}
    VERBATIM
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Source/generators.inc"
    COMMAND ${LY_PYTHON_CMD}
        "${spirv-tools_SOURCE_DIR}/utils/generate_registry_tables.py"
        "--xml=${spirv_headers_source_dir}/include/spirv/spir-v.xml"
        "--generator-output=${CMAKE_CURRENT_BINARY_DIR}/Source/generators.inc"
    DEPENDS
        "${spirv-tools_SOURCE_DIR}/utils/generate_registry_tables.py"
        "${spirv_headers_source_dir}/include/spirv/spir-v.xml"
    VERBATIM
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Source/build-version.inc"
    COMMAND ${LY_PYTHON_CMD}
        "${spirv-tools_SOURCE_DIR}/utils/update_build_version.py"
        "${spirv-tools_SOURCE_DIR}/CHANGES"
        "${CMAKE_CURRENT_BINARY_DIR}/Source/build-version.inc"
    DEPENDS
        "${spirv-tools_SOURCE_DIR}/utils/update_build_version.py"
        "${spirv-tools_SOURCE_DIR}/CHANGES"
    VERBATIM
)

target_sources(spirv-tools PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/Source/core_tables_body.inc"
    "${CMAKE_CURRENT_BINARY_DIR}/Source/core_tables_header.inc"
    "${CMAKE_CURRENT_BINARY_DIR}/Source/generators.inc"
    "${CMAKE_CURRENT_BINARY_DIR}/Source/build-version.inc"
)

set_target_properties(spirv-tools PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/spirv-tools
)
