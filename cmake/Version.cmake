#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

string(TIMESTAMP current_year "%Y")
set(LY_VERSION_COPYRIGHT_YEAR ${current_year} CACHE STRING "Open 3D Engine's copyright year")
set(LY_VERSION_STRING "0.0.0.0" CACHE STRING "Open 3D Engine's version")
set(LY_VERSION_BUILD_NUMBER 0 CACHE STRING "Open 3D Engine's build number")
set(LY_VERSION_ENGINE_NAME "o3de" CACHE STRING "Open 3D Engine's engine name")

if("$ENV{O3DE_VERSION}")
    # Overriding through environment
    set(LY_VERSION_STRING "$ENV{O3DE_VERSION}")
endif()

if("$ENV{O3DE_BUILD_VERSION}")
    # Overriding through environment
    set(LY_VERSION_BUILD_NUMBER "$ENV{O3DE_BUILD_VERSION}")
endif()
