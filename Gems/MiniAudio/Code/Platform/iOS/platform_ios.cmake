#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

find_library(AV_FOUNDATION_LIBRARY AVFoundation)
find_library(AUDIO_TOOLBOX_GRAPHICS_LIBRARY AudioToolbox)
find_library(CORE_AUDIO_SERVICES_LIBRARY CoreAudio)
find_library(FOUNDATION_LIBRARY Foundation)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${AV_FOUNDATION_LIBRARY}
        ${AUDIO_TOOLBOX_GRAPHICS_LIBRARY}
        ${CORE_AUDIO_SERVICES_LIBRARY}
        ${FOUNDATION_LIBRARY}
)

add_compile_definitions(MA_NO_RUNTIME_LINKING=1)

ly_add_source_properties(
    SOURCES Source/Clients/MiniAudioImplementation.cpp
    PROPERTY COMPILE_OPTIONS
    VALUES -xobjective-c++
)
