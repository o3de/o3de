"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt: off
class Tests:
    camera_creation =              ("Camera Entity successfully created",          "Camera Entity failed to be created")
    camera_component_added =       ("Camera component was added to entity",        "Camera component failed to be added to entity")
    camera_component_check =       ("Entity has a Camera component",               "Entity failed to find Camera component")
    creation_undo =                ("UNDO Entity creation success",                "UNDO Entity creation failed")
    creation_redo =                ("REDO Entity creation success",                "REDO Entity creation failed")
    exposure_control_creation =    ("ExposureControl Entity successfully created", "ExposureControl Entity failed to be created")
    exposure_control_component =   ("Entity has a Exposure Control component",     "Entity failed to find Exposure Control component")
    post_fx_component =            ("Entity has a Post FX Layer component",        "Entity did not have a Post FX Layer component")
    enter_game_mode =              ("Entered game mode",                           "Failed to enter game mode")
    exit_game_mode =               ("Exited game mode",                            "Couldn't exit game mode")
    is_visible =                   ("Entity is visible",                           "Entity was not visible")
    is_hidden =                    ("Entity is hidden",                            "Entity was not hidden")
    entity_deleted =               ("Entity deleted",                              "Entity was not deleted")
    deletion_undo =                ("UNDO deletion success",                       "UNDO deletion failed")
    deletion_redo =                ("REDO deletion success",                       "REDO deletion failed")
    no_error_occurred =            ("No errors detected",                          "Errors were detected")
# fmt: on


def AtomEditorComponents_ExposureControl_AddedToEntity():
    """
    Summary:
    Tests the Exposure Control component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Exposure Control entity with no components.
    2) Add Exposure Control component to Exposure Control entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Enter/Exit game mode.
    6) Test IsHidden.
    7) Test IsVisible.
    8) Add Post FX Layer component.
    9) Delete Exposure Control entity.
    10) UNDO deletion.
    11) REDO deletion.
    12) Look for errors.

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
        # 1. Creation of Exposure Control entity with no components.
        exposure_control_name = "Exposure Control"
        exposure_control_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(512.0, 512.0, 34.0), f"{exposure_control_name}")
        Report.critical_result(Tests.exposure_control_creation, exposure_control_entity.exists())

        # 2. Add Exposure Control component to Exposure Control entity.
        exposure_control_entity.add_component(exposure_control_name)
        Report.critical_result(
            Tests.exposure_control_component, exposure_control_entity.has_component(exposure_control_name))

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
        Report.result(Tests.creation_undo, not exposure_control_entity.exists())

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
        Report.result(Tests.creation_redo, exposure_control_entity.exists())

        # 5. Enter/Exit game mode.
        helper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        helper.exit_game_mode(Tests.exit_game_mode)

        # 6. Test IsHidden.
        exposure_control_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, exposure_control_entity.is_hidden() is True)

        # 7. Test IsVisible.
        exposure_control_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, exposure_control_entity.is_visible() is True)

        # 8. Add Post FX Layer component.
        post_fx_layer_name = "PostFX Layer"
        exposure_control_entity.add_component(post_fx_layer_name)
        Report.result(Tests.post_fx_component, exposure_control_entity.has_component(post_fx_layer_name))

        # 9. Delete ExposureControl entity.
        exposure_control_entity.delete()
        Report.result(Tests.entity_deleted, not exposure_control_entity.exists())

        # 10. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, exposure_control_entity.exists())

        # 11. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not exposure_control_entity.exists())

        # 12. Look for errors.
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_ExposureControl_AddedToEntity)
