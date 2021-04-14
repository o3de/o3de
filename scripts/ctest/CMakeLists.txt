#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

# Currently a sanity test is being registered here to validate that the ly_add_pytest function works
# don't change the names of the tests, they are used in a self test.

if(NOT PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    return()
endif()

################################################################################
# Asset Processing Target
# i.e. Tests depend on AutomatedTesting.Assets
################################################################################

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    get_property(LY_PROJECTS_TARGET_NAME GLOBAL PROPERTY LY_PROJECTS_TARGET_NAME)
    foreach(project_target_name project_path IN ZIP_LISTS LY_PROJECTS_TARGET_NAME LY_PROJECTS)
        file(REAL_PATH ${project_path} project_real_path BASE_DIRECTORY ${LY_ROOT_FOLDER})
        # With the lock file, asset processing jobs are serialized to avoid race conditions
        # on files that are created temporarily in source folders during shader processing.
        add_custom_target(${project_target_name}.Assets
            COMMENT "Processing ${project_target_name} assets..."
            COMMAND "${CMAKE_COMMAND}" 
                -DLY_LOCK_FILE=$<TARGET_FILE_DIR:AZ::AssetProcessorBatch>/project_assets.lock
                -P ${LY_ROOT_FOLDER}/cmake/CommandExecution.cmake
                    EXEC_COMMAND $<TARGET_FILE:AZ::AssetProcessorBatch> 
                        --zeroAnalysisMode 
                        --project-path=${project_real_path} 
                        --platforms=${LY_ASSET_DEPLOY_ASSET_TYPE}
        )
        set_target_properties(${project_target_name}.Assets
            PROPERTIES 
                EXCLUDE_FROM_ALL TRUE
                FOLDER ${project_target_name}
        )
    endforeach()
endif()

################################################################################
# Tests
################################################################################

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

# EPB Sanity test is being registered here to validate that the ly_add_editor_python_test function works.
if(PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_BUILD_TESTS_SUPPORTED AND AutomatedTesting IN_LIST LY_PROJECTS_TARGET_NAME)
    ly_add_editor_python_test(
        NAME epb_sanity_smoke_no_gpu
        TEST_PROJECT AutomatedTesting
        PATH ${CMAKE_CURRENT_LIST_DIR}/epb_sanity_test.py
        TEST_SUITE smoke
        TEST_SERIAL TRUE
        RUNTIME_DEPENDENCIES
            AutomatedTesting.Assets
    )
endif()

# add a custom test which makes sure that the test filtering works!

ly_add_test(
    NAME cli_test_driver
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
    TEST_COMMAND ${LY_PYTHON_CMD} ${CMAKE_CURRENT_LIST_DIR}/ctest_driver_test.py
        -x ${CMAKE_CTEST_COMMAND} 
        --build-path ${CMAKE_BINARY_DIR}
)

