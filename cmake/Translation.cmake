#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
set(LY_I18N_BUILD OFF CACHE BOOL "I18N generators")
set(LY_I18N_LANGUAGE "en_US" CACHE STRING "I18N target language")

# Not all platforms support I18N generators
if(LY_I18N_BUILD AND NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    message(ERROR "LY_I18N_BUILD is specified, but not supported for the current target platform")
    return()
endif()

# Not all platforms support I18N generators
if(LY_I18N_BUILD AND NOT LY_I18N_LANGUAGE)
    message(ERROR "LY_I18N_BUILD is specified, but lack of target language specified.")
    return()
endif()

function(add_translation_module target_source_dir module_name)
    if(NOT LY_I18N_BUILD)
        return()
    endif()

    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        find_package(Qt5 COMPONENTS LinguistTools REQUIRED)
        find_program(LUPDATE_EXECUTABLE lupdate PATHS "${Qt5_DIR}/../../../bin" REQUIRED)
        set(LANGUAGES ${LY_I18N_LANGUAGE})
        foreach(_language ${LANGUAGES})
            set(TS_DIR "${LY_ROOT_FOLDER}/Code/Editor/Translations/${_language}")
            file(MAKE_DIRECTORY ${TS_DIR})
            set(TS_FILE "${TS_DIR}/${module_name}_${_language}.ts")
            execute_process(COMMAND ${LUPDATE_EXECUTABLE} ${target_source_dir} -ts ${TS_FILE} -silent)
            if(LY_HOST_PLATFORM_DETECTION_Windows STREQUAL "Windows")
                file(READ ${TS_FILE} ts_file_content)
                file(CONFIGURE OUTPUT ${TS_FILE} CONTENT "${ts_file_content}" NEWLINE_STYLE CRLF)
            endif()
        endforeach()
    endif()
endfunction()
