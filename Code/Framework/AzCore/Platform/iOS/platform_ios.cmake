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

set(__azcore_dependencies)

find_library(FOUNDATION_LIBRARY Foundation)
list(APPEND __azcore_dependencies ${FOUNDATION_LIBRARY})


if (NOT LY_MONOLITHIC_GAME)
    find_library(UI_KIT_FRAMEWORK UIKit)
    list(APPEND __azcore_dependencies ${UI_KIT_FRAMEWORK})
endif()



set(LY_BUILD_DEPENDENCIES
    PRIVATE
        ${__azcore_dependencies}
)