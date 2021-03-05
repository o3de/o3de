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

set(QT_PATH ${BASE_PATH}/clang_64)
set(QT_LIB_PATH ${QT_PATH}/lib)

list(APPEND QT5_COMPONENTS MacExtras)


include(CMakeParseArguments)

# Clear the cache for found executable
unset(MACDEPLOYQT_EXECUTABLE CACHE)
find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${QT_PATH}/bin")
mark_as_advanced(MACDEPLOYQT_EXECUTABLE) # Hiding from GUI

function(ly_qt_deploy)

    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs)

    cmake_parse_arguments(ly_qt_deploy "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments
    if(NOT ly_qt_deploy_TARGET)
        message(FATAL_ERROR "You must provide a target to detect qt dependencies")
    endif()

    get_target_property(is_bundle ${ly_qt_deploy_TARGET} MACOSX_BUNDLE)
    if (is_bundle)
        add_custom_command(TARGET ${ly_qt_deploy_TARGET} POST_BUILD
            COMMAND "${CMAKE_COMMAND}"
                -DLY_TIMESTAMP_REFERENCE=$<TARGET_FILE:${ly_qt_deploy_TARGET}>
                -DLY_LOCK_FILE=$<TARGET_FILE_DIR:${ly_qt_deploy_TARGET}>/qtdeploy.lock
                -P ${LY_ROOT_FOLDER}/cmake/CommandExecution.cmake
                    EXEC_COMMAND "${CMAKE_COMMAND}" -E time
                        ${MACDEPLOYQT_EXECUTABLE}
                            $<TARGET_BUNDLE_DIR:${ly_qt_deploy_TARGET}>
                            -always-overwrite
                            -no-strip
                            -verbose=0
                            -fs=APFS
             DEPENDS $<TARGET_FILE:${ly_qt_deploy_TARGET}> 
             COMMENT "Deploying qt to the ${ly_qt_deploy_TARGET} bundle ..."
            VERBATIM
        )
   else()
       set(qt_conf_config "[Paths]\nPlugins=@plugin_path@")
       set(plugin_path "${QT_PATH}/plugins")
       string(CONFIGURE "${qt_conf_config}" qt_conf_output @ONLY)
       file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf" "${qt_conf_output}")

       # output the qt_conf file using "echo" and file redirection
       add_custom_command(TARGET ${ly_qt_deploy_TARGET} POST_BUILD
           COMMAND "${CMAKE_COMMAND}" 
               -DLY_TIMESTAMP_REFERENCE=$<TARGET_FILE:${ly_qt_deploy_TARGET}>
               -DLY_LOCK_FILE=$<TARGET_FILE_DIR:${ly_qt_deploy_TARGET}>/qtdeploy.lock
               -P ${LY_ROOT_FOLDER}/cmake/CommandExecution.cmake
                    EXEC_COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                        ${CMAKE_CURRENT_BINARY_DIR}/qt.conf
                        $<TARGET_FILE_DIR:${ly_qt_deploy_TARGET}>/qt.conf
           COMMENT "copying over qt.conf..."
           VERBATIM
       )
   endif()

endfunction()


