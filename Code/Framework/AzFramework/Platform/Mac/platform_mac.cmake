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

find_library(APPKIT_LIBRARY AppKit)
find_library(GAME_CONTROLLER_LIBRARY GameController)
find_library(CARBON_LIBRARY Carbon)
find_library(CORE_SERVICES_LIBRARY CoreServices)
find_library(CORE_GRAPHICS_LIBRARY CoreGraphics)

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${APPKIT_LIBRARY}
        ${GAME_CONTROLLER_LIBRARY}
        ${CARBON_LIBRARY}
        ${CORE_SERVICES_LIBRARY}
        ${CORE_GRAPHICS_LIBRARY}
)
