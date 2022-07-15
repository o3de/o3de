"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report, Tracer
import editor_python_test_tools.hydra_editor_utils as hydra
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.paths as paths
import editor_python_test_tools.pyside_utils as pyside_utils
import azlmbr.scriptcanvas as scriptcanvas

LEVEL_NAME = "Base"
ASSET_1 = os.path.join(paths.projectroot, "scriptcanvas", "ScriptCanvas_TwoComponents0.scriptcanvas")
ASSET_2 = os.path.join(paths.projectroot, "scriptcanvas", "ScriptCanvas_TwoComponents1.scriptcanvas")
EXP_LINE_1 = "Greetings from the first script"
EXP_LINE_2 = "Greetings from the second script"
WAIT_TIME = 3.0  # SECONDS
SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH = "Configuration|Source"

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

    def find_expected_line(self, expected_line, section_tracer):
        found_lines = [printInfo.message.strip() for printInfo in section_tracer.prints]
        return expected_line in found_lines

    #replace with the tools function later
    def get_entity(self, source_file, entity_name):
        sourcehandle = scriptcanvas.SourceHandleFromPath(source_file)

        position = math.Vector3(512.0, 512.0, 32.0)
        entity = hydra.Entity(entity_name)
        entity.create_entity(position, ["Script Canvas"])

        script_canvas_component = entity.components[0]
        hydra.set_component_property_value(script_canvas_component, SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH, sourcehandle)

        return entity

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Create temp level
        hydra.open_base_level()
        helper.wait_for_condition(lambda: general.get_current_level_name() == LEVEL_NAME, WAIT_TIME)
        general.close_pane("Error Report")

        # 2) Create new entity
        test_entity = self.get_entity(ASSET_1, "test_entity")
        result = helper.wait_for_condition(lambda: test_entity is not None, WAIT_TIME)
        Report.result(Tests.entity_created, result)

        # 3) Enter and Exit Game Mode
        helper.enter_game_mode(Tests.game_mode_entered)
        helper.exit_game_mode(Tests.game_mode_exited)

        # 4) Update Script Canvas file on entity's SC component
        #mvoe this to the tools file
        script_canvas_component = test_entity.components[0]
        sourcehandle = scriptcanvas.SourceHandleFromPath(ASSET_2)
        hydra.set_component_property_value(script_canvas_component, SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH, sourcehandle)
        script_file = hydra.get_component_property_value(script_canvas_component, SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH)
        result = helper.wait_for_condition(lambda: script_file is not None, WAIT_TIME)
        Report.result(Tests.script_file_update, result)

        # 5) Enter and Exit Game Mode
        helper.enter_game_mode(Tests.game_mode_entered)
        helper.exit_game_mode(Tests.game_mode_exited)

        # 6 Verify script canvas graph output
        with Tracer() as section_tracer:

            self.find_expected_line(EXP_LINE_1, section_tracer)
            self.find_expected_line(EXP_LINE_2, section_tracer)


test = ScriptCanvas_ChangingAssets_ComponentStable()
test.run_test()

