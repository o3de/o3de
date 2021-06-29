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

# Script for running a test that uses EditorPythonBindings. Takes care of:
# 1. Activating a project.
# 2. Enabling the EditorPythonBindings gem.
# 3. Invoking the Editor executable with the parameters to load the test script.
# 4. Kills the AssetProcessor process
# The following arguments are required:
# CMD_ARG_TEST_PROJECT - name of the project to enable via lmbr.
# CMD_ARG_EDITOR - full path to the Editor executable.
# CMD_ARG_PYTHON_SCRIPT - full path to the python script to be executed by the Editor.

# EditorPythonBindings need to be enabled for the project we launch

execute_process(
    COMMAND ${CMD_ARG_EDITOR} -NullRenderer --skipWelcomeScreenDialog --autotest_mode --regset="/Amazon/AzCore/Bootstrap/project_path=${CMD_ARG_TEST_PROJECT}" --runpython ${CMD_ARG_PYTHON_SCRIPT}
    TIMEOUT 1800
    RESULT_VARIABLE TEST_CMD_RESULT
)

if(${PLATFORM} STREQUAL "Windows")
    execute_process(
        COMMAND taskkill /F /IM AssetProcessor.exe
    )
else()
    execute_process(
        COMMAND killall -I AssetProcessor
    )
endif()

if(TEST_CMD_RESULT)
    message(FATAL_ERROR "Error running EditorPythonBindings Test via CMake Wrapper, result ${TEST_CMD_RESULT}")
endif()