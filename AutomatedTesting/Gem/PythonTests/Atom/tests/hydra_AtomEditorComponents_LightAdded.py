"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt: off
class Tests:
    camera_creation =          ("Camera Entity successfully created",   "Camera Entity failed to be created")
    camera_component_added =   ("Camera component was added to entity", "Camera component failed to be added to entity")
    camera_component_check =   ("Entity has a Camera component",        "Entity failed to find Camera component")
    creation_undo =            ("UNDO Entity creation success",         "UNDO Entity creation failed")
    creation_redo =            ("REDO Entity creation success",         "REDO Entity creation failed")
    light_creation =           ("Light Entity successfully created",    "Light Entity failed to be created")
    light_component =          ("Entity has a Light component",         "Entity failed to find Light component")
    enter_game_mode =          ("Entered game mode",                    "Failed to enter game mode")
    exit_game_mode =           ("Exited game mode",                     "Couldn't exit game mode")
    is_visible =               ("Entity is visible",                    "Entity was not visible")
    is_hidden =                ("Entity is hidden",                     "Entity was not hidden")
    entity_deleted =           ("Entity deleted",                       "Entity was not deleted")
    deletion_undo =            ("UNDO deletion success",                "UNDO deletion failed")
    deletion_redo =            ("REDO deletion success",                "REDO deletion failed")
    no_error_occurred =        ("No errors detected",                   "Errors were detected")
# fmt: on


def AtomEditorComponents_Light_AddedToEntity():
    """
    Summary:
    Tests the Light component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Light entity with no components.
    2) Add Light component to the Light entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Enter/Exit game mode.
    6) Test IsHidden.
    7) Test IsVisible.
    8) Delete Light entity.
    9) UNDO deletion.
    10) REDO deletion.
    11) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper as helper

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        helper.init_idle()
        helper.open_level("", "Base")

        # Test steps begin.
        # 1. Create a Light entity with no components.
        light_name = "Light"
        light_entity = EditorEntity.create_editor_entity_at(math.Vector3(512.0, 512.0, 34.0), light_name)
        Report.critical_result(Tests.light_creation, light_entity.exists())

        # 2. Add Light component to the Light entity.
        light_entity.add_component(light_name)
        Report.critical_result(Tests.light_component, light_entity.has_component(light_name))

        # 3. UNDO the entity creation and component addition.
        # -> UNDO component addition.
        general.undo()
        # -> UNDO naming entity.
        general.undo()
        # -> UNDO selecting entity.
        general.undo()
        # -> UNDO entity creation.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_undo, not light_entity.exists())

        # 4. REDO the entity creation and component addition.
        # -> REDO entity creation.
        general.redo()
        # -> REDO selecting entity.
        general.redo()
        # -> REDO naming entity.
        general.redo()
        # -> REDO component addition.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_redo, light_entity.exists())

        # 5. Enter/Exit game mode.
        helper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        helper.exit_game_mode(Tests.exit_game_mode)

        # 6. Test IsHidden.
        light_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, light_entity.is_hidden() is True)

        # 7. Test IsVisible.
        light_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, light_entity.is_visible() is True)

        # 8. Delete Light entity.
        light_entity.delete()
        Report.result(Tests.entity_deleted, not light_entity.exists())

        # 9. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, light_entity.exists())

        # 10. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not light_entity.exists())

        # 11. Look for errors.
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Light_AddedToEntity)
