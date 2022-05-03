"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    creation_undo = (
        "UNDO Entity creation success",
        "P0: UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "P0: REDO Entity creation failed")
    reflection_probe_creation = (
        "Reflection Probe Entity successfully created",
        "P0: Reflection Probe Entity failed to be created")
    reflection_probe_component = (
        "Entity has a Reflection Probe component",
        "P0: Entity failed to find Reflection Probe component")
    reflection_probe_disabled = (
        "Reflection Probe component disabled",
        "P0: Reflection Probe component was not disabled.")
    reflection_map_generated = (
        "Reflection Probe cubemap generated",
        "P0: Reflection Probe cubemap generation timed out waiting for AssetProcessor to return a path to cubemap")
    box_shape_component = (
        "Entity has a Box Shape component",
        "P0: Entity did not have a Box Shape component")
    reflection_probe_enabled = (
        "Reflection Probe component enabled",
        "P0: Reflection Probe component was not enabled.")
    enter_game_mode = (
        "Entered game mode",
        "P0: Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "P0: Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "P0: Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "P0: Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "P0: Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "P0: UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "P0: REDO deletion failed")
    parallax_correction = (
        "Parallax Correction property set",
        "P1: Parallax Correction property failed to set correctly")
    use_baked_cubemap = (
        "Use Baked Cubemap property set",
        "P1: Use Baked Cubemap property failed to set correctly")
    show_visualization = (
        "Show Visualization property set",
        "P1: Show Visualization property failed to set correctly")
    bake_exposure_low = (
        "Bake Exposure property set to minimum value",
        "P1: Bake Exposure property failed to take minimum value")
    bake_exposure_high = (
        "Bake Exposure property set to maximum value",
        "P1: Bake Exposure property failed to take maximum value")
    settings_exposure_low = (
        "Settings|Exposure property set to minimum value",
        "P1: Settings|Exposure property failed to take minimum value")
    settings_exposure_high = (
        "Settings|Exposure property set to maximum value",
        "P1: Settings|Exposure property failed to take maximum value")
    inner_extents_height_low = (
        "Inner Extents|Height property set to minimum value",
        "P1: Inner Extents|Height property failed to take minimum value")
    inner_extents_height_high = (
        "Inner Extents|Height property set to maximum value for default shape size",
        "P1: Inner Extents|Height property failed to take maximum value for default shape size")
    inner_extents_length_low = (
        "Inner Extents|Length property set to minimum value",
        "P1: Inner Extents|Length property failed to take minimum value")
    inner_extents_length_high = (
        "Inner Extents|Length property set to maximum value for default shape size",
        "P1: Inner Extents|Length property failed to take maximum value for default shape size")
    inner_extents_width_low = (
        "Inner Extents|Width property set to minimum value",
        "P1: Inner Extents|Width property failed to take minimum value")
    inner_extents_width_high = (
        "Inner Extents|Width property set to maximum value for default shape size",
        "P1: Inner Extents|Width property failed to take maximum value for default shape size")


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
    8) Toggle Parallax Correction (default True)
    9) Toggle Use Baked Cubemap (default True)
    10) Toggle Show Visualization (default True)
    11) Bake Exposure (float range -20.0 to 20.0)
    12) Inner Extents|Height (float 0.0 to required shape dimension constraint default 8.0)
    13) Inner Extents|Length (float 0.0 to required shape dimension constraint default 8.0)
    14) Inner Extents|Width (float 0.0 to required shape dimension constraint default 8.0)
    15) Settings|Exposure (float range -20.0 to 20.0)
    16) Baked Cubemap Quality (unsigned int use dictionary constant)
    17) Enter/Exit game mode.
    18) Test IsHidden.
    19) Test IsVisible.
    20) Delete Reflection Probe entity.
    21) UNDO deletion.
    22) REDO deletion.
    23) Look for errors and asserts.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.render as render

    from azlmbr.math import Math_IsClose
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties, BAKED_CUBEMAP_QUALITY

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a Reflection Probe entity with no components.
        reflection_probe_entity = EditorEntity.create_editor_entity(AtomComponentProperties.reflection_probe())
        Report.critical_result(Tests.reflection_probe_creation, reflection_probe_entity.exists())

        # 2. Add a Reflection Probe component to Reflection Probe entity.
        reflection_probe_component = reflection_probe_entity.add_component(AtomComponentProperties.reflection_probe())
        Report.critical_result(
            Tests.reflection_probe_component,
            reflection_probe_entity.has_component(AtomComponentProperties.reflection_probe()))

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

        # 6. Add Shape component since it is required by the Reflection Probe component.
        for shape in AtomComponentProperties.reflection_probe('shapes'):
            reflection_probe_entity.add_component(shape)
            test_shape = (
                f"Entity has a {shape} component",
                f"Entity did not have a {shape} component")
            Report.result(test_shape, reflection_probe_entity.has_component(shape))

            # 7. Check if required shape allows Reflection Probe to be enabled
            Report.result(Tests.reflection_probe_enabled, reflection_probe_component.is_enabled())

            # Undo to remove each added shape except the last one and verify Reflection Probe is not enabled.
            if not (shape == AtomComponentProperties.reflection_probe('shapes')[-1]):
                general.undo()
                TestHelper.wait_for_condition(lambda: not reflection_probe_entity.has_component(shape), 1.0)
                Report.result(Tests.reflection_probe_disabled, not reflection_probe_component.is_enabled())

        # 8. Toggle Parallax Correction (default True)
        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Parallax Correction'), False)
        Report.result(
            Tests.parallax_correction,
            reflection_probe_component.get_component_property_value(
                AtomComponentProperties.reflection_probe('Parallax Correction')) is False)

        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Parallax Correction'), True)
        Report.result(
            Tests.parallax_correction,
            reflection_probe_component.get_component_property_value(
                AtomComponentProperties.reflection_probe('Parallax Correction')) is True)

        # 9. Toggle Use Baked Cubemap (default True)
        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Use Baked Cubemap'), False)
        Report.result(
            Tests.use_baked_cubemap,
            reflection_probe_component.get_component_property_value(
                AtomComponentProperties.reflection_probe('Use Baked Cubemap')) is False)

        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Use Baked Cubemap'), True)
        Report.result(
            Tests.use_baked_cubemap,
            reflection_probe_component.get_component_property_value(
                AtomComponentProperties.reflection_probe('Use Baked Cubemap')) is True)

        # 10. Toggle Show Visualization (default True)
        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Show Visualization'), False)
        Report.result(
            Tests.show_visualization,
            reflection_probe_component.get_component_property_value(
                AtomComponentProperties.reflection_probe('Show Visualization')) is False)

        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Show Visualization'), True)
        Report.result(
            Tests.show_visualization,
            reflection_probe_component.get_component_property_value(
                AtomComponentProperties.reflection_probe('Show Visualization')) is True)

        # 11. Bake Exposure (float range -20.0 to 20.0)
        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Bake Exposure'), -20.0)
        Report.result(
            Tests.bake_exposure_low,
            Math_IsClose(
                reflection_probe_component.get_component_property_value(
                    AtomComponentProperties.reflection_probe('Bake Exposure')), -20.0))

        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Bake Exposure'), 20.0)
        Report.result(
            Tests.bake_exposure_high,
            Math_IsClose(
                reflection_probe_component.get_component_property_value(
                    AtomComponentProperties.reflection_probe('Bake Exposure')), 20.0))

        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Bake Exposure'), 0.0)

        # 12. Inner Extents|Height (float 0.0 to required shape dimension constraint default 8.0)
        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Height'), 0.0)
        Report.result(
            Tests.inner_extents_height_low,
            Math_IsClose(
                reflection_probe_component.get_component_property_value(
                    AtomComponentProperties.reflection_probe('Height')), 0.0))

        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Height'), 8.0)
        Report.result(
            Tests.inner_extents_height_high,
            Math_IsClose(
                reflection_probe_component.get_component_property_value(
                    AtomComponentProperties.reflection_probe('Height')), 8.0))

        # 13. Inner Extents|Length (float 0.0 to required shape dimension constraint default 8.0)
        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Length'), 0.0)
        Report.result(
            Tests.inner_extents_length_low,
            Math_IsClose(
                reflection_probe_component.get_component_property_value(
                    AtomComponentProperties.reflection_probe('Length')), 0.0))

        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Length'), 8.0)
        Report.result(
            Tests.inner_extents_length_high,
            Math_IsClose(
                reflection_probe_component.get_component_property_value(
                    AtomComponentProperties.reflection_probe('Length')), 8.0))

        # 14. Inner Extents|Width (float 0.0 to required shape dimension constraint default 8.0)
        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Width'), 0.0)
        Report.result(
            Tests.inner_extents_width_low,
            Math_IsClose(
                reflection_probe_component.get_component_property_value(
                    AtomComponentProperties.reflection_probe('Width')), 0.0))

        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Width'), 8.0)
        Report.result(
            Tests.inner_extents_width_high,
            Math_IsClose(
                reflection_probe_component.get_component_property_value(
                    AtomComponentProperties.reflection_probe('Width')), 8.0))

        # 15. Settings|Exposure (float range -20.0 to 20.0)
        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Settings Exposure'), -20.0)
        Report.result(
            Tests.settings_exposure_low,
            Math_IsClose(
                reflection_probe_component.get_component_property_value(
                    AtomComponentProperties.reflection_probe('Settings Exposure')), -20.0))

        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Settings Exposure'), 20.0)
        Report.result(
            Tests.settings_exposure_high,
            Math_IsClose(
                reflection_probe_component.get_component_property_value(
                    AtomComponentProperties.reflection_probe('Settings Exposure')), 20.0))

        reflection_probe_component.set_component_property_value(
            AtomComponentProperties.reflection_probe('Settings Exposure'), 0.0)

        # # 16. Baked Cubemap Quality (unsigned int use dictionary constant)
        for quality in BAKED_CUBEMAP_QUALITY:
            test_quality = (
                f"Baked Cubemap Quality ({quality}) set as expected",
                f"P1: Baked Cubemap Quality ({quality}) failed to be set")
            reflection_probe_component.set_component_property_value(
                AtomComponentProperties.reflection_probe('Baked Cubemap Quality'), BAKED_CUBEMAP_QUALITY[quality])
            Report.result(
                test_quality,
                reflection_probe_component.get_component_property_value(
                    AtomComponentProperties.reflection_probe('Baked Cubemap Quality')) == BAKED_CUBEMAP_QUALITY[quality])

        # 17. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 18. Test IsHidden.
        reflection_probe_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, reflection_probe_entity.is_hidden() is True)

        # 19. Test IsVisible.
        reflection_probe_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, reflection_probe_entity.is_visible() is True)

        # 20. Delete Reflection Probe entity.
        reflection_probe_entity.delete()
        Report.result(Tests.entity_deleted, not reflection_probe_entity.exists())

        # 21. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, reflection_probe_entity.exists())

        # 22. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not reflection_probe_entity.exists())

        # 23. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_ReflectionProbe_AddedToEntity)
