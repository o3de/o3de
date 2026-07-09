#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# ----------------------------------------------------------------------------
# Class Creation Wizard
#
# A standalone PySide6 tool for scaffolding O3DE C++ classes from templates.
# The custom target below adds a "ClassWizard" launch entry to the generated
# Visual Studio solution: set it as the Startup Project and press Ctrl+F5 to
# run the wizard GUI against this engine (and the active project, if any).
# ----------------------------------------------------------------------------

add_custom_target(ClassWizard
    SOURCES ClassWizard.py
)

# Launch the wizard GUI through the engine's bundled Python.
set(class_wizard_args "\"${CMAKE_CURRENT_SOURCE_DIR}/ClassWizard.py\" --engine-path \"${LY_ROOT_FOLDER}\"")
if(LY_DEFAULT_PROJECT_PATH)
    string(APPEND class_wizard_args " --project-path \"${LY_DEFAULT_PROJECT_PATH}\"")
endif()

set_target_properties(ClassWizard PROPERTIES
    FOLDER "Tools"
    VS_DEBUGGER_COMMAND "${LY_ROOT_FOLDER}/python/python.cmd"
    VS_DEBUGGER_COMMAND_ARGUMENTS "${class_wizard_args}"
    VS_DEBUGGER_WORKING_DIRECTORY "${LY_ROOT_FOLDER}"
)

# Ship the wizard with the installed SDK.
ly_install_directory(DIRECTORIES .)
