#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(PAL_TRAIT_TEST_IMPACT_FRAMEWORK_SUPPORTED AND PAL_TRAIT_BUILD_TESTS_SUPPORTED AND NOT LY_MONOLITHIC_GAME)
    add_subdirectory(Runtime)
    add_subdirectory(Frontend)

    if(O3DE_TEST_IMPACT_ACTIVE)
        # Only add the TIAF unit tests if one or more runtimes is enabled
        ly_add_pytest(
            NAME TiafToolsTest
            PATH ${LY_ROOT_FOLDER}/scripts/build/TestImpactAnalysis/Testing/test_tiaf_tools.py
            TEST_SERIAL
            COMPONENT 
                TestImpactFramework
            LABELS REQUIRES_tiaf
        )
        
        ly_add_pytest(
            NAME TiafPythonUnitTest
            PATH ${LY_ROOT_FOLDER}/scripts/build/TestImpactAnalysis/Testing/test_tiaf_unit_tests.py
            TEST_SERIAL
            COMPONENT
                TestImpactFramework
            LABELS REQUIRES_tiaf
        )
    endif()
endif()
