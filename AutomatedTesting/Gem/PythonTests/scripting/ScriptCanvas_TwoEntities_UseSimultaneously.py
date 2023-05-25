"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    level_created     = ("New level created successfully", "New level failed to create")
    game_mode_entered = ("Game Mode successfully entered", "Game mode failed to enter")
    game_mode_exited  = ("Game Mode successfully exited",  "Game mode failed to exited")
    found_lines       = ("Expected log lines were found",  "Expected log lines were not found")
# fmt: on


def ScriptCanvas_TwoEntities_UseSimultaneously():
    """
    Summary:
     Two Entities can use the same Graph asset successfully at RunTime. The script canvas asset
     attached to the entities will print the respective entity names.

    Expected Behavior:
     When game mode is entered, respective strings of different entities should be printed.

    Test Steps:
     1) Create temp level
     2) Create two new entities with different names
     3) Set ScriptCanvas asset to both the entities
     4) Enter/Exit game mode and verify log lines


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.editor_component.editor_script_canvas import ScriptCanvasComponent
    from editor_python_test_tools.utils import TestHelper
    from editor_python_test_tools.utils import Tracer
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    TEST_ENTITY_NAME_1 = "test_entity_1"
    TEST_ENTITY_NAME_2 = "test_entity_2"
    ASSET_PATH = os.path.join("scriptcanvas", "T92563191_test.scriptcanvas")
    EXPECTED_LINES = ["Entity Name: test_entity_1", "Entity Name: test_entity_2"]
    WAIT_TIME = 0.5  # SECONDS

    #Preconditions
    general.idle_enable(True)

    # 1) Create temp level
    TestHelper.open_level("", "Base")

    # 2) Create two new entities with different names
    position = math.Vector3(512.0, 512.0, 32.0)
    editor_entity_1 = EditorEntity.create_editor_entity_at(position, TEST_ENTITY_NAME_1)
    editor_entity_2 = EditorEntity.create_editor_entity_at(position, TEST_ENTITY_NAME_2)

    # 3) Set ScriptCanvas asset to both the entities
    scriptcanvas_component_1 = ScriptCanvasComponent(editor_entity_1)
    scriptcanvas_component_1.set_component_graph_file_from_path(ASSET_PATH)

    scriptcanvas_component_2 = ScriptCanvasComponent(editor_entity_2)
    scriptcanvas_component_2.set_component_graph_file_from_path(ASSET_PATH)

    # 4) Enter/Exit game mode and verify log lines
    with Tracer() as section_tracer:

        TestHelper.enter_game_mode(Tests.game_mode_entered)
        # wait for WAIT_TIME to let the script print strings
        general.idle_wait(WAIT_TIME)
        TestHelper.exit_game_mode(Tests.game_mode_exited)

    found_lines = [printInfo.message.strip() for printInfo in section_tracer.prints]
    result = all(line in found_lines for line in EXPECTED_LINES)

    Report.result(Tests.found_lines, result)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptCanvas_TwoEntities_UseSimultaneously)
