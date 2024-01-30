#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

add_subdirectory(NumericalMethods)

set(LY_PHYSX_ENABLE_RUNNING_BENCHMARKS OFF CACHE BOOL "Adds a target to allow running of the physx benchmarks.")

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME PhysX.Mocks HEADERONLY
        NAMESPACE Gem
        OUTPUT_NAME PhysX.Mocks.Gem
        FILES_CMAKE
            physx_mocks_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Mocks
    )
endif()
