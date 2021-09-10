#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include_guard(GLOBAL)


# In SDK builds CMAKE_CONFIGURATION_TYPES will be filled by entries generated per configuration build.
# At install time we generate `cmake/ConfigurationTypes_<config>.cmake` files that append the configuration
# to CMAKE_CONFIGURATION_TYPES
set(CMAKE_CONFIGURATION_TYPES "" CACHE STRING "" FORCE)

# For the SDK case, we want to only define the confiuguration types that have been added to the SDK
file(GLOB configuration_type_files "cmake/ConfigurationTypes_*.cmake")
foreach(configuration_type_file ${configuration_type_files})
    include(${configuration_type_file})
endforeach()
ly_set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES}) # propagate to parent