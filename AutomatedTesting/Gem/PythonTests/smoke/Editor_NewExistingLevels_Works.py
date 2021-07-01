"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


Test Case Title: Create Test for UI apps- Editor
"""


class Tests():
    level_created          = ("Level created",                     "Failed to create level")
    entity_found           = ("New Entity created in level",       "Failed to create New Entity in level")
    mesh_added             = ("Mesh Component added",              "Failed to add Mesh Component")
    enter_game_mode        = ("Game Mode successfully entered",    "Failed to enter in Game Mode")
    exit_game_mode         = ("Game Mode successfully exited",     "Failed to exit in Game Mode")
    level_opened           = ("Level opened successfully",         "Failed to open level")
    level_exported         = ("Level exported successfully",       "Failed to export level")
    mesh_removed           = ("Mesh Component removed",            "Failed to remove Mesh Component")
    entity_deleted         = ("Entity deleted",                    "Failed to delete Entity")
    level_edits_present    = ("Level edits persist after saving",  "Failed to save level edits after saving")


def Editor_NewExistingLevels_Works():
    """
    Summary: Perform the below operations on Editor

        1)  Launch & Close editor
        2)  Create new level
        3)  Saving and loading levels
        4)  Level edits persist after saving
        5)  Export Level
        6)  Can switch to play mode (ctrl+g) and exit that
        7)  Run editor python bindings test
        8)  Create an Entity
        9)  Delete an Entity
        10) Add a component to an Entity

    Expected Behavior:
    All operations succeed and do not cause a crash

    Test Steps:
    1)  Launch editor and Create a new level
    2)  Create a new entity
    3)  Add Mesh component
    4)  Verify enter/exit game mode
    5)  Save, Load and Export level
    6)  Remove Mesh component
    7)  Delete entity
    8)  Open an existing level
    9)  Create a new entity in an existing level
    10) Save, Load and Export an existing level and close editor

    Note:
    - This test file must be called from the Lumberyard Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Report
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    # 1) Launch editor and Create a new level
    helper.init_idle()
    test_level_name = "temp_level"
    general.create_level_no_prompt(test_level_name, 128, 1, 128, False)
    helper.wait_for_condition(lambda: general.get_current_level_name() == test_level_name, 2.0)
    Report.result(Tests.level_created, general.get_current_level_name() == test_level_name)

    # 2) Create a new entity
    entity_position = math.Vector3(200.0, 200.0, 38.0)
    new_entity = hydra.Entity("Entity1")
    new_entity.create_entity(entity_position, [])
    test_entity = hydra.find_entity_by_name("Entity1")
    Report.result(Tests.entity_found, test_entity.IsValid())

    # 3) Add Mesh component
    new_entity.add_component("Mesh")
    Report.result(Tests.mesh_added, hydra.has_components(new_entity.id, ["Mesh"]))

    # 4) Verify enter/exit game mode
    helper.enter_game_mode(Tests.enter_game_mode)
    helper.exit_game_mode(Tests.exit_game_mode)

    # 5) Save, Load and Export level
    # Save Level
    general.save_level()
    # Open Level
    general.open_level(test_level_name)
    Report.result(Tests.level_opened, general.get_current_level_name() == test_level_name)
    # Export Level
    general.export_to_engine()
    level_pak_file = os.path.join("AutomatedTesting", "Levels", test_level_name, "level.pak")
    Report.result(Tests.level_exported, os.path.exists(level_pak_file))

    # 6) Remove Mesh component
    new_entity.remove_component("Mesh")
    Report.result(Tests.mesh_removed, not hydra.has_components(new_entity.id, ["Mesh"]))

    # 7) Delete entity
    editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntityById", new_entity.id)
    test_entity = hydra.find_entity_by_name("Entity1")
    Report.result(Tests.entity_deleted, len(test_entity) == 0)

    # 8) Open an existing level
    general.open_level(test_level_name)
    Report.result(Tests.level_opened, general.get_current_level_name() == test_level_name)

    # 9) Create a new entity in an existing level
    entity_position = math.Vector3(200.0, 200.0, 38.0)
    new_entity_2 = hydra.Entity("Entity2")
    new_entity_2.create_entity(entity_position, [])
    test_entity = hydra.find_entity_by_name("Entity2")
    Report.result(Tests.entity_found, test_entity.IsValid())

    # 10) Save, Load and Export an existing level
    # Save Level
    general.save_level()
    # Open Level
    general.open_level(test_level_name)
    Report.result(Tests.level_opened, general.get_current_level_name() == test_level_name)
    entity_id = hydra.find_entity_by_name(new_entity_2.name)
    Report.result(Tests.level_edits_present, entity_id == new_entity_2.id)
    # Export Level
    general.export_to_engine()
    level_pak_file = os.path.join("AutomatedTesting", "Levels", test_level_name, "level.pak")
    Report.result(Tests.level_exported, os.path.exists(level_pak_file))


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report

    Report.start_test(Editor_NewExistingLevels_Works)
