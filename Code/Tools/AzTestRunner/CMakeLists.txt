#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

include(${pal_dir}/platform_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(PAL_TRAIT_AZTESTRUNNER_SUPPORTED AND PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    
    ly_add_target(
        NAME AzTestRunner ${PAL_TRAIT_AZTESTRUNNER_LAUNCHER_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            aztestrunner_files.cmake
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                src
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzTest
    )
    
    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    
        ly_add_target(
            NAME AzTestRunner.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE AZ
            FILES_CMAKE
                aztestrunner_test_files.cmake
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
        )

        ly_add_googletest(
            NAME AZ::AzTestRunner.Tests
            LABELS REQUIRES_tiaf
        )

    endif()

endif()
