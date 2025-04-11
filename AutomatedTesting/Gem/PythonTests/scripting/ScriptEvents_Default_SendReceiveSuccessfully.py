"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    enter_game_mode = ("Successfully entered game mode",       "Failed to enter game mode")
    lines_found     = ("Successfully found expected message",  "Failed to find expected message")
    exit_game_mode  = ("Successfully exited game mode",        "Failed to exit game mode")
# fmt: on


def ScriptEvents_Default_SendReceiveSuccessfully():
    """
    Summary:
     An entity exists in the level that contains a Script Canvas component. In the graph is both a Send Event
     and a Receive Event.

    Expected Behavior:
     After entering game mode the graph on the entity should print an expected message to the console

    Test Steps:
     1) Create test level
     2) Create test entity
     3) Enter Game Mode
     4) Read for line
     5) Exit Game Mode

    Note:
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import os
    from editor_python_test_tools.utils import Report, Tracer
    from editor_python_test_tools.utils import TestHelper as TestHelper
    import azlmbr.paths as paths
    import azlmbr.math as math
    import azlmbr.legacy.general as general
    import scripting_utils.scripting_tools as scripting_tools
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.editor_component.editor_script_canvas import ScriptCanvasComponent
    from scripting_utils.scripting_constants import (WAIT_TIME_3)

    ENTITY_NAME = "TestEntity"
    EXPECTED_LINES = ["T92567320: Message Received"]
    SC_ASSET_PATH = os.path.join(paths.projectroot, "ScriptCanvas", "T92567320.scriptcanvas")

    # Preconditions
    general.idle_enable(True)

    # 1) Create temp level
    TestHelper.open_level("", "Base")

    # 2) Create test entity
    position = math.Vector3(512.0, 512.0, 32.0)
    editor_entity = EditorEntity.create_editor_entity_at(position, ENTITY_NAME)
    scriptcanvas_component = ScriptCanvasComponent(editor_entity)
    scriptcanvas_component.set_component_graph_file_from_path(SC_ASSET_PATH)

    with Tracer() as section_tracer:

        # 3) Enter Game Mode
        TestHelper.enter_game_mode(Tests.enter_game_mode)

        # 4) Read for line
        lines_located = TestHelper.wait_for_condition(
            lambda: scripting_tools.located_expected_tracer_lines(section_tracer, EXPECTED_LINES), WAIT_TIME_3)
        Report.result(Tests.lines_found, lines_located)

    # 5) Exit Game Mode
    TestHelper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptEvents_Default_SendReceiveSuccessfully)
