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
    diffuse_probe_grid_creation = (
        "Diffuse Probe Grid Entity successfully created",
        "P0: Diffuse Probe Grid Entity failed to be created")
    diffuse_probe_grid_component = (
        "Entity has a Diffuse Probe Grid component",
        "P0: Entity failed to find Diffuse Probe Grid component")
    diffuse_probe_grid_disabled = (
        "Diffuse Probe Grid component disabled",
        "P0: Diffuse Probe Grid component was not disabled")
    diffuse_probe_grid_enabled = (
        "Diffuse Probe Grid component enabled",
        "P0: Diffuse Probe Grid component was not enabled")
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
    scrolling = (
        "Scaling property set",
        "P1: Scaling property failed to set correctly")
    show_visualization = (
        "Show Visualization property set",
        "P1: Show Visualization property failed to set correctly")
    show_inactive_probes = (
        "Show Inactive Probes property set",
        "P1: Show Inactive Probes property failed to set correctly")
    visualization_sphere_radius_low = (
        "Visualization Sphere Radius set to 0.1",
        "P1: Visualization Sphere Radius failed to take minimum value")
    visualization_sphere_radius_high = (
        "Visualization Sphere Radius set to 999.0",
        "P1: Visualization Sphere Radius failed to set a high value of 999.0")
    normal_bias_high = (
        "Normal Bias property set to 1.0",
        "P1: Normal Bias property failed to take a maximum value")
    normal_bias_low = (
        "Normal Bias property set to 0.0",
        "P1: Normal Bias property failed to take a minimum value")
    ambient_multiplier_high = (
        "Ambient Multiplier property set to 10.0",
        "P1: Ambient Multiplier property failed to take a maximum value")
    ambient_multiplier_low = (
        "Ambient Multiplier property set to 0.0",
        "P1: Ambient Multiplier property failed to take a minimum value")
    view_bias_high = (
        "View Bias property set to 1.0",
        "P1: View Bias property failed to take a maximum value")
    view_bias_low = (
        "View Bias property set to 0.0",
        "P1: View Bias property failed to take a minimum value")



def AtomEditorComponents_DiffuseProbeGrid_AddedToEntity():
    """
    Summary:
    Tests the Diffuse Probe Grid component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.
    We are intentionally not changing Editor/Runtime Mode since that requires a probe bake.
    We are not baking probes for gated AR because this requires file I/O that can be unreliable in batch/parallel tests.

    Test Steps:
    1) Create a Diffuse Probe Grid entity with no components.
    2) Add a Diffuse Probe Grid component to Diffuse Probe Grid entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify Diffuse Probe Grid component not enabled.
    6) Add Shape component since it is required by the Diffuse Probe Grid component.
    7) Verify Diffuse Probe Grid component is enabled.
    8) Toggle 'Scrolling'
    9) Toggle 'Show Inactive Probes'
    10) Toggle 'Show Visualization'
    11) 'Visualization Sphere Radius' (0.1 to very large float unbound)
    12) 'Normal Bias' (flaot 0.0 to 1.0)
    13) 'Ambient Multiplier' (float 0.0 to 10.0)
    14) 'View Bias' (float 0.0 to 1.0)
    15) 'Number of Rays Per Probe' from atom_constants.py NUM_RAYS_PER_PROBE
    16) Enter/Exit game mode.
    17) Test IsHidden.
    18) Test IsVisible.
    19) Delete Diffuse Probe Grid entity.
    20) UNDO deletion.
    21) REDO deletion.
    22) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general

    from azlmbr.math import Math_IsClose
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties, NUM_RAYS_PER_PROBE

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a Diffuse Probe Grid entity with no components.
        diffuse_probe_grid_entity = EditorEntity.create_editor_entity(AtomComponentProperties.diffuse_probe_grid())
        Report.critical_result(Tests.diffuse_probe_grid_creation, diffuse_probe_grid_entity.exists())

        # 2. Add a Diffuse Probe Grid component to Diffuse Probe Grid entity.
        diffuse_probe_grid_component = diffuse_probe_grid_entity.add_component(
            AtomComponentProperties.diffuse_probe_grid())
        Report.critical_result(
            Tests.diffuse_probe_grid_component,
            diffuse_probe_grid_entity.has_component(AtomComponentProperties.diffuse_probe_grid()))

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
        Report.result(Tests.creation_undo, not diffuse_probe_grid_entity.exists())

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
        Report.result(Tests.creation_redo, diffuse_probe_grid_entity.exists())

        # 5. Verify Diffuse Probe Grid component not enabled.
        Report.result(Tests.diffuse_probe_grid_disabled, not diffuse_probe_grid_component.is_enabled())

        # 6. Add Shape component since it is required by the Diffuse Probe Grid component.
        for shape in AtomComponentProperties.diffuse_probe_grid('shapes'):
            diffuse_probe_grid_entity.add_component(shape)
            test_shape = (
                f"Entity has a {shape} component",
                f"Entity did not have a {shape} component")
            Report.result(test_shape, diffuse_probe_grid_entity.has_component(shape))

            # 7. Check if required shape allows Diffuse Probe Grid to be enabled
            Report.result(Tests.diffuse_probe_grid_enabled, diffuse_probe_grid_component.is_enabled())

            # Undo to remove each added shape except the last one and verify Diffuse Probe Grid is not enabled.
            if not (shape == AtomComponentProperties.diffuse_probe_grid('shapes')[-1]):
                general.undo()
                TestHelper.wait_for_condition(lambda: not diffuse_probe_grid_entity.has_component(shape), 1.0)
                Report.result(Tests.diffuse_probe_grid_disabled, not diffuse_probe_grid_component.is_enabled())

        # 8. Toggle 'Scrolling'
        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Scrolling'), True)
        Report.result(
            Tests.scrolling,
            diffuse_probe_grid_component.get_component_property_value(
                AtomComponentProperties.diffuse_probe_grid('Scrolling')) is True)

        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Scrolling'), False)
        Report.result(
            Tests.scrolling,
            diffuse_probe_grid_component.get_component_property_value(
                AtomComponentProperties.diffuse_probe_grid('Scrolling')) is False)

        # 9. Toggle 'Show Inactive Probes'
        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Show Inactive Probes'), True)
        Report.result(
            Tests.show_inactive_probes,
            diffuse_probe_grid_component.get_component_property_value(
                AtomComponentProperties.diffuse_probe_grid('Show Inactive Probes')) is True)

        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Show Inactive Probes'), False)
        Report.result(
            Tests.show_inactive_probes,
            diffuse_probe_grid_component.get_component_property_value(
                AtomComponentProperties.diffuse_probe_grid('Show Inactive Probes')) is False)

        # 10. Toggle 'Show Visualization'
        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Show Visualization'), True)
        Report.result(
            Tests.show_visualization,
            diffuse_probe_grid_component.get_component_property_value(
                AtomComponentProperties.diffuse_probe_grid('Show Visualization')) is True)

        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Show Visualization'), False)
        Report.result(
            Tests.show_visualization,
            diffuse_probe_grid_component.get_component_property_value(
                AtomComponentProperties.diffuse_probe_grid('Show Visualization')) is False)

        # 11. 'Visualization Sphere Radius' (0.1 to very large float unbound)
        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Visualization Sphere Radius'), 0.1)
        Report.result(
            Tests.visualization_sphere_radius_low,
            Math_IsClose(
                diffuse_probe_grid_component.get_component_property_value(
                    AtomComponentProperties.diffuse_probe_grid('Visualization Sphere Radius')), 0.1))

        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Visualization Sphere Radius'), 999.0)
        Report.result(
            Tests.visualization_sphere_radius_high,
            Math_IsClose(
                diffuse_probe_grid_component.get_component_property_value(
                    AtomComponentProperties.diffuse_probe_grid('Visualization Sphere Radius')), 999.0))
        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Visualization Sphere Radius'), 0.5)


        # 12. 'Normal Bias' (flaot 0.0 to 1.0)
        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Normal Bias'), 1.0)
        Report.result(
            Tests.normal_bias_high,
            Math_IsClose(
                diffuse_probe_grid_component.get_component_property_value(
                    AtomComponentProperties.diffuse_probe_grid('Normal Bias')), 1.0))

        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Normal Bias'), 0.0)
        Report.result(
            Tests.normal_bias_low,
            Math_IsClose(
                diffuse_probe_grid_component.get_component_property_value(
                    AtomComponentProperties.diffuse_probe_grid('Normal Bias')), 0.0))

        # 13. 'Ambient Multiplier' (float 0.0 to 10.0)
        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Ambient Multiplier'), 10.0)
        Report.result(
            Tests.ambient_multiplier_high,
            Math_IsClose(
                diffuse_probe_grid_component.get_component_property_value(
                    AtomComponentProperties.diffuse_probe_grid('Ambient Multiplier')), 10.0))

        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('Ambient Multiplier'), 0.0)
        Report.result(
            Tests.ambient_multiplier_low,
            Math_IsClose(
                diffuse_probe_grid_component.get_component_property_value(
                    AtomComponentProperties.diffuse_probe_grid('Ambient Multiplier')), 0.0))

        # 14. 'View Bias' (float 0.0 to 1.0)
        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('View Bias'), 1.0)
        Report.result(
            Tests.view_bias_high,
            Math_IsClose(
                diffuse_probe_grid_component.get_component_property_value(
                    AtomComponentProperties.diffuse_probe_grid('View Bias')), 1.0))

        diffuse_probe_grid_component.set_component_property_value(
            AtomComponentProperties.diffuse_probe_grid('View Bias'), 0.0)
        Report.result(
            Tests.view_bias_low,
            Math_IsClose(
                diffuse_probe_grid_component.get_component_property_value(
                    AtomComponentProperties.diffuse_probe_grid('View Bias')), 0.0))

        # 15. 'Number of Rays Per Probe' from atom_constants.py NUM_RAYS_PER_PROBE
        for rays in NUM_RAYS_PER_PROBE:
            test_rays = (
                f"'Number of Rays Per Probe' successfully set to {rays}",
                f"P1: failed to set 'Number of Rays Per Probe' to {rays}")
            diffuse_probe_grid_component.set_component_property_value(
                AtomComponentProperties.diffuse_probe_grid('Number of Rays Per Probe'), NUM_RAYS_PER_PROBE[rays])
            Report.result(
                test_rays,
                diffuse_probe_grid_component.get_component_property_value(
                    AtomComponentProperties.diffuse_probe_grid('Number of Rays Per Probe')) == NUM_RAYS_PER_PROBE[rays])

        # 16. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 17. Test IsHidden.
        diffuse_probe_grid_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, diffuse_probe_grid_entity.is_hidden() is True)

        # 18. Test IsVisible.
        diffuse_probe_grid_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, diffuse_probe_grid_entity.is_visible() is True)

        # 19. Delete Diffuse Probe Grid entity.
        diffuse_probe_grid_entity.delete()
        Report.result(Tests.entity_deleted, not diffuse_probe_grid_entity.exists())

        # 20. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, diffuse_probe_grid_entity.exists())

        # 21. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not diffuse_probe_grid_entity.exists())

        # 22. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DiffuseProbeGrid_AddedToEntity)
