# {BEGIN_LICENSE}
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# {END_LICENSE}

# File to tweak compiler settings before compiler detection happens (before project() is called)
# We dont have PAL enabled at this point, so we can only use pure-CMake variables
if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
    include(cmake/Platform/Linux/CompilerSettings_linux.cmake)
endif()
