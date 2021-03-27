"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Tests the Python Game Mode API from PythonEditorFuncs.cpp while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.legacy.general as general

general.idle_enable(True)

general.idle_wait(0.125)

general.enter_game_mode()

general.idle_wait(1.0)

if(general.is_in_game_mode()):
    print("Game Mode is On")

general.idle_wait(0.125)

general.exit_game_mode()

general.idle_wait(1.0)

if not (general.is_in_game_mode()):
    print("Game Mode is Off")

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
