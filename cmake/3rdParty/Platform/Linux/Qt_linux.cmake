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

set(QT_PATH ${BASE_PATH}/gcc_64) # "gcc" means "linux" in this case
set(QT_LIB_PATH ${QT_PATH}/lib)

include(CMakeParseArguments)

unset(WINDEPLOYQT_EXECUTABLE CACHE)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt
    HINTS
        "${QT_PATH}/bin"
)
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

    # When winqtdeploy is used on a unix platform it copies over the hardcoded platform abstraction plugin
    # of qxcb plugin. The qxcb plugin requires an X server to be running on linux and order to load properly
    # To avoid the issue of requiring an X server to be running in a headless setup, the qminimal
    # platform plugin is also copied over to the target file output directory.
    set(plugin_path "${QT_PATH}/plugins")
    set(platform_plugins ${plugin_path}/platforms/libqminimal.so)
    set(xcbglintegrations_plugins ${plugin_path}/xcbglintegrations/libqxcb-glx-integration.so)

    add_custom_command(TARGET ${ly_qt_deploy_TARGET} POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            -DLY_TIMESTAMP_REFERENCE=$<TARGET_FILE:${ly_qt_deploy_TARGET}>
            -DLY_LOCK_FILE=$<TARGET_FILE_DIR:${ly_qt_deploy_TARGET}>/qtdeploy.lock
            -P ${LY_ROOT_FOLDER}/cmake/CommandExecution.cmake
                EXEC_COMMAND "${CMAKE_COMMAND}" -E
                    env PATH=${CMAKE_BINARY_DIR}:${QT_PATH}/bin:$ENV{PATH}
                    "${CMAKE_COMMAND}" -P "${LY_ROOT_FOLDER}/cmake/Platform/Linux/windeployqt_wrapper.cmake"
                        "$<TARGET_FILE_DIR:${ly_qt_deploy_TARGET}>"
                        "${WINDEPLOYQT_EXECUTABLE}"
                        --verbose 2
                        --no-compiler-runtime
                        --dir "$<TARGET_FILE_DIR:${ly_qt_deploy_TARGET}>"
                        "$<TARGET_FILE:${ly_qt_deploy_TARGET}>"
                EXEC_COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${ly_qt_deploy_TARGET}>/platforms"
                EXEC_COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                    ${platform_plugins}
                    "$<TARGET_FILE_DIR:${ly_qt_deploy_TARGET}>/platforms"
                EXEC_COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${ly_qt_deploy_TARGET}>/xcbglintegrations"
                EXEC_COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                    ${xcbglintegrations_plugins}
                    "$<TARGET_FILE_DIR:${ly_qt_deploy_TARGET}>/xcbglintegrations"
        DEPENDS $<TARGET_FILE:${ly_qt_deploy_TARGET}>
        COMMENT "Deploying qt..."
        VERBATIM
    )

endfunction()

# windeployqt uses qmake -query to introspect a given Qt installation. However,
# qmake is not relocatable, so the paths reported are those from the build
# machine, and not from wherever the user has their 3rdParty libraries. So we
# create a fake qmake executable to report the right paths to windeployqt.
configure_file(${CMAKE_CURRENT_LIST_DIR}/Qt_qmake_${PAL_PLATFORM_NAME_LOWERCASE}.cmake.in ${CMAKE_BINARY_DIR}/qmake)
