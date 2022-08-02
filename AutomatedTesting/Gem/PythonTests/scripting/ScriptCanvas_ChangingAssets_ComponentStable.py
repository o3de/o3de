"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
from editor_python_test_tools.utils import TestHelper as helper
import azlmbr.editor as editor
import azlmbr.bus as bus
from editor_python_test_tools.utils import Report, Tracer
import editor_python_test_tools.hydra_editor_utils as hydra
import scripting_utils.scripting_tools as scripting_tools
import azlmbr.legacy.general as general
import azlmbr.paths as paths
import pyside_utils
from scripting_utils.scripting_constants import (BASE_LEVEL_NAME, WAIT_TIME_3)

TEST_ENTITY_NAME = "test_entity"
ASSET_1 = os.path.join(paths.projectroot, "scriptcanvas", "ScriptCanvas_TwoComponents0.scriptcanvas")
ASSET_2 = os.path.join(paths.projectroot, "scriptcanvas", "ScriptCanvas_TwoComponents1.scriptcanvas")
EXPECTED_LINES = ["Greetings from the first script", "Greetings from the second script"]

# fmt: off
class Tests:
    entity_created    = ("Test Entity created",            "Test Entity not created")
    game_mode_entered = ("Game Mode successfully entered", "Game mode failed to enter")
    game_mode_exited  = ("Game Mode successfully exited",  "Game mode failed to exited")
    script_file_update = ("Script Canvas File Updated", "Script Canvas File Not Updated")
    found_lines       = ("Expected log lines were found",  "Expected log lines were not found")
# fmt: on


class ScriptCanvas_ChangingAssets_ComponentStable:
    """
    Summary:
     Changing the assigned Script Canvas Asset on an entity properly updates level functionality

    Expected Behavior:
     When game mode is entered, respective strings of assigned assets should be printed

    Test Steps:
     1) Create temp level
     2) Create new entity
     3) Start Tracer
     4) Set first script and evaluate
     5) Set second script and evaluate


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    def __init__(self):
        editor_window = None

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Create temp level
        hydra.open_base_level()
        helper.wait_for_condition(lambda: general.get_current_level_name() == BASE_LEVEL_NAME, WAIT_TIME_3)
        general.close_pane("Error Report")

        # 2) Create new entity
        entity = scripting_tools.create_entity_with_sc_component_asset(TEST_ENTITY_NAME, ASSET_1)
        result = helper.wait_for_condition(lambda: entity is not None, WAIT_TIME_3)
        Report.result(Tests.entity_created, result)

        with Tracer() as section_tracer:

            # 3) Enter and Exit Game Mode
            helper.enter_game_mode(Tests.game_mode_entered)
            helper.exit_game_mode(Tests.game_mode_exited)

            # 4) Update Script Canvas file on entity's SC component
            result = scripting_tools.change_entity_sc_asset(entity, ASSET_2)
            Report.result(Tests.script_file_update, result)

            # 5) Enter and Exit Game Mode
            helper.enter_game_mode(Tests.game_mode_entered)
            helper.exit_game_mode(Tests.game_mode_exited)

            # 6 Verify script canvas graph output
            result = scripting_tools.located_expected_tracer_lines(self, section_tracer, EXPECTED_LINES)
            Report.result(Tests.found_lines, result)

test = ScriptCanvas_ChangingAssets_ComponentStable()
test.run_test()

