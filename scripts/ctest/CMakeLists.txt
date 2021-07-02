#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Currently a sanity test is being registered here to validate that the ly_add_pytest function works
# don't change the names of the tests, they are used in a self test.

if(NOT PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    return()
endif()

################################################################################
# Tests
################################################################################

if(PAL_TRAIT_TEST_LYTESTTOOLS_SUPPORTED)
    foreach(suite_name ${LY_TEST_GLOBAL_KNOWN_SUITE_NAMES})
        ly_add_pytest(
            NAME pytest_sanity_${suite_name}_no_gpu
            PATH ${CMAKE_CURRENT_LIST_DIR}/sanity_test.py
            TEST_SUITE ${suite_name}
        )

        ly_add_pytest(
            NAME pytest_sanity_${suite_name}_requires_gpu
            PATH ${CMAKE_CURRENT_LIST_DIR}/sanity_test.py
            TEST_SUITE ${suite_name}
            TEST_REQUIRES gpu
        )
    endforeach()
endif()

# EPB Sanity test is being registered here to validate that the ly_add_editor_python_test function works.
#if(PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_BUILD_TESTS_SUPPORTED AND AutomatedTesting IN_LIST LY_PROJECTS_TARGET_NAME)
#    ly_add_editor_python_test(
#        NAME epb_sanity_smoke_no_gpu
#        TEST_PROJECT AutomatedTesting
#        PATH ${CMAKE_CURRENT_LIST_DIR}/epb_sanity_test.py
#        TEST_SUITE smoke
#        TEST_SERIAL TRUE
#        RUNTIME_DEPENDENCIES
#            AutomatedTesting.Assets
#    )
#endif()

# add a custom test which makes sure that the test filtering works!

ly_add_test(
    NAME cli_test_driver
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
    TEST_COMMAND ${LY_PYTHON_CMD} ${CMAKE_CURRENT_LIST_DIR}/ctest_driver_test.py
        -x ${CMAKE_CTEST_COMMAND} 
        --build-path ${CMAKE_BINARY_DIR}
)

