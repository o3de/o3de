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
    grid_entity_creation = (
        "Grid Entity successfully created",
        "P0: Grid Entity failed to be created")
    grid_component_added = (
        "Entity has a Grid component",
        "P0: Entity failed to find Grid component")
    grid_size = (
        "Grid Size value set successfully",
        "P0: Grid Size value could not be set")
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
    axis_color = (
        "Axis Color value set successfully",
        "P1: Axis Color value could not be set")
    primary_grid_spacing = (
        "Primary Grid Spacing value set successfully",
        "P1: Primary Grid Spacing value could not be set")
    primary_color = (
        "Primary Color value set successfully",
        "P1: Primary Color value could not be set")
    secondary_grid_spacing = (
        "Secondary Grid Spacing value set successfully",
        "P1: Secondary Grid Spacing value could not be set")
    secondary_color = (
        "Secondary Color value set successfully",
        "P1: Secondary Color value could not be set")


def AtomEditorComponents_Grid_AddedToEntity():
    """
    Summary:
    Tests the Grid component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "base_empty" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Property values for the component can be set.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Grid entity with no components.
    2) Add a Grid component to Grid entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Grid Size property value updated.
    6) Axis Color property value updated.
    7) Primary Grid Spacing property value updated.
    8) Primary Color property value updated.
    9) Secondary Grid Spacing property value updated.
    10) Secondary Color property value updated.
    11) Enter/Exit game mode.
    12) Test IsHidden.
    13) Test IsVisible.
    14) Delete Grid entity.
    15) UNDO deletion.
    16) REDO deletion.
    17) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a Grid entity with no components.
        grid_entity = EditorEntity.create_editor_entity(AtomComponentProperties.grid())
        Report.critical_result(Tests.grid_entity_creation, grid_entity.exists())

        # 2. Add a Grid component to Grid entity.
        grid_component = grid_entity.add_component(AtomComponentProperties.grid())
        Report.critical_result(
            Tests.grid_component_added,
            grid_entity.has_component(AtomComponentProperties.grid()))

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
        Report.result(Tests.creation_undo, not grid_entity.exists())

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
        Report.result(Tests.creation_redo, grid_entity.exists())

        # 5. Grid Size property value updated.
        grid_component.set_component_property_value(
            AtomComponentProperties.grid('Grid Size'), value=64)
        current_grid_size = grid_component.get_component_property_value(
            AtomComponentProperties.grid('Grid Size'))
        Report.result(Tests.grid_size, current_grid_size == 64)

        # 6. Axis Color property value updated.
        green_color_value = math.Color(13.0, 255.0, 0.0, 1.0)
        grid_component.set_component_property_value(
            AtomComponentProperties.grid('Axis Color'), value=green_color_value)
        Report.result(Tests.axis_color,
                      grid_component.get_component_property_value(
                          AtomComponentProperties.grid('Axis Color')) == green_color_value)

        # 7. Primary Grid Spacing property value updated.
        grid_component.set_component_property_value(
            AtomComponentProperties.grid('Primary Grid Spacing'), value=0.5)
        Report.result(Tests.primary_grid_spacing,
                      grid_component.get_component_property_value(
                          AtomComponentProperties.grid('Primary Grid Spacing')) == 0.5)

        # 8. Primary Color property value updated.
        brown_color_value = math.Color(129.0, 96.0, 0.0, 1.0)
        grid_component.set_component_property_value(
            AtomComponentProperties.grid('Primary Color'), value=brown_color_value)
        Report.result(Tests.primary_color,
                      grid_component.get_component_property_value(
                          AtomComponentProperties.grid('Primary Color')) == brown_color_value)

        # 9. Secondary Grid Spacing property value updated.
        grid_component.set_component_property_value(AtomComponentProperties.grid('Secondary Grid Spacing'), value=0.75)
        Report.result(Tests.secondary_grid_spacing,
                      grid_component.get_component_property_value(
                          AtomComponentProperties.grid('Secondary Grid Spacing')) == 0.75)

        # 10. Secondary Color property value updated.
        blue_color_value = math.Color(0.0, 35.0, 161.0, 1.0)
        grid_component.set_component_property_value(
            AtomComponentProperties.grid('Secondary Color'), value=blue_color_value)
        Report.result(Tests.secondary_color,
                      grid_component.get_component_property_value(
                          AtomComponentProperties.grid('Secondary Color')) == blue_color_value)

        # 11. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 12. Test IsHidden.
        grid_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, grid_entity.is_hidden() is True)

        # 13. Test IsVisible.
        grid_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, grid_entity.is_visible() is True)

        # 14. Delete Grid entity.
        grid_entity.delete()
        Report.result(Tests.entity_deleted, not grid_entity.exists())

        # 15. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, grid_entity.exists())

        # 16. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not grid_entity.exists())

        # 17. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Grid_AddedToEntity)
