#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_fetch_content(miniaudio
    VERSION "0.11.26"
    LICENSE "MIT No Attribution"
    URL "https://github.com/mackron/miniaudio/archive/refs/tags/0.11.26.tar.gz"
    URL_HASH "838ba55a21c9142adf7b76bdc4351e5cd8b7ae128f46b25ec25f94ae9969c600"
    GIT "https://github.com/mackron/miniaudio.git"
    GIT_HASH "877ea4a7c2601c7b0f2a44e83dfac42e6470fea6"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(miniaudio)

# Stage the headers at their existing SDK paths, including the relative includes used by the extras.
set(miniaudio_include_dir "${CMAKE_BINARY_DIR}/include/miniaudio")
file(COPY
    "${miniaudio_SOURCE_DIR}/miniaudio.h"
    DESTINATION "${miniaudio_include_dir}"
)
file(COPY
    "${miniaudio_SOURCE_DIR}/extras/decoders/libopus"
    "${miniaudio_SOURCE_DIR}/extras/decoders/libvorbis"
    DESTINATION "${miniaudio_include_dir}/extras/decoders"
    FILES_MATCHING PATTERN "*.h"
)
file(COPY
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_channel_combiner_node"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_channel_separator_node"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_ltrim_node"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_reverb_node"
    "${miniaudio_SOURCE_DIR}/extras/nodes/ma_vocoder_node"
    DESTINATION "${miniaudio_include_dir}/extras/nodes"
    FILES_MATCHING PATTERN "*.h"
)

o3de_disable_warnings()

if(APPLE)
    # The CoreAudio and AVFoundation backends use Objective-C.
    set_source_files_properties(
        "${miniaudio_SOURCE_DIR}/miniaudio.c"
        PROPERTIES COMPILE_OPTIONS -xobjective-c
    )
endif()

ly_add_target(
    NAME miniaudio STATIC
    NAMESPACE 3rdParty
    NO_UNITY
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            "${miniaudio_include_dir}"
            "${miniaudio_include_dir}/extras/decoders/libopus"
            "${miniaudio_include_dir}/extras/decoders/libvorbis"
            "${miniaudio_include_dir}/extras/nodes/ma_channel_combiner_node"
            "${miniaudio_include_dir}/extras/nodes/ma_channel_separator_node"
            "${miniaudio_include_dir}/extras/nodes/ma_ltrim_node"
            "${miniaudio_include_dir}/extras/nodes/ma_reverb_node"
            "${miniaudio_include_dir}/extras/nodes/ma_vocoder_node"
    COMPILE_DEFINITIONS
        PUBLIC
            $<$<PLATFORM_ID:Emscripten,iOS>:MA_NO_RUNTIME_LINKING=1>
            $<$<PLATFORM_ID:Emscripten>:MA_ENABLE_ONLY_SPECIFIC_BACKENDS>
            $<$<PLATFORM_ID:Emscripten>:MA_ENABLE_WEBAUDIO>
        PRIVATE $<$<PLATFORM_ID:Windows>:MA_COINIT_VALUE=2>
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::opusfile
            3rdParty::vorbis
)

set_target_properties(miniaudio PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SYSTEM TRUE
)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    ly_target_link_libraries(miniaudio PRIVATE
        "$<LINK_LIBRARY:FRAMEWORK,AudioToolbox>"
        "$<LINK_LIBRARY:FRAMEWORK,AVFoundation>"
        "$<LINK_LIBRARY:FRAMEWORK,CoreAudio>"
        "$<LINK_LIBRARY:FRAMEWORK,Foundation>"
    )
endif()

ly_install(
    FILES "${miniaudio_SOURCE_DIR}/LICENSE"
    DESTINATION include/miniaudio
    COMPONENT CORE
)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/miniaudio
)
