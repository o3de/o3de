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

set(QT_PATH ${BASE_PATH}/msvc2019_64)
set(QT_LIB_PATH ${QT_PATH}/lib)

include(CMakeParseArguments)

# Clear the cache for found executable
unset(WINDEPLOYQT_EXECUTABLE CACHE)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_PATH}/bin")
mark_as_advanced(WINDEPLOYQT_EXECUTABLE) # Hiding from GUI

function(ly_qt_deploy)

    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs)

    cmake_parse_arguments(ly_qt_deploy "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments
    if(NOT ly_qt_deploy_TARGET)
        message(FATAL_ERROR "You must provide a target to detect qt dependencies")
    endif()

    # CMake has an issue with POST_BUILD commands in msbuild when it is executed from outside VS:
    # https://gitlab.kitware.com/cmake/cmake/issues/18530

    add_custom_command(TARGET ${ly_qt_deploy_TARGET} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" 
            -DLY_TIMESTAMP_REFERENCE=$<TARGET_FILE:${ly_qt_deploy_TARGET}>
            -DLY_LOCK_FILE=$<TARGET_FILE_DIR:${ly_qt_deploy_TARGET}>/qtdeploy.lock
            -P ${LY_ROOT_FOLDER}/cmake/CommandExecution.cmake
                EXEC_COMMAND "${CMAKE_COMMAND}" -E
                    env PATH="${QT_PATH}/bin" 
                    ${WINDEPLOYQT_EXECUTABLE}
                        $<$<CONFIG:Debug>:--pdb>
                        --verbose 0
                        --no-compiler-runtime
                        $<TARGET_FILE:${ly_qt_deploy_TARGET}>
        DEPENDS $<TARGET_FILE:${ly_qt_deploy_TARGET}> $<TARGET_FILE:AZ::QtTGAImageFormatPlugin>
        COMMENT "Deploying qt..."
        VERBATIM
    )

endfunction()