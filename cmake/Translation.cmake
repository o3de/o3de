#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# ============================================================================
# O3DE Internationalization (i18n) Configuration
# ============================================================================

# Enable/disable translation file generation
set(LY_I18N_BUILD ON CACHE BOOL "Enable I18N translation file generation")

# Target language for generation (single language mode)
set(LY_I18N_LANGUAGE "zh_CN" CACHE STRING "I18N target language for single language generation")

# Generate all supported languages at once
set(LY_I18N_BUILD_ALL_LANGUAGES ON CACHE BOOL "Generate translation files for all supported languages")

# Compile .ts files to .qm files
set(LY_I18N_COMPILE_QM ON CACHE BOOL "Compile .ts translation files to .qm binary files")

# Gems to exclude from auto-discovery (semicolon-separated list)
# EMotionFX is always excluded because it requires special multi-directory handling
set(LY_I18N_EXCLUDE_GEMS "" CACHE STRING "Gems to exclude from automatic translation discovery")

# Define all supported languages (excluding default/source language en_US)
# Format: language_code (matches Translation.h Language enum)
# Note: en_US is the default source language and does not need translation files generated
set(LY_I18N_SUPPORTED_LANGUAGES
    "zh_CN"        # Simplified Chinese
    CACHE STRING "List of all supported languages"
)

# ============================================================================
# Validation
# ============================================================================

# We only support I18N Generators in tools, so turn it off unless the platform supports tools.
if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    set(LY_I18N_BUILD OFF)
    return()
endif()

# Validate language if not building all languages
if(LY_I18N_BUILD AND NOT LY_I18N_BUILD_ALL_LANGUAGES)
    if(NOT LY_I18N_LANGUAGE)
        set(LY_I18N_LANGUAGE "en_US")
    endif()
    
    # Validate the language is supported
    list(FIND LY_I18N_SUPPORTED_LANGUAGES "${LY_I18N_LANGUAGE}" _lang_index)
    if(_lang_index EQUAL -1)
        message(WARNING "Language '${LY_I18N_LANGUAGE}' is not in the supported languages list. Supported: ${LY_I18N_SUPPORTED_LANGUAGES}")
        set(LY_I18N_BUILD OFF)
        return()
    endif()
endif()

# Print configuration summary
if(LY_I18N_BUILD)
    message(STATUS "============================================")
    message(STATUS "O3DE Internationalization (i18n) Configuration")
    message(STATUS "============================================")
    message(STATUS "  Generation Enabled: ${LY_I18N_BUILD}")
    if(LY_I18N_BUILD_ALL_LANGUAGES)
        message(STATUS "  Mode: Generate ALL languages")
        message(STATUS "  Languages: ${LY_I18N_SUPPORTED_LANGUAGES}")
        list(LENGTH LY_I18N_SUPPORTED_LANGUAGES _i18n_lang_count)
        message(STATUS "  Total: ${_i18n_lang_count} languages")
    else()
        message(STATUS "  Mode: Single language")
        message(STATUS "  Target Language: ${LY_I18N_LANGUAGE}")
    endif()
    message(STATUS "  Translation Root: ${LY_ROOT_FOLDER}/Assets/Editor/Translations")
    message(STATUS "  Compile QM Files: ${LY_I18N_COMPILE_QM}")
    message(STATUS "============================================")
endif()

# ============================================================================
# Qt Linguist Tools Detection (Qt6-first with Qt5 fallback)
# ============================================================================
# Detect Qt LinguistTools once and cache the results for all functions below.
# Prefers Qt6; falls back to Qt5 if Qt6 is not available.

set(_LY_I18N_QT_TOOLS_FOUND FALSE)

find_package(Qt6 COMPONENTS LinguistTools QUIET)
if(Qt6_FOUND)
    set(_LY_I18N_QT_VERSION "6")
    set(_LY_I18N_QT_DIR "${Qt6_DIR}")
else()
    find_package(Qt5 COMPONENTS LinguistTools QUIET)
    if(Qt5_FOUND)
        set(_LY_I18N_QT_VERSION "5")
        set(_LY_I18N_QT_DIR "${Qt5_DIR}")
    endif()
endif()

if(_LY_I18N_QT_DIR)
    find_program(LUPDATE_EXECUTABLE lupdate PATHS "${_LY_I18N_QT_DIR}/../../../bin" NO_DEFAULT_PATH)
    find_program(LRELEASE_EXECUTABLE lrelease PATHS "${_LY_I18N_QT_DIR}/../../../bin" NO_DEFAULT_PATH)
endif()

if(LUPDATE_EXECUTABLE AND LRELEASE_EXECUTABLE)
    set(_LY_I18N_QT_TOOLS_FOUND TRUE)
    if(LY_I18N_BUILD)
        message(STATUS "  Qt Version: ${_LY_I18N_QT_VERSION}")
        message(STATUS "  lupdate:    ${LUPDATE_EXECUTABLE}")
        message(STATUS "  lrelease:   ${LRELEASE_EXECUTABLE}")
    endif()
else()
    if(LY_I18N_BUILD)
        message(WARNING "Qt Linguist tools (lupdate/lrelease) not found. "
            "Install Qt6 or Qt5 LinguistTools. Translation generation disabled.")
        set(LY_I18N_BUILD OFF)
    endif()
endif()

# ============================================================================
# QM Compilation Helper Function
# ============================================================================

#! compile_ts_to_qm
#
# Compiles a .ts translation file to .qm binary format
#
# \arg:ts_file - Path to the .ts file to compile
# \arg:language - Language code for logging
#
function(compile_ts_to_qm ts_file language)
    if(NOT LY_I18N_COMPILE_QM)
        return()
    endif()
    
    # Verify the parent directory exists
    get_filename_component(_ts_parent_dir "${ts_file}" DIRECTORY)
    if(NOT IS_DIRECTORY "${_ts_parent_dir}")
        message(WARNING "  [QM] Directory does not exist, skipping: ${_ts_parent_dir}")
        return()
    endif()
    
    if(NOT EXISTS "${ts_file}")
        message(STATUS "  [QM] TS file does not exist, skipping: ${ts_file}")
        return()
    endif()
    
    if(NOT _LY_I18N_QT_TOOLS_FOUND)
        message(WARNING "  [QM] lrelease not found. .qm files will not be generated. Install Qt6 or Qt5 Linguist Tools.")
        return()
    endif()
    
    # Determine output .qm file path
    get_filename_component(ts_dir "${ts_file}" DIRECTORY)
    get_filename_component(ts_name "${ts_file}" NAME_WE)
    set(qm_file "${ts_dir}/${ts_name}.qm")
    
    # Convert to native paths for lrelease on Windows
    file(TO_NATIVE_PATH "${ts_file}" _native_ts_file)
    file(TO_NATIVE_PATH "${qm_file}" _native_qm_file)
    
    # Compile .ts to .qm
    message(STATUS "  [QM]   ${language} -> ${qm_file}")
    execute_process(
        COMMAND "${LRELEASE_EXECUTABLE}" "${_native_ts_file}" -qm "${_native_qm_file}" -silent
        RESULT_VARIABLE _lrelease_result
        OUTPUT_VARIABLE _lrelease_output
        ERROR_VARIABLE _lrelease_error
    )
    
    if(NOT _lrelease_result EQUAL 0)
        message(WARNING "  [QM] lrelease failed for ${language} (exit code: ${_lrelease_result}):\n  stderr: ${_lrelease_error}\n  stdout: ${_lrelease_output}")
        return()
    endif()
    
    if(NOT EXISTS "${qm_file}")
        message(WARNING "  [QM] Failed to generate ${qm_file}")
    endif()
endfunction()

# ============================================================================
# Translation Module Generation Function
# ============================================================================

#! add_translation_module
#
# Generates translation files (.ts) for a given module
#
# \arg:target_source_dir - Source directory containing translatable strings
# \arg:module_name - Name of the module (e.g., "Editor", "MyGem")
# \arg:INCLUDE_DIRS - (Optional) Additional include directories for lupdate to resolve
#                     header files and properly detect C++ namespaces
#
function(add_translation_module target_source_dir module_name)
    if(NOT LY_I18N_BUILD)
        return()
    endif()

    # Parse optional arguments
    cmake_parse_arguments(ARG "" "" "INCLUDE_DIRS" ${ARGN})

    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        if(NOT _LY_I18N_QT_TOOLS_FOUND)
            message(WARNING "lupdate not found. Translation files will not be generated.")
            return()
        endif()
        
        # Build lupdate include path arguments (-I <dir>)
        set(_include_args "")
        if(ARG_INCLUDE_DIRS)
            foreach(_inc_dir ${ARG_INCLUDE_DIRS})
                if(EXISTS "${_inc_dir}")
                    list(APPEND _include_args "-I" "${_inc_dir}")
                endif()
            endforeach()
        endif()
        
        # Determine which languages to generate
        if(LY_I18N_BUILD_ALL_LANGUAGES)
            set(LANGUAGES ${LY_I18N_SUPPORTED_LANGUAGES})
            list(LENGTH LY_I18N_SUPPORTED_LANGUAGES _i18n_total_langs)
            message(STATUS "Generating translations for module '${module_name}': ALL languages (${_i18n_total_langs} total)")
        else()
            set(LANGUAGES ${LY_I18N_LANGUAGE})
            message(STATUS "Generating translations for module '${module_name}': ${LY_I18N_LANGUAGE}")
        endif()
        
        set(_generated_count 0)
        set(_skipped_count 0)
        
        # Generate translation files for each language
        foreach(_language ${LANGUAGES})
            # Skip English variants (source language)
            if(_language MATCHES "^en(_|$)")
                message(STATUS "  [SKIP] ${_language} - Source language, no translation needed")
                math(EXPR _skipped_count "${_skipped_count} + 1")
                continue()
            endif()
            
            # Create language-specific directory
            set(TS_DIR "${LY_ROOT_FOLDER}/Assets/Editor/Translations/${_language}")
            file(MAKE_DIRECTORY "${TS_DIR}")
            
            # Verify directory was created
            if(NOT IS_DIRECTORY "${TS_DIR}")
                message(WARNING "  [FAIL] Could not create directory: ${TS_DIR}")
                continue()
            endif()
            
            # Set output .ts file path
            set(TS_FILE "${TS_DIR}/${module_name}_${_language}.ts")
            
            # Run lupdate to extract translatable strings
            message(STATUS "  [GEN]  ${_language} -> ${TS_FILE}")
            execute_process(
                COMMAND "${LUPDATE_EXECUTABLE}" "${target_source_dir}" ${_include_args} -extensions cpp,cxx,cc,h,hh,hpp,inl,ui -ts "${TS_FILE}" -silent
                RESULT_VARIABLE _lupdate_result
                OUTPUT_VARIABLE _lupdate_output
                ERROR_VARIABLE _lupdate_error
            )
            
            if(NOT _lupdate_result EQUAL 0)
                message(WARNING "  [FAIL] lupdate failed for ${_language}: ${_lupdate_error}")
                continue()
            endif()
            
            # Verify lupdate created the .ts file
            if(NOT EXISTS "${TS_FILE}")
                message(WARNING "  [FAIL] lupdate returned success but did not create: ${TS_FILE}")
                continue()
            endif()
            
            # Normalize line endings on Windows
            if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
                file(READ "${TS_FILE}" ts_file_content)
                file(CONFIGURE OUTPUT "${TS_FILE}" CONTENT "${ts_file_content}" NEWLINE_STYLE CRLF)
            endif()
            
            # Compile .ts to .qm
            compile_ts_to_qm("${TS_FILE}" "${_language}")
            
            math(EXPR _generated_count "${_generated_count} + 1")
        endforeach()        
    endif()
endfunction()

# ============================================================================
# Batch Generation Helper
# ============================================================================

#! generate_all_translations
#
# Helper function to generate translations for multiple modules at once
#
# \arg:modules_list - List of module names to process
#
function(generate_all_translations)
    if(NOT LY_I18N_BUILD)
        return()
    endif()
    
    set(_modules ${ARGN})    
    
    foreach(_module ${_modules})
        # Try to find the module source directory
        set(_module_src_dir "${LY_ROOT_FOLDER}/Code/Editor")
        if(EXISTS "${_module_src_dir}")
            add_translation_module(${_module_src_dir} ${_module})
        else()
            message(WARNING "Module source directory not found: ${_module_src_dir}")
        endif()
    endforeach()
    
endfunction()

# ============================================================================
# Tools Translation Generation
# ============================================================================

#! add_tool_translation
#
# Generates translation files (.ts) for standalone tools
#
# \arg:tool_name - Name of the tool (e.g., "ProjectManager", "RemoteConsole")
# \arg:tool_source_dir - Source directory for the tool (optional, auto-detected if not provided)
#
function(add_tool_translation tool_name)
    if(NOT LY_I18N_BUILD)
        return()
    endif()

    # Parse optional source directory argument
    set(_tool_src_dir "")
    if(ARGC GREATER 1)
        set(_tool_src_dir ${ARGV1})
    endif()

    # Auto-detect tool source directory if not provided
    if(NOT _tool_src_dir)
        # Try common tool locations
        set(_possible_dirs
            "${LY_ROOT_FOLDER}/Code/Tools/${tool_name}"
            "${LY_ROOT_FOLDER}/Code/Tools/${tool_name}/Source"
            "${LY_ROOT_FOLDER}/Tools/${tool_name}"
        )
        
        foreach(_dir ${_possible_dirs})
            if(EXISTS "${_dir}")
                set(_tool_src_dir ${_dir})
                break()
            endif()
        endforeach()
        
        if(NOT _tool_src_dir)
            message(WARNING "Could not find source directory for tool '${tool_name}'. Skipping translation generation.")
            return()
        endif()
    endif()

    if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
        return()
    endif()

    if(NOT _LY_I18N_QT_TOOLS_FOUND)
        message(WARNING "lupdate not found. Translation files will not be generated for tool '${tool_name}'.")
        return()
    endif()
    
    # Determine which languages to generate
    if(LY_I18N_BUILD_ALL_LANGUAGES)
        set(LANGUAGES ${LY_I18N_SUPPORTED_LANGUAGES})
    else()
        set(LANGUAGES ${LY_I18N_LANGUAGE})
    endif()    
    
    set(_generated_count 0)
    set(_skipped_count 0)
    
    # Generate translation files for each language
    foreach(_language ${LANGUAGES})
        # Skip English variants (source language)
        if(_language MATCHES "^en(_|$)")
            math(EXPR _skipped_count "${_skipped_count} + 1")
            continue()
        endif()
        
        # Create language-specific directory
        set(TS_DIR "${LY_ROOT_FOLDER}/Assets/Editor/Translations/${_language}")
        file(MAKE_DIRECTORY "${TS_DIR}")
        
        # Verify directory was created
        if(NOT IS_DIRECTORY "${TS_DIR}")
            message(WARNING "  [FAIL] Could not create directory: ${TS_DIR}")
            continue()
        endif()
        
        # Set output .ts file path
        set(TS_FILE "${TS_DIR}/${tool_name}_${_language}.ts")
        
        # Run lupdate to extract translatable strings
        execute_process(
            COMMAND "${LUPDATE_EXECUTABLE}" "${_tool_src_dir}" -extensions cpp,cxx,cc,h,hh,hpp,inl,ui -ts "${TS_FILE}" -silent
            RESULT_VARIABLE _lupdate_result
            OUTPUT_VARIABLE _lupdate_output
            ERROR_VARIABLE _lupdate_error
        )
        
        if(NOT _lupdate_result EQUAL 0)
            message(WARNING "  [FAIL] lupdate failed for ${_language}: ${_lupdate_error}")
            continue()
        endif()
        
        # Verify lupdate created the .ts file
        if(NOT EXISTS "${TS_FILE}")
            message(WARNING "  [FAIL] lupdate returned success but did not create: ${TS_FILE}")
            continue()
        endif()
        
        # Normalize line endings on Windows
        if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
            file(READ "${TS_FILE}" ts_file_content)
            file(CONFIGURE OUTPUT "${TS_FILE}" CONTENT "${ts_file_content}" NEWLINE_STYLE CRLF)
        endif()
        
        # Compile .ts to .qm
        compile_ts_to_qm("${TS_FILE}" "${_language}")
        
        math(EXPR _generated_count "${_generated_count} + 1")
    endforeach()
    
endfunction()

#! generate_all_tools_translations
#
# Batch generate translations for all standalone tools
#
# This function automatically detects and processes tools in:
#   - Code/Tools/*
#   - Tools/*
#
function(generate_all_tools_translations)
    if(NOT LY_I18N_BUILD)
        return()
    endif()
        
    # Define list of standalone tools with GUI
    set(_tools_list
        # Code/Tools directory
        "ProjectManager"
        "AssetProcessor"
        "AssetBundler"
        "LuaIDE"
        "RemoteConsole"
        
        # Tools directory (if exists)
        "EventLogTools"
        "DebugVis"
    )
    
    set(_processed_count 0)
    set(_skipped_count 0)
    
    foreach(_tool ${_tools_list})
        
        # Try to find the tool directory
        set(_tool_found FALSE)
        set(_tool_paths
            "${LY_ROOT_FOLDER}/Code/Tools/${_tool}"
            "${LY_ROOT_FOLDER}/Code/Tools/${_tool}/Source"
            "${LY_ROOT_FOLDER}/Tools/${_tool}"
        )
        
        foreach(_path ${_tool_paths})
            if(EXISTS "${_path}")
                add_tool_translation(${_tool} ${_path})
                set(_tool_found TRUE)
                math(EXPR _processed_count "${_processed_count} + 1")
                break()
            endif()
        endforeach()
        
        if(NOT _tool_found)
            message(STATUS "  [SKIP] Tool '${_tool}' not found in common locations")
            math(EXPR _skipped_count "${_skipped_count} + 1")
        endif()
    endforeach()

endfunction()

# ============================================================================
# Gems Translation Generation
# ============================================================================

#! add_gem_translation_multi_dirs
#
# Generates translation files (.ts) for a Gem by scanning multiple source directories
# This is useful for Gems like EMotionFX that have UI code spread across multiple locations
#
# \arg:gem_name - Name of the Gem (e.g., "EMotionFX")
# \arg:source_dirs - List of source directories to scan (semicolon-separated)
# \arg:INCLUDE_DIRS - (Optional) Additional include directories for lupdate to resolve
#                     header files and properly detect C++ namespaces (e.g., EMStudio::)
#
function(add_gem_translation_multi_dirs gem_name source_dirs)
    if(NOT LY_I18N_BUILD)
        return()
    endif()

    if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
        return()
    endif()

    # Parse optional arguments
    # OUTPUT_ROOT: when set, .ts/.qm files go to {OUTPUT_ROOT}/{language}/ (gem-local mode)
    #              when not set, files go to the centralized Assets/Editor/Translations/{language}/
    cmake_parse_arguments(ARG "" "OUTPUT_ROOT" "INCLUDE_DIRS" ${ARGN})

    if(NOT _LY_I18N_QT_TOOLS_FOUND)
        message(WARNING "lupdate not found. Translation files will not be generated for Gem '${gem_name}'.")
        return()
    endif()
    
    # Build lupdate include path arguments (-I <dir>)
    # This allows lupdate to resolve header files and properly detect C++ namespaces
    # Without proper include paths, lupdate may fail to detect namespace scopes
    # (e.g., extracting context as "ClassName" instead of "Namespace::ClassName")
    set(_include_args "")
    if(ARG_INCLUDE_DIRS)
        foreach(_inc_dir ${ARG_INCLUDE_DIRS})
            if(EXISTS "${_inc_dir}")
                list(APPEND _include_args "-I" "${_inc_dir}")
            endif()
        endforeach()
    endif()
    
    # Determine which languages to generate
    if(LY_I18N_BUILD_ALL_LANGUAGES)
        set(LANGUAGES ${LY_I18N_SUPPORTED_LANGUAGES})
    else()
        set(LANGUAGES ${LY_I18N_LANGUAGE})
    endif()
       
    message(STATUS "Generating translations for Gem '${gem_name}' from multiple directories:")
    foreach(_dir ${source_dirs})
        message(STATUS "  - ${_dir}")
    endforeach()
    if(ARG_INCLUDE_DIRS)
        message(STATUS "  Include paths for namespace resolution:")
        foreach(_inc_dir ${ARG_INCLUDE_DIRS})
            message(STATUS "    -I ${_inc_dir}")
        endforeach()
    endif()
    
    # Generate translation files for each language
    foreach(_language ${LANGUAGES})
        # Skip English variants (source language)
        if(_language MATCHES "^en(_|$)")
            continue()
        endif()
        
        # Create language-specific directory
        # When OUTPUT_ROOT is set, output to gem-local directory; otherwise use centralized location
        if(ARG_OUTPUT_ROOT)
            set(TS_DIR "${ARG_OUTPUT_ROOT}/${_language}")
        else()
            set(TS_DIR "${LY_ROOT_FOLDER}/Assets/Editor/Translations/${_language}")
        endif()
        file(MAKE_DIRECTORY "${TS_DIR}")
        
        # Verify directory was created
        if(NOT IS_DIRECTORY "${TS_DIR}")
            message(WARNING "  [FAIL] Could not create directory: ${TS_DIR}")
            continue()
        endif()
        
        # Set output .ts file path
        set(TS_FILE "${TS_DIR}/${gem_name}_${_language}.ts")
        
        # Run lupdate with multiple source directories and include paths
        # The -I flags allow lupdate to resolve headers and detect C++ namespaces
        # (e.g., ensuring EMStudio::ClassName is used instead of just ClassName)
        message(STATUS "  [GEN]  ${_language} -> ${TS_FILE}")
        execute_process(
            COMMAND "${LUPDATE_EXECUTABLE}" ${source_dirs} ${_include_args} -extensions cpp,cxx,cc,h,hh,hpp,inl,ui -ts "${TS_FILE}" -silent
            RESULT_VARIABLE _lupdate_result
            OUTPUT_VARIABLE _lupdate_output
            ERROR_VARIABLE _lupdate_error
        )
        
        if(NOT _lupdate_result EQUAL 0)
            message(WARNING "  [FAIL] lupdate failed for ${_language}: ${_lupdate_error}")
            continue()
        endif()
        
        # Verify lupdate created the .ts file
        if(NOT EXISTS "${TS_FILE}")
            message(WARNING "  [FAIL] lupdate returned success but did not create: ${TS_FILE}")
            continue()
        endif()
        
        # Normalize line endings on Windows
        if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
            file(READ "${TS_FILE}" ts_file_content)
            file(CONFIGURE OUTPUT "${TS_FILE}" CONTENT "${ts_file_content}" NEWLINE_STYLE CRLF)
        endif()
        
        # Compile .ts to .qm
        compile_ts_to_qm("${TS_FILE}" "${_language}")
    endforeach()
    
endfunction()

#! add_gem_translation
#
# Generates translation files (.ts) for a Gem with UI components
#
# \arg:gem_name - Name of the Gem (e.g., "ScriptCanvas", "EMotionFX")
# \arg:gem_source_dir - Source directory for the Gem (optional, auto-detected if not provided)
# \arg:INCLUDE_DIRS - (Optional) Additional include directories for lupdate to resolve
#                     header files and properly detect C++ namespaces
#
function(add_gem_translation gem_name)
    if(NOT LY_I18N_BUILD)
        return()
    endif()

    # Parse arguments:
    #   First unparsed positional argument = source directory (optional)
    #   OUTPUT_ROOT = when set, .ts/.qm go to {OUTPUT_ROOT}/{language}/ (gem-local mode)
    #   INCLUDE_DIRS = additional include directories for namespace resolution (optional)
    cmake_parse_arguments(ARG "" "OUTPUT_ROOT" "INCLUDE_DIRS" ${ARGN})
    
    set(_gem_src_dir "")
    if(ARG_UNPARSED_ARGUMENTS)
        list(GET ARG_UNPARSED_ARGUMENTS 0 _gem_src_dir)
    endif()

    # Auto-detect gem source directory if not provided
    if(NOT _gem_src_dir)
        # Try common gem locations
        set(_possible_dirs
            "${LY_ROOT_FOLDER}/Gems/${gem_name}/Code/Source"
            "${LY_ROOT_FOLDER}/Gems/${gem_name}/Code"
            "${LY_ROOT_FOLDER}/Gems/${gem_name}"
            "${LY_ROOT_FOLDER}/Gems/${gem_name}/Editor"
        )
        
        foreach(_dir ${_possible_dirs})
            if(EXISTS "${_dir}")
                set(_gem_src_dir ${_dir})
                break()
            endif()
        endforeach()
        
        if(NOT _gem_src_dir)
            message(WARNING "Could not find source directory for Gem '${gem_name}'. Skipping translation generation.")
            return()
        endif()
    endif()

    if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
        return()
    endif()

    if(NOT _LY_I18N_QT_TOOLS_FOUND)
        message(WARNING "lupdate not found. Translation files will not be generated for Gem '${gem_name}'.")
        return()
    endif()
    
    # Build lupdate include path arguments (-I <dir>)
    set(_include_args "")
    if(ARG_INCLUDE_DIRS)
        foreach(_inc_dir ${ARG_INCLUDE_DIRS})
            if(EXISTS "${_inc_dir}")
                list(APPEND _include_args "-I" "${_inc_dir}")
            endif()
        endforeach()
    endif()
    
    # Determine which languages to generate
    if(LY_I18N_BUILD_ALL_LANGUAGES)
        set(LANGUAGES ${LY_I18N_SUPPORTED_LANGUAGES})
    else()
        set(LANGUAGES ${LY_I18N_LANGUAGE})
    endif()
       
    set(_generated_count 0)
    set(_skipped_count 0)
    
    # Generate translation files for each language
    foreach(_language ${LANGUAGES})
        # Skip English variants (source language)
        if(_language MATCHES "^en(_|$)")
            math(EXPR _skipped_count "${_skipped_count} + 1")
            continue()
        endif()
        
        # Create language-specific directory
        # When OUTPUT_ROOT is set, output to gem-local directory; otherwise use centralized location
        if(ARG_OUTPUT_ROOT)
            set(TS_DIR "${ARG_OUTPUT_ROOT}/${_language}")
        else()
            set(TS_DIR "${LY_ROOT_FOLDER}/Assets/Editor/Translations/${_language}")
        endif()
        file(MAKE_DIRECTORY "${TS_DIR}")
        
        # Verify directory was created
        if(NOT IS_DIRECTORY "${TS_DIR}")
            message(WARNING "  [FAIL] Could not create directory: ${TS_DIR}")
            continue()
        endif()
        
        # Set output .ts file path
        set(TS_FILE "${TS_DIR}/${gem_name}_${_language}.ts")
        
        # Run lupdate to extract translatable strings (with include paths if provided)
        execute_process(
            COMMAND "${LUPDATE_EXECUTABLE}" "${_gem_src_dir}" ${_include_args} -extensions cpp,cxx,cc,h,hh,hpp,inl,ui -ts "${TS_FILE}" -silent
            RESULT_VARIABLE _lupdate_result
            OUTPUT_VARIABLE _lupdate_output
            ERROR_VARIABLE _lupdate_error
        )
        
        if(NOT _lupdate_result EQUAL 0)
            message(WARNING "  [FAIL] lupdate failed for ${_language}: ${_lupdate_error}")
            continue()
        endif()
        
        # Verify lupdate created the .ts file
        if(NOT EXISTS "${TS_FILE}")
            message(WARNING "  [FAIL] lupdate returned success but did not create: ${TS_FILE}")
            continue()
        endif()
        
        # Normalize line endings on Windows
        if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
            file(READ "${TS_FILE}" ts_file_content)
            file(CONFIGURE OUTPUT "${TS_FILE}" CONTENT "${ts_file_content}" NEWLINE_STYLE CRLF)
        endif()
        
        # Compile .ts to .qm
        compile_ts_to_qm("${TS_FILE}" "${_language}")
        
        math(EXPR _generated_count "${_generated_count} + 1")
    endforeach()
    
endfunction()

#! _i18n_collect_gem_jsons_from_json_subdirs
#
# Helper: reads "external_subdirectories" from a JSON manifest file and
# appends any gem.json files found to the output list.
#
# \arg:output_var  - Variable name to append results to (in PARENT_SCOPE)
# \arg:json_path   - Path to the JSON file (e.g., o3de_manifest.json, project.json)
# \arg:base_dir    - Base directory for resolving relative paths in external_subdirectories
#
function(_i18n_collect_gem_jsons_from_json_subdirs output_var json_path base_dir)
    if(NOT EXISTS "${json_path}")
        return()
    endif()

    file(READ "${json_path}" _json_content)

    string(JSON _ext_count ERROR_VARIABLE _err LENGTH "${_json_content}" "external_subdirectories")
    if(_err OR _ext_count EQUAL 0)
        return()
    endif()

    set(_collected "")
    math(EXPR _last "${_ext_count} - 1")
    foreach(_idx RANGE ${_last})
        string(JSON _ext_subdir ERROR_VARIABLE _item_err GET "${_json_content}" "external_subdirectories" ${_idx})
        if(_item_err)
            continue()
        endif()

        # Resolve relative paths against the base directory
        if(NOT IS_ABSOLUTE "${_ext_subdir}")
            set(_ext_subdir "${base_dir}/${_ext_subdir}")
        endif()

        # Normalize the path
        cmake_path(NORMAL_PATH _ext_subdir)

        if(IS_DIRECTORY "${_ext_subdir}")
            if(EXISTS "${_ext_subdir}/gem.json")
                list(APPEND _collected "${_ext_subdir}/gem.json")
            endif()
            # Also check for nested gems (sub-gems within this external subdirectory)
            file(GLOB_RECURSE _nested_gem_jsons "${_ext_subdir}/*/gem.json")
            if(_nested_gem_jsons)
                list(APPEND _collected ${_nested_gem_jsons})
            endif()
        endif()
    endforeach()

    set(${output_var} ${${output_var}} ${_collected} PARENT_SCOPE)
endfunction()

#! generate_all_gems_translations
#
# Auto-discover and generate translations for ALL Gems with source code,
# from three sources:
#   1. Engine built-in Gems (under ${LY_ROOT_FOLDER}/Gems/)
#   2. External Gems registered in ~/.o3de/o3de_manifest.json (external_subdirectories)
#   3. Game project Gems (external_subdirectories in project.json + Gems/ under project)
#
# Translation files (.ts/.qm) are placed in each Gem's own directory:
#   {gem_root}/Editor/Translations/{language}/{gem_name}_{language}.ts
#
# This makes each Gem self-contained with its own translations, supporting
# engine Gems, external/downloaded Gems, and project-local Gems equally.
#
# Auto-discovery features:
#   - Finds ALL Gems regardless of nesting depth
#   - Reads gem_name from gem.json (requires CMake 3.19+ for string(JSON))
#   - Skips Asset-only Gems (type = "Asset")
#   - Skips Gems without source directories
#   - Auto-detects Include/ directories for namespace resolution
#   - Auto-detects Code/Tools/, Code/Editor/, Code/StaticLib/ directories
#   - EMotionFX/PhysX excluded (handled separately with multi-directory scanning)
#   - Additional exclusions via LY_I18N_EXCLUDE_GEMS CMake variable
#
function(generate_all_gems_translations)
    if(NOT LY_I18N_BUILD)
        return()
    endif()

    message(STATUS "  Auto-discovering Gems from all sources (engine, external, project)...")

    # ---- Step 1: Collect gem.json files from all three sources ----
    set(_all_gem_json_files "")

    # Source 1: Engine built-in Gems
    file(GLOB_RECURSE _engine_gem_jsons "${LY_ROOT_FOLDER}/Gems/*/gem.json")
    list(APPEND _all_gem_json_files ${_engine_gem_jsons})
    list(LENGTH _engine_gem_jsons _engine_gem_count)
    message(STATUS "  [SOURCE] Engine Gems: ${_engine_gem_count} gem.json files found")

    # Source 2: External Gems from o3de_manifest.json (external_subdirectories)
    o3de_get_manifest_path(_manifest_path)
    if(EXISTS "${_manifest_path}")
        get_filename_component(_manifest_dir "${_manifest_path}" DIRECTORY)
        set(_external_gem_jsons_before ${_all_gem_json_files})
        _i18n_collect_gem_jsons_from_json_subdirs(_all_gem_json_files "${_manifest_path}" "${_manifest_dir}")
        list(LENGTH _all_gem_json_files _after_external)
        list(LENGTH _external_gem_jsons_before _before_external)
        math(EXPR _external_count "${_after_external} - ${_before_external}")
        message(STATUS "  [SOURCE] External Gems (o3de_manifest.json): ${_external_count} gem.json files found")

        # Source 2b: Gems from default_gems_folder (safety net for gems not in external_subdirectories)
        file(READ "${_manifest_path}" _manifest_json_content)
        string(JSON _default_gems_folder ERROR_VARIABLE _dgf_err GET "${_manifest_json_content}" "default_gems_folder")
        if(NOT _dgf_err AND IS_DIRECTORY "${_default_gems_folder}")
            set(_before_dgf ${_all_gem_json_files})
            if(EXISTS "${_default_gems_folder}/gem.json")
                list(APPEND _all_gem_json_files "${_default_gems_folder}/gem.json")
            endif()
            file(GLOB_RECURSE _dgf_gem_jsons "${_default_gems_folder}/*/gem.json")
            list(APPEND _all_gem_json_files ${_dgf_gem_jsons})
            list(LENGTH _all_gem_json_files _after_dgf)
            list(LENGTH _before_dgf _before_dgf_count)
            math(EXPR _dgf_count "${_after_dgf} - ${_before_dgf_count}")
            if(_dgf_count GREATER 0)
                message(STATUS "  [SOURCE] default_gems_folder: ${_dgf_count} additional gem.json files found")
            endif()
        endif()
    else()
        message(STATUS "  [SOURCE] External Gems: o3de_manifest.json not found, skipping")
        set(_manifest_json_content "")
    endif()

    # Source 3: Game project Gems
    # Collect project paths from LY_PROJECTS (if available) and from o3de_manifest.json "projects" array
    set(_project_paths "")

    # Primary: LY_PROJECTS cache variable (set via -D flag or CMakePresets.json)
    if(LY_PROJECTS)
        list(APPEND _project_paths ${LY_PROJECTS})
    endif()

    # Fallback: read "projects" array from o3de_manifest.json
    # This covers project-centric builds where LY_PROJECTS may not yet be set
    # when Translation.cmake runs (Translation.cmake is included before Projects.cmake)
    if(_manifest_json_content)
        string(JSON _proj_count ERROR_VARIABLE _proj_err LENGTH "${_manifest_json_content}" "projects")
        if(NOT _proj_err AND _proj_count GREATER 0)
            math(EXPR _proj_last "${_proj_count} - 1")
            foreach(_idx RANGE ${_proj_last})
                string(JSON _proj_path ERROR_VARIABLE _item_err GET "${_manifest_json_content}" "projects" ${_idx})
                if(NOT _item_err AND IS_DIRECTORY "${_proj_path}")
                    list(APPEND _project_paths "${_proj_path}")
                endif()
            endforeach()
        endif()
    endif()

    # Deduplicate project paths
    if(_project_paths)
        list(REMOVE_DUPLICATES _project_paths)
    endif()

    set(_project_gem_count 0)
    foreach(_project_path ${_project_paths})
        # Resolve relative paths against engine root
        if(NOT IS_ABSOLUTE "${_project_path}")
            set(_project_path "${LY_ROOT_FOLDER}/${_project_path}")
        endif()

        if(NOT IS_DIRECTORY "${_project_path}")
            continue()
        endif()

        # Read external_subdirectories from project.json
        set(_before_project ${_all_gem_json_files})
        if(EXISTS "${_project_path}/project.json")
            _i18n_collect_gem_jsons_from_json_subdirs(_all_gem_json_files "${_project_path}/project.json" "${_project_path}")
        endif()

        # Also scan {project}/Gems/ directory for project-local gems
        if(IS_DIRECTORY "${_project_path}/Gems")
            file(GLOB_RECURSE _proj_local_gems "${_project_path}/Gems/*/gem.json")
            list(APPEND _all_gem_json_files ${_proj_local_gems})
        endif()

        list(LENGTH _all_gem_json_files _after_project)
        list(LENGTH _before_project _bp)
        math(EXPR _this_project_count "${_after_project} - ${_bp}")
        math(EXPR _project_gem_count "${_project_gem_count} + ${_this_project_count}")
    endforeach()
    list(LENGTH _project_paths _project_count)
    message(STATUS "  [SOURCE] Project Gems (${_project_count} projects): ${_project_gem_count} gem.json files found")

    # Deduplicate and sort for deterministic processing order
    list(REMOVE_DUPLICATES _all_gem_json_files)
    list(SORT _all_gem_json_files)
    list(LENGTH _all_gem_json_files _total_gem_count)
    message(STATUS "  [TOTAL]  ${_total_gem_count} unique gem.json files to process")

    # ---- Step 2: Build exclusion list ----
    set(_EXCLUDED_GEM_NAMES
        "EMotionFX"
        "PhysX5"
        "PhysX4"
        "PhysXCommon"
    )
    if(LY_I18N_EXCLUDE_GEMS)
        list(APPEND _EXCLUDED_GEM_NAMES ${LY_I18N_EXCLUDE_GEMS})
    endif()

    set(_processed_count 0)
    set(_skipped_count 0)

    # ---- Step 3: Process each discovered Gem ----
    foreach(_gem_json ${_all_gem_json_files})
        get_filename_component(_gem_root "${_gem_json}" DIRECTORY)

        # Read gem.json and extract gem_name
        file(READ "${_gem_json}" _gem_json_content)
        string(JSON _gem_name ERROR_VARIABLE _json_error GET "${_gem_json_content}" "gem_name")

        if(_json_error)
            message(STATUS "  [SKIP] Cannot parse gem_name from: ${_gem_json}")
            math(EXPR _skipped_count "${_skipped_count} + 1")
            continue()
        endif()

        # Check exclusion list
        list(FIND _EXCLUDED_GEM_NAMES "${_gem_name}" _excluded_index)
        if(NOT _excluded_index EQUAL -1)
            message(STATUS "  [SKIP] ${_gem_name} - excluded (handled separately)")
            math(EXPR _skipped_count "${_skipped_count} + 1")
            continue()
        endif()

        # Skip Asset-only Gems (no translatable source code)
        string(JSON _gem_type ERROR_VARIABLE _type_error GET "${_gem_json_content}" "type")
        if(NOT _type_error AND _gem_type STREQUAL "Asset")
            message(STATUS "  [SKIP] ${_gem_name} - Asset-only gem")
            math(EXPR _skipped_count "${_skipped_count} + 1")
            continue()
        endif()

        # ---- Step 4: Collect source directories ----
        set(_gem_src_dirs "")

        if(IS_DIRECTORY "${_gem_root}/Code/Source")
            list(APPEND _gem_src_dirs "${_gem_root}/Code/Source")
        endif()
        if(IS_DIRECTORY "${_gem_root}/Code/Tools")
            list(APPEND _gem_src_dirs "${_gem_root}/Code/Tools")
        endif()
        if(IS_DIRECTORY "${_gem_root}/Code/Editor")
            list(APPEND _gem_src_dirs "${_gem_root}/Code/Editor")
        endif()
        if(IS_DIRECTORY "${_gem_root}/Code/StaticLib")
            list(APPEND _gem_src_dirs "${_gem_root}/Code/StaticLib")
        endif()
        if(NOT _gem_src_dirs AND IS_DIRECTORY "${_gem_root}/Code")
            list(APPEND _gem_src_dirs "${_gem_root}/Code")
        endif()

        if(NOT _gem_src_dirs)
            message(STATUS "  [SKIP] ${_gem_name} - no source directory found")
            math(EXPR _skipped_count "${_skipped_count} + 1")
            continue()
        endif()

        # ---- Step 5: Auto-detect Include directories for namespace resolution ----
        set(_gem_include_dirs "")
        if(IS_DIRECTORY "${_gem_root}/Code/Include")
            list(APPEND _gem_include_dirs "${_gem_root}/Code/Include")
        endif()
        if(IS_DIRECTORY "${_gem_root}/Code/StaticLib")
            list(APPEND _gem_include_dirs "${_gem_root}/Code/StaticLib")
        endif()
        if(IS_DIRECTORY "${_gem_root}/Code/Source")
            list(APPEND _gem_include_dirs "${_gem_root}/Code/Source")
        endif()

        # ---- Step 6: Call translation function with gem-local output ----
        # Output to {gem_root}/Editor/Translations/ instead of centralized location
        set(_output_root "${_gem_root}/Editor/Translations")

        list(LENGTH _gem_src_dirs _src_count)

        if(_src_count GREATER 1)
            if(_gem_include_dirs)
                message(STATUS "  [INCL] ${_gem_name}: Multi-dir scan with Include path(s)")
                add_gem_translation_multi_dirs("${_gem_name}" "${_gem_src_dirs}"
                    OUTPUT_ROOT "${_output_root}"
                    INCLUDE_DIRS ${_gem_include_dirs})
            else()
                add_gem_translation_multi_dirs("${_gem_name}" "${_gem_src_dirs}"
                    OUTPUT_ROOT "${_output_root}")
            endif()
        else()
            list(GET _gem_src_dirs 0 _single_src_dir)
            if(_gem_include_dirs)
                message(STATUS "  [INCL] ${_gem_name}: Adding Include dir(s) for namespace resolution")
                add_gem_translation("${_gem_name}" "${_single_src_dir}"
                    OUTPUT_ROOT "${_output_root}"
                    INCLUDE_DIRS ${_gem_include_dirs})
            else()
                add_gem_translation("${_gem_name}" "${_single_src_dir}"
                    OUTPUT_ROOT "${_output_root}")
            endif()
        endif()

        math(EXPR _processed_count "${_processed_count} + 1")
    endforeach()

    message(STATUS "")
    message(STATUS "  [SUMMARY] All sources: processed=${_processed_count}, skipped=${_skipped_count}")

endfunction()

# ============================================================================
# JSON Property Translation Generation
# ============================================================================

#! add_json_property_translations
#
# Extracts displayName and description strings from JSON property definition
# files (such as Material property groups) and generates Qt translation entries.
#
# Many O3DE subsystems store user-visible strings in JSON data files rather
# than C++ source code. Since lupdate can only scan C++ / .ui files, these
# strings would be invisible to the translation pipeline. This function
# bridges that gap by:
#   1. Scanning all .json files in the given directory
#   2. Extracting every "displayName" and "description" value via regex
#   3. Generating a lightweight C++ file containing QT_TRANSLATE_NOOP() calls
#   4. Running lupdate on the generated file to merge entries into .ts files
#   5. Compiling the .ts files to .qm binaries
#
# At runtime, InspectorWidget::AddGroup() and PropertyRowWidget translate
# these strings through TranslatePropertyString(), which searches all
# registered Qt translation contexts. The context_name used here must be
# registered in PropertyEditorApi.cpp's GetRegisteredContexts().
#
# \arg:json_dir      - Directory containing JSON property definition files
# \arg:context_name  - Qt translation context (e.g., "MaterialInputs")
# \arg:module_name   - Output module name for .ts/.qm files (e.g., "MaterialInputs")
#
function(add_json_property_translations json_dir context_name module_name)
    if(NOT LY_I18N_BUILD)
        return()
    endif()

    if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
        return()
    endif()

    # Parse optional arguments
    # OUTPUT_ROOT: when set, .ts/.qm files go to {OUTPUT_ROOT}/{language}/ (gem-local mode)
    cmake_parse_arguments(ARG "" "OUTPUT_ROOT" "" ${ARGN})

    # ---- Step 1: Find all JSON files ----
    file(GLOB _json_files "${json_dir}/*.json")

    if(NOT _json_files)
        message(STATUS "  [JSON] No JSON files found in: ${json_dir}")
        return()
    endif()

    list(SORT _json_files)
    list(LENGTH _json_files _file_count)
    message(STATUS "  [JSON] Scanning ${_file_count} JSON files in: ${json_dir}")

    # ---- Step 2: Extract displayName and description strings via regex ----
    set(_all_strings "")

    foreach(_json_file ${_json_files})
        file(READ "${_json_file}" _content)
        get_filename_component(_filename "${_json_file}" NAME)

        # Extract "displayName": "..." values
        string(REGEX MATCHALL "\"displayName\"[ \t]*:[ \t]*\"[^\"]*\"" _display_matches "${_content}")
        foreach(_match ${_display_matches})
            string(REGEX REPLACE "\"displayName\"[ \t]*:[ \t]*\"([^\"]*)\"" "\\1" _value "${_match}")
            if(_value AND NOT _value STREQUAL "")
                list(APPEND _all_strings "${_value}")
            endif()
        endforeach()

        # Extract "description": "..." values
        string(REGEX MATCHALL "\"description\"[ \t]*:[ \t]*\"[^\"]*\"" _desc_matches "${_content}")
        foreach(_match ${_desc_matches})
            string(REGEX REPLACE "\"description\"[ \t]*:[ \t]*\"([^\"]*)\"" "\\1" _value "${_match}")
            if(_value AND NOT _value STREQUAL "")
                list(APPEND _all_strings "${_value}")
            endif()
        endforeach()
    endforeach()

    # Remove duplicates (many properties share names like "Color", "Texture", "UV")
    list(REMOVE_DUPLICATES _all_strings)

    if(NOT _all_strings)
        message(STATUS "  [JSON] No translatable strings found in: ${json_dir}")
        return()
    endif()

    list(LENGTH _all_strings _string_count)
    message(STATUS "  [JSON] Extracted ${_string_count} unique translatable strings")

    # ---- Step 3: Generate C++ file with QT_TRANSLATE_NOOP entries ----
    # This file is NEVER compiled; it exists solely for lupdate to extract
    # translatable strings that originate from JSON data files.
    set(_gen_dir "${CMAKE_BINARY_DIR}/i18n/generated")
    file(MAKE_DIRECTORY "${_gen_dir}")

    set(_gen_file "${_gen_dir}/${module_name}_TranslationStrings.cpp")

    set(_file_content "")
    string(APPEND _file_content "// Auto-generated by Translation.cmake from JSON property files.\n")
    string(APPEND _file_content "// Source directory: ${json_dir}\n")
    string(APPEND _file_content "// DO NOT EDIT - This file is regenerated during CMake configuration.\n")
    string(APPEND _file_content "//\n")
    string(APPEND _file_content "// This file is NOT compiled into any binary. It exists only so that\n")
    string(APPEND _file_content "// Qt lupdate can extract translatable strings from JSON data files.\n")
    string(APPEND _file_content "\n")
    string(APPEND _file_content "#include <QCoreApplication>\n")
    string(APPEND _file_content "\n")
    string(APPEND _file_content "// clang-format off\n")
    string(APPEND _file_content "static const char* ${module_name}_translationStrings[] = {\n")

    foreach(_str ${_all_strings})
        # Escape backslashes and double quotes for C++ string literals
        string(REPLACE "\\" "\\\\" _str_escaped "${_str}")
        string(REPLACE "\"" "\\\"" _str_escaped "${_str_escaped}")
        string(APPEND _file_content "    QT_TRANSLATE_NOOP(\"${context_name}\", \"${_str_escaped}\"),\n")
    endforeach()

    string(APPEND _file_content "};\n")
    string(APPEND _file_content "// clang-format on\n")

    file(WRITE "${_gen_file}" "${_file_content}")
    message(STATUS "  [JSON] Generated C++ extraction file: ${_gen_file}")

    # ---- Step 4: Run lupdate on the generated file ----
    if(NOT _LY_I18N_QT_TOOLS_FOUND)
        message(WARNING "  [JSON] lupdate not found. .ts files will not include JSON property strings.")
        return()
    endif()

    # Determine target languages
    if(LY_I18N_BUILD_ALL_LANGUAGES)
        set(LANGUAGES ${LY_I18N_SUPPORTED_LANGUAGES})
    else()
        set(LANGUAGES ${LY_I18N_LANGUAGE})
    endif()

    foreach(_language ${LANGUAGES})
        # Skip English variants (source language)
        if(_language MATCHES "^en(_|$)")
            continue()
        endif()

        # Create language-specific directory
        if(ARG_OUTPUT_ROOT)
            set(TS_DIR "${ARG_OUTPUT_ROOT}/${_language}")
        else()
            set(TS_DIR "${LY_ROOT_FOLDER}/Assets/Editor/Translations/${_language}")
        endif()
        file(MAKE_DIRECTORY "${TS_DIR}")

        if(NOT IS_DIRECTORY "${TS_DIR}")
            message(WARNING "  [JSON] Could not create directory: ${TS_DIR}")
            continue()
        endif()

        # Set output .ts file path (separate file for JSON-sourced strings)
        set(TS_FILE "${TS_DIR}/${module_name}_${_language}.ts")

        # Run lupdate on the generated C++ file

        # Run lupdate on the generated C++ file
        message(STATUS "  [JSON] ${_language} -> ${TS_FILE}")
        execute_process(
            COMMAND "${LUPDATE_EXECUTABLE}" "${_gen_file}" -ts "${TS_FILE}" -silent
            RESULT_VARIABLE _lupdate_result
            OUTPUT_VARIABLE _lupdate_output
            ERROR_VARIABLE _lupdate_error
        )

        if(NOT _lupdate_result EQUAL 0)
            message(WARNING "  [JSON] lupdate failed for ${_language}: ${_lupdate_error}")
            continue()
        endif()

        if(NOT EXISTS "${TS_FILE}")
            message(WARNING "  [JSON] lupdate returned success but did not create: ${TS_FILE}")
            continue()
        endif()

        # Normalize line endings on Windows
        if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
            file(READ "${TS_FILE}" ts_file_content)
            file(CONFIGURE OUTPUT "${TS_FILE}" CONTENT "${ts_file_content}" NEWLINE_STYLE CRLF)
        endif()

        # Compile .ts to .qm
        compile_ts_to_qm("${TS_FILE}" "${_language}")
    endforeach()

    message(STATUS "  [JSON] Finished processing ${module_name} (${_string_count} strings from ${_file_count} files)")
endfunction()

# ============================================================================
# EditContext Reflection String Translation Generation
# ============================================================================

#! add_editcontext_translations
#
# Extracts user-visible strings from O3DE EditContext reflection calls
# in C++ source files and generates Qt translation entries.
#
# O3DE's serialization system uses EditContext to define property editor
# labels and descriptions as plain C string literals:
#   ->DataElement(handler, &member, "Display Name", "Description")
#   ->Class<Type>("Display Name", "Description")
#
# Since these strings are not wrapped in Qt translation macros (tr(),
# QT_TRANSLATE_NOOP, etc.), lupdate cannot detect them. This function
# bridges that gap by:
#   1. Scanning C++ files for ->DataElement() and ->Class<>() patterns
#   2. Extracting display name and description strings via regex
#   3. Generating a lightweight C++ file with QT_TRANSLATE_NOOP() calls
#   4. Running lupdate on the generated file to merge entries into .ts files
#   5. Compiling .ts to .qm
#
# At runtime, PropertyRowWidget::SetNameLabel() translates these strings
# through TranslatePropertyString(), which searches all registered Qt
# translation contexts. The context_name used here must be registered
# in PropertyEditorApi.cpp's GetRegisteredContexts().
#
# \arg:source_dirs   - List of source directories to scan (semicolon-separated)
# \arg:context_name  - Qt translation context (e.g., "EMotionFX")
# \arg:module_name   - Output module name for .ts/.qm files (e.g., "EMotionFX_Reflect")
#
function(add_editcontext_translations source_dirs context_name module_name)
    if(NOT LY_I18N_BUILD)
        return()
    endif()

    if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
        return()
    endif()

    # Parse optional arguments
    # OUTPUT_ROOT: when set, .ts/.qm files go to {OUTPUT_ROOT}/{language}/ (gem-local mode)
    cmake_parse_arguments(ARG "" "OUTPUT_ROOT" "" ${ARGN})

    # ---- Step 1: Find all C++ files in source directories ----
    set(_all_cpp_files "")
    foreach(_dir ${source_dirs})
        if(IS_DIRECTORY "${_dir}")
            file(GLOB_RECURSE _dir_cpp_files "${_dir}/*.cpp")
            list(APPEND _all_cpp_files ${_dir_cpp_files})
        endif()
    endforeach()

    if(NOT _all_cpp_files)
        message(STATUS "  [EditContext] No .cpp files found in: ${source_dirs}")
        return()
    endif()

    list(SORT _all_cpp_files)
    list(LENGTH _all_cpp_files _file_count)
    message(STATUS "  [EditContext] Scanning ${_file_count} .cpp files for EditContext patterns")

    # ---- Step 2: Extract strings from EditContext reflection patterns ----
    #
    # Regex strategy:
    #   CMake's regex uses POSIX-like syntax where [^"] matches any character
    #   except a double-quote (including newlines). This naturally handles
    #   multi-line DataElement/Class calls.
    #
    #   For ->DataElement(handler, &member, "Name", "Desc"):
    #     - The first two arguments (handler, member pointer) never contain quotes
    #     - [^"]* after \( skips to the first quoted string (display name)
    #     - [^"]* between the two quoted strings matches the comma + whitespace
    #
    #   For ->Class<Type>("Name", "Desc"):
    #     - [^>]* matches the type name inside angle brackets
    #     - After >( we match two quoted strings separated by comma + whitespace
    #
    # Known limitation: if a matched string contains CMake list separators (;),
    # it may be split incorrectly during list iteration. EditContext display names
    # and descriptions in practice do not contain semicolons.
    #
    set(_all_strings "")
    set(_files_with_matches 0)

    foreach(_cpp_file ${_all_cpp_files})
        file(READ "${_cpp_file}" _content)

        # Fast path: skip files that don't contain EditContext patterns
        string(FIND "${_content}" "->DataElement(" _de_pos)
        string(FIND "${_content}" "->Class<" _cl_pos)
        if(_de_pos EQUAL -1 AND _cl_pos EQUAL -1)
            continue()
        endif()

        get_filename_component(_filename "${_cpp_file}" NAME)
        set(_file_had_matches FALSE)

        # Pattern 1: ->DataElement(handler, &member, "DisplayName", "Description")
        if(NOT _de_pos EQUAL -1)
            string(REGEX MATCHALL
                "->DataElement\\([^\"]*\"([^\"]*)\"[^\"]*\"([^\"]*)\""
                _de_matches "${_content}")
            foreach(_match ${_de_matches})
                string(REGEX REPLACE
                    "->DataElement\\([^\"]*\"([^\"]*)\"[^\"]*\"([^\"]*)\""
                    "\\1" _name "${_match}")
                string(REGEX REPLACE
                    "->DataElement\\([^\"]*\"([^\"]*)\"[^\"]*\"([^\"]*)\""
                    "\\2" _desc "${_match}")
                if(_name AND NOT _name STREQUAL "")
                    list(APPEND _all_strings "${_name}")
                    set(_file_had_matches TRUE)
                endif()
                if(_desc AND NOT _desc STREQUAL "")
                    list(APPEND _all_strings "${_desc}")
                endif()
            endforeach()
        endif()

        # Pattern 2: editContext->Class<Type>("DisplayName", "Description")
        if(NOT _cl_pos EQUAL -1)
            string(REGEX MATCHALL
                "->Class<[^>]*>\\([^\"]*\"([^\"]*)\"[^\"]*\"([^\"]*)\""
                _cl_matches "${_content}")
            foreach(_match ${_cl_matches})
                string(REGEX REPLACE
                    "->Class<[^>]*>\\([^\"]*\"([^\"]*)\"[^\"]*\"([^\"]*)\""
                    "\\1" _name "${_match}")
                string(REGEX REPLACE
                    "->Class<[^>]*>\\([^\"]*\"([^\"]*)\"[^\"]*\"([^\"]*)\""
                    "\\2" _desc "${_match}")
                if(_name AND NOT _name STREQUAL "")
                    list(APPEND _all_strings "${_name}")
                    set(_file_had_matches TRUE)
                endif()
                if(_desc AND NOT _desc STREQUAL "")
                    list(APPEND _all_strings "${_desc}")
                endif()
            endforeach()
        endif()

        if(_file_had_matches)
            math(EXPR _files_with_matches "${_files_with_matches} + 1")
        endif()
    endforeach()

    # Remove duplicates (many properties share labels like "Configuration", etc.)
    list(REMOVE_DUPLICATES _all_strings)

    if(NOT _all_strings)
        message(STATUS "  [EditContext] No translatable strings found in ${_file_count} files")
        return()
    endif()

    list(LENGTH _all_strings _string_count)
    message(STATUS "  [EditContext] Extracted ${_string_count} unique translatable strings from ${_files_with_matches} files")

    # ---- Step 3: Generate C++ file with QT_TRANSLATE_NOOP entries ----
    # This file is NEVER compiled; it exists solely for lupdate to extract
    # translatable strings that originate from EditContext reflection calls.
    set(_gen_dir "${CMAKE_BINARY_DIR}/i18n/generated")
    file(MAKE_DIRECTORY "${_gen_dir}")

    set(_gen_file "${_gen_dir}/${module_name}_EditContextStrings.cpp")

    set(_file_content "")
    string(APPEND _file_content "// Auto-generated by Translation.cmake from EditContext reflection patterns.\n")
    string(APPEND _file_content "// Source directories:\n")
    foreach(_dir ${source_dirs})
        string(APPEND _file_content "//   ${_dir}\n")
    endforeach()
    string(APPEND _file_content "// DO NOT EDIT - This file is regenerated during CMake configuration.\n")
    string(APPEND _file_content "//\n")
    string(APPEND _file_content "// This file is NOT compiled into any binary. It exists only so that\n")
    string(APPEND _file_content "// Qt lupdate can extract translatable strings from EditContext.\n")
    string(APPEND _file_content "\n")
    string(APPEND _file_content "#include <QCoreApplication>\n")
    string(APPEND _file_content "\n")
    string(APPEND _file_content "// clang-format off\n")
    string(APPEND _file_content "static const char* ${module_name}_editContextStrings[] = {\n")

    foreach(_str ${_all_strings})
        # Escape backslashes and double quotes for C++ string literals
        string(REPLACE "\\" "\\\\" _str_escaped "${_str}")
        string(REPLACE "\"" "\\\"" _str_escaped "${_str_escaped}")
        string(APPEND _file_content "    QT_TRANSLATE_NOOP(\"${context_name}\", \"${_str_escaped}\"),\n")
    endforeach()

    string(APPEND _file_content "};\n")
    string(APPEND _file_content "// clang-format on\n")

    file(WRITE "${_gen_file}" "${_file_content}")
    message(STATUS "  [EditContext] Generated C++ extraction file: ${_gen_file}")

    # ---- Step 4: Run lupdate on the generated file ----
    if(NOT _LY_I18N_QT_TOOLS_FOUND)
        message(WARNING "  [EditContext] lupdate not found. .ts files will not include EditContext strings.")
        return()
    endif()

    # Determine target languages
    if(LY_I18N_BUILD_ALL_LANGUAGES)
        set(LANGUAGES ${LY_I18N_SUPPORTED_LANGUAGES})
    else()
        set(LANGUAGES ${LY_I18N_LANGUAGE})
    endif()

    foreach(_language ${LANGUAGES})
        # Skip English variants (source language)
        if(_language MATCHES "^en(_|$)")
            continue()
        endif()

        # Create language-specific directory
        if(ARG_OUTPUT_ROOT)
            set(TS_DIR "${ARG_OUTPUT_ROOT}/${_language}")
        else()
            set(TS_DIR "${LY_ROOT_FOLDER}/Assets/Editor/Translations/${_language}")
        endif()
        file(MAKE_DIRECTORY "${TS_DIR}")

        if(NOT IS_DIRECTORY "${TS_DIR}")
            message(WARNING "  [EditContext] Could not create directory: ${TS_DIR}")
            continue()
        endif()

        # Set output .ts file path (separate from the main tr()-based .ts file)
        set(TS_FILE "${TS_DIR}/${module_name}_${_language}.ts")

        # Run lupdate on the generated C++ file
        # lupdate MERGES new entries into existing .ts files, preserving translations.
        message(STATUS "  [EditContext] ${_language} -> ${TS_FILE}")
        execute_process(
            COMMAND "${LUPDATE_EXECUTABLE}" "${_gen_file}" -ts "${TS_FILE}" -silent
            RESULT_VARIABLE _lupdate_result
            OUTPUT_VARIABLE _lupdate_output
            ERROR_VARIABLE _lupdate_error
        )

        if(NOT _lupdate_result EQUAL 0)
            message(WARNING "  [EditContext] lupdate failed for ${_language}: ${_lupdate_error}")
            continue()
        endif()

        if(NOT EXISTS "${TS_FILE}")
            message(WARNING "  [EditContext] lupdate returned success but did not create: ${TS_FILE}")
            continue()
        endif()

        # Normalize line endings on Windows
        if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
            file(READ "${TS_FILE}" ts_file_content)
            file(CONFIGURE OUTPUT "${TS_FILE}" CONTENT "${ts_file_content}" NEWLINE_STYLE CRLF)
        endif()

        # Compile .ts to .qm
        compile_ts_to_qm("${TS_FILE}" "${_language}")
    endforeach()

    message(STATUS "  [EditContext] Finished processing ${module_name} (${_string_count} strings from ${_files_with_matches} files)")
endfunction()

# ============================================================================
# Automatic Translation Generation
# ============================================================================
# Auto-generate translation files when LY_I18N_BUILD is enabled
# This runs during CMake configuration phase

if(LY_I18N_BUILD AND PAL_TRAIT_BUILD_HOST_TOOLS)
    message(STATUS "")
    message(STATUS "========================================================")
    message(STATUS "Starting Automatic Translation File Generation")
    message(STATUS "========================================================")
    message(STATUS "")
    
    # 1. Generate Editor module translations
    if(EXISTS "${LY_ROOT_FOLDER}/Code/Editor")
        message(STATUS ">>> Generating Editor Module Translations <<<")
        add_translation_module("${LY_ROOT_FOLDER}/Code/Editor" "Editor")
    endif()
    
    # 2. Generate AzCore translations (core framework, contains EditContext/NativeUI/Dialog user-visible strings)
    if(EXISTS "${LY_ROOT_FOLDER}/Code/Framework/AzCore/AzCore")
        message(STATUS ">>> Generating AzCore Translations <<<")
        add_translation_module("${LY_ROOT_FOLDER}/Code/Framework/AzCore/AzCore" "AzCore")
    endif()
    
    # 2.1 Generate AzFramework translations (Asset files with QT_TRANSLATE_NOOP)
    # AzFramework's Asset files (BenchmarkSettingsAsset, FileTagAsset, XmlSchemaAsset, etc.)
    # now use QT_TRANSLATE_NOOP("AzFramework", ...) for EditContext strings,
    # so lupdate can detect and extract them automatically.
    # Note: Physics files in AzFramework use "PhysX" context and are handled separately
    # in the PhysX multi-directory section (section 6.2).
    if(EXISTS "${LY_ROOT_FOLDER}/Code/Framework/AzFramework/AzFramework")
        message(STATUS ">>> Generating AzFramework Translations <<<")
        add_translation_module("${LY_ROOT_FOLDER}/Code/Framework/AzFramework/AzFramework" "AzFramework")
    endif()
    
    # 3. Generate AzToolsFramework translations (core tools framework, contains many UI components)
    if(EXISTS "${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework")
        message(STATUS ">>> Generating AzToolsFramework Translations <<<")
        add_translation_module("${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework/AzToolsFramework" "AzToolsFramework")
    endif()
    
    # 3.1 Generate AzQtComponents translations (Qt component library)
    if(EXISTS "${LY_ROOT_FOLDER}/Code/Framework/AzQtComponents")
        message(STATUS ">>> Generating AzQtComponents Translations <<<")
        add_translation_module("${LY_ROOT_FOLDER}/Code/Framework/AzQtComponents" "AzQtComponents")
    endif()
    
    # 4. Generate all standalone tools translations
    message(STATUS ">>> Generating Standalone Tools Translations <<<")
    generate_all_tools_translations()
    
    # 5. Auto-discover and generate all Gems translations
    # Uses gem.json files to automatically find ALL Gems with source code.
    # No hardcoded list needed - new Gems are automatically included.
    message(STATUS ">>> Auto-Discovering & Generating Gems Translations <<<")
    generate_all_gems_translations()
    
    # 6. Special handling for EMotionFX - scan all UI source directories
    # EMotionFX has UI code spread across multiple locations:
    # - Code/Source (Editor components, PropertyWidgets)
    # - Code/EMotionFX/Tools/EMotionStudio (EMStudio SDK and Plugins)
    # - Code/MysticQt (Qt utility classes)
    #
    # IMPORTANT: EMotionFX uses the EMStudio:: namespace for its UI classes.
    # lupdate needs proper include paths (-I) to resolve header files and detect
    # the namespace, otherwise it extracts context as "ClassName" instead of
    # "EMStudio::ClassName", causing translation lookups to fail at runtime.
    message(STATUS ">>> Generating EMotionFX Translations (Multi-Directory) <<<")
    set(_emfx_source_dirs "")
    
    # Collect all EMotionFX UI source directories
    if(EXISTS "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code/Source")
        list(APPEND _emfx_source_dirs "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code/Source")
    endif()
    if(EXISTS "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code/EMotionFX/Tools/EMotionStudio")
        list(APPEND _emfx_source_dirs "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code/EMotionFX/Tools/EMotionStudio")
    endif()
    if(EXISTS "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code/MysticQt")
        list(APPEND _emfx_source_dirs "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code/MysticQt")
    endif()
    
    # Collect include paths for proper namespace detection by lupdate
    # These paths allow lupdate to resolve headers like:
    #   #include <EMotionStudio/...>  -> resolves from Code/EMotionFX/Tools/
    #   #include <MysticQt/...>       -> resolves from Code/
    #   #include <EMotionFX/...>      -> resolves from Code/
    #   #include <Editor/...>         -> resolves from Code/Source/
    # Without these, lupdate cannot detect the EMStudio:: namespace scope,
    # causing translation context mismatch at runtime.
    set(_emfx_include_dirs "")
    if(EXISTS "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code/EMotionFX/Tools")
        list(APPEND _emfx_include_dirs "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code/EMotionFX/Tools")
    endif()
    if(EXISTS "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code")
        list(APPEND _emfx_include_dirs "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code")
    endif()
    if(EXISTS "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code/Source")
        list(APPEND _emfx_include_dirs "${LY_ROOT_FOLDER}/Gems/EMotionFX/Code/Source")
    endif()
    
    # Generate translations using multi-directory function with include paths
    # Output to EMotionFX gem's own Editor/Translations/ directory
    set(_emfx_output_root "${LY_ROOT_FOLDER}/Gems/EMotionFX/Editor/Translations")
    if(_emfx_source_dirs)
        add_gem_translation_multi_dirs("EMotionFX" "${_emfx_source_dirs}"
            OUTPUT_ROOT "${_emfx_output_root}"
            INCLUDE_DIRS ${_emfx_include_dirs})
    endif()
    
    # 6.1 Extract EMotionFX EditContext reflection strings
    # GUIOptions.cpp and RenderOptions.cpp define preferences dialog labels
    # via EditContext (->DataElement, ->Class<>) as plain C strings.
    # lupdate cannot detect these, so we extract them with regex and
    # generate a separate .ts/.qm file for translators to work with.
    message(STATUS ">>> Extracting EMotionFX EditContext Strings <<<")
    if(_emfx_source_dirs)
        add_editcontext_translations("${_emfx_source_dirs}" "EMotionFX" "EMotionFX_Reflect"
            OUTPUT_ROOT "${_emfx_output_root}")
    endif()
    
    # 6.2 Special handling for PhysX - scan shared Code directory and AzFramework physics config
    # PhysX has a non-standard directory structure:
    # - Gems/PhysX/Core/Code/ is SHARED between PhysX4 and PhysX5
    # - gem.json files are at Gems/PhysX/Core/PhysX4/ and PhysX5/
    #   (which don't contain the actual Code/Source or Code/Editor directories)
    # - Auto-discovery fails because it looks for Code/Source relative to gem.json
    #
    # Additionally, some PhysX-context strings are in AzFramework:
    # - Code/Framework/AzFramework/AzFramework/Physics/Configuration/SystemConfiguration.cpp
    #   (Max Time Step, Fixed Time Step, Raycast Buffer Size, Shapecast Buffer Size, etc.)
    # - Code/Framework/AzFramework/AzFramework/Physics/Configuration/SceneConfiguration.cpp
    #   (Scene Configuration, Gravity, Continuous Collision Detection, Enable CCD, etc.)
    # These files use QT_TRANSLATE_NOOP("PhysX", ...) context.
    #
    # Source directories to scan:
    # - Core/Code/Source/ (PhysXConfiguration, PhysXDebugConfiguration, EditorRigidBodyComponent)
    # - Core/Code/Editor/ (ConfigurationWidget, CollisionFilteringWidget, PvdWidget, SettingsWidget)
    # - AzFramework/Physics/Configuration/ (SystemConfiguration, SceneConfiguration)
    message(STATUS ">>> Generating PhysX Translations (Multi-Directory) <<<")
    set(_physx_source_dirs "")

    # Core PhysX source files (EditContext strings in Configuration/ and Debug/)
    if(EXISTS "${LY_ROOT_FOLDER}/Gems/PhysX/Core/Code/Source")
        list(APPEND _physx_source_dirs "${LY_ROOT_FOLDER}/Gems/PhysX/Core/Code/Source")
    endif()

    # PhysX Editor widgets (Qt tr() calls in ConfigurationWidget, CollisionFilteringWidget, etc.)
    if(EXISTS "${LY_ROOT_FOLDER}/Gems/PhysX/Core/Code/Editor")
        list(APPEND _physx_source_dirs "${LY_ROOT_FOLDER}/Gems/PhysX/Core/Code/Editor")
    endif()

    # AzFramework physics configuration files that use "PhysX" translation context
    # (SystemConfiguration.cpp, SceneConfiguration.cpp)
    if(EXISTS "${LY_ROOT_FOLDER}/Code/Framework/AzFramework/AzFramework/Physics/Configuration")
        list(APPEND _physx_source_dirs "${LY_ROOT_FOLDER}/Code/Framework/AzFramework/AzFramework/Physics/Configuration")
    endif()

    # AzFramework physics collision files that use "PhysX" translation context
    # (CollisionLayers.cpp - "Collision Layers", "Layers", etc.)
    if(EXISTS "${LY_ROOT_FOLDER}/Code/Framework/AzFramework/AzFramework/Physics/Collision")
        list(APPEND _physx_source_dirs "${LY_ROOT_FOLDER}/Code/Framework/AzFramework/AzFramework/Physics/Collision")
    endif()

    # Include paths for lupdate to resolve headers and detect C++ namespaces
    # These allow lupdate to find:
    #   #include <Editor/ConfigurationWidget.h>  -> resolves from Core/Code/
    #   #include <Source/NameConstants.h>         -> resolves from Core/Code/
    #   #include <PhysX/Configuration/...>        -> resolves from Core/Code/Include/
    set(_physx_include_dirs "")
    if(EXISTS "${LY_ROOT_FOLDER}/Gems/PhysX/Core/Code")
        list(APPEND _physx_include_dirs "${LY_ROOT_FOLDER}/Gems/PhysX/Core/Code")
    endif()
    if(EXISTS "${LY_ROOT_FOLDER}/Gems/PhysX/Core/Code/Include")
        list(APPEND _physx_include_dirs "${LY_ROOT_FOLDER}/Gems/PhysX/Core/Code/Include")
    endif()

    # Generate translations using multi-directory function with include paths
    # Output to PhysX gem's own Editor/Translations/ directory
    set(_physx_output_root "${LY_ROOT_FOLDER}/Gems/PhysX/Editor/Translations")
    if(_physx_source_dirs)
        add_gem_translation_multi_dirs("PhysX5" "${_physx_source_dirs}"
            OUTPUT_ROOT "${_physx_output_root}"
            INCLUDE_DIRS ${_physx_include_dirs})
    endif()

    # 7. Generate translation strings from Material JSON property files
    # Material property definitions store user-visible displayName/description
    # strings in JSON files that lupdate cannot scan. This step extracts those
    # strings and generates translation entries so InspectorWidget and
    # PropertyRowWidget can translate them at runtime via TranslatePropertyString().
    message(STATUS ">>> Generating Material JSON Property Translations <<<")
    set(_material_output_root "${LY_ROOT_FOLDER}/Gems/Atom/Feature/Common/Editor/Translations")
    if(EXISTS "${LY_ROOT_FOLDER}/Gems/Atom/Feature/Common/Assets/Materials/Types/MaterialInputs")
        add_json_property_translations(
            "${LY_ROOT_FOLDER}/Gems/Atom/Feature/Common/Assets/Materials/Types/MaterialInputs"
            "MaterialInputs"
            "MaterialInputs"
            OUTPUT_ROOT "${_material_output_root}"
        )
    endif()
    
    message(STATUS "")
    message(STATUS "========================================================")
    message(STATUS "Automatic Translation File Generation Complete!")
    message(STATUS "Translation files (.ts/.qm) locations:")
    message(STATUS "  Framework modules: ${LY_ROOT_FOLDER}/Assets/Editor/Translations/<language>/")
    message(STATUS "  Gem translations:  <gem_root>/Editor/Translations/<language>/")
    message(STATUS "========================================================")
    message(STATUS "")
endif()