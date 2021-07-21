"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
