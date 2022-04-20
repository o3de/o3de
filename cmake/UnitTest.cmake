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

enable_testing()

include(ProcessorCount)
ProcessorCount(PROCESSOR_COUNT)
if(NOT PROCESSOR_COUNT EQUAL 0)
  set(CTEST_RUN_FLAGS -j${PROCESSOR_COUNT})
endif()

mark_as_advanced(CTEST_RUN_FLAGS)
list(APPEND CTEST_RUN_FLAGS
    -C $<CONFIG>
    --output-on-failure)
list(JOIN CTEST_RUN_FLAGS " " CTEST_RUN_FLAGS_STRING)
set(CTEST_RUN_FLAGS ${CTEST_RUN_FLAGS_STRING} CACHE STRING "Command line arguments passed to ctest for running" FORCE)

# Updates the built-in RUN_TESTS target with the ctest run flags as well
# This requires CMake version 3.17
# Note that we omit benchmarks from running in the auto-generated "RUN_TESTS" target by 
# settings a custom value for CTEST_ARGUMENTS, however since we DO want them
# to run in other custom targets such as the "benchmark" suite, we don't re-use 
# CMAKE_TEST_ARGUMENTS in the other targets.
# Those targets use the CTEST_RUN_FLAGS variable instead, which is captured above,
# before we append this value:
set(CMAKE_CTEST_ARGUMENTS ${CTEST_RUN_FLAGS} -LE SUITE_benchmark)

#! ly_add_suite_build_and_run_targets - Add CMake Targets for associating dependencies with each
#  suite of test supported by Open 3D Engine
function(ly_add_suite_build_and_run_targets)
    if(NOT PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        return()
    endif()
    foreach(suite_name ${ARGV})       
        # Add a custom target for each suite
        add_custom_target(TEST_SUITE_${suite_name})

        set_target_properties(TEST_SUITE_${suite_name} PROPERTIES
            VS_DEBUGGER_COMMAND "${CMAKE_CTEST_COMMAND}"
            VS_DEBUGGER_COMMAND_ARGUMENTS "${CTEST_RUN_FLAGS_STRING} -L SUITE_${suite_name}"
            FOLDER "CMakePredefinedTargets/Test Suites"
        )
    endforeach()
endfunction()



# Templates used to generate the registry of unit test modules
set(test_json_template [[
{
    "Amazon":
    {
@target_test_dependencies_names@
    }
}]]
)
set(test_module_template [[
        "@stripped_test_target@":
        {
            "Modules":["$<TARGET_FILE_NAME:@namespace_and_target@>"]
        }]]
)


#! ly_delayed_generate_test_runner_registry: Generates a .json file for all the test modules that aztestrunner can process
#
function(ly_delayed_generate_unit_test_module_registry)

    if(NOT PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        return()
    endif()

    get_property(ly_delayed_aztestrunner_test_modules GLOBAL PROPERTY LY_AZTESTRUNNER_TEST_MODULES)
    list(REMOVE_DUPLICATES ly_delayed_aztestrunner_test_modules) # Strip out any duplicate test targets

    set(target_test_dependencies_names)
    set(test_module_name_components)

    foreach(namespace_and_target ${ly_delayed_aztestrunner_test_modules})

         # Strip target namespace from test targets before configuring them into the json template
        ly_strip_target_namespace(TARGET ${namespace_and_target} OUTPUT_VARIABLE stripped_test_target)

        string(CONFIGURE ${test_module_template} target_module_json @ONLY)
        list(APPEND target_test_dependencies_names ${target_module_json})

    endforeach()

    list(JOIN target_test_dependencies_names ",\n" target_test_dependencies_names)
    string(CONFIGURE ${test_json_template} testrunner_json @ONLY)
    set(dependencies_setreg ${CMAKE_BINARY_DIR}/unit_test_modules.json)
    file(GENERATE OUTPUT ${dependencies_setreg} CONTENT "${testrunner_json}")

endfunction()
