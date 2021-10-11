"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    creation_undo = (
        "UNDO Entity creation success",
        "UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "REDO Entity creation failed")
    reflection_probe_creation = (
        "Reflection Probe Entity successfully created",
        "Reflection Probe Entity failed to be created")
    reflection_probe_component = (
        "Entity has a Reflection Probe component",
        "Entity failed to find Reflection Probe component")
    reflection_probe_disabled = (
        "Reflection Probe component disabled",
        "Reflection Probe component was not disabled.")
    reflection_map_generated = (
        "Reflection Probe cubemap generated",
        "Reflection Probe cubemap not generated")
    box_shape_component = (
        "Entity has a Box Shape component",
        "Entity did not have a Box Shape component")
    reflection_probe_enabled = (
        "Reflection Probe component enabled",
        "Reflection Probe component was not enabled.")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "REDO deletion failed")


def AtomEditorComponents_ReflectionProbe_AddedToEntity():
    """
    Summary:
    Tests the Reflection Probe component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Reflection Probe entity with no components.
    2) Add a Reflection Probe component to Reflection Probe entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify Reflection Probe component not enabled.
    6) Add Shape component since it is required by the Reflection Probe component.
    7) Verify Reflection Probe component is enabled.
    8) Enter/Exit game mode.
    9) Test IsHidden.
    10) Test IsVisible.
    11) Verify cubemap generation
    12) Delete Reflection Probe entity.
    13) UNDO deletion.
    14) REDO deletion.
    15) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.render as render

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper as helper

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        helper.init_idle()
        helper.open_level("", "Base")

        # Test steps begin.
        # 1. Create a Reflection Probe entity with no components.
        reflection_probe_name = "Reflection Probe"
        reflection_probe_entity = EditorEntity.create_editor_entity_at(
            math.Vector3(512.0, 512.0, 34.0), reflection_probe_name)
        Report.critical_result(Tests.reflection_probe_creation, reflection_probe_entity.exists())

        # 2. Add a Reflection Probe component to Reflection Probe entity.
        reflection_probe_component = reflection_probe_entity.add_component(reflection_probe_name)
        Report.critical_result(
            Tests.reflection_probe_component,
            reflection_probe_entity.has_component(reflection_probe_name))

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
        Report.result(Tests.creation_undo, not reflection_probe_entity.exists())

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
        Report.result(Tests.creation_redo, reflection_probe_entity.exists())

        # 5. Verify Reflection Probe component not enabled.
        Report.result(Tests.reflection_probe_disabled, not reflection_probe_component.is_enabled())

        # 6. Add Box Shape component since it is required by the Reflection Probe component.
        box_shape = "Box Shape"
        reflection_probe_entity.add_component(box_shape)
        Report.result(Tests.box_shape_component, reflection_probe_entity.has_component(box_shape))

        # 7. Verify Reflection Probe component is enabled.
        Report.result(Tests.reflection_probe_enabled, reflection_probe_component.is_enabled())

        # 8. Enter/Exit game mode.
        helper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        helper.exit_game_mode(Tests.exit_game_mode)

        # 9. Test IsHidden.
        reflection_probe_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, reflection_probe_entity.is_hidden() is True)

        # 10. Test IsVisible.
        reflection_probe_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, reflection_probe_entity.is_visible() is True)

        # 11. Verify cubemap generation
        render.EditorReflectionProbeBus(azlmbr.bus.Event, "BakeReflectionProbe", reflection_probe_entity.id)
        Report.result(
            Tests.reflection_map_generated,
            helper.wait_for_condition(
                lambda: reflection_probe_component.get_component_property_value("Cubemap|Baked Cubemap Path") != "",
                20.0))

        # 12. Delete Reflection Probe entity.
        reflection_probe_entity.delete()
        Report.result(Tests.entity_deleted, not reflection_probe_entity.exists())

        # 13. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, reflection_probe_entity.exists())

        # 14. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not reflection_probe_entity.exists())

        # 15. Look for errors or asserts.
        helper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_ReflectionProbe_AddedToEntity)
