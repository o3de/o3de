"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test Case Title: Event can return a value of set type successfully
"""

import pyside_utils
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report, Tracer
import azlmbr.legacy.general as general
import scripting_utils.scripting_tools as scripting_tools
import azlmbr.paths as paths
import editor_python_test_tools.hydra_editor_utils as hydra
from scripting_utils.scripting_constants import (WAIT_TIME_3, BASE_LEVEL_NAME)

# fmt: off
class Tests():
    entity_created  = ("Successfully created test entity",     "Failed to create test entity")
    enter_game_mode = ("Successfully entered game mode",       "Failed to enter game mode")
    lines_found     = ("Successfully found expected message",  "Failed to find expected message")
    exit_game_mode  = ("Successfully exited game mode",        "Failed to exit game mode")
# fmt: on


EXPECTED_LINES = ["T92569006_ScriptEvent_Sent", "T92569006_ScriptEvent_Received"]
SC_FILE_PATH = os.path.join(paths.projectroot, "ScriptCanvas", "T92569006_ScriptCanvas.scriptcanvas")

class ScriptEvents_ReturnSetType_Successfully:


    def __init__(self):
        self.editor_main_window = None


    @pyside_utils.wrap_async
    async def run_test(self):

        """
            Summary: A temporary level is created with an Entity having ScriptCanvas component.
             ScriptEvent(T92569006_ScriptEvent.scriptevents) is created with one Method that has a return value.
             ScriptCanvas(T92569006_ScriptCanvas.scriptcanvas) is attached to Entity. Graph has Send node that sends the Method
             of the ScriptEvent and prints the returned result ( On Entity Activated -> Send node -> Print) and Receive node is
             set to return custom value ( Receive node -> Print).
             Verify that the entity containing T92569006_ScriptCanvas.scriptcanvas should print the custom value set in both
             Send and Receive nodes.

            Expected Behavior:
             After entering game mode, the graph on the entity should print an expected message to the console

            Test Steps:
             1) Open the base level
             2) Create test entity
             3) Start Tracer
             4) Enter Game Mode
             5) Read for line
             6) Exit Game Mode

            Note:
             - This test file must be called from the Open 3D Engine Editor command terminal
             - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

            :return: None
            """

        # Preconditions
        general.idle_enable(True)

        # 1) Open the base level
        hydra.open_base_level()
        helper.wait_for_condition(lambda: general.get_current_level_name() == BASE_LEVEL_NAME, WAIT_TIME_3)
        general.close_pane("Error Report")

        # 2) Create test entity
        entity = scripting_tools.create_entity_with_sc_component_asset("TestEntity", SC_FILE_PATH)
        helper.wait_for_condition(lambda: entity is not None, WAIT_TIME_3)
        Report.critical_result(Tests.entity_created, entity.id.isValid())

        # 3) Start Tracer
        with Tracer() as section_tracer:

            # 4) Enter Game Mode
            helper.enter_game_mode(Tests.enter_game_mode)

            # 5) Read for line
            lines_located = helper.wait_for_condition(
                lambda: scripting_tools.located_expected_tracer_lines(self, section_tracer, EXPECTED_LINES), WAIT_TIME_3)

            Report.result(Tests.lines_found, lines_located)

            # 6) Exit Game Mode
            helper.exit_game_mode(Tests.exit_game_mode)


test = ScriptEvents_ReturnSetType_Successfully()
test.run_test()
