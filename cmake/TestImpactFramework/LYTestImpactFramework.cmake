#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Switch to enable/disable test impact analysis (and related build targets)
option(LY_TEST_IMPACT_ACTIVE "Enable test impact framework" OFF)

# Path to test instrumentation binary
option(LY_TEST_IMPACT_INSTRUMENTATION_BIN "Path to test impact framework instrumentation binary" OFF)

# Name of test impact framework console target
set(LY_TEST_IMPACT_CONSOLE_TARGET "TestImpact.Frontend.Console")

# Directory for non-persistent test impact data trashed with each generation of build system
set(LY_TEST_IMPACT_WORKING_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/TestImpactFramework")

# Directory for temporary files generated at runtime
set(LY_TEST_IMPACT_TEMP_DIR "${LY_TEST_IMPACT_WORKING_DIR}/Temp")

# Directory for static artifacts produced as part of the build system generation process
set(LY_TEST_IMPACT_ARTIFACT_DIR "${LY_TEST_IMPACT_WORKING_DIR}/Artefact")

# Directory for source to build target mappings
set(LY_TEST_IMPACT_SOURCE_TARGET_MAPPING_DIR "${LY_TEST_IMPACT_ARTIFACT_DIR}/Mapping")

# Directory for build target dependency/depender graphs
set(LY_TEST_IMPACT_TARGET_DEPENDENCY_DIR "${LY_TEST_IMPACT_ARTIFACT_DIR}/Dependency")

# Directory for test type enumeration files
set(LY_TEST_IMPACT_TEST_TYPE_DIR "${LY_TEST_IMPACT_ARTIFACT_DIR}/TestType")

# Master test enumeration file for all test types
set(LY_TEST_IMPACT_TEST_TYPE_FILE "${LY_TEST_IMPACT_TEST_TYPE_DIR}/All.tests")

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

#! ly_test_impact_get_target_type_string: gets the target type string (either executable, dynalib or unknown) for the specified target.
#
# \arg:TARGET_NAME name of the target
# \arg:TARGET_TYPE the type string for the specified target
function(ly_test_impact_get_target_type_string TARGET_NAME TARGET_TYPE)
    # Get the test impact framework-friendly target type string
    get_target_property(target_type ${TARGET_NAME} TYPE)
    if("${target_type}" STREQUAL "SHARED_LIBRARY" OR "${target_type}" STREQUAL "MODULE_LIBRARY")
        set(${TARGET_TYPE} "dynlib" PARENT_SCOPE)
    elseif("${target_type}" STREQUAL "EXECUTABLE")
        set(${TARGET_TYPE} "executable" PARENT_SCOPE)
    else()
        set(${TARGET_TYPE} "unknown" PARENT_SCOPE)
    endif()
endfunction()

#! ly_test_impact_extract_google_test: explodes a composite google test string into namespace, test and suite components.
#
# \arg:COMPOSITE_TEST test in the form 'namespace::test'
# \arg:TEST_QUALIFER qualifier for the test (namespace)
# \arg:TEST_NAME name of test
function(ly_test_impact_extract_google_test COMPOSITE_TEST TEST_QUALIFER TEST_NAME)
    get_property(test_components GLOBAL PROPERTY LY_ALL_TESTS_${COMPOSITE_TEST}_TEST_NAME)
    # Namespace and test are mandetiry
    string(REPLACE "::" ";" test_components ${test_components})
    list(LENGTH test_components num_test_components)
    if(num_test_components LESS 2)
        message(FATAL_ERROR "The test ${test_components} appears to have been specified without a namespace, i.e.:\ly_add_googletest/benchmark(NAME ${test_components})\nInstead of (perhaps):\ly_add_googletest/benchmark(NAME Gem::${test_components})\nPlease add the missing namespace before proceeding.")
    endif()

    list(GET test_components 0 test_qualifier)
    list(GET test_components 1 test_name)
    set(${TEST_QUALIFER} ${test_qualifier} PARENT_SCOPE)
    set(${TEST_NAME} ${test_name} PARENT_SCOPE)
endfunction()

#! ly_test_impact_extract_python_test: explodes a composite python test string into filename, namespace, test and suite components.
#
# \arg:COMPOSITE_TEST test in form 'namespace::test' or 'test'
# \arg:TEST_QUALIFER qualifier for the test (optional)
# \arg:TEST_NAME name of test
# \arg:TEST_FILE the Python script path for this test
function(ly_test_impact_extract_python_test COMPOSITE_TEST TEST_QUALIFER TEST_NAME TEST_FILE)
    get_property(test_components GLOBAL PROPERTY LY_ALL_TESTS_${COMPOSITE_TEST}_TEST_NAME)
    get_property(test_file GLOBAL PROPERTY LY_ALL_TESTS_${COMPOSITE_TEST}_SCRIPT_PATH)
    
    # namespace is optional, in which case this component will be simply the test name
    string(REPLACE "::" ";" test_components ${test_components})
    list(LENGTH test_components num_test_components)
    if(num_test_components GREATER 1)
        list(GET test_components 0 test_qualifier)
        list(GET test_components 1 test_name)
    else()
        set(test_qualifier "")
        set(test_name ${test_components})
    endif()

    # Get python script path relative to repo root
    ly_test_impact_rebase_file_to_repo_root(
        ${test_file}
        test_file
        ${LY_ROOT_FOLDER}
    )

    set(${TEST_QUALIFER} ${test_qualifier} PARENT_SCOPE)
    set(${TEST_NAME} ${test_name} PARENT_SCOPE)
    set(${TEST_FILE} ${test_file} PARENT_SCOPE)
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
        if("${test_type}" STREQUAL "pytest")
            # Python tests
            ly_test_impact_extract_python_test(${test} test_qualifier test_name test_file)
            list(APPEND python_tests "{ name = \"${test_name}\", qualifier = \"${test_qualifier}\", suite = \"${test_suite}\", path = \"${test_file}\" }")
        elseif("${test_type}" STREQUAL "pytest_editor")
            # Python editor tests
            ly_test_impact_extract_python_test(${test} test_qualifier test_name test_file)
            list(APPEND python_editor_tests "{ name = \"${test_name}\", qualifier = \"${test_qualifier}\", suite = \"${test_suite}\", path = \"${test_file}\" }")
        elseif("${test_type}" STREQUAL "googletest")
            # Google tests
            ly_test_impact_extract_google_test(${test} test_qualifier test_name)
            ly_test_impact_get_target_type_string(${test_name} target_type)
            list(APPEND google_tests "{ name = \"${test_name}\", qualifier = \"${test_qualifier}\", suite = \"${test_suite}\", build_type = \"${target_type}\" }")
        elseif("${test_type}" STREQUAL "googlebenchmark")
            # Google benchmarks
            ly_test_impact_extract_google_test(${test} test_qualifier test_name)
            list(APPEND google_benchmarks "{ name = \"${test_name}\", qualifier = \"${test_qualifier}\", suite = \"${test_suite}\" }")
        else()
            message("${test} is of unknown type (TEST_LIBRARY property is empty)")
            list(APPEND unknown_tests "{ name = \"${test}\" }")
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
    foreach(aliased_target ${LY_ALL_TARGETS})

        unset(target)
        ly_de_alias_target(${aliased_target} target)
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
# \arg:RUNTIME_BIN_DIR path to repo binary ourput directory
function(ly_test_impact_write_config_file CONFIG_TEMPLATE_FILE PERSISTENT_DATA_DIR RUNTIME_BIN_DIR)
    set(repo_dir ${LY_ROOT_FOLDER})

    # SparTIA instrumentation binary
    if(NOT LY_TEST_IMPACT_INSTRUMENTATION_BIN)
        message(FATAL_ERROR "No test impact framework instrumentation binary was specified, please provide the path with option LY_TEST_IMPACT_INSTRUMENTATION_BIN")
    endif()
    set(instrumentation_bin ${LY_TEST_IMPACT_INSTRUMENTATION_BIN})
    
    # test impact framework working dir
    ly_test_impact_rebase_file_to_repo_root(
        ${LY_TEST_IMPACT_WORKING_DIR}
        working_dir
        ${LY_ROOT_FOLDER}
    )
    
    # test impact framework console binary dir
    ly_test_impact_rebase_file_to_repo_root(
        ${RUNTIME_BIN_DIR}
        runtime_bin_dir
        ${LY_ROOT_FOLDER}
    )

    # Test dir
    ly_test_impact_rebase_file_to_repo_root(
        "${PERSISTENT_DATA_DIR}/Tests"
        tests_dir
        ${LY_ROOT_FOLDER}
    )
    
    # Temp dir
    ly_test_impact_rebase_file_to_repo_root(
        "${LY_TEST_IMPACT_TEMP_DIR}"
        temp_dir
        ${LY_ROOT_FOLDER}
    )
    
    # Source to target mappings dir
    ly_test_impact_rebase_file_to_repo_root(
        "${LY_TEST_IMPACT_SOURCE_TARGET_MAPPING_DIR}"
        source_target_mapping_dir
        ${LY_ROOT_FOLDER}
    )
    
    # Test type artifact dir
    ly_test_impact_rebase_file_to_repo_root(
        "${LY_TEST_IMPACT_TEST_TYPE_DIR}"
        test_type_dir
        ${LY_ROOT_FOLDER}
    )
    
    # Bild dependency artifact dir
    ly_test_impact_rebase_file_to_repo_root(
        "${LY_TEST_IMPACT_TARGET_DEPENDENCY_DIR}"
        target_dependency_dir
        ${LY_ROOT_FOLDER}
    )
    
    # Substitute config file template with above vars
    ly_file_read("${CONFIG_TEMPLATE_FILE}" config_file)
    string(CONFIGURE ${config_file} config_file)
    
    # Write out entire config contents to a file in the build directory of the test impact framework console target
    string(TIMESTAMP timestamp "%Y-%m-%d %H:%M:%S")
    set(header "# Test Impact Framework configuration file for Lumberyard\n# Platform: ${CMAKE_SYSTEM_NAME}\n# Build: $<CONFIG>\n# ${timestamp}")
    file(GENERATE
        OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/$<TARGET_FILE_BASE_NAME:${LY_TEST_IMPACT_CONSOLE_TARGET}>.$<CONFIG>.cfg" 
        CONTENT "${header}\n\n${config_file}"
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
    set(runtime_bin_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")

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
        ${runtime_bin_dir}
    )
    
    # Copy over the graphviz options file for the build dependency graphs
    message(DEBUG "Test impact framework config file written")
    file(COPY "cmake/TestImpactFramework/CMakeGraphVizOptions.cmake" DESTINATION ${CMAKE_BINARY_DIR})

    # Set the above config file as the default config file to use for the test impact framework console target
    target_compile_definitions(${LY_TEST_IMPACT_CONSOLE_TARGET} PRIVATE "LY_TEST_IMPACT_DEFAULT_CONFIG_FILE=\"$<TARGET_FILE_BASE_NAME:${LY_TEST_IMPACT_CONSOLE_TARGET}>.$<CONFIG>.cfg\"")
    message(DEBUG "Test impact framework post steps complete")
endfunction()