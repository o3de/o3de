"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt: off
class Tests:
    camera_creation =                  ("Camera Entity successfully created",                         "Camera Entity failed to be created")
    camera_component_added =           ("Camera component was added to entity",                       "Camera component failed to be added to entity")
    camera_component_check =           ("Entity has a Camera component",                              "Entity failed to find Camera component")
    creation_undo =                    ("UNDO Entity creation success",                               "UNDO Entity creation failed")
    creation_redo =                    ("REDO Entity creation success",                               "REDO Entity creation failed")
    postfx_radius_weight_creation =    ("PostFX Radius Weight Modifier Entity successfully created",  "PostFX Radius Weight Modifier Entity failed to be created")
    postfx_radius_weight_component =   ("Entity has a PostFX Radius Weight Modifier component",       "Entity failed to find PostFX Radius Weight Modifier component")
    enter_game_mode =                  ("Entered game mode",                                          "Failed to enter game mode")
    exit_game_mode =                   ("Exited game mode",                                           "Couldn't exit game mode")
    is_visible =                       ("Entity is visible",                                          "Entity was not visible")
    is_hidden =                        ("Entity is hidden",                                           "Entity was not hidden")
    entity_deleted =                   ("Entity deleted",                                             "Entity was not deleted")
    deletion_undo =                    ("UNDO deletion success",                                      "UNDO deletion failed")
    deletion_redo =                    ("REDO deletion success",                                      "REDO deletion failed")
    no_error_occurred =                ("No errors detected",                                         "Errors were detected")
# fmt: on


def AtomEditorComponents_PostFXRadiusWeightModifier_AddedToEntity():
    """
    Summary:
    Tests the PostFX Radius Weight Modifier component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Post FX Radius Weight Modifier entity with no components.
    2) Add Post FX Radius Weight Modifier component to Post FX Radius Weight Modifier entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Enter/Exit game mode.
    6) Test IsHidden.
    7) Test IsVisible.
    8) Delete PostFX Radius Weight Modifier entity.
    9) UNDO deletion.
    10) REDO deletion.
    11) Look for errors.

    :return: None
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper as helper

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        helper.init_idle()
        helper.open_level("Physics", "Base")

        # Test steps begin.
        # 1. Create a Post FX Radius Weight Modifier entity with no components.
        postfx_radius_weight_name = "PostFX Radius Weight Modifier"
        postfx_radius_weight_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(512.0, 512.0, 34.0), postfx_radius_weight_name)
        Report.critical_result(Tests.postfx_radius_weight_creation, postfx_radius_weight_entity.exists())

        # 2. Add Post FX Radius Weight Modifier component to Post FX Radius Weight Modifier entity.
        postfx_radius_weight_entity.add_component(postfx_radius_weight_name)
        Report.critical_result(
            Tests.postfx_radius_weight_component, postfx_radius_weight_entity.has_component(postfx_radius_weight_name))

        # 3. UNDO the entity creation and component addition.
        # Requires 3 UNDO calls to remove the Entity completely.
        for x in range(4):
            general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_undo, not postfx_radius_weight_entity.exists())

        # 4. REDO the entity creation and component addition.
        # Requires 3 REDO calls to match the previous 3 UNDO calls.
        for x in range(4):
            general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_redo, postfx_radius_weight_entity.exists())

        # 5. Enter/Exit game mode.
        helper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        helper.exit_game_mode(Tests.exit_game_mode)

        # 6. Test IsHidden.
        postfx_radius_weight_entity.set_visibility_state(False)
        is_hidden = editor.EditorEntityInfoRequestBus(bus.Event, 'IsHidden', postfx_radius_weight_entity.id)
        Report.result(Tests.is_hidden, is_hidden is True)

        # 7. Test IsVisible.
        postfx_radius_weight_entity.set_visibility_state(True)
        is_visible = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', postfx_radius_weight_entity.id)
        Report.result(Tests.is_visible, is_visible is True)

        # 8. Delete PostFX Radius Weight Modifier entity.
        postfx_radius_weight_entity.delete()
        Report.result(Tests.entity_deleted, not postfx_radius_weight_entity.exists())

        # 9. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, postfx_radius_weight_entity.exists())

        # 10. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not postfx_radius_weight_entity.exists())

        # 11. Look for errors.
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_PostFXRadiusWeightModifier_AddedToEntity)
