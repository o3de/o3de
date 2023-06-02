#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
if(NOT PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    return()
endif()

# Path to test instrumentation binary
set(O3DE_TEST_IMPACT_INSTRUMENTATION_BIN "" CACHE PATH "Path to test impact framework instrumentation binary")

# Label to add to test for them to be included in TIAF
set(REQUIRES_TIAF_LABEL "REQUIRES_tiaf")

# Test impact analysis opt-in for native test targets
set(O3DE_TEST_IMPACT_NATIVE_TEST_TARGETS_ENABLED FALSE CACHE BOOL "Whether to enable native C++ test targets with the REQUIRES_TIAF_LABEL label for test impact analysis (otherwise, CTest will be used to run these targets).")

# Test impact analysis opt-in for Python test targets
set(O3DE_TEST_IMPACT_PYTHON_TEST_TARGETS_ENABLED FALSE CACHE BOOL "Whether to enable Python test targets with the REQUIRES_TIAF_LABEL label for test impact analysis (otherwise, CTest will be used to run these targets).")

if(LY_MONOLITHIC_GAME)
    # TIAF not supported for monolithic game builds
    set(O3DE_TEST_IMPACT_NATIVE_TEST_TARGETS_ENABLED false)
    set(O3DE_TEST_IMPACT_PYTHON_TEST_TARGETS_ENABLED false)
    set(O3DE_TEST_IMPACT_ACTIVE false)
elseif(O3DE_TEST_IMPACT_NATIVE_TEST_TARGETS_ENABLED OR O3DE_TEST_IMPACT_PYTHON_TEST_TARGETS_ENABLED)
    # TIAF is active if at least one runtime is enabled
    set(O3DE_TEST_IMPACT_ACTIVE true)
    if(O3DE_TEST_IMPACT_NATIVE_TEST_TARGETS_ENABLED)
        message(DEBUG "TIAF enabled for native tests.")
    else()
        message("TIAF disabled for native tests.")
    endif()
    if(O3DE_TEST_IMPACT_PYTHON_TEST_TARGETS_ENABLED)
        message(DEBUG "TIAF enabled for Python tests.")
    else()
        message(DEBUG "TIAF disabled for Python tests.")
    endif()
else()
    set(O3DE_TEST_IMPACT_NATIVE_TEST_TARGETS_ENABLED false)
    set(O3DE_TEST_IMPACT_PYTHON_TEST_TARGETS_ENABLED false)
    set(O3DE_TEST_IMPACT_ACTIVE false)
    message(DEBUG "TIAF disabled. No test target types will be opted in.")
endif()

#! o3de_test_impact_apply_test_labels: applies the the appropriate label to a test target for running in CTest according to whether
#  or not their test framework type is enabled for running in TIAF.
#
# \arg:TEST_FRAMEWORK The test framework type of the test target
# \arg:TEST_LABELS The existing test labels list that the TIAF label will be appended to
function(o3de_test_impact_apply_test_labels TEST_FRAMEWORK TEST_LABELS)
    if("${TEST_FRAMEWORK}" STREQUAL "pytest" OR "${TEST_FRAMEWORK}" STREQUAL "pytest_editor")
        if(NOT O3DE_TEST_IMPACT_PYTHON_TEST_TARGETS_ENABLED)
            set(remove_tiaf_label ON)
        endif()
    elseif("${TEST_FRAMEWORK}" STREQUAL "googletest" OR "${TEST_FRAMEWORK}" STREQUAL "googlebenchmark")
        if(NOT O3DE_TEST_IMPACT_NATIVE_TEST_TARGETS_ENABLED)
            set(remove_tiaf_label ON)
        endif()
    endif()
    
    if(remove_tiaf_label)
        list(REMOVE_ITEM ${TEST_LABELS} ${REQUIRES_TIAF_LABEL})
        set(${TEST_LABELS} ${${TEST_LABELS}} PARENT_SCOPE)
    endif()
endfunction()
