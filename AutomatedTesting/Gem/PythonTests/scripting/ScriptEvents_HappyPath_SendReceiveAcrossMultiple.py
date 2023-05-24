"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt: off
class Tests:
    enter_game_mode = ("Successfully entered game mode",       "Failed to enter game mode")
    lines_found     = ("Successfully found expected message",  "Failed to find expected message")
    exit_game_mode  = ("Successfully exited game mode",        "Failed to exit game mode")
# fmt: on


def ScriptEvents_HappyPath_SendReceiveAcrossMultiple():
    """
    Summary:
     EntityA and EntityB will be created in a level. Attached to both will be a Script Canvas component.
     The Script Event created for the test will be sent from EntityA to EntityB.

    Expected Behavior:
     The output of the Script Event should be printed to the console

    Test Steps:
     1) Create test level
     2) Create EntityA/EntityB (add scriptcanvas files part of entity setup)
     3) Enter Game Mode
     4) Read for line
     5) Exit Game Mode


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

    ASSET_PREFIX = "T92567321"
    asset_paths = {
        "event": os.path.join(paths.projectroot, "TestAssets", f"{ASSET_PREFIX}.scriptevents"),
        "assetA": os.path.join(paths.projectroot, "ScriptCanvas", f"{ASSET_PREFIX}A.scriptcanvas"),
        "assetB": os.path.join(paths.projectroot, "ScriptCanvas", f"{ASSET_PREFIX}B.scriptcanvas"),
    }
    ENTITY_NAME_FILEPATH_MAP = {"EntityA": asset_paths["assetA"], "EntityB": asset_paths["assetB"]}
    EXPECTED_LINES = ["Incoming Message Received"]
    position = math.Vector3(512.0, 512.0, 32.0)

    # Preconditions
    general.idle_enable(True)

    # 1) Create temp level
    TestHelper.open_level("", "Base")

    # 2) Create EntityA/EntityB
    for key_name in ENTITY_NAME_FILEPATH_MAP.keys():

        editor_entity = EditorEntity.create_editor_entity_at(position, key_name)
        scriptcanvas_component = ScriptCanvasComponent(editor_entity)
        scriptcanvas_component.set_component_graph_file_from_path(ENTITY_NAME_FILEPATH_MAP[key_name])
        TestHelper.wait_for_condition(lambda: editor_entity is not None, WAIT_TIME_3)

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

    Report.start_test(ScriptEvents_HappyPath_SendReceiveAcrossMultiple)
