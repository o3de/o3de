#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include_guard(GLOBAL)

# By default, CMAKE_CONFIGURATION_TYPES = LY_CONFIGURATION_TYPES, but in installed SDKs, this
# file will be replaced with cmake/install/ConfigurationTypes.cmake and discover configurations
# that are available from the SDK
set(CMAKE_CONFIGURATION_TYPES ${LY_CONFIGURATION_TYPES} CACHE STRING "" FORCE)
