"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    camera_creation = (
        "Camera Entity successfully created",
        "Camera Entity failed to be created")
    camera_component_added = (
        "Camera component was added to entity",
        "Camera component failed to be added to entity")
    camera_component_check = (
        "Entity has a Camera component",
        "Entity failed to find Camera component")
    creation_undo = (
        "UNDO Entity creation success",
        "UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "REDO Entity creation failed")
    physical_sky_creation = (
        "Physical Sky Entity successfully created",
        "Physical Sky Entity failed to be created")
    physical_sky_component = (
        "Entity has a Physical Sky component",
        "Entity failed to find Physical Sky component")
    intensity_mode_set_to_nit = (
        "Sky Intensity set to Nit",
        "Sky Intensity could not be set to Nit")
    intensity_mode_set_to_ev100 = (
        "Sky Intensity set to Ev100",
        "Sky Intensity could not be set to Ev100")
    sky_intensity_set_to_0 = (
        "Sky Intensity set to 0",
        "Sky Intensity could not be set to 0")
    sky_intensity_set_to_4 = (
        "Sky Intensity set to 4.0",
        "Sky Intensity could not be set to 4.0")
    sun_intensity_set_to_0 = (
        "Sun Intensity set to 1",
        "Sun Intensity could not be set to 1")
    sun_intensity_set_to_8 = (
        "Sun Intensity set to 8.0",
        "Sun Intensity could not be set to 8.0")
    turbidity_set_to_5 = (
        "Turbidity set to 5",
        "Turbidity could not be set to 5")
    turbidity_set_to_1 = (
        "Turbidity set to 1",
        "Turbidity could not be set to 1")
    sun_radius_factor_set_to_0 = (
        "Sun Radius Factor set to 0.0",
        "Sun Radius Factor could not be set to 0.0")
    sun_radius_factor_set_to_1 = (
        "Sun Radius Factor set to 1.0",
        "Sun Radius Factor could not be set to 1.0")
    enable_fog_set_to_true = (
        "Enable Fog set to True",
        "Enable Fog could not be set to True")
    enable_fog_set_to_false = (
        "Enable Fog value set to False",
        "Enable Fog value could not be set to False")
    fog_color_set_to_green = (
        "Fog Color set to 0.0, 255.0, 0.0",
        "Fog Color could not be set to 0.0, 255.0, 0.0")
    fog_color_set_to_default = (
        "Fog Color set to 0.0, 0.0, 0.0",
        "Fog Color could not be set to 0.0, 0.0, 0.0")
    fog_top_height_set_to_five_tenths = (
        "Fog Top Height set to 0.5",
        "Fog Top Height could not be set to 0.5")
    fog_top_height_set_to_one_hundredth = (
        "Fog Top Height set to 0.01",
        "Fog Top Height could not be set to 0.01")
    fog_bottom_height_set_to_three_tenths = (
        "Fog Bottom Height set to 0.3",
        "Fog Bottom Height could not be set to 0.3")
    fog_bottom_height_set_to_0 = (
        "Fog Bottom Height set to 0.0",
        "Fog Bottom Height could not be set to 0.0")
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


def AtomEditorComponents_PhysicalSky_AddedToEntity():
    """
    Summary:
    Tests the Physical Sky component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Physical Sky entity with no components.
    2) Add Physical Sky component to Physical Sky entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Intensity mode set to Nit then back to Ev100.
    6) Set Sky Intensity value to 0 then back to 4.
    7) Set Sun Intensity value to 0 then back to 8.
    8) Set Turbidity value to 5 then back to 1.
    9) Set Sun Radius Factor from 0.0 back to 1.0.
    10a) Enable Fog for subsequent tests.
    11) Change fog color to green, then return to default.
    12) Set Fog Top Height to 0.5 then back to 0.01.
    13) Set Fog Bottom Height to 0.3 then back to 0.0.
    10b) Disable Enable Fog to set back to default.
    14) Enter/Exit game mode.
    15) Test IsHidden.
    16) Test IsVisible.
    17) Delete Physical Sky entity.
    18) UNDO deletion.
    19) REDO deletion.
    20) Look for errors and asserts.

    :return: None
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties, PHYSICAL_SKY_INTENSITY_MODE
    from azlmbr.math import Math_IsClose, Color

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a Physical Sky entity with no components.
        physical_sky_entity = EditorEntity.create_editor_entity(AtomComponentProperties.physical_sky())
        Report.critical_result(Tests.physical_sky_creation, physical_sky_entity.exists())

        # 2. Add Physical Sky component to Physical Sky entity.
        physical_sky_component = physical_sky_entity.add_component(AtomComponentProperties.physical_sky())
        Report.critical_result(
            Tests.physical_sky_component,
            physical_sky_entity.has_component(AtomComponentProperties.physical_sky()))

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
        Report.result(Tests.creation_undo, not physical_sky_entity.exists())

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
        Report.result(Tests.creation_redo, physical_sky_entity.exists())

        # 5. Intensity mode set to Nit then back to Ev100.
        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Intensity Mode'), PHYSICAL_SKY_INTENSITY_MODE["Nit"])
        Report.result(
            Tests.intensity_mode_set_to_nit,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Intensity Mode')) == PHYSICAL_SKY_INTENSITY_MODE["Nit"])
    
        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Intensity Mode'), PHYSICAL_SKY_INTENSITY_MODE["Ev100"])
        general.idle_wait_frames(1)
        Report.result(
            Tests.intensity_mode_set_to_ev100,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Intensity Mode')) == PHYSICAL_SKY_INTENSITY_MODE["Ev100"])

        # 6. Set Sky Intensity value to 0 then back to 4.
        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Sky Intensity'), 0.0)
        Report.result(
            Tests.sky_intensity_set_to_0,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Sky Intensity')) == 0.0)

        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Sky Intensity'), 4.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.sky_intensity_set_to_4,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Sky Intensity')) == 4.0)

        # 7. Set Sun Intensity value to 0 then back to 8.
        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Sun Intensity'), 0)
        Report.result(
            Tests.sun_intensity_set_to_0,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Sun Intensity')) == 0)

        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Sun Intensity'), 8.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.sun_intensity_set_to_8,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Sun Intensity')) == 8.0)

        # 8. Set Turbidity value to 5 then back to 1.
        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Turbidity'), 5)
        Report.result(
            Tests.turbidity_set_to_5,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Turbidity')) == 5)

        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Turbidity'), 1)
        general.idle_wait_frames(1)
        Report.result(
            Tests.turbidity_set_to_1,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Sun Radius Factor')) == 1)

        # 9. Set Sun Radius Factor from 0.0 back to 1.0.
        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Sun Radius Factor'), 0.0)
        Report.result(
            Tests.sun_radius_factor_set_to_0,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Sun Radius Factor')) == 0.0)

        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Sun Radius Factor'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.sun_radius_factor_set_to_1,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Sun Radius Factor')) == 1.0)

        # 10a. Enable Fog for subsequent tests.
        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Enable Fog'), True)
        Report.result(
            Tests.enable_fog_set_to_true,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Enable Fog')) is True)
        general.idle_wait_frames(1)

        # 11. Change fog color to green, then return to default.
        color_green = Color(0.0, 255.0, 0.0, 255.0)
        color_default = Color(0.0, 0.0, 0.0, 255.0)

        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Fog Color'), color_green)
        general.idle_wait_frames(1)
        Report.result(
            Tests.fog_color_set_to_green,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Fog Color')) == color_green)

        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Fog Color'),
            color_default)
        general.idle_wait_frames(1)
        Report.result(
            Tests.fog_color_set_to_default,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Fog Color')) == color_default)

        # 12. Set Fog Top Height to 0.5 then back to 0.01.
        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Fog Top Height'), 0.5)
        Report.result(
            Tests.fog_top_height_set_to_five_tenths, Math_IsClose(
                physical_sky_component.get_component_property_value(
                    AtomComponentProperties.physical_sky('Fog Top Height')), 0.5))

        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Fog Top Height'), 0.01)
        general.idle_wait_frames(1)
        Report.result(
            Tests.fog_top_height_set_to_one_hundredth, Math_IsClose(
                physical_sky_component.get_component_property_value(
                    AtomComponentProperties.physical_sky('Fog Top Height')), 0.01))

        # 13. Set Fog Bottom Height to 0.3 then back to 0.0.
        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Fog Bottom Height'), 0.3)
        general.idle_wait_frames(1)
        Report.result(
            Tests.fog_bottom_height_set_to_three_tenths, Math_IsClose(
                physical_sky_component.get_component_property_value(
                    AtomComponentProperties.physical_sky('Fog Bottom Height')), 0.3))

        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Fog Bottom Height'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.fog_bottom_height_set_to_0,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Fog Bottom Height')) == 0.0)

        # 10b. Disable Enable Fog to set back to default.
        physical_sky_component.set_component_property_value(
            AtomComponentProperties.physical_sky('Enable Fog'), False)
        Report.result(
            Tests.enable_fog_set_to_true,
            physical_sky_component.get_component_property_value(
                AtomComponentProperties.physical_sky('Enable Fog')) is False)
        general.idle_wait_frames(1)

        # 14. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 15. Test IsHidden.
        physical_sky_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, physical_sky_entity.is_hidden() is True)

        # 16. Test IsVisible.
        physical_sky_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, physical_sky_entity.is_visible() is True)

        # 17. Delete Physical Sky entity.
        physical_sky_entity.delete()
        Report.result(Tests.entity_deleted, not physical_sky_entity.exists())

        # 18. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, physical_sky_entity.exists())

        # 19. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not physical_sky_entity.exists())

        # 20. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_PhysicalSky_AddedToEntity)
