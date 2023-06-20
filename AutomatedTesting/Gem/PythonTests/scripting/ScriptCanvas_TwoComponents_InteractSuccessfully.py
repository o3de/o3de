"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    entity_created     = ("New entity created",              "New entity not created")
    game_mode_entered = ("Game Mode successfully entered", "Game mode failed to enter")
    game_mode_exited  = ("Game Mode successfully exited",  "Game mode failed to exited")
    lines_found       = ("Expected log lines were found",  "Expected log lines were not found")


def ScriptCanvas_TwoComponents_InteractSuccessfully():
    """
    Summary:
     A test entity contains two Script Canvas components with different unique script canvas files.
     Each of these files will have a print node set to activate on graph start.

    Expected Behavior:
     When game mode is entered, two unique strings should be printed out to the console

    Test Steps:
     1) Create level
     2) Create entity with SC components
     3) Enter game mode
     4) Wait for expected lines to be found
     5) Report if expected lines were found
     6) Exit game mode

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    from editor_python_test_tools.utils import TestHelper
    from editor_python_test_tools.utils import Report, Tracer
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.editor_component.editor_script_canvas import ScriptCanvasComponent
    import scripting_utils.scripting_tools as scripting_tools
    import azlmbr.legacy.general as general
    import os
    import azlmbr.paths as paths
    import azlmbr.math as math
    from scripting_utils.scripting_constants import (WAIT_TIME_3)

    EXPECTED_LINES = ["Greetings from the first script", "Greetings from the second script"]
    SOURCE_FILE_0 = os.path.join(paths.projectroot, "ScriptCanvas", "ScriptCanvas_TwoComponents0.scriptcanvas")
    SOURCE_FILE_1 = os.path.join(paths.projectroot, "ScriptCanvas", "ScriptCanvas_TwoComponents1.scriptcanvas")
    TEST_ENTITY_NAME = "test_entity"

    # Preconditions
    general.idle_enable(True)

    # 1) Create level
    TestHelper.open_level("", "Base")

    # 2) Create entity with SC components
    position = math.Vector3(512.0, 512.0, 32.0)
    editor_entity = EditorEntity.create_editor_entity_at(position, TEST_ENTITY_NAME)

    scriptcanvas_component_1 = ScriptCanvasComponent(editor_entity)
    scriptcanvas_component_1.set_component_graph_file_from_path(SOURCE_FILE_0)

    scriptcanvas_component_2 = ScriptCanvasComponent(editor_entity)
    scriptcanvas_component_2.set_component_graph_file_from_path(SOURCE_FILE_1)

    with Tracer() as section_tracer:

        # 3) Enter game mode
        TestHelper.enter_game_mode(Tests.game_mode_entered)

        # 4) Wait for expected lines to be found
        lines_located = TestHelper.wait_for_condition(
            lambda: scripting_tools.located_expected_tracer_lines(section_tracer, EXPECTED_LINES),
            WAIT_TIME_3)
        Report.result(Tests.lines_found, lines_located)

    # 5) Exit game mode
    TestHelper.exit_game_mode(Tests.game_mode_exited)

if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptCanvas_TwoComponents_InteractSuccessfully)
