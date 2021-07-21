#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME CryCommon STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        crycommon_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .         # Lots of code without CryCommon/
            ..        # Dangerous since exports Legacy's path (client code can do CrySystem/ without depending on that target)
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME CryCommonMocks HEADERONLY
        NAMESPACE Legacy
        FILES_CMAKE
            crycommon_testing_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Mocks
    )

endif()
