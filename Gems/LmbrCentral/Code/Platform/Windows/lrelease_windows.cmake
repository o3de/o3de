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

add_custom_command(TARGET LmbrCentral.Editor POST_BUILD
    COMMAND "${CMAKE_COMMAND}" 
        -DLY_TIMESTAMP_REFERENCE=$<TARGET_FILE_DIR:LmbrCentral.Editor>/lrelease.exe
        -DLY_LOCK_FILE=$<TARGET_FILE_DIR:LmbrCentral.Editor>/qtdeploy.lock
        -P ${LY_ROOT_FOLDER}/cmake/CommandExecution.cmake
            EXEC_COMMAND "${CMAKE_COMMAND}" -E
                env PATH="${QT_PATH}/bin" 
                ${WINDEPLOYQT_EXECUTABLE}
                    $<$<CONFIG:Debug>:--pdb>
                    --verbose 0
                    --no-compiler-runtime
                    $<TARGET_FILE_DIR:LmbrCentral.Editor>/lrelease.exe
    COMMENT "Patching lrelease..."
    VERBATIM
)