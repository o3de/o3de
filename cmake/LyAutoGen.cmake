#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#! ly_add_autogen: adds a code generation step to the specified target.
#
# \arg:NAME name of the target
# \arg:OUTPUT_NAME (optional) overrides the name of the output target. If not specified, the name will be used.
# \arg:INCLUDE_DIRECTORIES list of directories to use as include paths
# \arg:AUTOGEN_RULES a set of AutoGeneration rules to be passed to the AzAutoGen expansion system
# \arg:ALLFILES list of all source files contained by the target
function(ly_add_autogen)

    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs INCLUDE_DIRECTORIES AUTOGEN_RULES ALLFILES)
    cmake_parse_arguments(ly_add_autogen "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(ly_add_autogen_AUTOGEN_RULES)
        set(AZCG_INPUTFILES ${ly_add_autogen_ALLFILES})
        list(FILTER AZCG_INPUTFILES INCLUDE REGEX ".*\.(xml|json|jinja)$")
        # Writes AzAutoGen input files into tmp ${ly_add_autogen_NAME}_input_files.txt file to avoid long command error
        set(input_files_path "${CMAKE_CURRENT_BINARY_DIR}/Azcg/Temp/${ly_add_autogen_NAME}_input_files.txt")
        file(CONFIGURE OUTPUT "${input_files_path}" CONTENT [[@AZCG_INPUTFILES@]] @ONLY)
        get_target_property(target_type ${ly_add_autogen_NAME} TYPE)
        if (target_type STREQUAL INTERFACE_LIBRARY)
            target_include_directories(${ly_add_autogen_NAME} INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Azcg/Generated/${ly_add_autogen_NAME}")
        else()
            target_include_directories(${ly_add_autogen_NAME} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Azcg/Generated/${ly_add_autogen_NAME}")
        endif()
        execute_process(
            COMMAND ${LY_PYTHON_CMD} "${LY_ROOT_FOLDER}/cmake/AzAutoGen.py" "${ly_add_autogen_NAME}" "${CMAKE_BINARY_DIR}/Azcg/TemplateCache/" "${CMAKE_CURRENT_BINARY_DIR}/Azcg/Generated/${ly_add_autogen_NAME}/" "${CMAKE_CURRENT_SOURCE_DIR}" "${input_files_path}" "${ly_add_autogen_AUTOGEN_RULES}" "-n"
            OUTPUT_VARIABLE AUTOGEN_OUTPUTS
        )
        string(STRIP "${AUTOGEN_OUTPUTS}" AUTOGEN_OUTPUTS)
        set(AZCG_DEPENDENCIES ${AZCG_INPUTFILES})
        list(APPEND AZCG_DEPENDENCIES "${LY_ROOT_FOLDER}/cmake/AzAutoGen.py")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AZCG_DEPENDENCIES})
        add_custom_command(
            OUTPUT ${AUTOGEN_OUTPUTS}
            DEPENDS ${AZCG_DEPENDENCIES}
            COMMAND ${LY_PYTHON_CMD} "${LY_ROOT_FOLDER}/cmake/AzAutoGen.py" "--prune" "${ly_add_autogen_NAME}" "${CMAKE_BINARY_DIR}/Azcg/TemplateCache/" "${CMAKE_CURRENT_BINARY_DIR}/Azcg/Generated/${ly_add_autogen_NAME}/" "${CMAKE_CURRENT_SOURCE_DIR}" "${input_files_path}" "${ly_add_autogen_AUTOGEN_RULES}"
            COMMENT "Running AutoGen for ${ly_add_autogen_NAME}"
            VERBATIM
        )
        set_target_properties(${ly_add_autogen_NAME} PROPERTIES AUTOGEN_INPUT_FILES "${AZCG_INPUTFILES}")
        set_target_properties(${ly_add_autogen_NAME} PROPERTIES AUTOGEN_OUTPUT_FILES "${AUTOGEN_OUTPUTS}")
        target_sources(${ly_add_autogen_NAME} PRIVATE ${AUTOGEN_OUTPUTS})
    endif()

endfunction()
