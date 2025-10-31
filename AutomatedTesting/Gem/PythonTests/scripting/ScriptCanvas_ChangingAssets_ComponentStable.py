"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    game_mode_entered = ("Game Mode successfully entered", "Game mode failed to enter")
    game_mode_exited  = ("Game Mode successfully exited",  "Game mode failed to exited")
    found_lines       = ("Expected log lines were found",  "Expected log lines were not found")

def ScriptCanvas_ChangingAssets_ComponentStable():
    """
    Summary:
     Changing the assigned Script Canvas Asset on an entity properly updates level functionality

    Expected Behavior:
     When game mode is entered, respective strings of assigned assets should be printed

    Test Steps:
     1) Create temp level
     2) Create new entity
     3) Enter and Exit Game Mode
     4) Update Script Canvas file on entity's SC component
     5) Enter and Exit Game Mode
     6) Verify script canvas graph output


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    from editor_python_test_tools.utils import TestHelper, Report, Tracer
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    import scripting_utils.scripting_tools as scripting_tools
    import azlmbr.legacy.general as general
    from editor_python_test_tools.editor_component.editor_script_canvas import ScriptCanvasComponent
    import azlmbr.math as math

    import os
    import azlmbr.paths as paths

    TEST_ENTITY_NAME = "test_entity"
    ASSET_1 = os.path.join(paths.projectroot, "scriptcanvas", "ScriptCanvas_TwoComponents0.scriptcanvas")
    ASSET_2 = os.path.join(paths.projectroot, "scriptcanvas", "ScriptCanvas_TwoComponents1.scriptcanvas")
    EXPECTED_LINES = ["Greetings from the first script", "Greetings from the second script"]

    # Preconditions
    general.idle_enable(True)

    # 1) Create temp level
    TestHelper.open_level("", "Base")

    # 2) Create new entity
    position = math.Vector3(512.0, 512.0, 32.0)
    editor_entity = EditorEntity.create_editor_entity_at(position, TEST_ENTITY_NAME)
    script_canvas_component = ScriptCanvasComponent(editor_entity)
    script_canvas_component.set_component_graph_file_from_path(ASSET_1)

    with Tracer() as section_tracer:

        # 3) Enter and Exit Game Mode
        TestHelper.enter_game_mode(Tests.game_mode_entered)
        TestHelper.exit_game_mode(Tests.game_mode_exited)

        # 4) Update Script Canvas file on entity's SC component
        script_canvas_component.set_component_graph_file_from_path(ASSET_2)

        # 5) Enter and Exit Game Mode
        TestHelper.enter_game_mode(Tests.game_mode_entered)
        TestHelper.exit_game_mode(Tests.game_mode_exited)

        # 6) Verify script canvas graph output
        result = scripting_tools.located_expected_tracer_lines(section_tracer, EXPECTED_LINES)
        Report.result(Tests.found_lines, result)


if __name__ == "__main__":

    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(ScriptCanvas_ChangingAssets_ComponentStable)

