"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C19536277
Test Case Title : Verify that when a group is modified using ToggleCollisionLayer node such that the new group is not in the pre-existing groups, GetCollisionGroupName node prints no value

"""


# fmt: off
class Tests():
    test_entity_enabled = ("Test Entity successfully enabled", "Failed to enable Test Entity")
    game_mode_entered   = ("Successfully entered Game Mode", "Failed to enter Game Mode")
# fmt: on


def ScriptCanvas_GetCollisionNameReturnsNothingWhenHasToggledLayer():
    """
    Summary:
     Loads a level that contains an entity with script canvas and PhysX Collider components

    Level Description:
     Mostly empty level that contains a few different entities (one for each test using the level).
     Each entity is named after the testrail id for the respective test. Each entity contains PhysX Collider component
     and a Script Canvas Component with a matching .scriptcanvas file provided in the testrail.

    Expected Behavior:
     The level loads, enters game mode, and the script canvas prints out "GroupName: "

    Test Steps:
     1) Load the test level
     2) Find and enable the test entity
     3) Enter game mode

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    # Helper Files

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.editor_entity_utils import EditorEntity as Entity
    from editor_python_test_tools.utils import TestHelper as helper

    ACTIVE_STATUS = azlmbr.globals.property.EditorEntityStartStatus_StartActive

    helper.init_idle()
    # 1) Load the test level
    helper.open_level("Physics", "NameNode_Prints")

    # 2) Find and enable the test entity
    test_entity = Entity.find_editor_entity("C19536277")
    test_entity.set_start_status("active")
    Report.result(Tests.test_entity_enabled, test_entity.get_start_status() == ACTIVE_STATUS)

    # 3) Enter game mode
    helper.enter_game_mode(Tests.game_mode_entered)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ScriptCanvas_GetCollisionNameReturnsNothingWhenHasToggledLayer)
