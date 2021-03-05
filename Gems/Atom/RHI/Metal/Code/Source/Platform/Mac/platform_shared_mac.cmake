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

# These libraries are frameworks, so we use find_library's framework detection
# to find them for us
find_library(METAL_LIBRARY Metal)
find_library(COREGRAPHICS_LIBRARY CoreGraphics)
find_library(CORESERVICES_LIBRARY CoreServices)
find_library(QUARTZCORE_LIBRARY QuartzCore)
mark_as_advanced(METAL_LIBRARY COREGRAPHICS_LIBRARY CORESERVICES_LIBRARY QUARTZCORE_LIBRARY)

set(LY_BUILD_DEPENDENCIES
    PUBLIC
        ${METAL_LIBRARY}
        ${COREGRAPHICS_LIBRARY}
        ${CORESERVICES_LIBRARY}
        ${QUARTZCORE_LIBRARY}
)
