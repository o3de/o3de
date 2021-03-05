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
