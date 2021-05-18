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

# Switch to enable/disable test impact analysis (and related build targets)
option(LY_TEST_IMPACT_ACTIVE "Enable test impact framework" OFF)

# Path to test instrumentation binary
option(LY_TEST_IMPACT_INSTRUMENTATION_BIN "Path to test impact framework instrumentation binary" OFF)

# Name of test impact framework console static library target
set(LY_TEST_IMPACT_CONSOLE_STATIC_TARGET "TestImpact.Frontend.Console.Static")

# Name of test impact framework console target
set(LY_TEST_IMPACT_CONSOLE_TARGET "TestImpact.Frontend.Console")

# Directory for non-persistent test impact data trashed with each generation of build system
set(LY_TEST_IMPACT_WORKING_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/TestImpactFramework")

# Directory for temporary files generated at runtime
set(LY_TEST_IMPACT_TEMP_DIR "${LY_TEST_IMPACT_WORKING_DIR}/Temp")

# Directory for static artifacts produced as part of the build system generation process
set(LY_TEST_IMPACT_ARTIFACT_DIR "${LY_TEST_IMPACT_WORKING_DIR}/Artifact")

# Directory for source to build target mappings
set(LY_TEST_IMPACT_SOURCE_TARGET_MAPPING_DIR "${LY_TEST_IMPACT_ARTIFACT_DIR}/Mapping")

# Directory for build target dependency/depender graphs
set(LY_TEST_IMPACT_TARGET_DEPENDENCY_DIR "${LY_TEST_IMPACT_ARTIFACT_DIR}/Dependency")

# Master test enumeration file for all test types
set(LY_TEST_IMPACT_TEST_TYPE_FILE "${LY_TEST_IMPACT_ARTIFACT_DIR}/TestType/All.tests")

#! ly_test_impact_rebase_file_to_repo_root: rebases the relative and/or absolute path to be relative to repo root directory and places the resulting path in quotes.
#
# \arg:INPUT_FILE the file to rebase
# \arg:OUTPUT_FILE the file after rebasing
# \arg:RELATIVE_DIR_ABS the absolute path that the file will be relatively rebased to
function(ly_test_impact_rebase_file_to_repo_root INPUT_FILE OUTPUT_FILE RELATIVE_DIR_ABS)
    # Transform the file paths to absolute paths
    set(rebased_file ${INPUT_FILE})
    if(NOT IS_ABSOLUTE ${rebased_file})
        get_filename_component(rebased_file "${rebased_file}"
            REALPATH BASE_DIR "${RELATIVE_DIR_ABS}"
        )
    endif()
    
    # Rebase absolute path to relative path from repo root
    file(RELATIVE_PATH rebased_file ${LY_ROOT_FOLDER} ${rebased_file})
    set(${OUTPUT_FILE} ${rebased_file} PARENT_SCOPE)
endfunction()

#! ly_test_impact_rebase_files_to_repo_root: rebases the relative and/or absolute paths to be relative to repo root directory and places the resulting paths in quotes.
#
# \arg:INPUT_FILEs the files to rebase
# \arg:OUTPUT_FILEs the files after rebasing
# \arg:RELATIVE_DIR_ABS the absolute path that the files will be relatively rebased to
function(ly_test_impact_rebase_files_to_repo_root INPUT_FILES OUTPUT_FILES RELATIVE_DIR_ABS)
    # Rebase all paths in list to repo root
    set(rebased_files "")
    foreach(in_file IN LISTS INPUT_FILES)
        ly_test_impact_rebase_file_to_repo_root(
            ${in_file}
            out_file
            ${RELATIVE_DIR_ABS}
        )
        list(APPEND rebased_files "\"${out_file}\"")
    endforeach()
    set(${OUTPUT_FILES} ${rebased_files} PARENT_SCOPE)
endfunction()

#! ly_test_impact_get_test_launch_method: gets the launch method (either standalone or testrunner) for the specified target.
#
# \arg:TARGET_NAME name of the target
# \arg:LAUNCH_METHOD the type string for the specified target
function(ly_test_impact_get_test_launch_method TARGET_NAME LAUNCH_METHOD)
    # Get the test impact framework-friendly launch method string
    get_target_property(target_type ${TARGET_NAME} TYPE)
    if("${target_type}" STREQUAL "SHARED_LIBRARY" OR "${target_type}" STREQUAL "MODULE_LIBRARY")
        set(${LAUNCH_METHOD} "test_runner" PARENT_SCOPE)
    elseif("${target_type}" STREQUAL "EXECUTABLE")
        set(${LAUNCH_METHOD} "stand_alone" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Cannot deduce test target launch method for the target type ${target_type}")
    endif()
endfunction()

#! ly_test_impact_extract_google_test_name: extracts the google test name from the composite 'namespace::test_name' string
#
# \arg:COMPOSITE_TEST test in the form 'namespace::test'
# \arg:TEST_NAME name of test
function(ly_test_impact_extract_google_test COMPOSITE_TEST TEST_NAMESPACE TEST_NAME)
    get_property(test_components GLOBAL PROPERTY LY_ALL_TESTS_${COMPOSITE_TEST}_TEST_NAME)
    # Namespace and test are mandetory
    string(REPLACE "::" ";" test_components ${test_components})
    list(LENGTH test_components num_test_components)
    if(num_test_components LESS 2)
        message(FATAL_ERROR "The test ${test_components} appears to have been specified without a namespace, i.e.:\ly_add_googletest/benchmark(NAME ${test_components})\nInstead of (perhaps):\ly_add_googletest/benchmark(NAME Gem::${test_components})\nPlease add the missing namespace before proceeding.")
    endif()

    list(GET test_components 0 test_namespace)
    list(GET test_components 1 test_name)
    set(${TEST_NAMESPACE} ${test_namespace} PARENT_SCOPE)
    set(${TEST_NAME} ${test_name} PARENT_SCOPE)
endfunction()

#! ly_test_impact_extract_python_test_name: extracts the python test name from the composite 'namespace::test_name' string
#
# \arg:COMPOSITE_TEST test in form 'namespace::test' or 'test'
# \arg:TEST_NAME name of test
function(ly_test_impact_extract_python_test COMPOSITE_TEST TEST_NAME)
    get_property(test_components GLOBAL PROPERTY LY_ALL_TESTS_${COMPOSITE_TEST}_TEST_NAME)
    
    # namespace is optional, in which case this component will be simply the test name
    string(REPLACE "::" ";" test_components ${test_components})
    list(LENGTH test_components num_test_components)
    if(num_test_components GREATER 1)
        list(GET test_components 1 test_name)
    else()
        set(test_name ${test_components})
    endif()

    set(${TEST_NAME} ${test_name} PARENT_SCOPE)
endfunction()

#! ly_test_impact_extract_google_test_params: extracts the google test name and command parameters.
#
# \arg:COMPOSITE_TEST test in the form 'namespace::test'
# \arg:TEST_NAMESPACE namespace for the test
# \arg:TEST_NAME name of test
# \arg:TEST_NAME optional command arguments to run the test
# \arg:TEST_NAME test timeout value
function(ly_test_impact_extract_google_test_params COMPOSITE_TEST TEST_NAME TEST_COMMAND)
    get_property(test_command GLOBAL PROPERTY LY_ALL_TESTS_${COMPOSITE_TEST}_TEST_COMMAND)
    # Namespace and test are mandetory
    string(REPLACE "::" ";" test_components ${COMPOSITE_TEST})
    list(LENGTH test_components num_test_components)
    if(num_test_components LESS 2)
        message(FATAL_ERROR "The test ${test_components} appears to have been specified without a namespace, i.e.:\ly_add_googletest/benchmark(NAME ${test_components})\nInstead of (perhaps):\ly_add_googletest/benchmark(NAME Gem::${test_components})\nPlease add the missing namespace before proceeding.")
    endif()

    list(GET test_components 0 test_namespace)
    list(GET test_components 1 test_name)
    set(${TEST_NAMESPACE} ${test_namespace} PARENT_SCOPE)
    set(${TEST_NAME} ${test_name} PARENT_SCOPE)
    set(${TEST_COMMAND} ${test_command} PARENT_SCOPE)
endfunction()

#! ly_test_impact_extract_python_test_params: extracts the python test name and relative script path parameters.
#
# \arg:COMPOSITE_TEST test in form 'namespace::test' or 'test'
# \arg:TEST_NAME name of test
# \arg:SCRIPT_PATH name of test
function(ly_test_impact_extract_python_test_params COMPOSITE_TEST TEST_NAME SCRIPT_PATH)
    get_property(script_path GLOBAL PROPERTY LY_ALL_TESTS_${COMPOSITE_TEST}_SCRIPT_PATH)
    
    # namespace is optional, in which case this component will be simply the test name
    string(REPLACE "::" ";" test_components ${COMPOSITE_TEST})
    list(LENGTH test_components num_test_components)
    if(num_test_components GREATER 1)
        list(GET test_components 1 test_name)
    else()
        set(test_name ${test_components})
    endif()

    # Get python script path relative to repo root
    ly_test_impact_rebase_file_to_repo_root(
        ${script_path}
        script_path
        ${LY_ROOT_FOLDER}
    )

    set(${TEST_NAME} ${test_name} PARENT_SCOPE)
    set(${SCRIPT_PATH} ${script_path} PARENT_SCOPE)
endfunction()

#! ly_test_impact_write_test_enumeration_file: exports the master test lists to file.
# 
# \arg:TEST_ENUMERATION_TEMPLATE_FILE path to test enumeration template file
function(ly_test_impact_write_test_enumeration_file TEST_ENUMERATION_TEMPLATE_FILE)
    get_property(LY_ALL_TESTS GLOBAL PROPERTY LY_ALL_TESTS)
    
    # Enumerated tests for each type
    set(google_tests "")
    set(google_benchmarks "")
    set(python_tests "")
    set(python_editor_tests "")
    set(unknown_tests "")

    # Walk the test list
    foreach(test ${LY_ALL_TESTS})
        message(TRACE "Parsing ${test}")
        get_property(test_type GLOBAL PROPERTY LY_ALL_TESTS_${test}_TEST_LIBRARY)
        get_property(test_suite GLOBAL PROPERTY LY_ALL_TESTS_${test}_TEST_SUITE)
        get_property(test_timeout GLOBAL PROPERTY LY_ALL_TESTS_${test}_TEST_TIMEOUT)
        if("${test_type}" STREQUAL "pytest")
            # Python tests
            ly_test_impact_extract_python_test_params(${test} test_name script_path)
            list(APPEND python_tests "        { \"name\": \"${test_name}\", \"suite\": \"${test_suite}\", \"script\": \"${script_path}\", \"timeout\":${test_timeout} }")
        elseif("${test_type}" STREQUAL "pytest_editor")
            # Python editor tests
            ly_test_impact_extract_python_test_params(${test} test_name script_path)
            list(APPEND python_editor_tests "        { \"name\": \"${test_name}\",  \"suite\": \"${test_suite}\", \"script\": \"${script_path}\", \"timeout\":${test_timeout} }")
        elseif("${test_type}" STREQUAL "googletest")
            # Google tests
            ly_test_impact_extract_google_test_params(${test} test_name test_command)
            ly_test_impact_get_test_launch_method(${test_name} launch_method)
            list(APPEND google_tests "        { \"name\": \"${test_name}\", \"suite\": \"${test_suite}\", \"command\": \"${test_command}\", \"timeout\":${test_timeout}, \"launch_method\": \"${launch_method}\" }")
        elseif("${test_type}" STREQUAL "googlebenchmark")
            # Google benchmarks
            ly_test_impact_extract_google_test_params(${test} test_name test_command)
            list(APPEND google_benchmarks "        { \"name\": \"${test_name}\", \"suite\": \"${test_suite}\",  \"command\": \"${test_command}\", \"timeout\":${test_timeout} }")
        else()
            message("${test_name} is of unknown type (TEST_LIBRARY property is empty)")
            list(APPEND unknown_tests "        { \"name\": \"${test}\" }")
        endif()
    endforeach()

    string(REPLACE ";" ",\n" google_tests "${google_tests}")
    string(REPLACE ";" ",\n" google_benchmarks "${google_benchmarks}")
    string(REPLACE ";" ",\n" python_editor_tests "${python_editor_tests}")
    string(REPLACE ";" ",\n" python_tests "${python_tests}")
    string(REPLACE ";" ",\n" unknown_tests "${unknown_tests}")

    # Write out the test enumeration file
    configure_file(${TEST_ENUMERATION_TEMPLATE_FILE} ${LY_TEST_IMPACT_TEST_TYPE_FILE})
endfunction()

#! ly_test_impact_export_source_target_mappings: exports the static source to target mappings to file.
#
# \arg:MAPPING_TEMPLATE_FILE path to source to target template file
function(ly_test_impact_export_source_target_mappings MAPPING_TEMPLATE_FILE)
    get_property(LY_ALL_TARGETS GLOBAL PROPERTY LY_ALL_TARGETS)

    # Walk the build targets
    foreach(target ${LY_ALL_TARGETS})
        message(TRACE "Exporting static source file mappings for ${target}")
        
        # Target name and path relative to root
        set(target_name ${target})
        get_target_property(target_path_abs ${target} SOURCE_DIR)
        file(RELATIVE_PATH target_path ${LY_ROOT_FOLDER} ${target_path_abs})

        # Output name
        get_target_property(target_output_name ${target} OUTPUT_NAME)
        if (target_output_name STREQUAL "target_output_name-NOTFOUND")
            # No custom output name was specified so use the target name
            set(target_output_name "${target}")
        endif()

        # Autogen source file mappings
        get_target_property(autogen_input_files ${target} AUTOGEN_INPUT_FILES)
        get_target_property(autogen_output_files ${target} AUTOGEN_OUTPUT_FILES)
        if(DEFINED autogen_input_files AND autogen_output_files)
            # Rebase input source file paths to repo root
            ly_test_impact_rebase_files_to_repo_root(
                "${autogen_input_files}"
                autogen_input_files
                ${target_path_abs}
            )
            
            # Rebase output source file paths to repo root
            ly_test_impact_rebase_files_to_repo_root(
                "${autogen_output_files}"
                autogen_output_files
                ${target_path_abs}
            )
        else()
            set(autogen_input_files "")
            set(autogen_output_files "")
        endif()

        # Static source file mappings
        get_target_property(target_type ${target} TYPE)
        if("${target_type}" STREQUAL "INTERFACE_LIBRARY")
            get_target_property(static_sources ${target}_HEADERS SOURCES)
        else()
            get_target_property(static_sources ${target} SOURCES)
        endif()

        # Rebase static source files to repo root
        ly_test_impact_rebase_files_to_repo_root(
            "${static_sources}"
            static_sources
            ${target_path_abs}
        )

        # Add the static source file mappings to the contents
        string (REPLACE ";" ",\n" autogen_input_files "${autogen_input_files}")
        string (REPLACE ";" ",\n" autogen_output_files "${autogen_output_files}")
        string (REPLACE ";" ",\n" static_sources "${static_sources}")

        # Write out source to target mapping file
        set(mapping_path "${LY_TEST_IMPACT_SOURCE_TARGET_MAPPING_DIR}/${target}.target")
        configure_file(${MAPPING_TEMPLATE_FILE} ${mapping_path})
    endforeach()
endfunction()

#! ly_test_impact_write_config_file: writes out the test impact framework config file using the data derived from the build generation process.
# 
# \arg:CONFIG_TEMPLATE_FILE path to the runtime configuration template file
# \arg:PERSISTENT_DATA_DIR path to the test impact framework persistent data directory
# \arg:BIN_DIR path to repo binary output directory
function(ly_test_impact_write_config_file CONFIG_TEMPLATE_FILE PERSISTENT_DATA_DIR BIN_DIR)
    # Platform this config file is being generated for
    set(platform ${PAL_PLATFORM_NAME})

    # Config file version
    set(version "0.0.1")

    # Timestamp this config file was generated at
    string(TIMESTAMP timestamp "%Y-%m-%d %H:%M:%S")

    # Instrumentation binary
    if(NOT LY_TEST_IMPACT_INSTRUMENTATION_BIN)
        message(FATAL_ERROR "No test impact framework instrumentation binary was specified, please provide the path with option LY_TEST_IMPACT_INSTRUMENTATION_BIN")
    endif()
    file(TO_CMAKE_PATH ${LY_TEST_IMPACT_INSTRUMENTATION_BIN} instrumentation_bin)

    # Testrunner binary
    set(test_runner_bin $<TARGET_FILE:AzTestRunner>)

    # Repository root
    set(repo_dir ${LY_ROOT_FOLDER})
    
    # Test impact framework output binary dir
    set(bin_dir ${BIN_DIR})
    
    # Temp dir
    set(temp_dir "${LY_TEST_IMPACT_TEMP_DIR}")

    # Persistent dir
    set(persistent_dir "${PERSISTENT_DATA_DIR}")

    # Source to target mappings dir
    set(source_target_mapping_dir "${LY_TEST_IMPACT_SOURCE_TARGET_MAPPING_DIR}")
    
    # Test type artifact file
    set(test_target_type_file "${LY_TEST_IMPACT_TEST_TYPE_FILE}")
    
    # Build dependency artifact dir
    set(target_dependency_dir "${LY_TEST_IMPACT_TARGET_DEPENDENCY_DIR}")
    
    # Substitute config file template with above vars
    file(READ "${CONFIG_TEMPLATE_FILE}" config_file)
    string(CONFIGURE ${config_file} config_file)
    
    # Write out entire config contents to a file in the build directory of the test impact framework console target
    file(GENERATE
        OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/$<TARGET_FILE_BASE_NAME:${LY_TEST_IMPACT_CONSOLE_TARGET}>.$<CONFIG>.json" 
        CONTENT ${config_file}
    )
endfunction()

#! ly_test_impact_post_step: runs the post steps to be executed after all other cmake scripts have been executed.
function(ly_test_impact_post_step)
    if(NOT ${LY_TEST_IMPACT_ACTIVE})
        return()
    endif()

    # Directory per build config for persistent test impact data (to be checked in)
    set(persistent_data_dir "${LY_ROOT_FOLDER}/Tests/test_impact_framework/${CMAKE_SYSTEM_NAME}/$<CONFIG>")

    # Directory for binaries built for this profile
    set(bin_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")

    # Erase any existing non-persistent data to avoid getting test impact framework out of sync with current repo state
    file(REMOVE_RECURSE "${LY_TEST_IMPACT_WORKING_DIR}")

    # Export the soruce to target mapping files
    ly_test_impact_export_source_target_mappings(
        "cmake/TestImpactFramework/SourceToTargetMapping.in"
    )

    # Export the enumerated tests
    ly_test_impact_write_test_enumeration_file(
        "cmake/TestImpactFramework/EnumeratedTests.in"
    )

    # Write out the configuration file
    ly_test_impact_write_config_file(
        "cmake/TestImpactFramework/ConsoleFrontendConfig.in"
        ${persistent_data_dir}
        ${bin_dir}
    )
    
    # Copy over the graphviz options file for the build dependency graphs
    message(DEBUG "Test impact framework config file written")
    file(COPY "cmake/TestImpactFramework/CMakeGraphVizOptions.cmake" DESTINATION ${CMAKE_BINARY_DIR})

    # Set the above config file as the default config file to use for the test impact framework console target
    target_compile_definitions(${LY_TEST_IMPACT_CONSOLE_STATIC_TARGET} PUBLIC "LY_TEST_IMPACT_DEFAULT_CONFIG_FILE=\"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/$<TARGET_FILE_BASE_NAME:${LY_TEST_IMPACT_CONSOLE_TARGET}>.$<CONFIG>.json\"")
    message(DEBUG "Test impact framework post steps complete")
endfunction()