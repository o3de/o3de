"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def ScriptEvents_ReturnSetType_Successfully():
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
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import os
    import azlmbr.math as math
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.editor_component.editor_script_canvas import ScriptCanvasComponent
    from editor_python_test_tools.utils import TestHelper as TestHelper
    from editor_python_test_tools.utils import Report, Tracer
    import azlmbr.legacy.general as general
    import azlmbr.paths as paths
    import scripting_utils.scripting_tools as scripting_tools
    from scripting_utils.scripting_constants import (WAIT_TIME_3)

    class Tests():
        enter_game_mode = ("Successfully entered game mode", "Failed to enter game mode")
        lines_found = ("Successfully found expected message", "Failed to find expected message")
        exit_game_mode = ("Successfully exited game mode", "Failed to exit game mode")

    ENTITY_NAME = "TestEntity"
    EXPECTED_LINES = ["T92569006_ScriptEvent_Sent", "T92569006_ScriptEvent_Received"]
    SC_FILE_PATH = os.path.join(paths.projectroot, "ScriptCanvas", "T92569006_ScriptCanvas.scriptcanvas")
    position = math.Vector3(512.0, 512.0, 32.0)

    # Preconditions
    general.idle_enable(True)

    # 1) Open the base level
    TestHelper.open_level("", "Base")

    # 2) Create test entity
    editor_entity = EditorEntity.create_editor_entity_at(position, ENTITY_NAME)
    scriptcanvas_component = ScriptCanvasComponent(editor_entity)
    scriptcanvas_component.set_component_graph_file_from_path(SC_FILE_PATH)

    # 3) Start Tracer
    with Tracer() as section_tracer:

        # 4) Enter Game Mode
        TestHelper.enter_game_mode(Tests.enter_game_mode)

        # 5) Read for line
        lines_located = TestHelper.wait_for_condition(
            lambda: scripting_tools.located_expected_tracer_lines(section_tracer, EXPECTED_LINES), WAIT_TIME_3)

        Report.result(Tests.lines_found, lines_located)

        # 6) Exit Game Mode
        TestHelper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptEvents_ReturnSetType_Successfully)
