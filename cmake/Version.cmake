#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

string(TIMESTAMP current_year "%Y")
set(O3DE_COPYRIGHT_YEAR ${current_year} CACHE STRING "Open 3D Engine's copyright year")
set(O3DE_VERSION_STRING "0.0.0" CACHE STRING "Open 3D Engine's version")
set(O3DE_DISPLAY_VERSION_STRING "00.00" CACHE STRING "Open 3D Engine's display version")
set(O3DE_BUILD_VERSION 0 CACHE STRING "Open 3D Engine's build number")
set(O3DE_ENGINE_NAME "o3de" CACHE STRING "Open 3D Engine's engine name")

# Optional environment overrides
if(NOT "$ENV{O3DE_VERSION}" STREQUAL "")
    set(O3DE_VERSION_STRING "$ENV{O3DE_VERSION}")
endif()

if(NOT "$ENV{O3DE_DISPLAY_VERSION}" STREQUAL "")
    set(O3DE_DISPLAY_VERSION_STRING "$ENV{O3DE_DISPLAY_VERSION}")
endif()

if(NOT "$ENV{O3DE_BUILD_VERSION}" STREQUAL "")
    set(O3DE_BUILD_VERSION "$ENV{O3DE_BUILD_VERSION}")
endif()
