#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

find_library(AUDIO_TOOLBOX_GRAPHICS_LIBRARY AudioToolbox)
find_library(CORE_AUDIO_SERVICES_LIBRARY CoreAudio)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${AUDIO_TOOLBOX_GRAPHICS_LIBRARY}
        ${CORE_AUDIO_SERVICES_LIBRARY}
)
